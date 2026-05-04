#include "Data/JargonAbilityContextMigrationCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Data/JargonAbilityDefinition.h"
#include "Data/JargonHeroDefinition.h"
#include "Data/JargonRelicDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "Data/JargonTileEffectDefinition.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogJargonAbilityContextMigration, Log, All);

namespace
{
	const FName JargonDataRoot(TEXT("/Game/Jargon/Data"));

	FString MakeCsvField(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	void AddCsvRow(TArray<FString>& Rows, const UObject* Object, const FString& FieldName, const FString& OldValue, const FString& NewValue, const FString& Reason)
	{
		const FString ObjectPath = Object ? Object->GetPathName() : TEXT("");
		Rows.Add(FString::Printf(TEXT("%s,%s,%s,%s,%s"),
			*MakeCsvField(ObjectPath),
			*MakeCsvField(FieldName),
			*MakeCsvField(OldValue),
			*MakeCsvField(NewValue),
			*MakeCsvField(Reason)));
	}

	void MarkObjectChanged(UObject* Object, TSet<UPackage*>& OutPackages)
	{
		if (!Object)
		{
			return;
		}

		Object->Modify();

		if (UPackage* Package = Object->GetOutermost())
		{
			Package->MarkPackageDirty();
			OutPackages.Add(Package);
		}
	}

	FString GetHookContextText(EJargonAbilityHookContextType ContextType)
	{
		return JargonEffectContracts::GetHookContextName(ContextType);
	}

	bool HookContextCanUseAbilityTargetTile(EJargonAbilityHookContextType ContextType)
	{
		const FJargonAbilityHookContextProfile Profile = FJargonAbilityHookContextProfile::FromContextType(ContextType);
		return Profile.bHasPrimaryTile || Profile.bHasPrimaryUnit || Profile.bHasTriggeringUnit;
	}

	EJargonAbilityPlacementAnchor GetPassiveSummonDefaultPlacement(EJargonAbilityHookContextType ContextType)
	{
		const FJargonAbilityHookContextProfile Profile = FJargonAbilityHookContextProfile::FromContextType(ContextType);
		if (Profile.bHasSourceTile)
		{
			return EJargonAbilityPlacementAnchor::NearestEmptyToSourceTile;
		}
		if (Profile.bHasOwningTileEffect)
		{
			return EJargonAbilityPlacementAnchor::NearestEmptyToOwningTile;
		}
		if (Profile.bHasTriggeringUnit)
		{
			return EJargonAbilityPlacementAnchor::NearestEmptyToTriggeringUnit;
		}
		return EJargonAbilityPlacementAnchor::AbilityTargetTile;
	}

	EJargonAbilityPlacementAnchor GetPassiveTileEffectDefaultPlacement(EJargonAbilityHookContextType ContextType)
	{
		const FJargonAbilityHookContextProfile Profile = FJargonAbilityHookContextProfile::FromContextType(ContextType);
		if (Profile.bHasSourceTile)
		{
			return EJargonAbilityPlacementAnchor::SourceTile;
		}
		if (Profile.bHasOwningTileEffect)
		{
			return EJargonAbilityPlacementAnchor::OwningTileEffectTile;
		}
		if (Profile.bHasTriggeringUnit)
		{
			return EJargonAbilityPlacementAnchor::TriggeringUnitTile;
		}
		return EJargonAbilityPlacementAnchor::AbilityTargetTile;
	}

	void ApplySpawnPlacementDefaults(UJargonAbilityDefinition* Ability, EJargonAbilityHookContextType ContextType, TSet<UPackage*>& OutPackages, TArray<FString>& Rows)
	{
		if (!Ability || HookContextCanUseAbilityTargetTile(ContextType))
		{
			return;
		}

		for (UJargonAbilityAction* Action : Ability->Actions)
		{
			if (UJargonAbilitySummonAction* SummonAction = Cast<UJargonAbilitySummonAction>(Action))
			{
				if (SummonAction->PlacementProfile.Anchor == EJargonAbilityPlacementAnchor::AbilityTargetTile)
				{
					const EJargonAbilityPlacementAnchor NewAnchor = GetPassiveSummonDefaultPlacement(ContextType);
					if (NewAnchor != EJargonAbilityPlacementAnchor::AbilityTargetTile)
					{
						MarkObjectChanged(SummonAction, OutPackages);
						const FString OldValue = JargonEffectContracts::GetPlacementAnchorName(SummonAction->PlacementProfile.Anchor);
						SummonAction->PlacementProfile.Anchor = NewAnchor;
						AddCsvRow(Rows, Ability, TEXT("Summon Placement"), OldValue, JargonEffectContracts::GetPlacementAnchorName(NewAnchor), TEXT("Passive summon hook has no selected tile; use nearest valid placement."));
					}
				}
			}
			else if (UJargonAbilityPlaceTileEffectAction* PlaceTileEffectAction = Cast<UJargonAbilityPlaceTileEffectAction>(Action))
			{
				if (PlaceTileEffectAction->PlacementProfile.Anchor == EJargonAbilityPlacementAnchor::AbilityTargetTile)
				{
					const EJargonAbilityPlacementAnchor NewAnchor = GetPassiveTileEffectDefaultPlacement(ContextType);
					if (NewAnchor != EJargonAbilityPlacementAnchor::AbilityTargetTile)
					{
						MarkObjectChanged(PlaceTileEffectAction, OutPackages);
						const FString OldValue = JargonEffectContracts::GetPlacementAnchorName(PlaceTileEffectAction->PlacementProfile.Anchor);
						PlaceTileEffectAction->PlacementProfile.Anchor = NewAnchor;
						AddCsvRow(Rows, Ability, TEXT("Tile Effect Placement"), OldValue, JargonEffectContracts::GetPlacementAnchorName(NewAnchor), TEXT("Passive tile-effect hook has no selected tile; use context placement."));
					}
				}
			}
		}
	}

	bool SetAbilityHookContext(UJargonAbilityDefinition* Ability, EJargonAbilityHookContextType ContextType, const FString& Reason, TSet<UPackage*>& OutPackages, TArray<FString>& Rows)
	{
		if (!Ability || ContextType == EJargonAbilityHookContextType::None)
		{
			return false;
		}

		bool bChanged = false;
		if (Ability->ExpectedHookContext != ContextType)
		{
			MarkObjectChanged(Ability, OutPackages);
			AddCsvRow(Rows, Ability, TEXT("ExpectedHookContext"), GetHookContextText(Ability->ExpectedHookContext), GetHookContextText(ContextType), Reason);
			Ability->ExpectedHookContext = ContextType;
			bChanged = true;
		}

		const int32 PreviousRowCount = Rows.Num();
		ApplySpawnPlacementDefaults(Ability, ContextType, OutPackages, Rows);
		bChanged |= Rows.Num() != PreviousRowCount;

		return bChanged;
	}

	bool TargetingProfileUsesPrimaryUnitTargeting(const FJargonAbilityTargetingProfile& TargetingProfile)
	{
		if (TargetingProfile.bUseCustomResolverTargeting)
		{
			return TargetingProfile.CustomDelivery == EJargonEffectDelivery::ExplicitUnit ||
				TargetingProfile.CustomDelivery == EJargonEffectDelivery::ChainUnits;
		}

		return TargetingProfile.Preset == EJargonAbilityTargetingPreset::SelectedEnemy ||
			TargetingProfile.Preset == EJargonAbilityTargetingPreset::SelectedAlly ||
			TargetingProfile.Preset == EJargonAbilityTargetingPreset::SelectedUnit ||
			TargetingProfile.Preset == EJargonAbilityTargetingPreset::ChainEnemies;
	}

	EJargonAbilityTargetingPreset GetRadiusPresetForTargetingProfile(const FJargonAbilityTargetingProfile& TargetingProfile)
	{
		if (TargetingProfile.bUseCustomResolverTargeting)
		{
			if (TargetingProfile.CustomTargetFilter == EJargonEffectTargetFilter::FriendlyToSource ||
				TargetingProfile.CustomTargetFilter == EJargonEffectTargetFilter::SourceOnly)
			{
				return EJargonAbilityTargetingPreset::AlliesInRadius;
			}
			if (TargetingProfile.CustomTargetFilter == EJargonEffectTargetFilter::EnemyToSource)
			{
				return EJargonAbilityTargetingPreset::EnemiesInRadius;
			}
			return EJargonAbilityTargetingPreset::UnitsInRadius;
		}

		if (TargetingProfile.Preset == EJargonAbilityTargetingPreset::SelectedAlly)
		{
			return EJargonAbilityTargetingPreset::AlliesInRadius;
		}
		if (TargetingProfile.Preset == EJargonAbilityTargetingPreset::SelectedUnit)
		{
			return EJargonAbilityTargetingPreset::UnitsInRadius;
		}
		return EJargonAbilityTargetingPreset::EnemiesInRadius;
	}

	void ApplyAuraTriggerTargetingDefaults(
		const UJargonTileEffectDefinition* TileEffect,
		TSet<UPackage*>& OutPackages,
		TArray<FString>& Rows)
	{
		if (!TileEffect ||
			TileEffect->Trigger != EJargonTileEffectTrigger::OnPlayerTurnStart ||
			!TileEffect->TriggerAbility)
		{
			return;
		}

		UJargonAbilityDefinition* Ability = TileEffect->TriggerAbility;

		if (TargetingProfileUsesPrimaryUnitTargeting(Ability->TargetingProfile))
		{
			MarkObjectChanged(Ability, OutPackages);

			const FString OldValue = Ability->TargetingProfile.GetSummary();
			const int32 PreviousRadius = Ability->TargetingProfile.bUseCustomResolverTargeting
				? Ability->TargetingProfile.CustomRadius
				: Ability->TargetingProfile.Radius;

			Ability->TargetingProfile.Preset = GetRadiusPresetForTargetingProfile(Ability->TargetingProfile);
			Ability->TargetingProfile.Radius = FMath::Max(TileEffect->EffectRadius, PreviousRadius);
			Ability->TargetingProfile.bUseCustomResolverTargeting = false;
			Ability->TargetingProfile.CustomDelivery = EJargonEffectDelivery::ExplicitUnit;
			Ability->TargetingProfile.CustomTargetFilter = EJargonEffectTargetFilter::Any;
			Ability->TargetingProfile.CustomRadius = 0;
			Ability->TargetingProfile.CustomChainCount = 3;

			AddCsvRow(Rows, Ability, TEXT("TargetingProfile"), OldValue, Ability->TargetingProfile.GetSummary(), TEXT("Aura-style player-turn-start trigger cannot use Primary/Triggering Unit targeting; use radius targeting around the tile effect."));
		}

		for (UJargonAbilityAction* Action : Ability->Actions)
		{
			if (!Action ||
				!Action->bOverrideTargetingProfile)
			{
				continue;
			}

			const bool bDestroyTileEffectAction = Action->IsA<UJargonAbilityDestroyTileEffectAction>();
			if (!bDestroyTileEffectAction && !TargetingProfileUsesPrimaryUnitTargeting(Action->TargetingOverride))
			{
				continue;
			}

			MarkObjectChanged(Action, OutPackages);

			const FString ActionOldValue = Action->TargetingOverride.GetSummary();
			if (bDestroyTileEffectAction)
			{
				Action->TargetingOverride.Preset = EJargonAbilityTargetingPreset::TargetTile;
				Action->TargetingOverride.Radius = 0;
				Action->TargetingOverride.ChainCount = 3;
				Action->TargetingOverride.bUseCustomResolverTargeting = false;
				Action->TargetingOverride.CustomDelivery = EJargonEffectDelivery::ExplicitUnit;
				Action->TargetingOverride.CustomTargetFilter = EJargonEffectTargetFilter::Any;
				Action->TargetingOverride.CustomRadius = 0;
				Action->TargetingOverride.CustomChainCount = 3;
				AddCsvRow(Rows, Ability, TEXT("Action Targeting Override"), ActionOldValue, Action->TargetingOverride.GetSummary(), TEXT("DestroyTileEffect on an aura-style trigger should target the owning/primary tile, not a triggering unit."));
			}
			else
			{
				Action->TargetingOverride.Preset = GetRadiusPresetForTargetingProfile(Action->TargetingOverride);
				Action->TargetingOverride.Radius = TileEffect->EffectRadius;
				Action->TargetingOverride.ChainCount = 3;
				Action->TargetingOverride.bUseCustomResolverTargeting = false;
				Action->TargetingOverride.CustomDelivery = EJargonEffectDelivery::ExplicitUnit;
				Action->TargetingOverride.CustomTargetFilter = EJargonEffectTargetFilter::Any;
				Action->TargetingOverride.CustomRadius = 0;
				Action->TargetingOverride.CustomChainCount = 3;
				AddCsvRow(Rows, Ability, TEXT("Action Targeting Override"), ActionOldValue, Action->TargetingOverride.GetSummary(), TEXT("Aura-style trigger effect line cannot use Primary/Triggering Unit targeting; use radius targeting around the tile effect."));
			}
		}
	}

	EJargonAbilityHookContextType InferContextFromAbilityPath(const FString& ObjectPath)
	{
		if (ObjectPath.Contains(TEXT("/Hero/Abilities/")))
		{
			if (ObjectPath.Contains(TEXT("CombatStart")))
			{
				return EJargonAbilityHookContextType::HeroClassCombatStart;
			}
			if (ObjectPath.Contains(TEXT("_Class_")) && ObjectPath.Contains(TEXT("TurnStart")))
			{
				return EJargonAbilityHookContextType::HeroClassTurnStart;
			}
			if (ObjectPath.Contains(TEXT("Transform")))
			{
				return EJargonAbilityHookContextType::HeroAspectTransformed;
			}
			if (ObjectPath.Contains(TEXT("EnemyDeath")))
			{
				return EJargonAbilityHookContextType::HeroAspectEnemyDeath;
			}
			if (ObjectPath.Contains(TEXT("TurnStart")))
			{
				return EJargonAbilityHookContextType::HeroAspectTurnStart;
			}
		}

		if (ObjectPath.Contains(TEXT("/SummonedDefinitions/Abilities/")))
		{
			if (ObjectPath.Contains(TEXT("OnSummoned")))
			{
				return EJargonAbilityHookContextType::SummonOnSummoned;
			}
			if (ObjectPath.Contains(TEXT("OnTurnStart")))
			{
				return EJargonAbilityHookContextType::SummonTurnStart;
			}
			if (ObjectPath.Contains(TEXT("OnDeath")))
			{
				return EJargonAbilityHookContextType::SummonDeath;
			}
		}

		if (ObjectPath.Contains(TEXT("/Relics/Abilities/")))
		{
			if (ObjectPath.Contains(TEXT("OnCombatStart")))
			{
				return EJargonAbilityHookContextType::BoonCombatStart;
			}
			if (ObjectPath.Contains(TEXT("OnPlayerTurnStart")))
			{
				return EJargonAbilityHookContextType::BoonPlayerTurnStart;
			}
			if (ObjectPath.Contains(TEXT("OnEnemyDeath")))
			{
				return EJargonAbilityHookContextType::BoonEnemyDeath;
			}
		}

		return EJargonAbilityHookContextType::None;
	}
}

UJargonAbilityContextMigrationCommandlet::UJargonAbilityContextMigrationCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UJargonAbilityContextMigrationCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	UE_LOG(LogJargonAbilityContextMigration, Display, TEXT("Starting Jargon ability context migration."));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().SearchAllAssets(true);

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(JargonDataRoot, AssetDataList, true);

	TArray<UJargonHeroDefinition*> Heroes;
	TArray<UJargonSummonedUnitDefinition*> Summons;
	TArray<UJargonTileEffectDefinition*> TileEffects;
	TArray<UJargonRelicDefinition*> Relics;
	TArray<UJargonAbilityDefinition*> Abilities;

	for (const FAssetData& AssetData : AssetDataList)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!Asset)
		{
			continue;
		}

		if (UJargonHeroDefinition* Hero = Cast<UJargonHeroDefinition>(Asset))
		{
			Heroes.Add(Hero);
		}
		else if (UJargonSummonedUnitDefinition* Summon = Cast<UJargonSummonedUnitDefinition>(Asset))
		{
			Summons.Add(Summon);
		}
		else if (UJargonTileEffectDefinition* TileEffect = Cast<UJargonTileEffectDefinition>(Asset))
		{
			TileEffects.Add(TileEffect);
		}
		else if (UJargonRelicDefinition* Relic = Cast<UJargonRelicDefinition>(Asset))
		{
			Relics.Add(Relic);
		}
		else if (UJargonAbilityDefinition* Ability = Cast<UJargonAbilityDefinition>(Asset))
		{
			Abilities.Add(Ability);
		}
	}

	TSet<UPackage*> PackagesToSave;
	TArray<FString> Rows;
	Rows.Add(TEXT("ObjectPath,Field,OldValue,NewValue,Reason"));

	for (const UJargonHeroDefinition* Hero : Heroes)
	{
		SetAbilityHookContext(Hero->CombatStartPassive.Ability, EJargonAbilityHookContextType::HeroClassCombatStart, TEXT("Referenced by hero combat-start passive."), PackagesToSave, Rows);
		SetAbilityHookContext(Hero->PlayerTurnStartPassive.Ability, EJargonAbilityHookContextType::HeroClassTurnStart, TEXT("Referenced by hero player-turn-start passive."), PackagesToSave, Rows);

		for (const FJargonHeroAspectDefinition& Aspect : Hero->HeroAspects)
		{
			SetAbilityHookContext(Aspect.TransformationAbility, EJargonAbilityHookContextType::HeroAspectTransformed, TEXT("Referenced by hero aspect transformation hook."), PackagesToSave, Rows);
			SetAbilityHookContext(Aspect.TurnStartAbility, EJargonAbilityHookContextType::HeroAspectTurnStart, TEXT("Referenced by hero aspect turn-start hook."), PackagesToSave, Rows);
			SetAbilityHookContext(Aspect.EnemyDeathAbility, EJargonAbilityHookContextType::HeroAspectEnemyDeath, TEXT("Referenced by hero aspect enemy-death hook."), PackagesToSave, Rows);
		}
	}

	for (const UJargonSummonedUnitDefinition* Summon : Summons)
	{
		SetAbilityHookContext(Summon->OnSummonedAbility, EJargonAbilityHookContextType::SummonOnSummoned, TEXT("Referenced by summon on-summoned hook."), PackagesToSave, Rows);
		SetAbilityHookContext(Summon->OnTurnStartAbility, EJargonAbilityHookContextType::SummonTurnStart, TEXT("Referenced by summon turn-start hook."), PackagesToSave, Rows);
		SetAbilityHookContext(Summon->OnDeathAbility, EJargonAbilityHookContextType::SummonDeath, TEXT("Referenced by summon death hook."), PackagesToSave, Rows);
	}

	for (const UJargonTileEffectDefinition* TileEffect : TileEffects)
	{
		const EJargonAbilityHookContextType ContextType = TileEffect->Trigger == EJargonTileEffectTrigger::OnPlayerTurnStart
			? EJargonAbilityHookContextType::AuraPlayerTurnStart
			: EJargonAbilityHookContextType::TrapUnitEnter;
		SetAbilityHookContext(TileEffect->TriggerAbility, ContextType, TEXT("Referenced by tile-effect trigger hook."), PackagesToSave, Rows);
		ApplyAuraTriggerTargetingDefaults(TileEffect, PackagesToSave, Rows);
	}

	for (const UJargonRelicDefinition* Relic : Relics)
	{
		SetAbilityHookContext(Relic->OnCombatStartAbility, EJargonAbilityHookContextType::BoonCombatStart, TEXT("Referenced by hero boon combat-start hook."), PackagesToSave, Rows);
		SetAbilityHookContext(Relic->OnPlayerTurnStartAbility, EJargonAbilityHookContextType::BoonPlayerTurnStart, TEXT("Referenced by hero boon player-turn-start hook."), PackagesToSave, Rows);
		SetAbilityHookContext(Relic->OnEnemyDeathAbility, EJargonAbilityHookContextType::BoonEnemyDeath, TEXT("Referenced by hero boon enemy-death hook."), PackagesToSave, Rows);
	}

	for (UJargonAbilityDefinition* Ability : Abilities)
	{
		if (!Ability || Ability->ExpectedHookContext != EJargonAbilityHookContextType::None)
		{
			continue;
		}

		const EJargonAbilityHookContextType InferredContext = InferContextFromAbilityPath(Ability->GetPathName());
		SetAbilityHookContext(Ability, InferredContext, TEXT("Inferred from migrated ability asset path/name."), PackagesToSave, Rows);
	}

	const FString OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("AbilityAudit"));
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);
	const FString ReportPath = FPaths::Combine(OutputDirectory, TEXT("AbilityContextMigration.csv"));
	FFileHelper::SaveStringArrayToFile(Rows, *ReportPath);
	UE_LOG(LogJargonAbilityContextMigration, Display, TEXT("Wrote ability context migration report: %s"), *ReportPath);

	if (PackagesToSave.Num() > 0)
	{
		TArray<UPackage*> PackageArray = PackagesToSave.Array();
		UE_LOG(LogJargonAbilityContextMigration, Display, TEXT("Saving %d changed package(s)."), PackageArray.Num());
		for (UPackage* Package : PackageArray)
		{
			if (!Package)
			{
				continue;
			}

			const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			SaveArgs.SaveFlags = SAVE_NoError;
			UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);
		}
	}
	else
	{
		UE_LOG(LogJargonAbilityContextMigration, Display, TEXT("No ability context assets needed changes."));
	}

	UE_LOG(LogJargonAbilityContextMigration, Display, TEXT("Finished Jargon ability context migration."));
	return 0;
#else
	UE_LOG(LogJargonAbilityContextMigration, Error, TEXT("Jargon ability context migration is editor-only."));
	return 1;
#endif
}
