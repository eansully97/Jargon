#include "Data/JargonAbilityAuditTool.h"

#include "Combat/Effects/JargonEffectTypes.h"
#include "Data/JargonAbilityDefinition.h"
#include "Data/JargonHeroDefinition.h"
#include "Data/JargonArtifactDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "Data/JargonTileEffectDefinition.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogJargonAbilityAudit, Log, All);

namespace
{
const FName DefaultAbilityAuditScanPath(TEXT("/Game/Jargon/Data"));

FString CsvEscape(const FString& Value)
{
	FString Escaped = Value;
	Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));
	return FString::Printf(TEXT("\"%s\""), *Escaped);
}

FString EffectSummary(const TArray<FJargonEffectSpec>& Effects)
{
	TArray<FString> Parts;
	for (const FJargonEffectSpec& Effect : Effects)
	{
		Parts.Add(FString::Printf(
			TEXT("%s/%s/%s"),
			*JargonEffectContracts::GetOperationName(Effect.Operation),
			*JargonEffectContracts::GetDeliveryName(Effect.Delivery),
			*JargonEffectContracts::GetTargetFilterName(Effect.TargetFilter)));
	}

	return Parts.Num() > 0 ? FString::Join(Parts, TEXT(" | ")) : TEXT("None");
}

FString AbilityActionSummary(const UJargonAbilityDefinition* AbilityDefinition)
{
	return AbilityDefinition ? AbilityDefinition->GetAuditSummary() : TEXT("None");
}

FString GetAbilityPath(const UJargonAbilityDefinition* AbilityDefinition)
{
	return AbilityDefinition ? AbilityDefinition->GetPathName() : TEXT("");
}

FString AbilityPlacementSummary(const UJargonAbilityDefinition* AbilityDefinition)
{
	if (!AbilityDefinition)
	{
		return TEXT("");
	}

	TArray<FString> Placements;
	for (const TObjectPtr<UJargonAbilityAction>& Action : AbilityDefinition->Actions)
	{
		if (const UJargonAbilitySummonAction* SummonAction = Cast<UJargonAbilitySummonAction>(Action.Get()))
		{
			Placements.Add(FString::Printf(TEXT("Summon:%s"), *SummonAction->PlacementProfile.GetSummary()));
		}
		else if (const UJargonAbilityPlaceTileEffectAction* TileEffectAction = Cast<UJargonAbilityPlaceTileEffectAction>(Action.Get()))
		{
			Placements.Add(FString::Printf(TEXT("PlaceTileEffect:%s"), *TileEffectAction->PlacementProfile.GetSummary()));
		}
	}

	return Placements.Num() > 0 ? FString::Join(Placements, TEXT(" | ")) : TEXT("None");
}

FString AbilityContextWarnings(const UJargonAbilityDefinition* AbilityDefinition, EJargonAbilityHookContextType HookContextType)
{
	if (!AbilityDefinition)
	{
		return TEXT("");
	}

	TArray<FString> Warnings;
	if (AbilityDefinition->ExpectedHookContext == EJargonAbilityHookContextType::None)
	{
		Warnings.Add(TEXT("Ability ExpectedHookContext=None"));
	}
	else if (HookContextType != EJargonAbilityHookContextType::None &&
		AbilityDefinition->ExpectedHookContext != HookContextType)
	{
		Warnings.Add(FString::Printf(
			TEXT("Ability expects %s but hook is %s"),
			*JargonEffectContracts::GetHookContextName(AbilityDefinition->ExpectedHookContext),
			*JargonEffectContracts::GetHookContextName(HookContextType)));
	}

	if (AbilityDefinition->TargetingProfile.bUseCustomResolverTargeting)
	{
		Warnings.Add(TEXT("Advanced custom resolver targeting"));
	}

	return Warnings.Num() > 0 ? FString::Join(Warnings, TEXT(" | ")) : TEXT("None");
}

void AppendCsvLine(
	FString& Csv,
	const FString& RecordType,
	const FString& AssetPath,
	const FString& DisplayName,
	const FString& HookName,
	const UJargonAbilityDefinition* AbilityDefinition,
	const FString& Trigger,
	const FString& Targeting,
	EJargonAbilityHookContextType HookContextType,
	const TArray<FJargonEffectSpec>& RawEffects,
	const FString& Status,
	const FString& Notes)
{
	const FJargonAbilityHookContextProfile HookContextProfile =
		FJargonAbilityHookContextProfile::FromContextType(HookContextType);

	TArray<FString> Fields;
	Fields.Add(CsvEscape(RecordType));
	Fields.Add(CsvEscape(AssetPath));
	Fields.Add(CsvEscape(DisplayName));
	Fields.Add(CsvEscape(HookName));
	Fields.Add(CsvEscape(GetAbilityPath(AbilityDefinition)));
	Fields.Add(CsvEscape(Trigger));
	Fields.Add(CsvEscape(JargonEffectContracts::GetHookContextName(HookContextType)));
	Fields.Add(CsvEscape(HookContextProfile.GetAvailableRolesSummary()));
	Fields.Add(CsvEscape(Targeting));
	Fields.Add(CsvEscape(AbilityPlacementSummary(AbilityDefinition)));
	Fields.Add(CsvEscape(AbilityContextWarnings(AbilityDefinition, HookContextType)));
	Fields.Add(CsvEscape(AbilityActionSummary(AbilityDefinition)));
	Fields.Add(CsvEscape(FString::FromInt(RawEffects.Num())));
	Fields.Add(CsvEscape(EffectSummary(RawEffects)));
	Fields.Add(CsvEscape(Status));
	Fields.Add(CsvEscape(Notes));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendAbilityOnlyCsvLine(
	FString& Csv,
	const FString& RecordType,
	const FString& AssetPath,
	const FString& DisplayName,
	const FString& HookName,
	const UJargonAbilityDefinition* AbilityDefinition,
	const FString& Trigger,
	const FString& Targeting,
	EJargonAbilityHookContextType HookContextType,
	const FString& Status,
	const FString& Notes)
{
	const FJargonAbilityHookContextProfile HookContextProfile =
		FJargonAbilityHookContextProfile::FromContextType(HookContextType);

	TArray<FString> Fields;
	Fields.Add(CsvEscape(RecordType));
	Fields.Add(CsvEscape(AssetPath));
	Fields.Add(CsvEscape(DisplayName));
	Fields.Add(CsvEscape(HookName));
	Fields.Add(CsvEscape(GetAbilityPath(AbilityDefinition)));
	Fields.Add(CsvEscape(Trigger));
	Fields.Add(CsvEscape(JargonEffectContracts::GetHookContextName(HookContextType)));
	Fields.Add(CsvEscape(HookContextProfile.GetAvailableRolesSummary()));
	Fields.Add(CsvEscape(Targeting));
	Fields.Add(CsvEscape(AbilityPlacementSummary(AbilityDefinition)));
	Fields.Add(CsvEscape(AbilityContextWarnings(AbilityDefinition, HookContextType)));
	Fields.Add(CsvEscape(AbilityActionSummary(AbilityDefinition)));
	Fields.Add(CsvEscape(TEXT("")));
	Fields.Add(CsvEscape(TEXT("Ability-only hook")));
	Fields.Add(CsvEscape(Status));
	Fields.Add(CsvEscape(Notes));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

#if WITH_EDITOR
template <typename AssetType>
void FindAssetsByType(const TArray<FName>& ScanPaths, TArray<AssetType*>& OutAssets)
{
	OutAssets.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FString> PathStrings;
	PathStrings.Reserve(ScanPaths.Num());
	for (const FName& Path : ScanPaths)
	{
		PathStrings.Add(Path.ToString());
	}

	AssetRegistry.ScanPathsSynchronous(PathStrings, true);

	TSet<FString> SeenAssetPaths;
	for (const FName& Path : ScanPaths)
	{
		TArray<FAssetData> AssetDataList;
		AssetRegistry.GetAssetsByPath(Path, AssetDataList, true);

		for (const FAssetData& AssetData : AssetDataList)
		{
			AssetType* TypedAsset = Cast<AssetType>(AssetData.GetAsset());
			if (!TypedAsset)
			{
				continue;
			}

			const FString AssetPath = TypedAsset->GetPathName();
			if (SeenAssetPaths.Contains(AssetPath))
			{
				continue;
			}

			SeenAssetPaths.Add(AssetPath);
			OutAssets.Add(TypedAsset);
		}
	}

	OutAssets.Sort([](const AssetType& Left, const AssetType& Right)
	{
		return Left.GetPathName() < Right.GetPathName();
	});
}
#endif
}

UJargonAbilityAuditTool::UJargonAbilityAuditTool()
{
	ScanPaths.Add(DefaultAbilityAuditScanPath);
	OutputSubfolder = TEXT("AbilityAudit");
}

void UJargonAbilityAuditTool::RunAbilityAudit()
{
#if WITH_EDITOR
	TArray<UJargonAbilityDefinition*> AbilityDefinitions;
	TArray<UJargonHeroDefinition*> HeroDefinitions;
	TArray<UJargonSummonedUnitDefinition*> SummonDefinitions;
	TArray<UJargonTileEffectDefinition*> TileEffectDefinitions;
	TArray<UJargonArtifactDefinition*> ArtifactDefinitions;

	FindAssetsByType(ScanPaths, AbilityDefinitions);
	FindAssetsByType(ScanPaths, HeroDefinitions);
	FindAssetsByType(ScanPaths, SummonDefinitions);
	FindAssetsByType(ScanPaths, TileEffectDefinitions);
	FindAssetsByType(ScanPaths, ArtifactDefinitions);

	FString Csv;
	Csv += TEXT("RecordType,AssetPath,DisplayName,HookName,AbilityPath,Trigger,HookContext,AvailableRoles,TargetingSummary,PlacementSummary,ContextWarnings,AbilitySummary,RawEffectCount,RawEffectSummary,Status,Notes") LINE_TERMINATOR;

	for (const UJargonAbilityDefinition* AbilityDefinition : AbilityDefinitions)
	{
		TArray<FJargonEffectSpec> BuiltEffects;
		if (AbilityDefinition)
		{
			AbilityDefinition->BuildEffectSpecs(BuiltEffects);
		}

		AppendCsvLine(
			Csv,
			TEXT("AbilityDefinition"),
			AbilityDefinition ? AbilityDefinition->GetPathName() : TEXT(""),
			AbilityDefinition ? AbilityDefinition->DisplayName.ToString() : TEXT(""),
			TEXT("Definition"),
			AbilityDefinition,
			AbilityDefinition ? JargonEffectContracts::GetEnumTokenName(StaticEnum<EJargonEffectTrigger>(), static_cast<int64>(AbilityDefinition->ExpectedTrigger)) : TEXT(""),
			AbilityDefinition ? AbilityDefinition->TargetingProfile.GetSummary() : TEXT(""),
			AbilityDefinition ? AbilityDefinition->ExpectedHookContext : EJargonAbilityHookContextType::None,
			BuiltEffects,
			AbilityDefinition && AbilityDefinition->IsValidDefinition() ? TEXT("Valid") : TEXT("Invalid"),
			AbilityDefinition ? AbilityDefinition->CueDefinition.GetAuditSummary() : TEXT(""));
	}

	for (const UJargonHeroDefinition* HeroDefinition : HeroDefinitions)
	{
		if (!HeroDefinition)
		{
			continue;
		}

		AppendAbilityOnlyCsvLine(
			Csv,
			TEXT("HeroDefaultArtifact"),
			HeroDefinition->GetPathName(),
			HeroDefinition->DisplayName.ToString(),
			TEXT("DefaultClassArtifact"),
			nullptr,
			TEXT(""),
			TEXT(""),
			EJargonAbilityHookContextType::None,
			HeroDefinition->DefaultClassArtifact ? TEXT("Artifact") : TEXT("Empty"),
			HeroDefinition->DefaultClassArtifact
				? HeroDefinition->DefaultClassArtifact->GetPathName()
				: TEXT("No default class Artifact assigned."));

		for (int32 AspectIndex = 0; AspectIndex < HeroDefinition->HeroAspects.Num(); ++AspectIndex)
		{
			const FJargonHeroAspectDefinition& Aspect = HeroDefinition->HeroAspects[AspectIndex];
			const FString AspectLabel = FString::Printf(TEXT("Aspect[%d] %s"), AspectIndex, *Aspect.DisplayName.ToString());
			AppendAbilityOnlyCsvLine(Csv, TEXT("HeroAspect"), HeroDefinition->GetPathName(), AspectLabel, TEXT("Transformation"), Aspect.TransformationAbility, TEXT("Activated"), TEXT("Source/Primary Unit self"), EJargonAbilityHookContextType::HeroAspectTransformed, Aspect.TransformationAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Hero aspect hooks are ability-authored only."));
			AppendAbilityOnlyCsvLine(Csv, TEXT("HeroAspect"), HeroDefinition->GetPathName(), AspectLabel, TEXT("TurnStart"), Aspect.TurnStartAbility, TEXT("OnTurnStart"), TEXT("Source/Primary Unit self"), EJargonAbilityHookContextType::HeroAspectTurnStart, Aspect.TurnStartAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Hero aspect hooks are ability-authored only."));
			AppendAbilityOnlyCsvLine(Csv, TEXT("HeroAspect"), HeroDefinition->GetPathName(), AspectLabel, TEXT("EnemyDeath"), Aspect.EnemyDeathAbility, TEXT("OnEnemyDeath"), TEXT("Death tile / triggering enemy"), EJargonAbilityHookContextType::HeroAspectEnemyDeath, Aspect.EnemyDeathAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Hero aspect hooks are ability-authored only."));
		}
	}

	for (const UJargonSummonedUnitDefinition* SummonDefinition : SummonDefinitions)
	{
		if (!SummonDefinition)
		{
			continue;
		}

		AppendAbilityOnlyCsvLine(Csv, TEXT("Summon"), SummonDefinition->GetPathName(), SummonDefinition->DisplayName.ToString(), TEXT("OnSummoned"), SummonDefinition->OnSummonedAbility, TEXT("OnSummoned"), TEXT("Summoned unit source/primary"), EJargonAbilityHookContextType::SummonOnSummoned, SummonDefinition->OnSummonedAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Summon hooks are ability-authored only."));
		AppendAbilityOnlyCsvLine(Csv, TEXT("Summon"), SummonDefinition->GetPathName(), SummonDefinition->DisplayName.ToString(), TEXT("OnTurnStart"), SummonDefinition->OnTurnStartAbility, TEXT("OnTurnStart"), TEXT("Summoned unit source/primary"), EJargonAbilityHookContextType::SummonTurnStart, SummonDefinition->OnTurnStartAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Summon hooks are ability-authored only."));
		AppendAbilityOnlyCsvLine(Csv, TEXT("Summon"), SummonDefinition->GetPathName(), SummonDefinition->DisplayName.ToString(), TEXT("OnDeath"), SummonDefinition->OnDeathAbility, TEXT("OnDeath"), TEXT("Summon death tile"), EJargonAbilityHookContextType::SummonDeath, SummonDefinition->OnDeathAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Summon hooks are ability-authored only."));
	}

	for (const UJargonTileEffectDefinition* TileEffectDefinition : TileEffectDefinitions)
	{
		if (!TileEffectDefinition)
		{
			continue;
		}

		const FString Trigger = TileEffectDefinition->Trigger == EJargonTileEffectTrigger::OnPlayerTurnStart
			? TEXT("OnTurnStart")
			: TEXT("OnEnterTile");
		const EJargonAbilityHookContextType HookContextType = TileEffectDefinition->Trigger == EJargonTileEffectTrigger::OnPlayerTurnStart
			? EJargonAbilityHookContextType::AuraPlayerTurnStart
			: EJargonAbilityHookContextType::TrapUnitEnter;
		AppendAbilityOnlyCsvLine(Csv, TEXT("TileEffect"), TileEffectDefinition->GetPathName(), TileEffectDefinition->DisplayName.ToString(), TEXT("Trigger"), TileEffectDefinition->TriggerAbility, Trigger, TEXT("Placed tile effect"), HookContextType, TileEffectDefinition->TriggerAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Tile-effect hooks are ability-authored only."));
	}

	for (const UJargonArtifactDefinition* ArtifactDefinition : ArtifactDefinitions)
	{
		if (!ArtifactDefinition)
		{
			continue;
		}

		AppendAbilityOnlyCsvLine(Csv, TEXT("Artifact"), ArtifactDefinition->GetPathName(), ArtifactDefinition->DisplayName.ToString(), TEXT("OnCombatStart"), ArtifactDefinition->OnCombatStartAbility, TEXT("OnCombatStart"), TEXT("Player unit"), EJargonAbilityHookContextType::ArtifactCombatStart, ArtifactDefinition->OnCombatStartAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Hero artifact hooks are ability-authored only."));
		AppendAbilityOnlyCsvLine(Csv, TEXT("Artifact"), ArtifactDefinition->GetPathName(), ArtifactDefinition->DisplayName.ToString(), TEXT("OnPlayerTurnStart"), ArtifactDefinition->OnPlayerTurnStartAbility, TEXT("OnTurnStart"), TEXT("Player unit"), EJargonAbilityHookContextType::ArtifactPlayerTurnStart, ArtifactDefinition->OnPlayerTurnStartAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Hero artifact hooks are ability-authored only."));
		AppendAbilityOnlyCsvLine(Csv, TEXT("Artifact"), ArtifactDefinition->GetPathName(), ArtifactDefinition->DisplayName.ToString(), TEXT("OnEnemyDeath"), ArtifactDefinition->OnEnemyDeathAbility, TEXT("OnEnemyDeath"), TEXT("Enemy death tile"), EJargonAbilityHookContextType::ArtifactEnemyDeath, ArtifactDefinition->OnEnemyDeathAbility ? TEXT("Ability") : TEXT("Empty"), TEXT("Hero artifact hooks are ability-authored only."));
	}

	UE_LOG(LogJargonAbilityAudit, Display, TEXT("Ability audit scanned Abilities=%d Heroes=%d Summons=%d TileEffects=%d Artifacts=%d"),
		AbilityDefinitions.Num(),
		HeroDefinitions.Num(),
		SummonDefinitions.Num(),
		TileEffectDefinitions.Num(),
		ArtifactDefinitions.Num());

	if (bExportCsvReport)
	{
		const FString OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), OutputSubfolder);
		IFileManager::Get().MakeDirectory(*OutputDirectory, true);
		const FString CsvPath = FPaths::Combine(OutputDirectory, TEXT("AbilityAudit.csv"));
		if (FFileHelper::SaveStringToFile(Csv, *CsvPath))
		{
			UE_LOG(LogJargonAbilityAudit, Display, TEXT("Wrote ability audit CSV: %s"), *CsvPath);
		}
		else
		{
			UE_LOG(LogJargonAbilityAudit, Error, TEXT("Failed to write ability audit CSV: %s"), *CsvPath);
		}
	}
#else
	UE_LOG(LogJargonAbilityAudit, Warning, TEXT("Ability Audit is editor-only and cannot scan assets in this build."));
#endif
}
