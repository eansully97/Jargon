#include "Data/JargonHeroAspectAbilityMigrationCommandlet.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/JargonAbilityAuditTool.h"
#include "Data/JargonAbilityDefinition.h"
#include "Data/JargonHeroDefinition.h"
#include "Data/JargonStatusEffectDefinition.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogJargonHeroAspectAbilityMigration, Log, All);

namespace
{
const TCHAR* MageHeroPath = TEXT("/Game/Jargon/Data/Hero/DA_Mage.DA_Mage");
const TCHAR* AbilityRootPath = TEXT("/Game/Jargon/Data/Hero/Abilities");
const FString ReportDirectoryName(TEXT("AbilityAudit"));
const FString ReportFileName(TEXT("HeroAspectAbilityMigration.csv"));

enum class EJargonHeroAspectHook : uint8
{
	Transformation,
	TurnStart,
	EnemyDeath
};

struct FHookMigrationData
{
	const TCHAR* HookName = TEXT("");
	EJargonEffectTrigger Trigger = EJargonEffectTrigger::Activated;
	const FText* DisplayName = nullptr;
	const TArray<FJargonEffectSpec>* RawEffects = nullptr;
	TObjectPtr<UJargonAbilityDefinition>* AbilityPtr = nullptr;
	TArray<FJargonEffectSpec>* MutableRawEffects = nullptr;
};

FString CsvEscape(const FString& Value)
{
	FString Escaped = Value;
	Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""), ESearchCase::CaseSensitive);
	return FString::Printf(TEXT("\"%s\""), *Escaped);
}

void AppendReportLine(
	FString& ReportCsv,
	const FString& AspectName,
	const FString& HookName,
	const FString& AbilityPath,
	const FString& Status,
	const FString& ConvertedActions,
	const FString& Details)
{
	ReportCsv += FString::Printf(
		TEXT("%s,%s,%s,%s,%s,%s\r\n"),
		*CsvEscape(AspectName),
		*CsvEscape(HookName),
		*CsvEscape(AbilityPath),
		*CsvEscape(Status),
		*CsvEscape(ConvertedActions),
		*CsvEscape(Details));
}

FString EnumTokenName(const UEnum* Enum, int64 Value)
{
	return JargonEffectContracts::GetEnumTokenName(Enum, Value);
}

FString AspectTokenName(EJargonHeroAspect Aspect)
{
	return EnumTokenName(StaticEnum<EJargonHeroAspect>(), static_cast<int64>(Aspect));
}

FString HookSuffix(EJargonHeroAspectHook Hook)
{
	switch (Hook)
	{
	case EJargonHeroAspectHook::Transformation:
		return TEXT("Transform");

	case EJargonHeroAspectHook::TurnStart:
		return TEXT("TurnStart");

	case EJargonHeroAspectHook::EnemyDeath:
		return TEXT("EnemyDeath");

	default:
		return TEXT("Ability");
	}
}

FString HookDisplayName(EJargonHeroAspectHook Hook)
{
	switch (Hook)
	{
	case EJargonHeroAspectHook::Transformation:
		return TEXT("Transform");

	case EJargonHeroAspectHook::TurnStart:
		return TEXT("Turn Start");

	case EJargonHeroAspectHook::EnemyDeath:
		return TEXT("Enemy Death");

	default:
		return TEXT("Ability");
	}
}

FString AbilityAssetName(EJargonHeroAspect Aspect, EJargonHeroAspectHook Hook)
{
	return FString::Printf(TEXT("DA_Ability_Mage_%s_%s"), *AspectTokenName(Aspect), *HookSuffix(Hook));
}

FString AbilityPackageName(EJargonHeroAspect Aspect, EJargonHeroAspectHook Hook)
{
	return FString::Printf(TEXT("%s/%s"), AbilityRootPath, *AbilityAssetName(Aspect, Hook));
}

UJargonStatusEffectDefinition* LoadStatusDefinition(const TCHAR* AssetPath)
{
	return Cast<UJargonStatusEffectDefinition>(
		StaticLoadObject(UJargonStatusEffectDefinition::StaticClass(), nullptr, AssetPath));
}

UJargonStatusEffectDefinition* LoadStatusDefinitionForOperation(EJargonEffectOperation Operation)
{
	switch (Operation)
	{
	case EJargonEffectOperation::ApplyStun:
		return LoadStatusDefinition(TEXT("/Game/Jargon/Data/StatusEffects/DA_Status_Stun.DA_Status_Stun"));

	case EJargonEffectOperation::ApplyFreeze:
		return LoadStatusDefinition(TEXT("/Game/Jargon/Data/StatusEffects/DA_Status_Freeze.DA_Status_Freeze"));

	case EJargonEffectOperation::ApplyBurn:
		return LoadStatusDefinition(TEXT("/Game/Jargon/Data/StatusEffects/DA_Status_Burn.DA_Status_Burn"));

	case EJargonEffectOperation::ApplyRoot:
		return LoadStatusDefinition(TEXT("/Game/Jargon/Data/StatusEffects/DA_Status_Root.DA_Status_Root"));

	case EJargonEffectOperation::ApplyVulnerable:
		return LoadStatusDefinition(TEXT("/Game/Jargon/Data/StatusEffects/DA_Status_Vulnerable.DA_Status_Vulnerable"));

	default:
		return nullptr;
	}
}

bool IsSelfOnlyAbilityOperation(EJargonEffectOperation Operation)
{
	return Operation == EJargonEffectOperation::DrawCards
		|| Operation == EJargonEffectOperation::GainEnergy
		|| Operation == EJargonEffectOperation::GainElementCharge;
}

bool TryBuildTargetingProfileFromEffect(const FJargonEffectSpec& Effect, FJargonAbilityTargetingProfile& OutProfile, FString& OutReason)
{
	OutProfile.Radius = FMath::Max(0, Effect.Radius);
	OutProfile.ChainCount = FMath::Max(1, Effect.ChainCount);

	switch (Effect.Delivery)
	{
	case EJargonEffectDelivery::Self:
		OutProfile.Preset = EJargonAbilityTargetingPreset::Self;
		return true;

	case EJargonEffectDelivery::ExplicitTile:
		OutProfile.Preset = EJargonAbilityTargetingPreset::TargetTile;
		return true;

	case EJargonEffectDelivery::ExplicitUnit:
		if (Effect.TargetFilter == EJargonEffectTargetFilter::SourceOnly)
		{
			OutProfile.Preset = EJargonAbilityTargetingPreset::Self;
		}
		else if (Effect.TargetFilter == EJargonEffectTargetFilter::FriendlyToSource)
		{
			OutProfile.Preset = EJargonAbilityTargetingPreset::SelectedAlly;
		}
		else if (Effect.TargetFilter == EJargonEffectTargetFilter::EnemyToSource)
		{
			OutProfile.Preset = EJargonAbilityTargetingPreset::SelectedEnemy;
		}
		else
		{
			OutProfile.Preset = EJargonAbilityTargetingPreset::SelectedUnit;
		}
		return true;

	case EJargonEffectDelivery::UnitsInRadius:
		if (Effect.TargetFilter == EJargonEffectTargetFilter::FriendlyToSource)
		{
			OutProfile.Preset = EJargonAbilityTargetingPreset::AlliesInRadius;
		}
		else if (Effect.TargetFilter == EJargonEffectTargetFilter::EnemyToSource)
		{
			OutProfile.Preset = EJargonAbilityTargetingPreset::EnemiesInRadius;
		}
		else
		{
			OutProfile.Preset = EJargonAbilityTargetingPreset::UnitsInRadius;
		}
		return true;

	case EJargonEffectDelivery::TilesInRadius:
		OutProfile.Preset = EJargonAbilityTargetingPreset::TilesInRadius;
		return true;

	case EJargonEffectDelivery::ChainUnits:
		if (Effect.TargetFilter != EJargonEffectTargetFilter::EnemyToSource)
		{
			OutReason = FString::Printf(TEXT("ChainUnits with filter %s has no clean ability targeting preset yet."),
				*JargonEffectContracts::GetTargetFilterName(Effect.TargetFilter));
			return false;
		}
		OutProfile.Preset = EJargonAbilityTargetingPreset::ChainEnemies;
		return true;

	default:
		OutReason = FString::Printf(TEXT("Unsupported delivery %s."),
			*JargonEffectContracts::GetDeliveryName(Effect.Delivery));
		return false;
	}
}

bool AreProfilesEquivalent(const FJargonAbilityTargetingProfile& First, const FJargonAbilityTargetingProfile& Second)
{
	if (First.Preset != Second.Preset)
	{
		return false;
	}

	if (First.UsesRadius() && FMath::Max(0, First.Radius) != FMath::Max(0, Second.Radius))
	{
		return false;
	}

	if (First.UsesChain() && FMath::Max(1, First.ChainCount) != FMath::Max(1, Second.ChainCount))
	{
		return false;
	}

	return true;
}

bool TryBuildTargetingProfile(
	const TArray<FJargonEffectSpec>& Effects,
	FJargonAbilityTargetingProfile& OutProfile,
	FString& OutReason)
{
	bool bFoundTargetedEffect = false;
	for (const FJargonEffectSpec& Effect : Effects)
	{
		if (IsSelfOnlyAbilityOperation(Effect.Operation))
		{
			continue;
		}

		FJargonAbilityTargetingProfile EffectProfile;
		if (!TryBuildTargetingProfileFromEffect(Effect, EffectProfile, OutReason))
		{
			return false;
		}

		if (!bFoundTargetedEffect)
		{
			OutProfile = EffectProfile;
			bFoundTargetedEffect = true;
			continue;
		}

		if (!AreProfilesEquivalent(OutProfile, EffectProfile))
		{
			OutReason = FString::Printf(TEXT("Hook has mixed targeting profiles: '%s' and '%s'."),
				*OutProfile.GetSummary(),
				*EffectProfile.GetSummary());
			return false;
		}
	}

	if (!bFoundTargetedEffect)
	{
		OutProfile.Preset = EJargonAbilityTargetingPreset::Self;
		OutProfile.Radius = 0;
		OutProfile.ChainCount = 3;
	}

	return true;
}

template <typename ActionType>
ActionType* NewAbilityAction(UJargonAbilityDefinition* Ability)
{
	return NewObject<ActionType>(Ability, NAME_None, RF_Transactional);
}

UJargonAbilityAction* ConvertEffectToAction(
	UJargonAbilityDefinition* Ability,
	const FJargonEffectSpec& Effect,
	FString& OutReason)
{
	if (!Ability)
	{
		OutReason = TEXT("Missing ability outer.");
		return nullptr;
	}

	switch (Effect.Operation)
	{
	case EJargonEffectOperation::DealDamage:
		if (Effect.bLifesteal)
		{
			UJargonAbilityLifestealDamageAction* Action = NewAbilityAction<UJargonAbilityLifestealDamageAction>(Ability);
			Action->Damage = FMath::Max(1, Effect.Value);
			return Action;
		}
		else
		{
			UJargonAbilityDamageAction* Action = NewAbilityAction<UJargonAbilityDamageAction>(Ability);
			Action->Damage = FMath::Max(1, Effect.Value);
			return Action;
		}

	case EJargonEffectOperation::Heal:
	{
		UJargonAbilityHealAction* Action = NewAbilityAction<UJargonAbilityHealAction>(Ability);
		Action->Healing = FMath::Max(1, Effect.Value);
		return Action;
	}

	case EJargonEffectOperation::ApplyShield:
	{
		UJargonAbilityShieldAction* Action = NewAbilityAction<UJargonAbilityShieldAction>(Ability);
		Action->Shield = FMath::Max(1, Effect.Value);
		return Action;
	}

	case EJargonEffectOperation::ApplyStatus:
	{
		if (!Effect.StatusEffectDefinition)
		{
			OutReason = TEXT("ApplyStatus is missing StatusEffectDefinition.");
			return nullptr;
		}

		UJargonAbilityStatusAction* Action = NewAbilityAction<UJargonAbilityStatusAction>(Ability);
		Action->StatusEffectDefinition = Effect.StatusEffectDefinition;
		Action->Amount = FMath::Max(1, Effect.Value);
		return Action;
	}

	case EJargonEffectOperation::ApplyStun:
	case EJargonEffectOperation::ApplyFreeze:
	case EJargonEffectOperation::ApplyBurn:
	case EJargonEffectOperation::ApplyRoot:
	case EJargonEffectOperation::ApplyVulnerable:
	{
		UJargonStatusEffectDefinition* StatusDefinition = LoadStatusDefinitionForOperation(Effect.Operation);
		if (!StatusDefinition)
		{
			OutReason = FString::Printf(TEXT("Could not load built-in status definition for %s."),
				*JargonEffectContracts::GetOperationName(Effect.Operation));
			return nullptr;
		}

		UJargonAbilityStatusAction* Action = NewAbilityAction<UJargonAbilityStatusAction>(Ability);
		Action->StatusEffectDefinition = StatusDefinition;
		Action->Amount = FMath::Max(1, Effect.Value);
		return Action;
	}

	case EJargonEffectOperation::CleanseStatus:
	{
		UJargonAbilityCleanseStatusAction* Action = NewAbilityAction<UJargonAbilityCleanseStatusAction>(Ability);
		Action->StatusEffectDefinition = Effect.StatusEffectDefinition;
		return Action;
	}

	case EJargonEffectOperation::MoveSource:
	{
		UJargonAbilityMoveAction* Action = NewAbilityAction<UJargonAbilityMoveAction>(Ability);
		Action->Distance = FMath::Max(1, Effect.MoveDistance);
		return Action;
	}

	case EJargonEffectOperation::PushTarget:
	{
		UJargonAbilityPushAction* Action = NewAbilityAction<UJargonAbilityPushAction>(Ability);
		Action->Distance = FMath::Max(1, Effect.PushDistance);
		Action->CollisionDamage = FMath::Max(0, Effect.CollisionDamage);
		return Action;
	}

	case EJargonEffectOperation::PullTarget:
	{
		UJargonAbilityPullAction* Action = NewAbilityAction<UJargonAbilityPullAction>(Ability);
		Action->Distance = FMath::Max(1, Effect.PullDistance);
		return Action;
	}

	case EJargonEffectOperation::SummonUnit:
	{
		if (!Effect.SummonedUnitDefinition || !Effect.RuntimeSummonedUnitClass)
		{
			OutReason = TEXT("SummonUnit is missing SummonedUnitDefinition or RuntimeSummonedUnitClass.");
			return nullptr;
		}

		UJargonAbilitySummonAction* Action = NewAbilityAction<UJargonAbilitySummonAction>(Ability);
		Action->SummonedUnitDefinition = Effect.SummonedUnitDefinition;
		Action->RuntimeSummonedUnitClass = Effect.RuntimeSummonedUnitClass;
		Action->bSummonEntersWithAttackExhausted = Effect.bSummonEntersWithAttackExhausted;
		return Action;
	}

	case EJargonEffectOperation::PlaceTileEffect:
	{
		if (!Effect.TileEffectDefinition || !Effect.RuntimeTileEffectClass)
		{
			OutReason = TEXT("PlaceTileEffect is missing TileEffectDefinition or RuntimeTileEffectClass.");
			return nullptr;
		}

		UJargonAbilityPlaceTileEffectAction* Action = NewAbilityAction<UJargonAbilityPlaceTileEffectAction>(Ability);
		Action->TileEffectDefinition = Effect.TileEffectDefinition;
		Action->RuntimeTileEffectClass = Effect.RuntimeTileEffectClass;
		Action->TileEffectCategory = Effect.TileEffectCategory;
		return Action;
	}

	case EJargonEffectOperation::DestroyTileEffect:
		return NewAbilityAction<UJargonAbilityDestroyTileEffectAction>(Ability);

	case EJargonEffectOperation::DrawCards:
	{
		UJargonAbilityDrawCardsAction* Action = NewAbilityAction<UJargonAbilityDrawCardsAction>(Ability);
		Action->Count = FMath::Max(1, Effect.Value);
		return Action;
	}

	case EJargonEffectOperation::GainEnergy:
	{
		UJargonAbilityGainEnergyAction* Action = NewAbilityAction<UJargonAbilityGainEnergyAction>(Ability);
		Action->Amount = FMath::Max(1, Effect.Value);
		return Action;
	}

	case EJargonEffectOperation::GainElementCharge:
	{
		if (Effect.ElementType == EJargonElementType::None)
		{
			OutReason = TEXT("GainElementCharge has ElementType=None.");
			return nullptr;
		}

		UJargonAbilityGainElementChargeAction* Action = NewAbilityAction<UJargonAbilityGainElementChargeAction>(Ability);
		Action->ElementType = Effect.ElementType;
		Action->Amount = FMath::Max(1, Effect.Value);
		return Action;
	}

	default:
		OutReason = FString::Printf(TEXT("Unsupported operation %s."),
			*JargonEffectContracts::GetOperationName(Effect.Operation));
		return nullptr;
	}
}

UJargonAbilityDefinition* LoadOrCreateAbility(EJargonHeroAspect Aspect, EJargonHeroAspectHook Hook, TArray<UPackage*>& PackagesToSave, bool& bOutCreated)
{
	bOutCreated = false;
	const FString AssetName = AbilityAssetName(Aspect, Hook);
	const FString PackageName = AbilityPackageName(Aspect, Hook);
	const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

	if (UJargonAbilityDefinition* ExistingAbility = Cast<UJargonAbilityDefinition>(
		StaticLoadObject(UJargonAbilityDefinition::StaticClass(), nullptr, *ObjectPath)))
	{
		if (UPackage* Package = ExistingAbility->GetOutermost())
		{
			PackagesToSave.AddUnique(Package);
		}
		return ExistingAbility;
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}

	UJargonAbilityDefinition* NewAbility = NewObject<UJargonAbilityDefinition>(
		Package,
		*AssetName,
		RF_Public | RF_Standalone | RF_Transactional);
	if (!NewAbility)
	{
		return nullptr;
	}

	FAssetRegistryModule::AssetCreated(NewAbility);
	Package->MarkPackageDirty();
	PackagesToSave.AddUnique(Package);
	bOutCreated = true;
	return NewAbility;
}

FText MakeAbilityDisplayName(
	const FJargonHeroAspectDefinition& AspectDefinition,
	EJargonHeroAspectHook Hook)
{
	const FString AspectName = AspectTokenName(AspectDefinition.Aspect);
	if (Hook == EJargonHeroAspectHook::Transformation && !AspectDefinition.TransformationName.IsEmpty())
	{
		return AspectDefinition.TransformationName;
	}

	if (Hook == EJargonHeroAspectHook::TurnStart && !AspectDefinition.TurnStartPassiveName.IsEmpty())
	{
		return AspectDefinition.TurnStartPassiveName;
	}

	if (Hook == EJargonHeroAspectHook::EnemyDeath && !AspectDefinition.EnemyDeathPassiveName.IsEmpty())
	{
		return AspectDefinition.EnemyDeathPassiveName;
	}

	return FText::FromString(FString::Printf(TEXT("%s %s"), *AspectName, *HookDisplayName(Hook)));
}

bool ConvertHookToAbility(
	UJargonHeroDefinition* HeroDefinition,
	FJargonHeroAspectDefinition& AspectDefinition,
	EJargonHeroAspectHook Hook,
	FHookMigrationData HookData,
	TArray<UPackage*>& PackagesToSave,
	FString& ReportCsv)
{
	const FString AspectName = AspectTokenName(AspectDefinition.Aspect);
	const FString HookName(HookData.HookName);
	if (!HookData.MutableRawEffects || !HookData.AbilityPtr)
	{
		AppendReportLine(ReportCsv, AspectName, HookName, TEXT(""), TEXT("Failed"), TEXT(""), TEXT("Missing hook mutation pointers."));
		return false;
	}

	if (HookData.MutableRawEffects->Num() <= 0)
	{
		const UJargonAbilityDefinition* ExistingAbility = HookData.AbilityPtr->Get();
		AppendReportLine(
			ReportCsv,
			AspectName,
			HookName,
			ExistingAbility ? ExistingAbility->GetPathName() : TEXT(""),
			ExistingAbility ? TEXT("Unchanged") : TEXT("Skipped"),
			ExistingAbility ? ExistingAbility->GetAuditSummary() : TEXT(""),
			ExistingAbility ? TEXT("No raw effects to migrate; ability already assigned.") : TEXT("No raw effects and no ability assigned."));
		return true;
	}

	FJargonAbilityTargetingProfile TargetingProfile;
	FString FailureReason;
	if (!TryBuildTargetingProfile(*HookData.MutableRawEffects, TargetingProfile, FailureReason))
	{
		AppendReportLine(ReportCsv, AspectName, HookName, TEXT(""), TEXT("Manual"), TEXT(""), FailureReason);
		return false;
	}

	bool bCreatedAbility = false;
	UJargonAbilityDefinition* Ability = LoadOrCreateAbility(AspectDefinition.Aspect, Hook, PackagesToSave, bCreatedAbility);
	if (!Ability)
	{
		AppendReportLine(ReportCsv, AspectName, HookName, TEXT(""), TEXT("Failed"), TEXT(""), TEXT("Could not load or create ability asset."));
		return false;
	}

	Ability->Modify();
	Ability->DisplayName = MakeAbilityDisplayName(AspectDefinition, Hook);
	Ability->Description = FText::FromString(FString::Printf(
		TEXT("Migrated Mage %s %s aspect hook."),
		*AspectName,
		*HookDisplayName(Hook)));
	Ability->ExpectedTrigger = HookData.Trigger;
	Ability->TargetingProfile = TargetingProfile;
	Ability->CueDefinition.CueLabel = HookData.DisplayName && !HookData.DisplayName->IsEmpty()
		? *HookData.DisplayName
		: Ability->DisplayName;
	Ability->Actions.Reset();

	TArray<FString> ConvertedActionSummaries;
	for (const FJargonEffectSpec& Effect : *HookData.MutableRawEffects)
	{
		FString ActionFailureReason;
		UJargonAbilityAction* Action = ConvertEffectToAction(Ability, Effect, ActionFailureReason);
		if (!Action)
		{
			Ability->Actions.Reset();
			AppendReportLine(ReportCsv, AspectName, HookName, Ability->GetPathName(), TEXT("Manual"), TEXT(""), ActionFailureReason);
			return false;
		}

		Action->RefreshEditorTitle();
		Ability->Actions.Add(Action);
		ConvertedActionSummaries.Add(Action->GetActionSummary());
	}

	TArray<FString> RulesLines;
	for (const TObjectPtr<UJargonAbilityAction>& Action : Ability->Actions)
	{
		if (Action)
		{
			RulesLines.Add(Action->GetRulesText());
		}
	}
	Ability->RulesText = FText::FromString(FString::Join(RulesLines, TEXT(" ")));
	Ability->MarkPackageDirty();
	if (UPackage* AbilityPackage = Ability->GetOutermost())
	{
		PackagesToSave.AddUnique(AbilityPackage);
	}

	HeroDefinition->Modify();
	*HookData.AbilityPtr = Ability;
	HookData.MutableRawEffects->Reset();
	HeroDefinition->MarkPackageDirty();
	if (UPackage* HeroPackage = HeroDefinition->GetOutermost())
	{
		PackagesToSave.AddUnique(HeroPackage);
	}

	AppendReportLine(
		ReportCsv,
		AspectName,
		HookName,
		Ability->GetPathName(),
		bCreatedAbility ? TEXT("CreatedMigratedClearedRaw") : TEXT("UpdatedMigratedClearedRaw"),
		FString::Join(ConvertedActionSummaries, TEXT(" | ")),
		TEXT("Assigned ability reference and cleared raw aspect effects."));
	return true;
}

void ClearOutOfScopeHeroClassPassiveAbilityRefs(
	UJargonHeroDefinition* HeroDefinition,
	TArray<UPackage*>& PackagesToSave,
	FString& ReportCsv)
{
	if (!HeroDefinition)
	{
		return;
	}

	bool bChanged = false;
	if (HeroDefinition->CombatStartPassive.Ability)
	{
		HeroDefinition->Modify();
		AppendReportLine(
			ReportCsv,
			TEXT("HeroClassPassive"),
			TEXT("CombatStartPassive"),
			HeroDefinition->CombatStartPassive.Ability->GetPathName(),
			TEXT("ClearedOutOfScopeAbilityReference"),
			TEXT(""),
			TEXT("This pass migrates hero aspect hooks only; class passive raw effects remain authoritative."));
		HeroDefinition->CombatStartPassive.Ability = nullptr;
		bChanged = true;
	}

	if (HeroDefinition->PlayerTurnStartPassive.Ability)
	{
		HeroDefinition->Modify();
		AppendReportLine(
			ReportCsv,
			TEXT("HeroClassPassive"),
			TEXT("PlayerTurnStartPassive"),
			HeroDefinition->PlayerTurnStartPassive.Ability->GetPathName(),
			TEXT("ClearedOutOfScopeAbilityReference"),
			TEXT(""),
			TEXT("This pass migrates hero aspect hooks only; class passive raw effects remain authoritative."));
		HeroDefinition->PlayerTurnStartPassive.Ability = nullptr;
		bChanged = true;
	}

	if (bChanged)
	{
		HeroDefinition->MarkPackageDirty();
		if (UPackage* HeroPackage = HeroDefinition->GetOutermost())
		{
			PackagesToSave.AddUnique(HeroPackage);
		}
	}
}
}

#endif

UJargonHeroAspectAbilityMigrationCommandlet::UJargonHeroAspectAbilityMigrationCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UJargonHeroAspectAbilityMigrationCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	UE_LOG(LogJargonHeroAspectAbilityMigration, Display, TEXT("Starting Mage hero aspect ability migration."));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().SearchAllAssets(true);

	UJargonHeroDefinition* MageDefinition = Cast<UJargonHeroDefinition>(
		StaticLoadObject(UJargonHeroDefinition::StaticClass(), nullptr, MageHeroPath));
	if (!MageDefinition)
	{
		UE_LOG(LogJargonHeroAspectAbilityMigration, Error, TEXT("Could not load Mage hero definition: %s"), MageHeroPath);
		return 1;
	}

	FString ReportCsv;
	ReportCsv += TEXT("Aspect,Hook,AbilityPath,Status,ConvertedActions,Details") LINE_TERMINATOR;

	TArray<UPackage*> PackagesToSave;
	int32 FailureCount = 0;
	int32 MigratedHookCount = 0;

	ClearOutOfScopeHeroClassPassiveAbilityRefs(MageDefinition, PackagesToSave, ReportCsv);

	for (FJargonHeroAspectDefinition& AspectDefinition : MageDefinition->HeroAspects)
	{
		if (AspectDefinition.Aspect == EJargonHeroAspect::None)
		{
			continue;
		}

		FHookMigrationData TransformationHook;
		TransformationHook.HookName = TEXT("Transformation");
		TransformationHook.Trigger = EJargonEffectTrigger::Activated;
		TransformationHook.DisplayName = &AspectDefinition.TransformationName;
		TransformationHook.RawEffects = &AspectDefinition.TransformationEffects;
		TransformationHook.MutableRawEffects = &AspectDefinition.TransformationEffects;
		TransformationHook.AbilityPtr = &AspectDefinition.TransformationAbility;
		if (ConvertHookToAbility(MageDefinition, AspectDefinition, EJargonHeroAspectHook::Transformation, TransformationHook, PackagesToSave, ReportCsv))
		{
			++MigratedHookCount;
		}
		else
		{
			++FailureCount;
		}

		FHookMigrationData TurnStartHook;
		TurnStartHook.HookName = TEXT("TurnStart");
		TurnStartHook.Trigger = EJargonEffectTrigger::OnTurnStart;
		TurnStartHook.DisplayName = &AspectDefinition.TurnStartPassiveName;
		TurnStartHook.RawEffects = &AspectDefinition.TurnStartEffects;
		TurnStartHook.MutableRawEffects = &AspectDefinition.TurnStartEffects;
		TurnStartHook.AbilityPtr = &AspectDefinition.TurnStartAbility;
		if (ConvertHookToAbility(MageDefinition, AspectDefinition, EJargonHeroAspectHook::TurnStart, TurnStartHook, PackagesToSave, ReportCsv))
		{
			++MigratedHookCount;
		}
		else
		{
			++FailureCount;
		}

		FHookMigrationData EnemyDeathHook;
		EnemyDeathHook.HookName = TEXT("EnemyDeath");
		EnemyDeathHook.Trigger = EJargonEffectTrigger::OnEnemyDeath;
		EnemyDeathHook.DisplayName = &AspectDefinition.EnemyDeathPassiveName;
		EnemyDeathHook.RawEffects = &AspectDefinition.EnemyDeathEffects;
		EnemyDeathHook.MutableRawEffects = &AspectDefinition.EnemyDeathEffects;
		EnemyDeathHook.AbilityPtr = &AspectDefinition.EnemyDeathAbility;
		if (ConvertHookToAbility(MageDefinition, AspectDefinition, EJargonHeroAspectHook::EnemyDeath, EnemyDeathHook, PackagesToSave, ReportCsv))
		{
			++MigratedHookCount;
		}
		else
		{
			++FailureCount;
		}
	}

	if (PackagesToSave.Num() > 0)
	{
		const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
		if (!bSaved)
		{
			UE_LOG(LogJargonHeroAspectAbilityMigration, Error, TEXT("One or more hero aspect ability migration packages failed to save."));
			return 2;
		}
	}

	const FString OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), ReportDirectoryName);
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);
	const FString ReportPath = FPaths::Combine(OutputDirectory, ReportFileName);
	if (!FFileHelper::SaveStringToFile(ReportCsv, *ReportPath))
	{
		UE_LOG(LogJargonHeroAspectAbilityMigration, Error, TEXT("Failed to write hero aspect ability migration report: %s"), *ReportPath);
		return 3;
	}

	UJargonAbilityAuditTool* AbilityAuditTool = NewObject<UJargonAbilityAuditTool>();
	if (AbilityAuditTool)
	{
		AbilityAuditTool->RunAbilityAudit();
	}

	UE_LOG(LogJargonHeroAspectAbilityMigration, Display, TEXT("Mage hero aspect ability migration complete. HookRows=%d Failures=%d PackagesSaved=%d Report=%s"),
		MigratedHookCount,
		FailureCount,
		PackagesToSave.Num(),
		*ReportPath);

	return FailureCount > 0 ? 1 : 0;
#else
	UE_LOG(LogTemp, Error, TEXT("JargonHeroAspectAbilityMigration commandlet is editor-only."));
	return 1;
#endif
}
