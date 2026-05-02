#include "Data/JargonRuntimeShellAssignmentCommandlet.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Combat/Grid/Effects/AuraTileEffect.h"
#include "Combat/Grid/Effects/TrapTileEffect.h"
#include "Combat/Units/SummonedBattleUnit.h"
#include "Data/CardDefinition.h"
#include "Data/CardScriptDefinition.h"
#include "Data/JargonStatusEffectDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "Data/JargonTileEffectDefinition.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogJargonRuntimeShellAssignment, Log, All);

namespace
{
	const FString ReportDirectoryName(TEXT("CardCatalog"));
	const FString ReportFileName(TEXT("RuntimeShellAssignmentReport.csv"));

	struct FCardRuntimeShellSeed
	{
		const TCHAR* CardPath = TEXT("");
		const TCHAR* RuntimeClassPath = TEXT("");
	};

	const TArray<FCardRuntimeShellSeed>& GetSummonRuntimeShellSeeds()
	{
		static const TArray<FCardRuntimeShellSeed> Seeds = {
			{ TEXT("/Game/Jargon/Data/Cards/Summons/DA_Card_Summon_FireImp.DA_Card_Summon_FireImp"), TEXT("/Game/Jargon/Blueprints/Units/Summons/BP_FireImp.BP_FireImp_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Summons/DA_Card_Summon_MartyrSprite.DA_Card_Summon_MartyrSprite"), TEXT("/Game/Jargon/Blueprints/Units/Summons/BP_MartyrSprite.BP_MartyrSprite_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Summons/DA_Card_Summon_OgreMercenary.DA_Card_Summon_OgreMercenary"), TEXT("/Game/Jargon/Blueprints/Units/Summons/BP_OgreMercenary.BP_OgreMercenary_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Summons/DA_Card_Summon_PigeonProtector.DA_Card_Summon_PigeonProtector"), TEXT("/Game/Jargon/Blueprints/Units/Summons/BP_PigeonProtector.BP_PigeonProtector_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Summons/DA_Card_Summon_Reapsassin.DA_Card_Summon_Reapsassin"), TEXT("/Game/Jargon/Blueprints/Units/Summons/BP_Reapsassin.BP_Reapsassin_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Summons/DA_Card_Summon_ShieldGuardian.DA_Card_Summon_ShieldGuardian"), TEXT("/Game/Jargon/Blueprints/Units/Summons/BP_ShieldGuardian.BP_ShieldGuardian_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Summons/DA_Card_UnfriendlyYeti.DA_Card_UnfriendlyYeti"), TEXT("/Game/Jargon/Blueprints/Units/Summons/BP_Yeti.BP_Yeti_C") }
		};
		return Seeds;
	}

	const TArray<FCardRuntimeShellSeed>& GetTileRuntimeShellSeeds()
	{
		static const TArray<FCardRuntimeShellSeed> Seeds = {
			{ TEXT("/Game/Jargon/Data/Cards/Auras/DA_Card_AreaOfAegis.DA_Card_AreaOfAegis"), TEXT("/Game/Jargon/Blueprints/Actors/Aura/BP_EffectActor_AuraOfAegis.BP_EffectActor_AuraOfAegis_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Auras/DA_Card_Aura_HealingGrove.DA_Card_Aura_HealingGrove"), TEXT("/Game/Jargon/Blueprints/Actors/Aura/BP_EffectActor_HealingGrove.BP_EffectActor_HealingGrove_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Auras/DA_Card_Aura_StaticField.DA_Card_Aura_StaticField"), TEXT("/Game/Jargon/Blueprints/Actors/Aura/BP_EffectActor_StaticField.BP_EffectActor_StaticField_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Auras/DA_Card_RallyingBanner.DA_Card_RallyingBanner"), TEXT("/Game/Jargon/Blueprints/Actors/Aura/BP_EffectActor_RallyingBanner.BP_EffectActor_RallyingBanner_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Traps/DA_Card_Trap_SnareTrap.DA_Card_Trap_SnareTrap"), TEXT("/Game/Jargon/Blueprints/Actors/Trap/BP_EffectActor_SnareTrap.BP_EffectActor_SnareTrap_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Traps/DA_Card_Trap_SpikeTrap.DA_Card_Trap_SpikeTrap"), TEXT("/Game/Jargon/Blueprints/Actors/Trap/BP_EffectActor_SpikeTrap.BP_EffectActor_SpikeTrap_C") },
			{ TEXT("/Game/Jargon/Data/Cards/Traps/DA_Card_Trap_WildFire.DA_Card_Trap_WildFire"), TEXT("/Game/Jargon/Blueprints/Actors/BP_EffectActor_WildFire.BP_EffectActor_WildFire_C") }
		};
		return Seeds;
	}

	FString CsvEscape(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""), ESearchCase::CaseSensitive);
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	void AppendReportLine(
		FString& ReportCsv,
		const FString& RecordType,
		const FString& Subject,
		const FString& Status,
		const FString& Details)
	{
		ReportCsv += FString::Printf(
			TEXT("%s,%s,%s,%s") LINE_TERMINATOR,
			*CsvEscape(RecordType),
			*CsvEscape(Subject),
			*CsvEscape(Status),
			*CsvEscape(Details));
	}

	template <typename TClass>
	TSubclassOf<TClass> LoadRuntimeClass(const TCHAR* ClassPath)
	{
		UClass* LoadedClass = LoadObject<UClass>(nullptr, ClassPath);
		if (!LoadedClass)
		{
			return nullptr;
		}

		if (!LoadedClass->IsChildOf(TClass::StaticClass()))
		{
			return nullptr;
		}

		return LoadedClass;
	}

	UCardDefinition* LoadCard(const TCHAR* CardPath)
	{
		return Cast<UCardDefinition>(StaticLoadObject(UCardDefinition::StaticClass(), nullptr, CardPath));
	}

	template <typename TAction>
	TAction* FindFirstAction(UCardDefinition* Card)
	{
		if (!Card || !Card->CardScript)
		{
			return nullptr;
		}

		for (TObjectPtr<UJargonCardAction>& Action : Card->CardScript->Actions)
		{
			if (TAction* TypedAction = Cast<TAction>(Action.Get()))
			{
				return TypedAction;
			}
		}

		for (FJargonCardElementalBonusScript& Bonus : Card->CardScript->ElementalBonuses)
		{
			for (TObjectPtr<UJargonCardAction>& Action : Bonus.Actions)
			{
				if (TAction* TypedAction = Cast<TAction>(Action.Get()))
				{
					return TypedAction;
				}
			}
		}

		return nullptr;
	}

	bool AssignSummonRuntimeShell(
		const FCardRuntimeShellSeed& Seed,
		TArray<UPackage*>& PackagesToSave,
		FString& ReportCsv)
	{
		UCardDefinition* Card = LoadCard(Seed.CardPath);
		if (!Card)
		{
			AppendReportLine(ReportCsv, TEXT("SummonCard"), Seed.CardPath, TEXT("Failed"), TEXT("Could not load card."));
			return false;
		}

		UJargonCardSummonAction* Action = FindFirstAction<UJargonCardSummonAction>(Card);
		if (!Action)
		{
			AppendReportLine(ReportCsv, TEXT("SummonCard"), Seed.CardPath, TEXT("Skipped"), TEXT("No summon effect line found."));
			return true;
		}

		TSubclassOf<ASummonedBattleUnit> RuntimeClass = LoadRuntimeClass<ASummonedBattleUnit>(Seed.RuntimeClassPath);
		if (!RuntimeClass)
		{
			AppendReportLine(ReportCsv, TEXT("SummonCard"), Seed.CardPath, TEXT("Failed"), FString::Printf(TEXT("Could not load ASummonedBattleUnit runtime class %s."), Seed.RuntimeClassPath));
			return false;
		}

		if (Action->RuntimeSummonedUnitClass == RuntimeClass)
		{
			AppendReportLine(ReportCsv, TEXT("SummonCard"), Seed.CardPath, TEXT("Unchanged"), FString::Printf(TEXT("Runtime shell already assigned: %s."), Seed.RuntimeClassPath));
			return true;
		}

		Card->Modify();
		if (Card->CardScript)
		{
			Card->CardScript->Modify();
		}
		Action->Modify();
		Action->RuntimeSummonedUnitClass = RuntimeClass;
		Action->RefreshEditorTitle();
		Card->MarkPackageDirty();
		if (UPackage* Package = Card->GetOutermost())
		{
			PackagesToSave.AddUnique(Package);
		}

		AppendReportLine(ReportCsv, TEXT("SummonCard"), Seed.CardPath, TEXT("Assigned"), FString::Printf(TEXT("Assigned runtime shell %s."), Seed.RuntimeClassPath));
		return true;
	}

	bool AssignTileRuntimeShell(
		const FCardRuntimeShellSeed& Seed,
		TArray<UPackage*>& PackagesToSave,
		FString& ReportCsv)
	{
		UCardDefinition* Card = LoadCard(Seed.CardPath);
		if (!Card)
		{
			AppendReportLine(ReportCsv, TEXT("TileEffectCard"), Seed.CardPath, TEXT("Failed"), TEXT("Could not load card."));
			return false;
		}

		UJargonCardPlaceTileEffectAction* Action = FindFirstAction<UJargonCardPlaceTileEffectAction>(Card);
		if (!Action)
		{
			AppendReportLine(ReportCsv, TEXT("TileEffectCard"), Seed.CardPath, TEXT("Skipped"), TEXT("No place tile effect line found."));
			return true;
		}

		TSubclassOf<ABattleTileEffect> RuntimeClass = LoadRuntimeClass<ABattleTileEffect>(Seed.RuntimeClassPath);
		if (!RuntimeClass)
		{
			AppendReportLine(ReportCsv, TEXT("TileEffectCard"), Seed.CardPath, TEXT("Failed"), FString::Printf(TEXT("Could not load ABattleTileEffect runtime class %s."), Seed.RuntimeClassPath));
			return false;
		}

		if (Action->RuntimeTileEffectClass == RuntimeClass)
		{
			AppendReportLine(ReportCsv, TEXT("TileEffectCard"), Seed.CardPath, TEXT("Unchanged"), FString::Printf(TEXT("Runtime shell already assigned: %s."), Seed.RuntimeClassPath));
			return true;
		}

		Card->Modify();
		if (Card->CardScript)
		{
			Card->CardScript->Modify();
		}
		Action->Modify();
		Action->RuntimeTileEffectClass = RuntimeClass;
		Action->RefreshEditorTitle();
		Card->MarkPackageDirty();
		if (UPackage* Package = Card->GetOutermost())
		{
			PackagesToSave.AddUnique(Package);
		}

		AppendReportLine(ReportCsv, TEXT("TileEffectCard"), Seed.CardPath, TEXT("Assigned"), FString::Printf(TEXT("Assigned runtime shell %s."), Seed.RuntimeClassPath));
		return true;
	}

	UJargonStatusEffectDefinition* LoadStatusDefinition(const TCHAR* StatusAssetPath)
	{
		return Cast<UJargonStatusEffectDefinition>(StaticLoadObject(
			UJargonStatusEffectDefinition::StaticClass(),
			nullptr,
			StatusAssetPath));
	}

	void ClearUnusedRuntimePayload(FJargonEffectSpec& Effect)
	{
		Effect.ElementType = EJargonElementType::None;
		Effect.Radius = 0;
		Effect.ChainCount = 3;
		Effect.MoveDistance = 0;
		Effect.PushDistance = 1;
		Effect.PullDistance = 1;
		Effect.CollisionDamage = 1;
		Effect.SummonedUnitDefinition = nullptr;
		Effect.RuntimeSummonedUnitClass = nullptr;
		Effect.TileEffectDefinition = nullptr;
		Effect.RuntimeTileEffectClass = nullptr;
		Effect.TileEffectCategory = ECardCategory::Trap;
	}

	bool ConvertDirectStatusToDefinition(
		FJargonEffectSpec& Effect,
		EJargonEffectOperation DirectOperation,
		const TCHAR* StatusDefinitionPath,
		const TCHAR* DefinitionPath,
		const TCHAR* EffectLabel,
		FString& ReportCsv)
	{
		if (Effect.Operation != DirectOperation)
		{
			return false;
		}

		UJargonStatusEffectDefinition* StatusDefinition = LoadStatusDefinition(StatusDefinitionPath);
		if (!StatusDefinition)
		{
			AppendReportLine(
				ReportCsv,
				TEXT("EffectPayload"),
				DefinitionPath,
				TEXT("Failed"),
				FString::Printf(TEXT("%s could not load status definition %s."), EffectLabel, StatusDefinitionPath));
			return false;
		}

		const int32 PreservedAmount = FMath::Max(1, Effect.Value);
		Effect.Operation = EJargonEffectOperation::ApplyStatus;
		Effect.Delivery = EJargonEffectDelivery::ExplicitUnit;
		Effect.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
		Effect.Value = PreservedAmount;
		Effect.StatusEffectDefinition = StatusDefinition;
		ClearUnusedRuntimePayload(Effect);

		AppendReportLine(
			ReportCsv,
			TEXT("EffectPayload"),
			DefinitionPath,
			TEXT("Fixed"),
			FString::Printf(TEXT("%s converted direct status operation to ApplyStatus using %s."), EffectLabel, StatusDefinitionPath));
		return true;
	}

	bool CleanUnitEffectPayload(
		FJargonEffectSpec& Effect,
		const TCHAR* DefinitionPath,
		const TCHAR* EffectLabel,
		FString& ReportCsv)
	{
		bool bChanged = false;
		if (Effect.Radius != 0 ||
			Effect.TileEffectDefinition ||
			Effect.RuntimeTileEffectClass ||
			Effect.CollisionDamage != 1)
		{
			Effect.Radius = 0;
			Effect.TileEffectDefinition = nullptr;
			Effect.RuntimeTileEffectClass = nullptr;
			Effect.CollisionDamage = 1;
			bChanged = true;
		}

		if (bChanged)
		{
			AppendReportLine(
				ReportCsv,
				TEXT("EffectPayload"),
				DefinitionPath,
				TEXT("Fixed"),
				FString::Printf(TEXT("%s cleared ignored radius/tile-effect payload fields."), EffectLabel));
		}

		return bChanged;
	}

	bool FixTileEffectDefinitions(TArray<UPackage*>& PackagesToSave, FString& ReportCsv)
	{
		struct FTileDefinitionSeed
		{
			const TCHAR* DefinitionPath = TEXT("");
		};

		const TArray<FTileDefinitionSeed> Seeds = {
			{ TEXT("/Game/Jargon/Data/Cards/Traps/Definitions/DA_SnareTrapDefinition.DA_SnareTrapDefinition") },
			{ TEXT("/Game/Jargon/Data/Cards/Traps/Definitions/DA_SpikeTrapDefinition.DA_SpikeTrapDefinition") },
			{ TEXT("/Game/Jargon/Data/Cards/Traps/Definitions/DA_WildfireDefinition.DA_WildfireDefinition") }
		};

		bool bAllSucceeded = true;
		for (const FTileDefinitionSeed& Seed : Seeds)
		{
			UJargonTileEffectDefinition* Definition = Cast<UJargonTileEffectDefinition>(StaticLoadObject(
				UJargonTileEffectDefinition::StaticClass(),
				nullptr,
				Seed.DefinitionPath));
			if (!Definition)
			{
				AppendReportLine(ReportCsv, TEXT("TileEffectDefinition"), Seed.DefinitionPath, TEXT("Failed"), TEXT("Could not load tile effect definition."));
				bAllSucceeded = false;
				continue;
			}

			bool bChanged = false;
			for (int32 EffectIndex = 0; EffectIndex < Definition->Effects.Num(); ++EffectIndex)
			{
				FJargonEffectSpec& Effect = Definition->Effects[EffectIndex];
				const FString EffectLabel = FString::Printf(TEXT("Effect %d"), EffectIndex);

				if (Effect.Operation == EJargonEffectOperation::PlaceTileEffect)
				{
					UJargonStatusEffectDefinition* BurnDefinition = LoadStatusDefinition(TEXT("/Game/Jargon/Data/StatusEffects/DA_Status_Burn.DA_Status_Burn"));
					if (!BurnDefinition)
					{
						AppendReportLine(ReportCsv, TEXT("TileEffectDefinition"), Seed.DefinitionPath, TEXT("Failed"), TEXT("Nested PlaceTileEffect payload found, but DA_Status_Burn could not be loaded."));
						bAllSucceeded = false;
						continue;
					}

					Definition->Modify();
					const int32 PreservedAmount = FMath::Max(1, Effect.Value);
					Effect.Operation = EJargonEffectOperation::ApplyStatus;
					Effect.Delivery = EJargonEffectDelivery::ExplicitUnit;
					Effect.TargetFilter = EJargonEffectTargetFilter::EnemyToSource;
					Effect.Value = PreservedAmount;
					Effect.StatusEffectDefinition = BurnDefinition;
					ClearUnusedRuntimePayload(Effect);
					bChanged = true;
					AppendReportLine(ReportCsv, TEXT("TileEffectDefinition"), Seed.DefinitionPath, TEXT("Fixed"), TEXT("Replaced nested PlaceTileEffect payload with ApplyStatus Burn using the existing amount."));
					continue;
				}

				if (ConvertDirectStatusToDefinition(
					Effect,
					EJargonEffectOperation::ApplyRoot,
					TEXT("/Game/Jargon/Data/StatusEffects/DA_Status_Root.DA_Status_Root"),
					Seed.DefinitionPath,
					*EffectLabel,
					ReportCsv))
				{
					Definition->Modify();
					bChanged = true;
				}

				if (Effect.Operation == EJargonEffectOperation::DealDamage ||
					Effect.Operation == EJargonEffectOperation::ApplyStatus)
				{
					if (CleanUnitEffectPayload(Effect, Seed.DefinitionPath, *EffectLabel, ReportCsv))
					{
						Definition->Modify();
						bChanged = true;
					}
				}
			}

			if (!bChanged)
			{
				AppendReportLine(ReportCsv, TEXT("TileEffectDefinition"), Seed.DefinitionPath, TEXT("Unchanged"), TEXT("No stale tile-effect payload fields needed cleanup."));
				continue;
			}

			Definition->MarkPackageDirty();
			if (UPackage* Package = Definition->GetOutermost())
			{
				PackagesToSave.AddUnique(Package);
			}
		}

		return bAllSucceeded;
	}

	bool FixSummonDefinitionStatusPayloads(TArray<UPackage*>& PackagesToSave, FString& ReportCsv)
	{
		const TCHAR* DefinitionPath = TEXT("/Game/Jargon/Data/SummonedUnits/DA_SummonFireImp.DA_SummonFireImp");
		UJargonSummonedUnitDefinition* Definition = Cast<UJargonSummonedUnitDefinition>(StaticLoadObject(
			UJargonSummonedUnitDefinition::StaticClass(),
			nullptr,
			DefinitionPath));
		if (!Definition)
		{
			AppendReportLine(ReportCsv, TEXT("SummonDefinition"), DefinitionPath, TEXT("Failed"), TEXT("Could not load summon definition."));
			return false;
		}

		bool bChanged = false;
		for (int32 EffectIndex = 0; EffectIndex < Definition->OnSummonedEffects.Num(); ++EffectIndex)
		{
			if (ConvertDirectStatusToDefinition(
				Definition->OnSummonedEffects[EffectIndex],
				EJargonEffectOperation::ApplyBurn,
				TEXT("/Game/Jargon/Data/StatusEffects/DA_Status_Burn.DA_Status_Burn"),
				DefinitionPath,
				*FString::Printf(TEXT("OnSummonedEffects effect %d"), EffectIndex),
				ReportCsv))
			{
				Definition->Modify();
				bChanged = true;
			}
		}

		if (!bChanged)
		{
			AppendReportLine(ReportCsv, TEXT("SummonDefinition"), DefinitionPath, TEXT("Unchanged"), TEXT("No direct status payloads needed cleanup."));
			return true;
		}

		Definition->MarkPackageDirty();
		if (UPackage* Package = Definition->GetOutermost())
		{
			PackagesToSave.AddUnique(Package);
		}
		return true;
	}
}

#endif

UJargonRuntimeShellAssignmentCommandlet::UJargonRuntimeShellAssignmentCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UJargonRuntimeShellAssignmentCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	UE_LOG(LogJargonRuntimeShellAssignment, Display, TEXT("Starting runtime shell assignment cleanup."));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().SearchAllAssets(true);

	FString ReportCsv;
	ReportCsv += TEXT("RecordType,Subject,Status,Details") LINE_TERMINATOR;

	TArray<UPackage*> PackagesToSave;
	int32 FailureCount = 0;

	for (const FCardRuntimeShellSeed& Seed : GetSummonRuntimeShellSeeds())
	{
		if (!AssignSummonRuntimeShell(Seed, PackagesToSave, ReportCsv))
		{
			++FailureCount;
		}
	}

	for (const FCardRuntimeShellSeed& Seed : GetTileRuntimeShellSeeds())
	{
		if (!AssignTileRuntimeShell(Seed, PackagesToSave, ReportCsv))
		{
			++FailureCount;
		}
	}

	if (!FixTileEffectDefinitions(PackagesToSave, ReportCsv))
	{
		++FailureCount;
	}

	if (!FixSummonDefinitionStatusPayloads(PackagesToSave, ReportCsv))
	{
		++FailureCount;
	}

	if (PackagesToSave.Num() > 0)
	{
		const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
		if (!bSaved)
		{
			UE_LOG(LogJargonRuntimeShellAssignment, Error, TEXT("One or more runtime shell cleanup packages failed to save."));
			return 2;
		}
	}

	const FString OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), ReportDirectoryName);
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);
	const FString ReportPath = FPaths::Combine(OutputDirectory, ReportFileName);
	if (!FFileHelper::SaveStringToFile(ReportCsv, *ReportPath))
	{
		UE_LOG(LogJargonRuntimeShellAssignment, Error, TEXT("Failed to write runtime shell cleanup report: %s"), *ReportPath);
		return 3;
	}

	UE_LOG(LogJargonRuntimeShellAssignment, Display, TEXT("Runtime shell assignment cleanup complete. Failures=%d PackagesSaved=%d Report=%s"),
		FailureCount,
		PackagesToSave.Num(),
		*ReportPath);

	return FailureCount > 0 ? 1 : 0;
#else
	UE_LOG(LogTemp, Error, TEXT("JargonRuntimeShellAssignment commandlet is editor-only."));
	return 1;
#endif
}
