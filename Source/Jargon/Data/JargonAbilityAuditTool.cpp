#include "Data/JargonAbilityAuditTool.h"

#include "Combat/Effects/JargonEffectTypes.h"
#include "Data/JargonAbilityDefinition.h"
#include "Data/JargonHeroDefinition.h"
#include "Data/JargonRelicDefinition.h"
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

void AppendCsvLine(
	FString& Csv,
	const FString& RecordType,
	const FString& AssetPath,
	const FString& DisplayName,
	const FString& HookName,
	const UJargonAbilityDefinition* AbilityDefinition,
	const FString& Trigger,
	const FString& Targeting,
	const TArray<FJargonEffectSpec>& RawEffects,
	const FString& Status,
	const FString& Notes)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(RecordType));
	Fields.Add(CsvEscape(AssetPath));
	Fields.Add(CsvEscape(DisplayName));
	Fields.Add(CsvEscape(HookName));
	Fields.Add(CsvEscape(GetAbilityPath(AbilityDefinition)));
	Fields.Add(CsvEscape(Trigger));
	Fields.Add(CsvEscape(Targeting));
	Fields.Add(CsvEscape(AbilityActionSummary(AbilityDefinition)));
	Fields.Add(CsvEscape(FString::FromInt(RawEffects.Num())));
	Fields.Add(CsvEscape(EffectSummary(RawEffects)));
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
	TArray<UJargonRelicDefinition*> RelicDefinitions;

	FindAssetsByType(ScanPaths, AbilityDefinitions);
	FindAssetsByType(ScanPaths, HeroDefinitions);
	FindAssetsByType(ScanPaths, SummonDefinitions);
	FindAssetsByType(ScanPaths, TileEffectDefinitions);
	FindAssetsByType(ScanPaths, RelicDefinitions);

	FString Csv;
	Csv += TEXT("RecordType,AssetPath,DisplayName,HookName,AbilityPath,Trigger,Targeting,AbilitySummary,RawEffectCount,RawEffectSummary,Status,Notes") LINE_TERMINATOR;

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

		AppendCsvLine(Csv, TEXT("Hero"), HeroDefinition->GetPathName(), HeroDefinition->DisplayName.ToString(), TEXT("CombatStartPassive"), HeroDefinition->CombatStartPassive.Ability, TEXT("OnCombatStart"), TEXT("Unit self"), HeroDefinition->CombatStartPassive.Effects, HeroDefinition->CombatStartPassive.Ability ? TEXT("Ability") : TEXT("Raw"), HeroDefinition->CombatStartPassive.Ability && HeroDefinition->CombatStartPassive.Effects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
		AppendCsvLine(Csv, TEXT("Hero"), HeroDefinition->GetPathName(), HeroDefinition->DisplayName.ToString(), TEXT("PlayerTurnStartPassive"), HeroDefinition->PlayerTurnStartPassive.Ability, TEXT("OnTurnStart"), TEXT("Unit self"), HeroDefinition->PlayerTurnStartPassive.Effects, HeroDefinition->PlayerTurnStartPassive.Ability ? TEXT("Ability") : TEXT("Raw"), HeroDefinition->PlayerTurnStartPassive.Ability && HeroDefinition->PlayerTurnStartPassive.Effects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));

		for (int32 AspectIndex = 0; AspectIndex < HeroDefinition->HeroAspects.Num(); ++AspectIndex)
		{
			const FJargonHeroAspectDefinition& Aspect = HeroDefinition->HeroAspects[AspectIndex];
			const FString AspectLabel = FString::Printf(TEXT("Aspect[%d] %s"), AspectIndex, *Aspect.DisplayName.ToString());
			AppendCsvLine(Csv, TEXT("HeroAspect"), HeroDefinition->GetPathName(), AspectLabel, TEXT("Transformation"), Aspect.TransformationAbility, TEXT("Activated"), TEXT("Unit self"), Aspect.TransformationEffects, Aspect.TransformationAbility ? TEXT("Ability") : TEXT("Raw"), Aspect.TransformationAbility && Aspect.TransformationEffects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
			AppendCsvLine(Csv, TEXT("HeroAspect"), HeroDefinition->GetPathName(), AspectLabel, TEXT("TurnStart"), Aspect.TurnStartAbility, TEXT("OnTurnStart"), TEXT("Unit self"), Aspect.TurnStartEffects, Aspect.TurnStartAbility ? TEXT("Ability") : TEXT("Raw"), Aspect.TurnStartAbility && Aspect.TurnStartEffects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
			AppendCsvLine(Csv, TEXT("HeroAspect"), HeroDefinition->GetPathName(), AspectLabel, TEXT("EnemyDeath"), Aspect.EnemyDeathAbility, TEXT("OnEnemyDeath"), TEXT("Death tile/triggering enemy"), Aspect.EnemyDeathEffects, Aspect.EnemyDeathAbility ? TEXT("Ability") : TEXT("Raw"), Aspect.EnemyDeathAbility && Aspect.EnemyDeathEffects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
		}
	}

	for (const UJargonSummonedUnitDefinition* SummonDefinition : SummonDefinitions)
	{
		if (!SummonDefinition)
		{
			continue;
		}

		AppendCsvLine(Csv, TEXT("Summon"), SummonDefinition->GetPathName(), SummonDefinition->DisplayName.ToString(), TEXT("OnSummoned"), SummonDefinition->OnSummonedAbility, TEXT("OnSummoned"), TEXT("Summoned unit"), SummonDefinition->OnSummonedEffects, SummonDefinition->OnSummonedAbility ? TEXT("Ability") : TEXT("Raw"), SummonDefinition->OnSummonedAbility && SummonDefinition->OnSummonedEffects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
		AppendCsvLine(Csv, TEXT("Summon"), SummonDefinition->GetPathName(), SummonDefinition->DisplayName.ToString(), TEXT("OnTurnStart"), SummonDefinition->OnTurnStartAbility, TEXT("OnTurnStart"), TEXT("Summoned unit"), SummonDefinition->OnTurnStartEffects, SummonDefinition->OnTurnStartAbility ? TEXT("Ability") : TEXT("Raw"), SummonDefinition->OnTurnStartAbility && SummonDefinition->OnTurnStartEffects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
		AppendCsvLine(Csv, TEXT("Summon"), SummonDefinition->GetPathName(), SummonDefinition->DisplayName.ToString(), TEXT("OnDeath"), SummonDefinition->OnDeathAbility, TEXT("OnDeath"), TEXT("Summoned unit death tile"), SummonDefinition->OnDeathEffects, SummonDefinition->OnDeathAbility ? TEXT("Ability") : TEXT("Raw"), SummonDefinition->OnDeathAbility && SummonDefinition->OnDeathEffects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
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
		AppendCsvLine(Csv, TEXT("TileEffect"), TileEffectDefinition->GetPathName(), TileEffectDefinition->DisplayName.ToString(), TEXT("Trigger"), TileEffectDefinition->TriggerAbility, Trigger, TEXT("Placed tile effect"), TileEffectDefinition->Effects, TileEffectDefinition->TriggerAbility ? TEXT("Ability") : TEXT("Raw"), TileEffectDefinition->TriggerAbility && TileEffectDefinition->Effects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
	}

	for (const UJargonRelicDefinition* RelicDefinition : RelicDefinitions)
	{
		if (!RelicDefinition)
		{
			continue;
		}

		AppendCsvLine(Csv, TEXT("HeroBoon"), RelicDefinition->GetPathName(), RelicDefinition->DisplayName.ToString(), TEXT("OnCombatStart"), RelicDefinition->OnCombatStartAbility, TEXT("OnCombatStart"), TEXT("Player unit"), RelicDefinition->OnCombatStartEffects, RelicDefinition->OnCombatStartAbility ? TEXT("Ability") : TEXT("Raw"), RelicDefinition->OnCombatStartAbility && RelicDefinition->OnCombatStartEffects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
		AppendCsvLine(Csv, TEXT("HeroBoon"), RelicDefinition->GetPathName(), RelicDefinition->DisplayName.ToString(), TEXT("OnPlayerTurnStart"), RelicDefinition->OnPlayerTurnStartAbility, TEXT("OnTurnStart"), TEXT("Player unit"), RelicDefinition->OnPlayerTurnStartEffects, RelicDefinition->OnPlayerTurnStartAbility ? TEXT("Ability") : TEXT("Raw"), RelicDefinition->OnPlayerTurnStartAbility && RelicDefinition->OnPlayerTurnStartEffects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
		AppendCsvLine(Csv, TEXT("HeroBoon"), RelicDefinition->GetPathName(), RelicDefinition->DisplayName.ToString(), TEXT("OnEnemyDeath"), RelicDefinition->OnEnemyDeathAbility, TEXT("OnEnemyDeath"), TEXT("Enemy death tile"), RelicDefinition->OnEnemyDeathEffects, RelicDefinition->OnEnemyDeathAbility ? TEXT("Ability") : TEXT("Raw"), RelicDefinition->OnEnemyDeathAbility && RelicDefinition->OnEnemyDeathEffects.Num() > 0 ? TEXT("Both ability and raw effects authored") : TEXT(""));
	}

	UE_LOG(LogJargonAbilityAudit, Display, TEXT("Ability audit scanned Abilities=%d Heroes=%d Summons=%d TileEffects=%d HeroBoons=%d"),
		AbilityDefinitions.Num(),
		HeroDefinitions.Num(),
		SummonDefinitions.Num(),
		TileEffectDefinitions.Num(),
		RelicDefinitions.Num());

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
