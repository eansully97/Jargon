#include "Data/CardCatalogAuditTool.h"

#include "Core/JargonRunStateTypes.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Units/BattleUnit.h"
#include "Combat/Units/SummonedBattleUnit.h"
#include "Data/CardDefinition.h"
#include "Data/CardScriptDefinition.h"
#include "Data/CardPackDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "Data/JargonStatusEffectDefinition.h"
#include "Data/JargonTileEffectDefinition.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCardCatalogAudit, Log, All);

namespace
{
const FName DefaultCardScanPath(TEXT("/Game/Jargon/Data/Cards"));
const FName LegacyCardScanPath(TEXT("/Game/Jargon/Cards"));
const FName DefaultPackScanPath(TEXT("/Game/Jargon/Data/CardPacks"));
const FName LegacyPackScanPath(TEXT("/Game/Jargon/CardPacks"));
const FName DefaultSummonedUnitScanPath(TEXT("/Game/Jargon/Data/Cards/SummonedDefinitions"));
const FName LegacySummonedUnitScanPath(TEXT("/Game/Jargon/Data/SummonedUnits"));

#if WITH_EDITOR
struct FCardPackUsage
{
	FString PackName;
	int32 Weight = 0;
};

struct FCardAuditRow
{
	FString AssetPath;
	FString AssetName;
	FString DisplayName;
	FString Category;
	FString CardElement;
	FString TargetType;
	int32 Cost = 0;
	int32 Range = 0;
	bool bHasArt = false;
	bool bDescriptionEmpty = false;
	int32 EffectsCount = 0;
	FString EffectsSummary;
	FString PrimaryOperation;
	bool bUsedInPacks = false;
	FString PackSummary;
	FString ValidationStatus;
	TArray<FString> Warnings;
	bool bHasFatalWarnings = false;
};

struct FPackAuditRow
{
	FString AssetPath;
	FString PackName;
	int32 PriceCopper = 0;
	int32 AmountToGrant = 0;
	int32 CardPoolCount = 0;
	int32 ValidEntryCount = 0;
	int32 TotalValidWeight = 0;
	bool bCanRollValidCard = false;
	TArray<FString> Warnings;
};

struct FSummonAuditRow
{
	FString AssetPath;
	FString AssetName;
	FString DisplayName;
	int32 MaxHP = 0;
	int32 MoveRange = 0;
	int32 AttackRange = 0;
	int32 AttackDamage = 0;
	FString Team;
	int32 OnSummonedEffectsCount = 0;
	int32 OnTurnStartEffectsCount = 0;
	int32 OnDeathEffectsCount = 0;
	bool bIsValidDefinition = false;
	bool bScannedByPath = false;
	bool bReferencedByScannedCards = false;
	FString Summary;
	TArray<FString> Warnings;
};

struct FCardBalanceAuditRow
{
	FString AssetPath;
	FString AssetName;
	FString DisplayName;
	FString Category;
	FString TargetType;
	int32 Cost = 0;
	int32 BaseBalanceBudget = 0;
	int32 ElementalBonusBudget = 0;
	int32 ExpectedCostMin = 0;
	int32 ExpectedCostMax = 0;
	int32 SuggestedCost = 0;
	FString BalanceStatus;
	FString RoleTags;
	FString ElementTags;
	FString ProductionStatus;
	FString PackStatus;
	FString EffectsSummary;
	TArray<FString> Warnings;
};

struct FCardDescriptionSuggestionRow
{
	FString AssetPath;
	FString AssetName;
	FString DisplayName;
	FString CurrentDescription;
	FString SuggestedDescription;
	FString Status;
	TArray<FString> Warnings;
};

struct FCardVarietyMatrixRow
{
	FString Element;
	FString Role;
	FString Category;
	FString ProductionStatus;
	FString PackStatus;
	int32 CardCount = 0;
	TArray<FString> CardNames;
};

struct FCardDesignAuditRow
{
	FString AssetPath;
	FString AssetName;
	FString DisplayName;
	FString Category;
	int32 Cost = 0;
	FString ElementTags;
	FString RoleTags;
	FString PrimaryKeyword;
	FString SecondaryKeywords;
	FString OperationSummary;
	FString DeliverySummary;
	FString TargetFilterSummary;
	FString PayloadSummary;
	FString ConditionSummary;
	FString KeywordTerms;
	FString SuspiciousPattern;
	FString SamenessFingerprint;
	int32 MatchingFingerprintCount = 0;
	int32 TacticalScore = 0;
	FString DesignStatus;
	TArray<FString> Warnings;
};

struct FElementIdentityAuditRow
{
	FString Element;
	int32 ProductionCards = 0;
	int32 Generators = 0;
	int32 Payoffs = 0;
	int32 DamageCards = 0;
	int32 DefenseUtilityCards = 0;
	int32 TacticalCards = 0;
	FString UniquePrimaryKeywords;
	FString MissingLanes;
	FString RecommendedActions;
};

struct FPackExperienceAuditRow
{
	FString PackAssetPath;
	FString PackName;
	int32 CardCount = 0;
	int32 UniqueRoles = 0;
	int32 UniqueElements = 0;
	int32 UniquePrimaryKeywords = 0;
	float AverageCost = 0.f;
	int32 RepeatedFingerprintGroups = 0;
	FString RoleSpread;
	FString ElementMix;
	FString CostCurve;
	FString ExperienceStatus;
	TArray<FString> Warnings;
};

struct FCardRewritePlanRow
{
	FString AssetPath;
	FString DisplayName;
	FString CurrentIssue;
	FString RecommendedAction;
	FString ProposedRulesText;
};

struct FCardElementCoverageAuditRow
{
	FString Element;
	int32 ProductionCards = 0;
	int32 SpellCards = 0;
	int32 SummonCards = 0;
	int32 TrapCards = 0;
	int32 AuraCards = 0;
	int32 DamageCards = 0;
	int32 DefenseCards = 0;
	int32 HealingCards = 0;
	int32 ControlCards = 0;
	int32 MobilityCards = 0;
	int32 DrawCards = 0;
	int32 EnergyCards = 0;
	int32 Generators = 0;
	int32 Payoffs = 0;
	int32 TacticalCards = 0;
	int32 UniqueOperations = 0;
	FString StatusTerms;
	FString CardNames;
	FString MissingCardTypes;
	FString MissingRoles;
	FString CoverageStatus;
	FString RecommendedFocus;
};

struct FElementPackGapAuditRow
{
	FString PackAssetPath;
	FString PackName;
	FString DetectedElement;
	int32 NeutralSupportCount = 0;
	int32 NeutralSupportWeight = 0;
	int32 ElementPoolCount = 0;
	int32 ElementPoolWeight = 0;
	int32 OtherElementCount = 0;
	int32 DebugCardCount = 0;
	int32 SpellCards = 0;
	int32 SummonCards = 0;
	int32 TrapCards = 0;
	int32 AuraCards = 0;
	FString RoleSpread;
	FString ElementMix;
	FString RepeatedFingerprintGroups;
	FString MissingCardTypes;
	FString MissingRoles;
	FString PackStatus;
	FString Recommendations;
};

struct FCardContentRecommendationRow
{
	FString Priority;
	FString Element;
	FString SuggestedCardType;
	FString RoleFilled;
	FString Reason;
	FString IntendedOperation;
	FString Delivery;
	FString Payload;
	FString DraftRulesText;
	FString CardArtPrompt;
	FString SourceReport;
};

struct FEffectVocabularyFitAuditRow
{
	FString Scope;
	int32 ProductionCards = 0;
	int32 TotalEffectLines = 0;
	int32 UniqueOperations = 0;
	int32 UniqueDeliveries = 0;
	FString StructuralCoverageStatus;
	FString MechanicalVarietyStatus;
	FString OperationMix;
	FString DeliveryMix;
	FString StatusTerms;
	FString DominantOperation;
	FString RecommendedFirstBatch;
	FString Notes;
};

struct FCardBalanceEstimate
{
	int32 BaseBudget = 0;
	int32 ElementalBonusBudget = 0;
	int32 ExpectedCostMin = 0;
	int32 ExpectedCostMax = 0;
	int32 SuggestedCost = 0;
	TArray<FString> Warnings;
};

struct FCardAuditElementalBonusGroup
{
	EJargonElementType ElementType = EJargonElementType::None;
	int32 RequiredCharges = 0;
	bool bSpendCharges = false;
	TArray<FJargonEffectSpec> BonusEffects;
};

FString BoolToYesNo(bool bValue)
{
	return bValue ? TEXT("Yes") : TEXT("No");
}

FString TextOrFallbackName(const FText& Text, const UObject* Object)
{
	const FString TextString = Text.ToString().TrimStartAndEnd();
	return TextString.IsEmpty() ? GetNameSafe(Object) : TextString;
}

FString GetAuditEnumDisplayName(const UEnum* Enum, int64 Value)
{
	if (!Enum)
	{
		return FString::FromInt(static_cast<int32>(Value));
	}

	return Enum->GetDisplayNameTextByValue(Value).ToString();
}

FString GetCardCategoryName(ECardCategory Category)
{
	return GetAuditEnumDisplayName(StaticEnum<ECardCategory>(), static_cast<int64>(Category));
}

FString GetCardTargetTypeName(ECardTargetType TargetType)
{
	return GetAuditEnumDisplayName(StaticEnum<ECardTargetType>(), static_cast<int64>(TargetType));
}

FString GetCardEffectOperationName(EJargonEffectOperation Operation)
{
	return GetAuditEnumDisplayName(StaticEnum<EJargonEffectOperation>(), static_cast<int64>(Operation));
}

FString GetElementTypeName(EJargonElementType ElementType)
{
	if (ElementType == EJargonElementType::None)
	{
		return TEXT("Neutral");
	}

	return GetAuditEnumDisplayName(StaticEnum<EJargonElementType>(), static_cast<int64>(ElementType));
}

FString GetTeamName(ETeam Team)
{
	return GetAuditEnumDisplayName(StaticEnum<ETeam>(), static_cast<int64>(Team));
}

FString GetClassDisplayName(const UClass* Class)
{
	return Class ? Class->GetName() : TEXT("None");
}

FString GetObjectPathOrNone(const UObject* Object)
{
	return Object ? Object->GetPathName() : TEXT("None");
}

FString JoinStrings(const TArray<FString>& Values, const TCHAR* Separator = TEXT(" | "))
{
	return Values.Num() > 0 ? FString::Join(Values, Separator) : FString();
}

FString CsvEscape(const FString& Value)
{
	FString Escaped = Value;
	Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""), ESearchCase::CaseSensitive);

	if (Escaped.Contains(TEXT(",")) || Escaped.Contains(TEXT("\"")) || Escaped.Contains(TEXT("\n")) || Escaped.Contains(TEXT("\r")))
	{
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	return Escaped;
}

FString CsvEscapeInt(int32 Value)
{
	return FString::FromInt(Value);
}

FString CsvEscapeBool(bool bValue)
{
	return BoolToYesNo(bValue);
}

TArray<FName> GetEffectiveScanPaths(const TArray<FName>& ConfiguredPaths, const TArray<FName>& FallbackPaths)
{
	TArray<FName> EffectivePaths;
	TSet<FName> SeenPaths;

	const TArray<FName>& SourcePaths = ConfiguredPaths.Num() > 0 ? ConfiguredPaths : FallbackPaths;
	for (const FName& Path : SourcePaths)
	{
		if (!Path.IsNone() && !SeenPaths.Contains(Path))
		{
			EffectivePaths.Add(Path);
			SeenPaths.Add(Path);
		}
	}

	return EffectivePaths;
}

template <typename AssetType>
void LoadAssetsFromPaths(const TArray<FName>& ScanPaths, TArray<AssetType*>& OutAssets)
{
	OutAssets.Reset();

	if (ScanPaths.Num() == 0)
	{
		return;
	}

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
			UObject* Asset = AssetData.GetAsset();
			AssetType* TypedAsset = Cast<AssetType>(Asset);
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

void BuildAuditEffectSpecs(const UCardDefinition* Card, TArray<FJargonEffectSpec>& OutEffects)
{
	OutEffects.Reset();
	if (Card)
	{
		Card->BuildBaseEffectSpecs(OutEffects);
	}
}

void BuildAuditElementalBonusGroups(const UCardDefinition* Card, TArray<FCardAuditElementalBonusGroup>& OutBonusGroups)
{
	OutBonusGroups.Reset();
	if (!Card || !Card->CardScript)
	{
		return;
	}

	for (const FJargonCardElementalBonusScript& BonusScript : Card->CardScript->ElementalBonuses)
	{
		FCardAuditElementalBonusGroup AuditBonus;
		AuditBonus.ElementType = BonusScript.ElementType;
		AuditBonus.RequiredCharges = BonusScript.RequiredCharges;
		AuditBonus.bSpendCharges = BonusScript.bSpendCharges;
		BonusScript.BuildEffectSpecs(Card, AuditBonus.BonusEffects);
		OutBonusGroups.Add(MoveTemp(AuditBonus));
	}
}

FString GetElementalBonusAuditSummary(const FCardAuditElementalBonusGroup& BonusGroup)
{
	TArray<FString> EffectSummaries;
	EffectSummaries.Reserve(BonusGroup.BonusEffects.Num());
	for (int32 EffectIndex = 0; EffectIndex < BonusGroup.BonusEffects.Num(); ++EffectIndex)
	{
		const FJargonEffectSpec& EffectSpec = BonusGroup.BonusEffects[EffectIndex];
		EffectSummaries.Add(FString::Printf(
			TEXT("Effect %d: Operation=%s Delivery=%s Filter=%s Payload=%s"),
			EffectIndex,
			*JargonEffectContracts::GetOperationName(EffectSpec.Operation),
			*JargonEffectContracts::GetDeliveryName(EffectSpec.Delivery),
			*JargonEffectContracts::GetTargetFilterName(EffectSpec.TargetFilter),
			*JargonEffectContracts::BuildPayloadSummary(EffectSpec)));
	}

	return FString::Printf(
		TEXT("%s Required=%d Spend=%s Effects=[%s]"),
		*GetElementTypeName(BonusGroup.ElementType),
		BonusGroup.RequiredCharges,
		*BoolToYesNo(BonusGroup.bSpendCharges),
		*JoinStrings(EffectSummaries, TEXT("; ")));
}

bool OperationUsesValue(EJargonEffectOperation Operation)
{
	return JargonEffectContracts::RequiresValue(Operation);
}

bool IsChainDelivery(const FJargonEffectSpec& EffectSpec)
{
	return EffectSpec.Delivery == EJargonEffectDelivery::ChainUnits;
}

bool IsFriendlyTargetingOperation(EJargonEffectOperation Operation)
{
	return Operation == EJargonEffectOperation::Heal
		|| Operation == EJargonEffectOperation::ApplyShield
		|| Operation == EJargonEffectOperation::CleanseStatus;
}

bool IsHostileTargetingOperation(EJargonEffectOperation Operation)
{
	return Operation == EJargonEffectOperation::DealDamage
		|| Operation == EJargonEffectOperation::ApplyStun
		|| Operation == EJargonEffectOperation::ApplyFreeze
		|| Operation == EJargonEffectOperation::ApplyBurn
		|| Operation == EJargonEffectOperation::ApplyRoot
		|| Operation == EJargonEffectOperation::ApplyVulnerable
		|| Operation == EJargonEffectOperation::ApplyStatus
		|| Operation == EJargonEffectOperation::PushTarget
		|| Operation == EJargonEffectOperation::PullTarget;
}

bool IsStatusMigrationOperation(EJargonEffectOperation Operation)
{
	return Operation == EJargonEffectOperation::ApplyStun
		|| Operation == EJargonEffectOperation::ApplyFreeze
		|| Operation == EJargonEffectOperation::ApplyBurn
		|| Operation == EJargonEffectOperation::ApplyRoot
		|| Operation == EJargonEffectOperation::ApplyVulnerable;
}

bool OperationSupportsRadius(const FJargonEffectSpec& EffectSpec)
{
	return JargonEffectContracts::SupportsRadius(EffectSpec.Operation, EffectSpec.Delivery);
}

FString GetEffectDeliverySummary(const FJargonEffectSpec& EffectSpec)
{
	return JargonEffectContracts::GetDeliveryName(EffectSpec.Delivery);
}

FString GetEffectTargetFilterSummary(const FJargonEffectSpec& EffectSpec)
{
	return JargonEffectContracts::GetTargetFilterName(EffectSpec.TargetFilter);
}

FString BuildCardEffectPayloadSummary(const FJargonEffectSpec& EffectSpec)
{
	TArray<FString> Fields;
	if (OperationUsesValue(EffectSpec.Operation))
	{
		Fields.Add(FString::Printf(TEXT("Value=%d"), EffectSpec.Value));
	}
	if (OperationSupportsRadius(EffectSpec))
	{
		Fields.Add(FString::Printf(TEXT("Radius=%d"), EffectSpec.Radius));
	}
	if (IsChainDelivery(EffectSpec))
	{
		Fields.Add(FString::Printf(TEXT("ChainCount=%d"), EffectSpec.ChainCount));
	}

	switch (EffectSpec.Operation)
	{
	case EJargonEffectOperation::MoveSource:
		Fields.Add(FString::Printf(TEXT("MoveDistance=%d"), EffectSpec.MoveDistance));
		break;
	case EJargonEffectOperation::PushTarget:
		Fields.Add(FString::Printf(TEXT("PushDistance=%d"), EffectSpec.PushDistance));
		Fields.Add(FString::Printf(TEXT("CollisionDamage=%d"), EffectSpec.CollisionDamage));
		break;
	case EJargonEffectOperation::PullTarget:
		Fields.Add(FString::Printf(TEXT("PullDistance=%d"), EffectSpec.PullDistance));
		break;
	case EJargonEffectOperation::SummonUnit:
		Fields.Add(FString::Printf(TEXT("SummonDefinition=%s"), EffectSpec.SummonedUnitDefinition ? TEXT("Assigned") : TEXT("None")));
		Fields.Add(FString::Printf(TEXT("RuntimeSummonClass=%s"), EffectSpec.RuntimeSummonedUnitClass ? *GetClassDisplayName(EffectSpec.RuntimeSummonedUnitClass.Get()) : TEXT("None")));
		break;
	case EJargonEffectOperation::PlaceTileEffect:
		Fields.Add(FString::Printf(TEXT("TileEffectDefinition=%s"), EffectSpec.TileEffectDefinition ? TEXT("Assigned") : TEXT("None")));
		Fields.Add(FString::Printf(TEXT("RuntimeTileEffectClass=%s"), EffectSpec.RuntimeTileEffectClass ? *GetClassDisplayName(EffectSpec.RuntimeTileEffectClass.Get()) : TEXT("None")));
		break;
	case EJargonEffectOperation::GainElementCharge:
		Fields.Add(FString::Printf(TEXT("Element=%s"), *GetElementTypeName(EffectSpec.ElementType)));
		break;
	case EJargonEffectOperation::CleanseStatus:
		Fields.Add(EffectSpec.StatusEffectDefinition
			? FString::Printf(TEXT("StatusDefinition=%s"), *GetNameSafe(EffectSpec.StatusEffectDefinition.Get()))
			: TEXT("StatusDefinition=AllNegative"));
		break;
	default:
		break;
	}

	if (EffectSpec.Operation == EJargonEffectOperation::DealDamage && EffectSpec.bLifesteal)
	{
		Fields.Add(TEXT("Lifesteal=true"));
	}

	return Fields.Num() > 0 ? FString::Join(Fields, TEXT(" ")) : TEXT("None");
}

FString BuildConditionSummary(const TArray<FCardAuditElementalBonusGroup>& BonusGroups)
{
	TArray<FString> Conditions;
	for (const FCardAuditElementalBonusGroup& BonusGroup : BonusGroups)
	{
		Conditions.Add(FString::Printf(
			TEXT("%s>=%d Spend=%s Effects=%d"),
			*GetElementTypeName(BonusGroup.ElementType),
			BonusGroup.RequiredCharges,
			*BoolToYesNo(BonusGroup.bSpendCharges),
			BonusGroup.BonusEffects.Num()));
	}
	return Conditions.Num() > 0 ? FString::Join(Conditions, TEXT("; ")) : TEXT("None");
}

void AddEffectArchitectureWarnings(const FJargonEffectSpec& EffectSpec, TArray<FString>& OutWarnings)
{
	const FString OperationName = GetCardEffectOperationName(EffectSpec.Operation);
	if (OperationName.Contains(TEXT("If")) ||
		OperationName.Contains(TEXT("Per")) ||
		OperationName.Contains(TEXT("Ignoring")) ||
		OperationName.Contains(TEXT("When")) ||
		OperationName.Contains(TEXT("While")) ||
		OperationName.Contains(TEXT("With")) ||
		OperationName.Contains(TEXT("Without")))
	{
		OutWarnings.Add(FString::Printf(TEXT("Architecture: operation '%s' looks one-off. Prefer primitive operation + delivery/filter/payload/condition."), *OperationName));
	}

	if (IsStatusMigrationOperation(EffectSpec.Operation))
	{
		OutWarnings.Add(FString::Printf(TEXT("Migration: direct status operation '%s' should become ApplyStatus + StatusEffectDefinition."), *OperationName));
	}
	if (!OperationUsesValue(EffectSpec.Operation) && EffectSpec.Value != 1)
	{
		OutWarnings.Add(FString::Printf(TEXT("Payload: '%s' ignores Value=%d."), *OperationName, EffectSpec.Value));
	}
	if (!OperationSupportsRadius(EffectSpec) && EffectSpec.Radius > 0)
	{
		OutWarnings.Add(FString::Printf(TEXT("Payload: '%s' ignores Radius=%d."), *OperationName, EffectSpec.Radius));
	}
	if (!IsChainDelivery(EffectSpec) && EffectSpec.ChainCount != 3)
	{
		OutWarnings.Add(FString::Printf(TEXT("Payload: '%s' ignores ChainCount=%d."), *OperationName, EffectSpec.ChainCount));
	}
	if (EffectSpec.Operation != EJargonEffectOperation::DealDamage && EffectSpec.bLifesteal)
	{
		OutWarnings.Add(FString::Printf(TEXT("Payload: '%s' ignores Lifesteal."), *OperationName));
	}
}

bool BonusGroupMixesFriendlyAndHostileEffects(const FCardAuditElementalBonusGroup& BonusGroup)
{
	bool bHasFriendlyOperation = false;
	bool bHasHostileOperation = false;
	for (const FJargonEffectSpec& BonusEffect : BonusGroup.BonusEffects)
	{
		bHasFriendlyOperation |= IsFriendlyTargetingOperation(BonusEffect.Operation);
		bHasHostileOperation |= IsHostileTargetingOperation(BonusEffect.Operation);
	}

	return bHasFriendlyOperation && bHasHostileOperation;
}

bool IsLikelyDebugPathOrName(const FString& AssetPath, const FString& AssetName)
{
	const FString LowerPath = AssetPath.ToLower();
	const FString LowerName = AssetName.ToLower();
	return LowerPath.Contains(TEXT("/debug/"))
		|| LowerPath.Contains(TEXT("/dev/"))
		|| LowerName.StartsWith(TEXT("da_card_debug"))
		|| LowerName.StartsWith(TEXT("debug_"))
		|| LowerName.Contains(TEXT("_debug"));
}

bool CardDescriptionMentionsElementCharges(const UCardDefinition* Card)
{
	if (!Card)
	{
		return true;
	}

	const FString Description = Card->Description.ToString().ToLower();
	return Description.Contains(TEXT("charge"))
		|| Description.Contains(TEXT("element"));
}

void AddCardOrderingWarnings(const UCardDefinition* Card, TArray<FString>& OutSoftWarnings)
{
	if (!Card)
	{
		return;
	}

	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	for (int32 EffectIndex = 0; EffectIndex < AuditEffects.Num(); ++EffectIndex)
	{
		const FJargonEffectSpec& EffectSpec = AuditEffects[EffectIndex];
		if (EffectSpec.Operation != EJargonEffectOperation::MoveSource)
		{
			continue;
		}

		if (EffectIndex < AuditEffects.Num() - 1)
		{
			OutSoftWarnings.Add(FString::Printf(
				TEXT("Warning: Effect %d MoveSource appears before later base effects. MoveSource can resolve asynchronously, so later base effects may be skipped for that resolve pass."),
				EffectIndex));
		}

		if (AuditBonusGroups.Num() > 0)
		{
			OutSoftWarnings.Add(FString::Printf(
				TEXT("Warning: Effect %d MoveSource appears before CardScript elemental bonuses. MoveSource can resolve asynchronously, so elemental bonuses may be skipped for that resolve pass."),
				EffectIndex));
		}

		return;
	}
}

void AddSummonDefinitionWarnings(
	const UJargonSummonedUnitDefinition* Definition,
	bool bScannedByPath,
	bool bReferencedByScannedCards,
	TArray<FString>& OutWarnings)
{
	if (!Definition)
	{
		OutWarnings.Add(TEXT("Invalid: null summon definition."));
		return;
	}

	if (!Definition->IsValidDefinition())
	{
		OutWarnings.Add(TEXT("Invalid: IsValidDefinition returned false."));
	}

	if (Definition->DisplayName.ToString().TrimStartAndEnd().IsEmpty())
	{
		OutWarnings.Add(TEXT("Invalid: DisplayName is empty."));
	}

	if (Definition->MaxHP <= 0)
	{
		OutWarnings.Add(FString::Printf(TEXT("Invalid: MaxHP <= 0 (%d)."), Definition->MaxHP));
	}

	if (Definition->MoveRange < 0)
	{
		OutWarnings.Add(FString::Printf(TEXT("Invalid: MoveRange is negative (%d)."), Definition->MoveRange));
	}

	if (Definition->AttackRange <= 0)
	{
		OutWarnings.Add(FString::Printf(TEXT("Invalid: AttackRange <= 0 (%d)."), Definition->AttackRange));
	}

	if (Definition->AttackDamage < 0)
	{
		OutWarnings.Add(FString::Printf(TEXT("Invalid: AttackDamage is negative (%d)."), Definition->AttackDamage));
	}

	if (!bScannedByPath && bReferencedByScannedCards)
	{
		OutWarnings.Add(TEXT("Warning: referenced by scanned cards but outside configured summon definition scan paths."));
	}

	if (bScannedByPath && !bReferencedByScannedCards)
	{
		OutWarnings.Add(TEXT("Warning: not referenced by scanned cards. This may be intentional test or future content."));
	}
}

bool CardHasElementGenerator(const UCardDefinition* Card);

void AddCardEffectSpecWarnings(
	const FJargonEffectSpec& EffectSpec,
	const FString& EffectLabel,
	TArray<FString>& OutFatalWarnings,
	TArray<FString>& OutSoftWarnings)
{
	if (EffectSpec.Operation == EJargonEffectOperation::None)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has operation None."), *EffectLabel));
		return;
	}

	if (OperationUsesValue(EffectSpec.Operation) && EffectSpec.Value <= 0)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires Value > 0."), *EffectLabel));
	}

	if (EffectSpec.Radius < 0)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has negative Radius."), *EffectLabel));
	}

	if (IsChainDelivery(EffectSpec) && EffectSpec.ChainCount <= 0)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires ChainCount > 0."), *EffectLabel));
	}

	switch (EffectSpec.Operation)
	{
	case EJargonEffectOperation::MoveSource:
		if (EffectSpec.MoveDistance <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires delivery payload MoveDistance > 0."), *EffectLabel));
		}
		break;

	case EJargonEffectOperation::PushTarget:
		if (EffectSpec.PushDistance <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires delivery payload PushDistance > 0."), *EffectLabel));
		}

		if (EffectSpec.CollisionDamage < 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has negative CollisionDamage payload."), *EffectLabel));
		}
		break;

	case EJargonEffectOperation::PullTarget:
		if (EffectSpec.PullDistance <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires delivery payload PullDistance > 0."), *EffectLabel));
		}
		break;

	case EJargonEffectOperation::SummonUnit:
		if (!EffectSpec.SummonedUnitDefinition)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires SummonedUnitDefinition payload."), *EffectLabel));
		}
		if (!EffectSpec.RuntimeSummonedUnitClass)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires RuntimeSummonedUnitClass payload."), *EffectLabel));
		}
		break;

	case EJargonEffectOperation::PlaceTileEffect:
		if (!EffectSpec.TileEffectDefinition)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires TileEffectDefinition payload."), *EffectLabel));
		}
		if (!EffectSpec.RuntimeTileEffectClass)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires RuntimeTileEffectClass payload."), *EffectLabel));
		}
		break;

	case EJargonEffectOperation::GainElementCharge:
		if (EffectSpec.ElementType == EJargonElementType::None)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires ElementType payload other than None."), *EffectLabel));
		}
		break;

	default:
		break;
	}
}

void AddCardScriptKeywordDiagnostics(
	const UCardDefinition* Card,
	TArray<FString>& OutFatalWarnings,
	TArray<FString>& OutSoftWarnings)
{
	if (!Card)
	{
		return;
	}

	const UJargonCardScript* Script = Card->CardScript;
	if (!Script)
	{
		return;
	}

	auto CheckKeyword = [&OutFatalWarnings, &OutSoftWarnings](const UJargonCardAction* Action, const FString& Label)
	{
		if (!Action)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s is null."), *Label));
			return;
		}

		const FString Summary = Action->GetActionSummary();
		if (Summary.Contains(TEXT("MissingDefinition")))
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s is missing a required Data Asset payload (%s)."), *Label, *Summary));
		}

		if (Summary.Len() > 180)
		{
			OutSoftWarnings.Add(FString::Printf(TEXT("Review: %s collapsed keyword summary is long/noisy; consider simplifying the payload or display name (%s)."), *Label, *Summary));
		}

		if (const UJargonCardStatusAction* StatusAction = Cast<UJargonCardStatusAction>(Action))
		{
			if (!StatusAction->StatusEffectDefinition)
			{
				OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires StatusEffectDefinition payload."), *Label));
			}
		}
		else if (const UJargonCardSummonAction* SummonAction = Cast<UJargonCardSummonAction>(Action))
		{
			if (!SummonAction->SummonedUnitDefinition)
			{
				OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires SummonedUnitDefinition payload."), *Label));
			}
			if (!SummonAction->RuntimeSummonedUnitClass)
			{
				OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires RuntimeSummonedUnitClass payload."), *Label));
			}
		}
		else if (const UJargonCardPlaceTileEffectAction* TileEffectAction = Cast<UJargonCardPlaceTileEffectAction>(Action))
		{
			if (!TileEffectAction->TileEffectDefinition)
			{
				OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires TileEffectDefinition payload."), *Label));
			}
			if (!TileEffectAction->RuntimeTileEffectClass)
			{
				OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires RuntimeTileEffectClass payload."), *Label));
			}
		}
	};

	for (int32 ActionIndex = 0; ActionIndex < Script->Actions.Num(); ++ActionIndex)
	{
		CheckKeyword(
			Script->Actions[ActionIndex],
			FString::Printf(TEXT("CardScript keyword %d"), ActionIndex));
	}

	for (int32 BonusIndex = 0; BonusIndex < Script->ElementalBonuses.Num(); ++BonusIndex)
	{
		const FJargonCardElementalBonusScript& BonusScript = Script->ElementalBonuses[BonusIndex];
		if (BonusScript.Actions.Num() <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: elemental bonus %d has no keyword entries."), BonusIndex));
			continue;
		}

		for (int32 ActionIndex = 0; ActionIndex < BonusScript.Actions.Num(); ++ActionIndex)
		{
			CheckKeyword(
				BonusScript.Actions[ActionIndex],
				FString::Printf(TEXT("Elemental bonus %d keyword %d"), BonusIndex, ActionIndex));
		}
	}
}

void AddCardIntrinsicWarnings(
	const UCardDefinition* Card,
	TArray<FString>& OutFatalWarnings,
	TArray<FString>& OutSoftWarnings)
{
	if (!Card)
	{
		OutFatalWarnings.Add(TEXT("Invalid: null card asset."));
		return;
	}

	if (Card->DisplayName.ToString().TrimStartAndEnd().IsEmpty())
	{
		OutFatalWarnings.Add(TEXT("Invalid: empty DisplayName."));
	}

	if (Card->Description.ToString().TrimStartAndEnd().IsEmpty())
	{
		OutSoftWarnings.Add(TEXT("Warning: empty Description."));
	}

	if (Card->Cost < 0)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: Cost is negative (%d)."), Card->Cost));
	}

	if (Card->Range < 0)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: Range is negative (%d)."), Card->Range));
	}

	if (!Card->CardArt)
	{
		OutSoftWarnings.Add(TEXT("Warning: missing CardArt."));
	}

	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);
	AddCardScriptKeywordDiagnostics(Card, OutFatalWarnings, OutSoftWarnings);

	if (AuditEffects.Num() == 0)
	{
		OutFatalWarnings.Add(TEXT("Invalid: no CardScript base keyword entries authored."));
		if (AuditBonusGroups.Num() > 0)
		{
			OutSoftWarnings.Add(TEXT("Warning: CardScript elemental bonuses are optional and cannot replace base keywords."));
		}
	}
	else
	{
		for (int32 EffectIndex = 0; EffectIndex < AuditEffects.Num(); ++EffectIndex)
		{
			const FJargonEffectSpec& EffectSpec = AuditEffects[EffectIndex];
			const FString EffectLabel = FString::Printf(TEXT("Effect %d (%s)"), EffectIndex, *GetCardEffectOperationName(EffectSpec.Operation));
			AddCardEffectSpecWarnings(EffectSpec, EffectLabel, OutFatalWarnings, OutSoftWarnings);
		}
	}

	for (int32 BonusIndex = 0; BonusIndex < AuditBonusGroups.Num(); ++BonusIndex)
	{
		const FCardAuditElementalBonusGroup& BonusGroup = AuditBonusGroups[BonusIndex];
		const FString BonusMode = BonusGroup.bSpendCharges ? TEXT("Spend") : TEXT("Check");
		const FString BonusLabel = FString::Printf(TEXT("ElementalBonus %d (%s)"), BonusIndex, *BonusMode);

		if (BonusGroup.ElementType == EJargonElementType::None)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires ElementType payload other than None."), *BonusLabel));
		}

		if (BonusGroup.RequiredCharges <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires RequiredCharges payload > 0. Element bonuses are optional, but their authored requirement must still be meaningful."), *BonusLabel));
		}

		if (BonusGroup.BonusEffects.Num() <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has no bonus keywords."), *BonusLabel));
			continue;
		}

		for (int32 BonusEffectIndex = 0; BonusEffectIndex < BonusGroup.BonusEffects.Num(); ++BonusEffectIndex)
		{
			const FJargonEffectSpec& BonusEffectSpec = BonusGroup.BonusEffects[BonusEffectIndex];
			const FString EffectLabel = FString::Printf(
				TEXT("%s Effect %d (%s)"),
				*BonusLabel,
				BonusEffectIndex,
				*GetCardEffectOperationName(BonusEffectSpec.Operation));
			AddCardEffectSpecWarnings(BonusEffectSpec, EffectLabel, OutFatalWarnings, OutSoftWarnings);
		}

		if (BonusGroup.bSpendCharges && BonusGroupMixesFriendlyAndHostileEffects(BonusGroup))
		{
			OutSoftWarnings.Add(FString::Printf(
				TEXT("Warning: %s spends charges and mixes friendly and hostile effect operations. Verify the shared target context is intentional."),
				*BonusLabel));
		}
	}

	AddCardOrderingWarnings(Card, OutSoftWarnings);

	if (CardHasElementGenerator(Card) && !CardDescriptionMentionsElementCharges(Card))
	{
		OutSoftWarnings.Add(TEXT("Warning: card gains element charges, but Description may not mention element charges."));
	}
}

bool DoesCardHaveFatalIntrinsicWarnings(const UCardDefinition* Card)
{
	TArray<FString> FatalWarnings;
	TArray<FString> SoftWarnings;
	AddCardIntrinsicWarnings(Card, FatalWarnings, SoftWarnings);
	return FatalWarnings.Num() > 0;
}

bool CardHasElementGenerator(const UCardDefinition* Card)
{
	if (!Card)
	{
		return false;
	}

	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	for (const FJargonEffectSpec& EffectSpec : AuditEffects)
	{
		if (EffectSpec.Operation == EJargonEffectOperation::GainElementCharge)
		{
			return true;
		}
	}

	for (const FCardAuditElementalBonusGroup& BonusGroup : AuditBonusGroups)
	{
		for (const FJargonEffectSpec& BonusEffectSpec : BonusGroup.BonusEffects)
		{
			if (BonusEffectSpec.Operation == EJargonEffectOperation::GainElementCharge)
			{
				return true;
			}
		}
	}

	return false;
}

FString BuildEffectSummary(const UCardDefinition* Card)
{
	if (!Card)
	{
		return TEXT("None");
	}

	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	if (AuditEffects.Num() == 0)
	{
		return TEXT("None");
	}

	TArray<FString> EffectSummaries;
	EffectSummaries.Reserve(AuditEffects.Num());

	for (int32 EffectIndex = 0; EffectIndex < AuditEffects.Num(); ++EffectIndex)
	{
		const FJargonEffectSpec& EffectSpec = AuditEffects[EffectIndex];
		EffectSummaries.Add(FString::Printf(
			TEXT("Effect %d: %s"),
			EffectIndex,
			*Card->GetEffectAuditSummary(EffectSpec)));
	}

	const FString BaseEffectSummary = JoinStrings(EffectSummaries, TEXT("; "));
	if (AuditBonusGroups.Num() <= 0)
	{
		return BaseEffectSummary;
	}

	TArray<FString> BonusSummaries;
	BonusSummaries.Reserve(AuditBonusGroups.Num());
	for (int32 BonusIndex = 0; BonusIndex < AuditBonusGroups.Num(); ++BonusIndex)
	{
		const FCardAuditElementalBonusGroup& BonusGroup = AuditBonusGroups[BonusIndex];
		BonusSummaries.Add(FString::Printf(
			TEXT("Bonus %d: %s"),
			BonusIndex,
			*GetElementalBonusAuditSummary(BonusGroup)));
	}

	return FString::Printf(TEXT("%s | Bonuses: %s"), *BaseEffectSummary, *JoinStrings(BonusSummaries, TEXT("; ")));
}

FString BuildPackSummaryForCard(const TArray<FCardPackUsage>& PackUsages)
{
	TArray<FString> PackParts;
	PackParts.Reserve(PackUsages.Num());

	for (const FCardPackUsage& PackUsage : PackUsages)
	{
		PackParts.Add(FString::Printf(TEXT("%s(w=%d)"), *PackUsage.PackName, PackUsage.Weight));
	}

	return JoinStrings(PackParts, TEXT("; "));
}

void AddUniqueString(TArray<FString>& Values, const FString& Value)
{
	if (!Value.IsEmpty())
	{
		Values.AddUnique(Value);
	}
}

FString BuildStringCountSummary(const TMap<FString, int32>& CountMap)
{
	TArray<FString> Parts;
	for (const TPair<FString, int32>& Pair : CountMap)
	{
		Parts.Add(FString::Printf(TEXT("%s=%d"), *Pair.Key, Pair.Value));
	}
	Parts.Sort();
	return JoinStrings(Parts, TEXT("; "));
}

bool StringListContainsAny(const TArray<FString>& Values, const TArray<FString>& AcceptedValues)
{
	for (const FString& AcceptedValue : AcceptedValues)
	{
		if (Values.Contains(AcceptedValue))
		{
			return true;
		}
	}
	return false;
}

TArray<EJargonElementType> GetOrderedContentGapElements()
{
	return
	{
		EJargonElementType::None,
		EJargonElementType::Fire,
		EJargonElementType::Frost,
		EJargonElementType::Storm,
		EJargonElementType::Nature,
		EJargonElementType::Radiance,
		EJargonElementType::Quietus
	};
}

FString GetStatusKindName(EJargonStatusEffectKind StatusKind)
{
	return GetAuditEnumDisplayName(StaticEnum<EJargonStatusEffectKind>(), static_cast<int64>(StatusKind));
}

FString GetCardOwnedElementName(const UCardDefinition* Card)
{
	return Card ? GetElementTypeName(Card->CardElement) : TEXT("Neutral");
}

bool IsProductionCard(const UCardDefinition* Card)
{
	return Card && !IsLikelyDebugPathOrName(Card->GetPathName(), Card->GetName());
}

FString GetPluralSuffix(int32 Count)
{
	return Count == 1 ? TEXT("") : TEXT("s");
}

FString GetEffectValueText(int32 Value)
{
	return FString::FromInt(FMath::Max(0, Value));
}

FString GetSummonDisplayName(const FJargonEffectSpec& EffectSpec)
{
	if (EffectSpec.SummonedUnitDefinition)
	{
		const FString SummonName = EffectSpec.SummonedUnitDefinition->DisplayName.ToString().TrimStartAndEnd();
		if (!SummonName.IsEmpty())
		{
			return SummonName;
		}

		return GetNameSafe(EffectSpec.SummonedUnitDefinition.Get());
	}

	return TEXT("a unit");
}

FString GetTileEffectDisplayName(const FJargonEffectSpec& EffectSpec)
{
	if (EffectSpec.TileEffectDefinition)
	{
		const FString TileEffectName = EffectSpec.TileEffectDefinition->DisplayName.ToString().TrimStartAndEnd();
		if (!TileEffectName.IsEmpty())
		{
			return TileEffectName;
		}

		return GetNameSafe(EffectSpec.TileEffectDefinition.Get());
	}

	return TEXT("a tile effect");
}

FString BuildRulesTextForEffect(const FJargonEffectSpec& EffectSpec)
{
	const bool bChainDelivery = IsChainDelivery(EffectSpec);
	if (bChainDelivery)
	{
		const FString ChainPrefix = FString::Printf(
			TEXT("Chain to up to %d target%s, "),
			FMath::Max(0, EffectSpec.ChainCount),
			*GetPluralSuffix(EffectSpec.ChainCount));

		if (EffectSpec.Operation == EJargonEffectOperation::DealDamage)
		{
			return FString::Printf(TEXT("%sdealing %s damage each."), *ChainPrefix, *GetEffectValueText(EffectSpec.Value));
		}
		if (EffectSpec.Operation == EJargonEffectOperation::Heal)
		{
			return FString::Printf(TEXT("%shealing %s HP each."), *ChainPrefix, *GetEffectValueText(EffectSpec.Value));
		}
		if (EffectSpec.Operation == EJargonEffectOperation::ApplyStatus)
		{
			const FString StatusName = EffectSpec.StatusEffectDefinition
				? EffectSpec.StatusEffectDefinition->DisplayName.ToString().TrimStartAndEnd()
				: TEXT("Status");
			return FString::Printf(TEXT("%sapplying %s %s each."), *ChainPrefix, *GetEffectValueText(EffectSpec.Value), *StatusName);
		}
	}

	switch (EffectSpec.Operation)
	{
	case EJargonEffectOperation::DealDamage:
		return EffectSpec.bLifesteal
			? FString::Printf(TEXT("Deal %s damage. Heal for unblocked damage dealt."), *GetEffectValueText(EffectSpec.Value))
			: FString::Printf(TEXT("Deal %s damage."), *GetEffectValueText(EffectSpec.Value));

	case EJargonEffectOperation::Heal:
		return FString::Printf(TEXT("Heal %s HP."), *GetEffectValueText(EffectSpec.Value));

	case EJargonEffectOperation::ApplyShield:
		return FString::Printf(TEXT("Apply %s Shield."), *GetEffectValueText(EffectSpec.Value));

	case EJargonEffectOperation::ApplyStun:
		return FString::Printf(TEXT("Apply %s Stun."), *GetEffectValueText(EffectSpec.Value));

	case EJargonEffectOperation::ApplyFreeze:
		return FString::Printf(TEXT("Apply %s Freeze."), *GetEffectValueText(EffectSpec.Value));

	case EJargonEffectOperation::ApplyBurn:
		return FString::Printf(TEXT("Apply %s Burn."), *GetEffectValueText(EffectSpec.Value));

	case EJargonEffectOperation::ApplyRoot:
		return FString::Printf(TEXT("Apply %s Root."), *GetEffectValueText(EffectSpec.Value));

	case EJargonEffectOperation::ApplyVulnerable:
		return FString::Printf(TEXT("Apply Vulnerable +%s."), *GetEffectValueText(EffectSpec.Value));

	case EJargonEffectOperation::ApplyStatus:
		return FString::Printf(
			TEXT("Apply %s %s."),
			*GetEffectValueText(EffectSpec.Value),
			EffectSpec.StatusEffectDefinition ? *EffectSpec.StatusEffectDefinition->DisplayName.ToString() : TEXT("Status"));

	case EJargonEffectOperation::CleanseStatus:
		return EffectSpec.StatusEffectDefinition
			? FString::Printf(TEXT("Cleanse %s."), *EffectSpec.StatusEffectDefinition->DisplayName.ToString())
			: TEXT("Cleanse all negative statuses.");

	case EJargonEffectOperation::MoveSource:
		return FString::Printf(TEXT("Move up to %d tile%s."), FMath::Max(0, EffectSpec.MoveDistance), *GetPluralSuffix(EffectSpec.MoveDistance));

	case EJargonEffectOperation::PushTarget:
		if (EffectSpec.CollisionDamage > 0)
		{
			return FString::Printf(
				TEXT("Push the target %d tile%s. Collision deals %d damage."),
				FMath::Max(0, EffectSpec.PushDistance),
				*GetPluralSuffix(EffectSpec.PushDistance),
				FMath::Max(0, EffectSpec.CollisionDamage));
		}
		return FString::Printf(TEXT("Push the target %d tile%s."), FMath::Max(0, EffectSpec.PushDistance), *GetPluralSuffix(EffectSpec.PushDistance));

	case EJargonEffectOperation::PullTarget:
		return FString::Printf(TEXT("Pull the target %d tile%s."), FMath::Max(0, EffectSpec.PullDistance), *GetPluralSuffix(EffectSpec.PullDistance));

	case EJargonEffectOperation::SummonUnit:
		return FString::Printf(TEXT("Summon %s."), *GetSummonDisplayName(EffectSpec));

	case EJargonEffectOperation::PlaceTileEffect:
		return FString::Printf(TEXT("Place %s."), *GetTileEffectDisplayName(EffectSpec));

	case EJargonEffectOperation::DrawCards:
		return FString::Printf(TEXT("Draw %s card%s."), *GetEffectValueText(EffectSpec.Value), *GetPluralSuffix(EffectSpec.Value));

	case EJargonEffectOperation::GainEnergy:
		return FString::Printf(TEXT("Gain %s Energy."), *GetEffectValueText(EffectSpec.Value));

	case EJargonEffectOperation::DestroyTileEffect:
		return TEXT("Destroy a tile effect.");

	case EJargonEffectOperation::GainElementCharge:
		return FString::Printf(
			TEXT("Gain %s %s charge%s."),
			*GetEffectValueText(EffectSpec.Value),
			*GetElementTypeName(EffectSpec.ElementType),
			*GetPluralSuffix(EffectSpec.Value));

	default:
		return FString::Printf(TEXT("%s."), *GetCardEffectOperationName(EffectSpec.Operation));
	}
}

FString BuildRulesTextForEffects(const TArray<FJargonEffectSpec>& Effects)
{
	TArray<FString> Parts;
	Parts.Reserve(Effects.Num());
	for (const FJargonEffectSpec& EffectSpec : Effects)
	{
		if (EffectSpec.Operation != EJargonEffectOperation::None)
		{
			Parts.Add(BuildRulesTextForEffect(EffectSpec));
		}
	}

	return JoinStrings(Parts, TEXT(" "));
}

FString BuildRulesTextForBonusGroup(const FCardAuditElementalBonusGroup& BonusGroup)
{
	const FString BonusPrefix = FString::Printf(
		TEXT("%s %d %s charge%s:"),
		BonusGroup.bSpendCharges ? TEXT("Optional: Spend") : TEXT("Optional, if you have"),
		FMath::Max(0, BonusGroup.RequiredCharges),
		*GetElementTypeName(BonusGroup.ElementType),
		*GetPluralSuffix(BonusGroup.RequiredCharges));

	const FString BonusEffects = BuildRulesTextForEffects(BonusGroup.BonusEffects);
	return BonusEffects.IsEmpty()
		? BonusPrefix
		: FString::Printf(TEXT("%s %s"), *BonusPrefix, *BonusEffects);
}

FString BuildSuggestedRulesTextForCard(const UCardDefinition* Card)
{
	if (!Card)
	{
		return FString();
	}

	TArray<FString> Parts;
	if (Card->CardScript)
	{
		const FString ScriptRules = Card->CardScript->GetRulesText();
		if (!ScriptRules.IsEmpty())
		{
			Parts.Add(ScriptRules);
		}
		return JoinStrings(Parts, TEXT(" "));
	}

	return JoinStrings(Parts, TEXT(" "));
}

FString NormalizeDescriptionForComparison(FString Description)
{
	Description.TrimStartAndEndInline();
	Description.ReplaceInline(TEXT("\r"), TEXT(" "));
	Description.ReplaceInline(TEXT("\n"), TEXT(" "));
	while (Description.Contains(TEXT("  ")))
	{
		Description.ReplaceInline(TEXT("  "), TEXT(" "));
	}
	return Description.ToLower();
}

FString GetRulesKeywordForOperation(EJargonEffectOperation Operation)
{
	switch (Operation)
	{
	case EJargonEffectOperation::DealDamage:
		return TEXT("damage");

	case EJargonEffectOperation::Heal:
		return TEXT("heal");

	case EJargonEffectOperation::ApplyShield:
		return TEXT("shield");

	case EJargonEffectOperation::ApplyStun:
		return TEXT("stun");

	case EJargonEffectOperation::ApplyFreeze:
		return TEXT("freeze");

	case EJargonEffectOperation::ApplyBurn:
		return TEXT("burn");

	case EJargonEffectOperation::ApplyRoot:
		return TEXT("root");

	case EJargonEffectOperation::ApplyVulnerable:
		return TEXT("vulnerable");

	case EJargonEffectOperation::ApplyStatus:
		return TEXT("status");

	case EJargonEffectOperation::CleanseStatus:
		return TEXT("cleanse");

	case EJargonEffectOperation::MoveSource:
		return TEXT("move");

	case EJargonEffectOperation::PushTarget:
		return TEXT("push");

	case EJargonEffectOperation::PullTarget:
		return TEXT("pull");

	case EJargonEffectOperation::SummonUnit:
		return TEXT("summon");

	case EJargonEffectOperation::PlaceTileEffect:
		return TEXT("place");

	case EJargonEffectOperation::DrawCards:
		return TEXT("draw");

	case EJargonEffectOperation::GainEnergy:
		return TEXT("energy");

	case EJargonEffectOperation::DestroyTileEffect:
		return TEXT("destroy");

	case EJargonEffectOperation::GainElementCharge:
		return TEXT("gain");

	default:
		return FString();
	}
}

void AddDescriptionWarningsForEffect(const FJargonEffectSpec& EffectSpec, const FString& NormalizedDescription, const FString& EffectLabel, TArray<FString>& OutWarnings)
{
	const FString Keyword = GetRulesKeywordForOperation(EffectSpec.Operation);
	if (!Keyword.IsEmpty() && !NormalizedDescription.Contains(Keyword))
	{
		OutWarnings.Add(FString::Printf(TEXT("Review: description may omit '%s' for %s."), *Keyword, *EffectLabel));
	}

	if (OperationUsesValue(EffectSpec.Operation) && EffectSpec.Value > 0 && !NormalizedDescription.Contains(FString::FromInt(EffectSpec.Value)))
	{
		OutWarnings.Add(FString::Printf(TEXT("Review: description may omit value %d for %s."), EffectSpec.Value, *EffectLabel));
	}

	if (EffectSpec.Operation == EJargonEffectOperation::GainElementCharge)
	{
		const FString ElementName = GetElementTypeName(EffectSpec.ElementType).ToLower();
		if (!ElementName.IsEmpty() && !NormalizedDescription.Contains(ElementName))
		{
			OutWarnings.Add(FString::Printf(TEXT("Review: description may omit element '%s' for %s."), *GetElementTypeName(EffectSpec.ElementType), *EffectLabel));
		}
		if (!NormalizedDescription.Contains(TEXT("charge")) && !NormalizedDescription.Contains(TEXT("element")))
		{
			OutWarnings.Add(FString::Printf(TEXT("Review: description may omit element charge wording for %s."), *EffectLabel));
		}
	}
}

FCardDescriptionSuggestionRow BuildDescriptionSuggestionRow(const UCardDefinition* Card)
{
	FCardDescriptionSuggestionRow Row;
	if (!Card)
	{
		Row.Status = TEXT("Invalid");
		Row.Warnings.Add(TEXT("Invalid: null card."));
		return Row;
	}

	Row.AssetPath = Card->GetPathName();
	Row.AssetName = Card->GetName();
	Row.DisplayName = TextOrFallbackName(Card->DisplayName, Card);
	Row.CurrentDescription = Card->Description.ToString().TrimStartAndEnd();
	Row.SuggestedDescription = BuildSuggestedRulesTextForCard(Card);

	const FString NormalizedCurrent = NormalizeDescriptionForComparison(Row.CurrentDescription);
	const FString NormalizedSuggested = NormalizeDescriptionForComparison(Row.SuggestedDescription);

	if (Row.CurrentDescription.IsEmpty())
	{
		Row.Status = TEXT("Missing");
		Row.Warnings.Add(TEXT("Warning: Description is empty."));
	}
	else if (!Row.SuggestedDescription.IsEmpty() && NormalizedCurrent == NormalizedSuggested)
	{
		Row.Status = TEXT("Matches");
	}
	else
	{
		Row.Status = TEXT("Review");
		Row.Warnings.Add(TEXT("Review: authored Description differs from generated rules-first suggestion."));
	}

	if (NormalizedCurrent.Contains(TEXT("light")) || NormalizedCurrent.Contains(TEXT("shadow")))
	{
		Row.Warnings.Add(TEXT("Review: description may use legacy element wording. Use Radiance or Quietus."));
	}
	if (NormalizedCurrent.Contains(TEXT("mana")))
	{
		Row.Warnings.Add(TEXT("Review: description mentions Mana. Use Energy."));
	}
	if (NormalizedCurrent.Contains(TEXT("armor")))
	{
		Row.Warnings.Add(TEXT("Review: description mentions Armor. Use Shield."));
	}

	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	for (int32 EffectIndex = 0; EffectIndex < AuditEffects.Num(); ++EffectIndex)
	{
		AddDescriptionWarningsForEffect(
			AuditEffects[EffectIndex],
			NormalizedCurrent,
			FString::Printf(TEXT("Effect %d"), EffectIndex),
			Row.Warnings);
	}

	for (int32 BonusIndex = 0; BonusIndex < AuditBonusGroups.Num(); ++BonusIndex)
	{
		const FCardAuditElementalBonusGroup& BonusGroup = AuditBonusGroups[BonusIndex];
		const FString ElementName = GetElementTypeName(BonusGroup.ElementType).ToLower();
		if (!ElementName.IsEmpty() && !NormalizedCurrent.Contains(ElementName))
		{
			Row.Warnings.Add(FString::Printf(TEXT("Review: description may omit bonus element '%s'."), *GetElementTypeName(BonusGroup.ElementType)));
		}
		if (!NormalizedCurrent.Contains(FString::FromInt(BonusGroup.RequiredCharges)))
		{
			Row.Warnings.Add(FString::Printf(TEXT("Review: description may omit bonus required charge count %d."), BonusGroup.RequiredCharges));
		}

		for (int32 BonusEffectIndex = 0; BonusEffectIndex < BonusGroup.BonusEffects.Num(); ++BonusEffectIndex)
		{
			AddDescriptionWarningsForEffect(
				BonusGroup.BonusEffects[BonusEffectIndex],
				NormalizedCurrent,
				FString::Printf(TEXT("ElementalBonus %d Effect %d"), BonusIndex, BonusEffectIndex),
				Row.Warnings);
		}
	}

	return Row;
}

int32 EstimateEffectBudget(const FJargonEffectSpec& EffectSpec)
{
	switch (EffectSpec.Operation)
	{
	case EJargonEffectOperation::DealDamage:
		return FMath::Max(0, EffectSpec.Value) + FMath::Max(0, EffectSpec.Radius) + (EffectSpec.bLifesteal ? 1 : 0);

	case EJargonEffectOperation::Heal:
	case EJargonEffectOperation::ApplyShield:
		return FMath::Max(0, EffectSpec.Value) + FMath::Max(0, EffectSpec.Radius);

	case EJargonEffectOperation::ApplyStun:
	case EJargonEffectOperation::ApplyFreeze:
	case EJargonEffectOperation::ApplyRoot:
		return (FMath::Max(0, EffectSpec.Value) * 2) + FMath::Max(0, EffectSpec.Radius);

	case EJargonEffectOperation::ApplyBurn:
		return FMath::Max(0, EffectSpec.Value) + FMath::Max(0, EffectSpec.Radius);

	case EJargonEffectOperation::ApplyVulnerable:
		return FMath::Max(0, EffectSpec.Value) + 1 + FMath::Max(0, EffectSpec.Radius);

	case EJargonEffectOperation::MoveSource:
		return FMath::Max(0, EffectSpec.MoveDistance);

	case EJargonEffectOperation::PushTarget:
		return FMath::Max(0, EffectSpec.PushDistance) + FMath::Max(0, EffectSpec.CollisionDamage);

	case EJargonEffectOperation::PullTarget:
		return FMath::Max(0, EffectSpec.PullDistance);

	case EJargonEffectOperation::SummonUnit:
		return 5;

	case EJargonEffectOperation::PlaceTileEffect:
		return 2 + FMath::Max(0, EffectSpec.Radius);

	case EJargonEffectOperation::DrawCards:
	case EJargonEffectOperation::GainEnergy:
		return FMath::Max(0, EffectSpec.Value) * 2;

	case EJargonEffectOperation::ApplyStatus:
		return FMath::Max(0, EffectSpec.Value) * (IsChainDelivery(EffectSpec) ? FMath::Max(1, EffectSpec.ChainCount) : 1);

	case EJargonEffectOperation::CleanseStatus:
		return 1 + FMath::Max(0, EffectSpec.Radius);

	case EJargonEffectOperation::DestroyTileEffect:
		return 1 + FMath::Max(0, EffectSpec.Radius);

	case EJargonEffectOperation::GainElementCharge:
		return FMath::Max(0, EffectSpec.Value);

	default:
		return 0;
	}
}

int32 EstimateEffectsBudget(const TArray<FJargonEffectSpec>& Effects)
{
	int32 Budget = 0;
	for (const FJargonEffectSpec& EffectSpec : Effects)
	{
		Budget += EstimateEffectBudget(EffectSpec);
	}
	return Budget;
}

void GetExpectedCostBandForBudget(int32 Budget, int32& OutMinCost, int32& OutMaxCost, int32& OutSuggestedCost)
{
	if (Budget <= 1)
	{
		OutMinCost = 0;
		OutMaxCost = 1;
		OutSuggestedCost = 0;
	}
	else if (Budget <= 4)
	{
		OutMinCost = 1;
		OutMaxCost = 1;
		OutSuggestedCost = 1;
	}
	else if (Budget <= 7)
	{
		OutMinCost = 1;
		OutMaxCost = 2;
		OutSuggestedCost = 2;
	}
	else if (Budget <= 11)
	{
		OutMinCost = 2;
		OutMaxCost = 3;
		OutSuggestedCost = 2;
	}
	else
	{
		OutMinCost = 3;
		OutMaxCost = 4;
		OutSuggestedCost = 3;
	}
}

bool CardHasDirectHostileValueAbove(const UCardDefinition* Card, int32 Value)
{
	if (!Card)
	{
		return false;
	}

	TArray<FJargonEffectSpec> AuditEffects;
	BuildAuditEffectSpecs(Card, AuditEffects);

	for (const FJargonEffectSpec& EffectSpec : AuditEffects)
	{
		if ((EffectSpec.Operation == EJargonEffectOperation::DealDamage ||
			EffectSpec.Operation == EJargonEffectOperation::ApplyStun ||
			EffectSpec.Operation == EJargonEffectOperation::ApplyFreeze ||
			EffectSpec.Operation == EJargonEffectOperation::ApplyBurn ||
			EffectSpec.Operation == EJargonEffectOperation::ApplyRoot ||
			EffectSpec.Operation == EJargonEffectOperation::ApplyVulnerable) &&
			EffectSpec.Value > Value)
		{
			return true;
		}
	}

	return false;
}

FCardBalanceEstimate EstimateCardBalance(const UCardDefinition* Card)
{
	FCardBalanceEstimate Estimate;
	if (!Card)
	{
		Estimate.Warnings.Add(TEXT("Invalid: null card."));
		return Estimate;
	}

	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	Estimate.BaseBudget = EstimateEffectsBudget(AuditEffects);
	for (const FCardAuditElementalBonusGroup& BonusGroup : AuditBonusGroups)
	{
		const int32 BonusBudget = EstimateEffectsBudget(BonusGroup.BonusEffects);
		Estimate.ElementalBonusBudget += BonusBudget;
		if (BonusBudget > (BonusGroup.RequiredCharges * 2) + 2)
		{
			Estimate.Warnings.Add(FString::Printf(
				TEXT("Balance: elemental bonus %s Required=%d has high payoff budget %d. Verify charge requirement or payoff size."),
				*GetElementTypeName(BonusGroup.ElementType),
				BonusGroup.RequiredCharges,
				BonusBudget));
		}
	}

	GetExpectedCostBandForBudget(Estimate.BaseBudget, Estimate.ExpectedCostMin, Estimate.ExpectedCostMax, Estimate.SuggestedCost);

	if (Card->Cost < Estimate.ExpectedCostMin)
	{
		Estimate.Warnings.Add(FString::Printf(
			TEXT("Balance: likely undercosted. Cost=%d, expected tactical low-number band=%d-%d."),
			Card->Cost,
			Estimate.ExpectedCostMin,
			Estimate.ExpectedCostMax));
	}
	else if (Card->Cost > Estimate.ExpectedCostMax)
	{
		Estimate.Warnings.Add(FString::Printf(
			TEXT("Balance: likely overcosted. Cost=%d, expected tactical low-number band=%d-%d."),
			Card->Cost,
			Estimate.ExpectedCostMin,
			Estimate.ExpectedCostMax));
	}

	if (Card->Cost == 0 && CardHasDirectHostileValueAbove(Card, 1))
	{
		Estimate.Warnings.Add(TEXT("Balance: 0-cost cards should usually be setup, movement, light utility, or small element generation; review direct hostile impact."));
	}

	if (Card->Cost > 3)
	{
		Estimate.Warnings.Add(TEXT("Balance: cost above 3 should be rare in the current tactical low-number curve."));
	}

	return Estimate;
}

TArray<FString> BuildRoleTagsForCard(const UCardDefinition* Card)
{
	TArray<FString> RoleTags;
	if (!Card)
	{
		return RoleTags;
	}

	const auto AddRoleForEffect = [&RoleTags](const FJargonEffectSpec& EffectSpec)
	{
		switch (EffectSpec.Operation)
		{
		case EJargonEffectOperation::DealDamage:
			AddUniqueString(RoleTags, TEXT("Damage"));
			if (EffectSpec.bLifesteal)
			{
				AddUniqueString(RoleTags, TEXT("Healing"));
				AddUniqueString(RoleTags, TEXT("Lifesteal"));
			}
			break;

		case EJargonEffectOperation::ApplyBurn:
			AddUniqueString(RoleTags, TEXT("Damage"));
			AddUniqueString(RoleTags, TEXT("Burn"));
			AddUniqueString(RoleTags, TEXT("Damage Over Time"));
			break;

		case EJargonEffectOperation::Heal:
			AddUniqueString(RoleTags, TEXT("Healing"));
			break;

		case EJargonEffectOperation::ApplyShield:
			AddUniqueString(RoleTags, TEXT("Defense"));
			break;

		case EJargonEffectOperation::ApplyStun:
		case EJargonEffectOperation::ApplyFreeze:
		case EJargonEffectOperation::ApplyRoot:
		case EJargonEffectOperation::PushTarget:
		case EJargonEffectOperation::PullTarget:
		case EJargonEffectOperation::DestroyTileEffect:
			AddUniqueString(RoleTags, TEXT("Control"));
			break;

		case EJargonEffectOperation::ApplyStatus:
			if (EffectSpec.StatusEffectDefinition)
			{
				switch (EffectSpec.StatusEffectDefinition->StatusKind)
				{
				case EJargonStatusEffectKind::Burn:
					AddUniqueString(RoleTags, TEXT("Damage"));
					AddUniqueString(RoleTags, TEXT("Burn"));
					AddUniqueString(RoleTags, TEXT("Damage Over Time"));
					break;

				case EJargonStatusEffectKind::Vulnerable:
					AddUniqueString(RoleTags, TEXT("Damage Setup"));
					AddUniqueString(RoleTags, TEXT("Control"));
					break;

				case EJargonStatusEffectKind::Regen:
					AddUniqueString(RoleTags, TEXT("Healing"));
					AddUniqueString(RoleTags, TEXT("Defense"));
					break;

				case EJargonStatusEffectKind::Weak:
					AddUniqueString(RoleTags, TEXT("Control"));
					AddUniqueString(RoleTags, TEXT("Defense"));
					break;

				case EJargonStatusEffectKind::Stun:
				case EJargonStatusEffectKind::Freeze:
				case EJargonStatusEffectKind::Root:
				default:
					AddUniqueString(RoleTags, TEXT("Control"));
					break;
				}
			}
			else
			{
				AddUniqueString(RoleTags, TEXT("Control"));
			}
			break;

		case EJargonEffectOperation::ApplyVulnerable:
			AddUniqueString(RoleTags, TEXT("Damage Setup"));
			AddUniqueString(RoleTags, TEXT("Control"));
			break;

		case EJargonEffectOperation::CleanseStatus:
			AddUniqueString(RoleTags, TEXT("Defense"));
			AddUniqueString(RoleTags, TEXT("Utility"));
			break;

		case EJargonEffectOperation::MoveSource:
			AddUniqueString(RoleTags, TEXT("Mobility"));
			break;

		case EJargonEffectOperation::DrawCards:
			AddUniqueString(RoleTags, TEXT("Draw"));
			break;

		case EJargonEffectOperation::GainEnergy:
			AddUniqueString(RoleTags, TEXT("Energy"));
			break;

		case EJargonEffectOperation::GainElementCharge:
			AddUniqueString(RoleTags, TEXT("Generator"));
			break;

		case EJargonEffectOperation::SummonUnit:
			AddUniqueString(RoleTags, TEXT("Summon"));
			break;

		case EJargonEffectOperation::PlaceTileEffect:
			AddUniqueString(RoleTags, TEXT("Tile Effect"));
			break;

		default:
			break;
		}
	};

	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	for (const FJargonEffectSpec& EffectSpec : AuditEffects)
	{
		AddRoleForEffect(EffectSpec);
	}

	if (AuditBonusGroups.Num() > 0)
	{
		AddUniqueString(RoleTags, TEXT("Element Payoff"));
		for (const FCardAuditElementalBonusGroup& BonusGroup : AuditBonusGroups)
		{
			for (const FJargonEffectSpec& BonusEffect : BonusGroup.BonusEffects)
			{
				AddRoleForEffect(BonusEffect);
			}
		}
	}

	if (Card->Category == ECardCategory::Summon)
	{
		AddUniqueString(RoleTags, TEXT("Summon"));
	}
	if (Card->Category == ECardCategory::Trap)
	{
		AddUniqueString(RoleTags, TEXT("Trap"));
	}
	if (Card->Category == ECardCategory::Aura)
	{
		AddUniqueString(RoleTags, TEXT("Aura"));
	}

	if (RoleTags.Num() == 0)
	{
		RoleTags.Add(TEXT("Unclassified"));
	}

	RoleTags.Sort();
	return RoleTags;
}

TArray<FString> BuildElementTagsForCard(const UCardDefinition* Card)
{
	TArray<FString> ElementTags;
	if (!Card)
	{
		return ElementTags;
	}

	AddUniqueString(ElementTags, GetCardOwnedElementName(Card));

	const auto AddElementFromEffect = [&ElementTags](const FJargonEffectSpec& EffectSpec)
	{
		if (EffectSpec.Operation == EJargonEffectOperation::GainElementCharge && EffectSpec.ElementType != EJargonElementType::None)
		{
			AddUniqueString(ElementTags, GetElementTypeName(EffectSpec.ElementType));
		}
	};

	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	for (const FJargonEffectSpec& EffectSpec : AuditEffects)
	{
		AddElementFromEffect(EffectSpec);
	}

	for (const FCardAuditElementalBonusGroup& BonusGroup : AuditBonusGroups)
	{
		if (BonusGroup.ElementType != EJargonElementType::None)
		{
			AddUniqueString(ElementTags, GetElementTypeName(BonusGroup.ElementType));
		}

		for (const FJargonEffectSpec& BonusEffect : BonusGroup.BonusEffects)
		{
			AddElementFromEffect(BonusEffect);
		}
	}

	if (ElementTags.Num() == 0)
	{
		ElementTags.Add(TEXT("Neutral"));
	}

	ElementTags.Sort();
	return ElementTags;
}

void AddKeywordsForEffect(const FJargonEffectSpec& EffectSpec, TArray<FString>& InOutKeywords)
{
	const FString Keyword = GetCardEffectOperationName(EffectSpec.Operation);
	if (!Keyword.IsEmpty() && Keyword != TEXT("None"))
	{
		AddUniqueString(InOutKeywords, Keyword);
	}
}

TArray<FString> BuildKeywordListForCard(const UCardDefinition* Card)
{
	TArray<FString> Keywords;
	if (!Card)
	{
		return Keywords;
	}

	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	for (const FJargonEffectSpec& EffectSpec : AuditEffects)
	{
		AddKeywordsForEffect(EffectSpec, Keywords);
	}

	for (const FCardAuditElementalBonusGroup& BonusGroup : AuditBonusGroups)
	{
		for (const FJargonEffectSpec& BonusEffect : BonusGroup.BonusEffects)
		{
			AddKeywordsForEffect(BonusEffect, Keywords);
		}
	}

	Keywords.Sort();
	return Keywords;
}

FString BuildCardSamenessFingerprint(const UCardDefinition* Card)
{
	if (!Card)
	{
		return TEXT("Invalid");
	}

	const TArray<FString> RoleTags = BuildRoleTagsForCard(Card);
	const TArray<FString> ElementTags = BuildElementTagsForCard(Card);
	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);
	const FString PrimaryOperation = AuditEffects.Num() > 0
		? GetCardEffectOperationName(AuditEffects[0].Operation)
		: TEXT("None");

	return FString::Printf(
		TEXT("%s|%s|Cost%d|Range%d|%s|Roles=%s|Elements=%s|Bonus%d"),
		*GetCardCategoryName(Card->Category),
		*GetCardTargetTypeName(Card->TargetType),
		Card->Cost,
		Card->Range,
		*PrimaryOperation,
		*JoinStrings(RoleTags, TEXT("+")),
		*JoinStrings(ElementTags, TEXT("+")),
		AuditBonusGroups.Num());
}

int32 GetCardTacticalScore(const UCardDefinition* Card)
{
	if (!Card)
	{
		return 0;
	}

	const auto ScoreEffect = [](const FJargonEffectSpec& EffectSpec)
	{
		switch (EffectSpec.Operation)
		{
		case EJargonEffectOperation::MoveSource:
		case EJargonEffectOperation::PushTarget:
		case EJargonEffectOperation::PullTarget:
		case EJargonEffectOperation::PlaceTileEffect:
		case EJargonEffectOperation::DestroyTileEffect:
			return 2;

		case EJargonEffectOperation::ApplyStun:
		case EJargonEffectOperation::ApplyFreeze:
		case EJargonEffectOperation::ApplyBurn:
		case EJargonEffectOperation::ApplyRoot:
		case EJargonEffectOperation::ApplyVulnerable:
		case EJargonEffectOperation::ApplyStatus:
			return 2;

		case EJargonEffectOperation::SummonUnit:
		case EJargonEffectOperation::DrawCards:
		case EJargonEffectOperation::GainEnergy:
		case EJargonEffectOperation::GainElementCharge:
			return 1;

		default:
			return 0;
		}
	};

	int32 Score = 0;
	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	for (const FJargonEffectSpec& EffectSpec : AuditEffects)
	{
		Score += ScoreEffect(EffectSpec);
		if (EffectSpec.Radius > 0)
		{
			Score++;
		}
	}

	for (const FCardAuditElementalBonusGroup& BonusGroup : AuditBonusGroups)
	{
		Score++;
		for (const FJargonEffectSpec& BonusEffect : BonusGroup.BonusEffects)
		{
			Score += ScoreEffect(BonusEffect);
		}
	}

	return Score;
}

FCardDesignAuditRow BuildCardDesignAuditRow(
	const UCardDefinition* Card,
	const TMap<FString, int32>& FingerprintCounts)
{
	FCardDesignAuditRow Row;
	if (!Card)
	{
		Row.DesignStatus = TEXT("Invalid");
		Row.Warnings.Add(TEXT("Invalid: null card."));
		return Row;
	}

	const TArray<FString> RoleTags = BuildRoleTagsForCard(Card);
	const TArray<FString> ElementTags = BuildElementTagsForCard(Card);
	const TArray<FString> Keywords = BuildKeywordListForCard(Card);
	TArray<FJargonEffectSpec> AuditEffects;
	TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
	BuildAuditEffectSpecs(Card, AuditEffects);
	BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

	Row.AssetPath = Card->GetPathName();
	Row.AssetName = Card->GetName();
	Row.DisplayName = TextOrFallbackName(Card->DisplayName, Card);
	Row.Category = GetCardCategoryName(Card->Category);
	Row.Cost = Card->Cost;
	Row.ElementTags = JoinStrings(ElementTags, TEXT("; "));
	Row.RoleTags = JoinStrings(RoleTags, TEXT("; "));
	Row.PrimaryKeyword = AuditEffects.Num() > 0 ? GetCardEffectOperationName(AuditEffects[0].Operation) : TEXT("None");
	Row.SamenessFingerprint = BuildCardSamenessFingerprint(Card);
	Row.TacticalScore = GetCardTacticalScore(Card);
	Row.KeywordTerms = JoinStrings(Keywords, TEXT("; "));
	Row.ConditionSummary = BuildConditionSummary(AuditBonusGroups);

	TArray<FString> OperationParts;
	TArray<FString> DeliveryParts;
	TArray<FString> FilterParts;
	TArray<FString> PayloadParts;
	TArray<FString> ArchitectureWarnings;
	for (int32 EffectIndex = 0; EffectIndex < AuditEffects.Num(); ++EffectIndex)
	{
		const FJargonEffectSpec& EffectSpec = AuditEffects[EffectIndex];
		OperationParts.Add(FString::Printf(TEXT("%d:%s"), EffectIndex, *GetCardEffectOperationName(EffectSpec.Operation)));
		DeliveryParts.Add(FString::Printf(TEXT("%d:%s"), EffectIndex, *GetEffectDeliverySummary(EffectSpec)));
		FilterParts.Add(FString::Printf(TEXT("%d:%s"), EffectIndex, *GetEffectTargetFilterSummary(EffectSpec)));
		PayloadParts.Add(FString::Printf(TEXT("%d:%s"), EffectIndex, *BuildCardEffectPayloadSummary(EffectSpec)));
		AddEffectArchitectureWarnings(EffectSpec, ArchitectureWarnings);
	}
	Row.OperationSummary = JoinStrings(OperationParts, TEXT("; "));
	Row.DeliverySummary = JoinStrings(DeliveryParts, TEXT("; "));
	Row.TargetFilterSummary = JoinStrings(FilterParts, TEXT("; "));
	Row.PayloadSummary = JoinStrings(PayloadParts, TEXT("; "));
	Row.SuspiciousPattern = JoinStrings(ArchitectureWarnings, TEXT("; "));
	Row.Warnings.Append(ArchitectureWarnings);

	TArray<FString> SecondaryKeywords = Keywords;
	SecondaryKeywords.Remove(Row.PrimaryKeyword);
	Row.SecondaryKeywords = JoinStrings(SecondaryKeywords, TEXT("; "));

	if (const int32* Count = FingerprintCounts.Find(Row.SamenessFingerprint))
	{
		Row.MatchingFingerprintCount = *Count;
		if (*Count >= 3)
		{
			Row.Warnings.Add(FString::Printf(TEXT("Sameness: %d cards share this tactical fingerprint."), *Count));
		}
	}

	if (Row.TacticalScore <= 1 && (Row.PrimaryKeyword == TEXT("Deal Damage") || Row.PrimaryKeyword == TEXT("Gain Element Charge")))
	{
		Row.Warnings.Add(TEXT("Boring pattern: low tactical score with a simple damage/generator primary keyword."));
	}

	if (Keywords.Num() <= 1 && AuditBonusGroups.Num() <= 0 && Card->Category == ECardCategory::Spell)
	{
		Row.Warnings.Add(TEXT("Boring pattern: single-keyword spell with no elemental payoff or tactical rider."));
	}

	if (RoleTags.Num() == 1 && RoleTags.Contains(TEXT("Damage")))
	{
		Row.Warnings.Add(TEXT("Boring pattern: pure damage card. Consider status, movement, targeting, or element interaction."));
	}

	Row.DesignStatus = Row.Warnings.Num() > 0 ? TEXT("Review") : TEXT("Distinct");
	return Row;
}

FCardBalanceAuditRow BuildCardBalanceAuditRow(
	const UCardDefinition* Card,
	bool bUsedInPacks,
	const FString& PackSummary)
{
	FCardBalanceAuditRow Row;
	if (!Card)
	{
		Row.BalanceStatus = TEXT("Invalid");
		Row.Warnings.Add(TEXT("Invalid: null card."));
		return Row;
	}

	const FCardBalanceEstimate Estimate = EstimateCardBalance(Card);
	const TArray<FString> RoleTags = BuildRoleTagsForCard(Card);
	const TArray<FString> ElementTags = BuildElementTagsForCard(Card);
	const bool bLooksDebug = IsLikelyDebugPathOrName(Card->GetPathName(), Card->GetName());

	Row.AssetPath = Card->GetPathName();
	Row.AssetName = Card->GetName();
	Row.DisplayName = TextOrFallbackName(Card->DisplayName, Card);
	Row.Category = GetCardCategoryName(Card->Category);
	Row.TargetType = GetCardTargetTypeName(Card->TargetType);
	Row.Cost = Card->Cost;
	Row.BaseBalanceBudget = Estimate.BaseBudget;
	Row.ElementalBonusBudget = Estimate.ElementalBonusBudget;
	Row.ExpectedCostMin = Estimate.ExpectedCostMin;
	Row.ExpectedCostMax = Estimate.ExpectedCostMax;
	Row.SuggestedCost = Estimate.SuggestedCost;
	Row.BalanceStatus = Estimate.Warnings.Num() > 0 ? TEXT("Review") : TEXT("OnCurve");
	Row.RoleTags = JoinStrings(RoleTags, TEXT("; "));
	Row.ElementTags = JoinStrings(ElementTags, TEXT("; "));
	Row.ProductionStatus = bLooksDebug ? TEXT("Debug") : TEXT("Production");
	Row.PackStatus = bUsedInPacks ? TEXT("InPack") : TEXT("NotInPack");
	Row.EffectsSummary = Card->GetAuditSummary();
	Row.Warnings = Estimate.Warnings;
	if (bUsedInPacks && !PackSummary.IsEmpty())
	{
		Row.PackStatus = FString::Printf(TEXT("InPack: %s"), *PackSummary);
	}

	return Row;
}

void AddVarietyMatrixEntriesForCard(
	const UCardDefinition* Card,
	bool bUsedInPacks,
	TMap<FString, FCardVarietyMatrixRow>& InOutRows)
{
	if (!Card)
	{
		return;
	}

	const TArray<FString> RoleTags = BuildRoleTagsForCard(Card);
	const TArray<FString> ElementTags = BuildElementTagsForCard(Card);
	const FString Category = GetCardCategoryName(Card->Category);
	const FString ProductionStatus = IsLikelyDebugPathOrName(Card->GetPathName(), Card->GetName()) ? TEXT("Debug") : TEXT("Production");
	const FString PackStatus = bUsedInPacks ? TEXT("InPack") : TEXT("NotInPack");
	const FString DisplayName = TextOrFallbackName(Card->DisplayName, Card);

	for (const FString& ElementTag : ElementTags)
	{
		for (const FString& RoleTag : RoleTags)
		{
			const FString Key = FString::Printf(TEXT("%s|%s|%s|%s|%s"), *ElementTag, *RoleTag, *Category, *ProductionStatus, *PackStatus);
			FCardVarietyMatrixRow& Row = InOutRows.FindOrAdd(Key);
			Row.Element = ElementTag;
			Row.Role = RoleTag;
			Row.Category = Category;
			Row.ProductionStatus = ProductionStatus;
			Row.PackStatus = PackStatus;
			Row.CardCount++;
			Row.CardNames.AddUnique(DisplayName);
		}
	}
}

int32 CountProductionElementRoleCards(const TArray<FCardBalanceAuditRow>& BalanceRows, const FString& ElementName, const TArray<FString>& AcceptedRoles)
{
	int32 Count = 0;
	for (const FCardBalanceAuditRow& Row : BalanceRows)
	{
		if (Row.ProductionStatus != TEXT("Production") || !Row.ElementTags.Contains(ElementName))
		{
			continue;
		}

		for (const FString& AcceptedRole : AcceptedRoles)
		{
			if (Row.RoleTags.Contains(AcceptedRole))
			{
				Count++;
				break;
			}
		}
	}
	return Count;
}

void AddVarietyGapWarnings(const TArray<FCardBalanceAuditRow>& BalanceRows, TArray<FString>& OutWarnings)
{
	const TArray<FString> Elements =
	{
		TEXT("Fire"),
		TEXT("Frost"),
		TEXT("Storm"),
		TEXT("Nature"),
		TEXT("Radiance"),
		TEXT("Quietus")
	};

	for (const FString& ElementName : Elements)
	{
		const TArray<FString> GeneratorRoles = { TEXT("Generator") };
		const TArray<FString> PayoffRoles = { TEXT("Element Payoff") };
		const TArray<FString> DamageRoles = { TEXT("Damage") };
		const TArray<FString> DefenseUtilityRoles = { TEXT("Defense"), TEXT("Healing"), TEXT("Control"), TEXT("Draw"), TEXT("Energy") };
		const TArray<FString> TacticalRoles = { TEXT("Mobility"), TEXT("Control"), TEXT("Summon"), TEXT("Trap"), TEXT("Aura"), TEXT("Tile Effect") };
		const int32 GeneratorCount = CountProductionElementRoleCards(BalanceRows, ElementName, GeneratorRoles);
		const int32 PayoffCount = CountProductionElementRoleCards(BalanceRows, ElementName, PayoffRoles);
		const int32 DamageCount = CountProductionElementRoleCards(BalanceRows, ElementName, DamageRoles);
		const int32 DefenseUtilityCount = CountProductionElementRoleCards(BalanceRows, ElementName, DefenseUtilityRoles);
		const int32 TacticalCount = CountProductionElementRoleCards(BalanceRows, ElementName, TacticalRoles);

		if (GeneratorCount < 2)
		{
			OutWarnings.Add(FString::Printf(TEXT("Variety: %s has %d production generator card(s); target is at least 2."), *ElementName, GeneratorCount));
		}
		if (PayoffCount < 2)
		{
			OutWarnings.Add(FString::Printf(TEXT("Variety: %s has %d production payoff card(s); target is at least 2."), *ElementName, PayoffCount));
		}
		if (DamageCount < 1)
		{
			OutWarnings.Add(FString::Printf(TEXT("Variety: %s has no production damage card."), *ElementName));
		}
		if (DefenseUtilityCount < 1)
		{
			OutWarnings.Add(FString::Printf(TEXT("Variety: %s has no production defense/utility card."), *ElementName));
		}
		if (TacticalCount < 1)
		{
			OutWarnings.Add(FString::Printf(TEXT("Variety: %s has no production tactical card."), *ElementName));
		}
	}
}

TArray<FElementIdentityAuditRow> BuildElementIdentityRows(
	const TArray<FCardBalanceAuditRow>& BalanceRows,
	const TArray<FCardDesignAuditRow>& DesignRows)
{
	const TArray<FString> Elements =
	{
		TEXT("Fire"),
		TEXT("Frost"),
		TEXT("Storm"),
		TEXT("Nature"),
		TEXT("Radiance"),
		TEXT("Quietus")
	};

	TArray<FElementIdentityAuditRow> Rows;
	Rows.Reserve(Elements.Num());

	for (const FString& ElementName : Elements)
	{
		FElementIdentityAuditRow Row;
		Row.Element = ElementName;

		TArray<FString> PrimaryKeywords;
		for (const FCardBalanceAuditRow& BalanceRow : BalanceRows)
		{
			if (BalanceRow.ProductionStatus != TEXT("Production") || !BalanceRow.ElementTags.Contains(ElementName))
			{
				continue;
			}

			Row.ProductionCards++;
			if (BalanceRow.RoleTags.Contains(TEXT("Generator")))
			{
				Row.Generators++;
			}
			if (BalanceRow.RoleTags.Contains(TEXT("Element Payoff")))
			{
				Row.Payoffs++;
			}
			if (BalanceRow.RoleTags.Contains(TEXT("Damage")))
			{
				Row.DamageCards++;
			}
			if (BalanceRow.RoleTags.Contains(TEXT("Defense")) ||
				BalanceRow.RoleTags.Contains(TEXT("Healing")) ||
				BalanceRow.RoleTags.Contains(TEXT("Control")) ||
				BalanceRow.RoleTags.Contains(TEXT("Draw")) ||
				BalanceRow.RoleTags.Contains(TEXT("Energy")))
			{
				Row.DefenseUtilityCards++;
			}
			if (BalanceRow.RoleTags.Contains(TEXT("Mobility")) ||
				BalanceRow.RoleTags.Contains(TEXT("Control")) ||
				BalanceRow.RoleTags.Contains(TEXT("Summon")) ||
				BalanceRow.RoleTags.Contains(TEXT("Trap")) ||
				BalanceRow.RoleTags.Contains(TEXT("Aura")) ||
				BalanceRow.RoleTags.Contains(TEXT("Tile Effect")))
			{
				Row.TacticalCards++;
			}
		}

		for (const FCardDesignAuditRow& DesignRow : DesignRows)
		{
			if (DesignRow.ElementTags.Contains(ElementName) && !DesignRow.PrimaryKeyword.IsEmpty())
			{
				AddUniqueString(PrimaryKeywords, DesignRow.PrimaryKeyword);
			}
		}
		PrimaryKeywords.Sort();
		Row.UniquePrimaryKeywords = JoinStrings(PrimaryKeywords, TEXT("; "));

		TArray<FString> MissingLanes;
		if (Row.Generators < 2)
		{
			MissingLanes.Add(TEXT("needs more generators"));
		}
		if (Row.Payoffs < 2)
		{
			MissingLanes.Add(TEXT("needs more payoffs"));
		}
		if (Row.DamageCards < 1)
		{
			MissingLanes.Add(TEXT("needs damage"));
		}
		if (Row.DefenseUtilityCards < 1)
		{
			MissingLanes.Add(TEXT("needs defense/utility"));
		}
		if (Row.TacticalCards < 1)
		{
			MissingLanes.Add(TEXT("needs tactical cards"));
		}

		Row.MissingLanes = MissingLanes.Num() > 0 ? JoinStrings(MissingLanes, TEXT("; ")) : TEXT("None");
		Row.RecommendedActions = MissingLanes.Num() > 0
			? FString::Printf(TEXT("Add or rewrite %s cards to cover: %s."), *ElementName, *Row.MissingLanes)
			: FString::Printf(TEXT("%s has baseline generator/payoff/tactical coverage."), *ElementName);

		Rows.Add(Row);
	}

	return Rows;
}

TArray<FPackExperienceAuditRow> BuildPackExperienceRows(const TArray<UCardPackDefinition*>& Packs)
{
	TArray<FPackExperienceAuditRow> Rows;
	Rows.Reserve(Packs.Num());

	for (const UCardPackDefinition* Pack : Packs)
	{
		if (!Pack)
		{
			continue;
		}

		FPackExperienceAuditRow Row;
		Row.PackAssetPath = Pack->GetPathName();
		Row.PackName = TextOrFallbackName(Pack->DisplayName, Pack);
		Row.CardCount = Pack->CardPool.Num();

		TArray<FString> Roles;
		TArray<FString> Elements;
		TArray<FString> PrimaryKeywords;
		TMap<FString, int32> FingerprintCounts;
		TMap<int32, int32> CostCounts;
		TMap<FString, int32> RoleCounts;
		TMap<FString, int32> ElementCounts;
		int32 TotalCost = 0;
		int32 ValidCards = 0;

		for (const FWeightedCardPackEntry& Entry : Pack->CardPool)
		{
			const UCardDefinition* Card = Entry.CardDefinition.Get();
			if (!Card)
			{
				continue;
			}

			ValidCards++;
			TotalCost += Card->Cost;
			CostCounts.FindOrAdd(Card->Cost)++;
			FingerprintCounts.FindOrAdd(BuildCardSamenessFingerprint(Card))++;

			TArray<FJargonEffectSpec> AuditEffects;
			BuildAuditEffectSpecs(Card, AuditEffects);
			const FString PrimaryKeyword = AuditEffects.Num() > 0
				? GetCardEffectOperationName(AuditEffects[0].Operation)
				: TEXT("None");
			AddUniqueString(PrimaryKeywords, PrimaryKeyword);

			for (const FString& Role : BuildRoleTagsForCard(Card))
			{
				AddUniqueString(Roles, Role);
				RoleCounts.FindOrAdd(Role)++;
			}

			for (const FString& Element : BuildElementTagsForCard(Card))
			{
				AddUniqueString(Elements, Element);
				ElementCounts.FindOrAdd(Element)++;
			}
		}

		Row.UniqueRoles = Roles.Num();
		Row.UniqueElements = Elements.Num();
		Row.UniquePrimaryKeywords = PrimaryKeywords.Num();
		Row.AverageCost = ValidCards > 0 ? static_cast<float>(TotalCost) / static_cast<float>(ValidCards) : 0.f;

		for (const TPair<FString, int32>& Pair : FingerprintCounts)
		{
			if (Pair.Value > 1)
			{
				Row.RepeatedFingerprintGroups++;
			}
		}

		const auto BuildStringCountSummary = [](const TMap<FString, int32>& CountMap)
		{
			TArray<FString> Parts;
			for (const TPair<FString, int32>& Pair : CountMap)
			{
				Parts.Add(FString::Printf(TEXT("%s=%d"), *Pair.Key, Pair.Value));
			}
			Parts.Sort();
			return JoinStrings(Parts, TEXT("; "));
		};
		const auto BuildCostCountSummary = [](const TMap<int32, int32>& CountMap)
		{
			TArray<FString> Parts;
			for (const TPair<int32, int32>& Pair : CountMap)
			{
				Parts.Add(FString::Printf(TEXT("%d=%d"), Pair.Key, Pair.Value));
			}
			Parts.Sort();
			return JoinStrings(Parts, TEXT("; "));
		};

		Row.RoleSpread = BuildStringCountSummary(RoleCounts);
		Row.ElementMix = BuildStringCountSummary(ElementCounts);
		Row.CostCurve = BuildCostCountSummary(CostCounts);

	if (ValidCards > 0 && Row.UniquePrimaryKeywords < 4)
	{
		Row.Warnings.Add(TEXT("Pack monotony: fewer than four unique primary keywords."));
	}
		if (ValidCards > 0 && Row.UniqueElements < 2)
		{
			Row.Warnings.Add(TEXT("Pack monotony: narrow element mix."));
		}
		if (Row.RepeatedFingerprintGroups > 0)
		{
			Row.Warnings.Add(FString::Printf(TEXT("Pack monotony: %d repeated tactical fingerprint group(s)."), Row.RepeatedFingerprintGroups));
		}

		Row.ExperienceStatus = Row.Warnings.Num() > 0 ? TEXT("Review") : TEXT("Varied");
		Rows.Add(Row);
	}

	Rows.Sort([](const FPackExperienceAuditRow& Left, const FPackExperienceAuditRow& Right)
	{
		return Left.PackName < Right.PackName;
	});

	return Rows;
}

void AddStatusTermForEffect(const FJargonEffectSpec& EffectSpec, TArray<FString>& InOutStatusTerms)
{
	switch (EffectSpec.Operation)
	{
	case EJargonEffectOperation::ApplyStun:
		AddUniqueString(InOutStatusTerms, TEXT("Stun"));
		break;

	case EJargonEffectOperation::ApplyFreeze:
		AddUniqueString(InOutStatusTerms, TEXT("Freeze"));
		break;

	case EJargonEffectOperation::ApplyBurn:
		AddUniqueString(InOutStatusTerms, TEXT("Burn"));
		break;

	case EJargonEffectOperation::ApplyRoot:
		AddUniqueString(InOutStatusTerms, TEXT("Root"));
		break;

	case EJargonEffectOperation::ApplyVulnerable:
		AddUniqueString(InOutStatusTerms, TEXT("Vulnerable"));
		break;

	case EJargonEffectOperation::ApplyStatus:
		if (EffectSpec.StatusEffectDefinition)
		{
			const FString StatusDisplayName = EffectSpec.StatusEffectDefinition->DisplayName.ToString().TrimStartAndEnd();
			AddUniqueString(
				InOutStatusTerms,
				StatusDisplayName.IsEmpty()
					? GetStatusKindName(EffectSpec.StatusEffectDefinition->StatusKind)
					: StatusDisplayName);
		}
		else
		{
			AddUniqueString(InOutStatusTerms, TEXT("Status"));
		}
		break;

	default:
		break;
	}
}

FString GetElementLaneDirection(const FString& ElementName)
{
	if (ElementName == TEXT("Fire"))
	{
		return TEXT("burn, hazard pressure, risk damage");
	}
	if (ElementName == TEXT("Frost"))
	{
		return TEXT("freeze/root control, shielded setup, shatter payoff");
	}
	if (ElementName == TEXT("Storm"))
	{
		return TEXT("chain, push/pull, movement, draw tempo");
	}
	if (ElementName == TEXT("Nature"))
	{
		return TEXT("root, healing, shields, growth/sustain");
	}
	if (ElementName == TEXT("Radiance"))
	{
		return TEXT("shield, heal, protection, smite payoff");
	}
	if (ElementName == TEXT("Quietus"))
	{
		return TEXT("vulnerable, drain, death/reaping payoff");
	}

	return TEXT("simple support, positioning, draw, and defensive utility");
}

FString GetElementStatusKeyword(const FString& ElementName)
{
	if (ElementName == TEXT("Fire"))
	{
		return TEXT("Burn");
	}
	if (ElementName == TEXT("Frost"))
	{
		return TEXT("Freeze");
	}
	if (ElementName == TEXT("Storm"))
	{
		return TEXT("Stun");
	}
	if (ElementName == TEXT("Nature"))
	{
		return TEXT("Root");
	}
	if (ElementName == TEXT("Quietus"))
	{
		return TEXT("Vulnerable");
	}

	return TEXT("Shield");
}

FString GetElementThemeNoun(const FString& ElementName)
{
	if (ElementName == TEXT("Fire"))
	{
		return TEXT("cinder");
	}
	if (ElementName == TEXT("Frost"))
	{
		return TEXT("glacier");
	}
	if (ElementName == TEXT("Storm"))
	{
		return TEXT("static");
	}
	if (ElementName == TEXT("Nature"))
	{
		return TEXT("bramble");
	}
	if (ElementName == TEXT("Radiance"))
	{
		return TEXT("sunward");
	}
	if (ElementName == TEXT("Quietus"))
	{
		return TEXT("grave");
	}

	return TEXT("neutral");
}

FString GetEffectVocabularyShortlistForScope(const FString& Scope)
{
	if (Scope == TEXT("All Production"))
	{
		return TEXT("Cleanse/Remove Status; Regen; Weak; Lifesteal");
	}
	if (Scope == TEXT("Nature"))
	{
		return TEXT("Regen first; Cleanse second");
	}
	if (Scope == TEXT("Radiance"))
	{
		return TEXT("Cleanse first; Regen second");
	}
	if (Scope == TEXT("Frost"))
	{
		return TEXT("Weak for softer control after Freeze/Root");
	}
	if (Scope == TEXT("Quietus"))
	{
		return TEXT("Weak next; Lifesteal for drain identity");
	}
	if (Scope == TEXT("Fire"))
	{
		return TEXT("Use existing Burn and hazards first; Poison/Bleed only if rules differ from Burn");
	}
	if (Scope == TEXT("Storm"))
	{
		return TEXT("Use existing chain, draw, push, pull, and movement before adding new status verbs");
	}
	if (Scope == TEXT("Neutral"))
	{
		return TEXT("Use existing draw, energy, move, push, summon, and tile tools");
	}

	return TEXT("Cleanse/Remove Status; Regen; Weak; Lifesteal");
}

FString GetMechanicalVarietyStatus(
	const TMap<FString, int32>& OperationCounts,
	int32 TotalEffectLines,
	FString& OutDominantOperation)
{
	OutDominantOperation = TEXT("None");
	if (TotalEffectLines <= 0)
	{
		return TEXT("NoEffects");
	}

	FString DominantOperation;
	int32 DominantCount = 0;
	for (const TPair<FString, int32>& Pair : OperationCounts)
	{
		if (Pair.Value > DominantCount)
		{
			DominantOperation = Pair.Key;
			DominantCount = Pair.Value;
		}
	}

	const float DominantPercent = TotalEffectLines > 0
		? (static_cast<float>(DominantCount) / static_cast<float>(TotalEffectLines)) * 100.f
		: 0.f;
	OutDominantOperation = FString::Printf(TEXT("%s=%d/%d (%.0f%%)"), *DominantOperation, DominantCount, TotalEffectLines, DominantPercent);

	TArray<int32> OperationUseCounts;
	OperationCounts.GenerateValueArray(OperationUseCounts);
	OperationUseCounts.Sort([](int32 Left, int32 Right)
	{
		return Left > Right;
	});
	int32 TopThreeCount = 0;
	for (int32 Index = 0; Index < FMath::Min(3, OperationUseCounts.Num()); ++Index)
	{
		TopThreeCount += OperationUseCounts[Index];
	}
	const float TopThreePercent = TotalEffectLines > 0
		? (static_cast<float>(TopThreeCount) / static_cast<float>(TotalEffectLines)) * 100.f
		: 0.f;

	if (OperationCounts.Num() < 4)
	{
		return TEXT("ThinOperationVocabulary");
	}
	if (DominantPercent >= 45.f)
	{
		return TEXT("StructurallyCoveredButConcentrated");
	}
	if (TopThreePercent >= 60.f)
	{
		return TEXT("BroadButTopHeavy");
	}
	if (OperationCounts.Num() < 6)
	{
		return TEXT("ModerateOperationVariety");
	}

	return TEXT("BroadOperationVariety");
}

void AccumulateEffectVocabularyCounts(
	const FJargonEffectSpec& EffectSpec,
	TMap<FString, int32>& InOutOperationCounts,
	TMap<FString, int32>& InOutDeliveryCounts,
	TArray<FString>& InOutStatusTerms,
	int32& InOutTotalEffectLines)
{
	InOutTotalEffectLines++;
	InOutOperationCounts.FindOrAdd(GetCardEffectOperationName(EffectSpec.Operation))++;
	InOutDeliveryCounts.FindOrAdd(GetEffectDeliverySummary(EffectSpec))++;
	AddStatusTermForEffect(EffectSpec, InOutStatusTerms);
}

FEffectVocabularyFitAuditRow BuildEffectVocabularyFitRow(
	const FString& Scope,
	const TArray<UCardDefinition*>& Cards,
	const TMap<FString, FString>& StructuralCoverageByElement)
{
	FEffectVocabularyFitAuditRow Row;
	Row.Scope = Scope;
	if (const FString* StructuralCoverage = StructuralCoverageByElement.Find(Scope))
	{
		Row.StructuralCoverageStatus = *StructuralCoverage;
	}
	else
	{
		Row.StructuralCoverageStatus = TEXT("AllScopes");
	}

	TMap<FString, int32> OperationCounts;
	TMap<FString, int32> DeliveryCounts;
	TArray<FString> StatusTerms;

	for (const UCardDefinition* Card : Cards)
	{
		if (!IsProductionCard(Card))
		{
			continue;
		}

		const FString CardElementName = GetCardOwnedElementName(Card);
		if (Scope != TEXT("All Production") && CardElementName != Scope)
		{
			continue;
		}

		Row.ProductionCards++;

		TArray<FJargonEffectSpec> AuditEffects;
		TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
		BuildAuditEffectSpecs(Card, AuditEffects);
		BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

		for (const FJargonEffectSpec& EffectSpec : AuditEffects)
		{
			AccumulateEffectVocabularyCounts(EffectSpec, OperationCounts, DeliveryCounts, StatusTerms, Row.TotalEffectLines);
		}
		for (const FCardAuditElementalBonusGroup& BonusGroup : AuditBonusGroups)
		{
			for (const FJargonEffectSpec& BonusEffectSpec : BonusGroup.BonusEffects)
			{
				AccumulateEffectVocabularyCounts(BonusEffectSpec, OperationCounts, DeliveryCounts, StatusTerms, Row.TotalEffectLines);
			}
		}
	}

	StatusTerms.Sort();
	Row.UniqueOperations = OperationCounts.Num();
	Row.UniqueDeliveries = DeliveryCounts.Num();
	Row.OperationMix = OperationCounts.Num() > 0 ? BuildStringCountSummary(OperationCounts) : TEXT("None");
	Row.DeliveryMix = DeliveryCounts.Num() > 0 ? BuildStringCountSummary(DeliveryCounts) : TEXT("None");
	Row.StatusTerms = StatusTerms.Num() > 0 ? JoinStrings(StatusTerms, TEXT("; ")) : TEXT("None");
	Row.MechanicalVarietyStatus = GetMechanicalVarietyStatus(OperationCounts, Row.TotalEffectLines, Row.DominantOperation);
	Row.RecommendedFirstBatch = GetEffectVocabularyShortlistForScope(Scope);

	if (Row.StructuralCoverageStatus == TEXT("Covered") &&
		(Row.MechanicalVarietyStatus == TEXT("ThinOperationVocabulary") || Row.MechanicalVarietyStatus == TEXT("StructurallyCoveredButConcentrated")))
	{
		Row.Notes = TEXT("Card type/role coverage is present, but effect vocabulary is still concentrated; add effects before adding more same-shaped cards.");
	}
	else if (Scope == TEXT("All Production"))
	{
		Row.Notes = TEXT("Use this row to judge global operation pressure; card creation should prefer existing primitives plus the first expansion batch.");
	}
	else
	{
		Row.Notes = FString::Printf(TEXT("Lane direction: %s."), *GetElementLaneDirection(Scope));
	}

	return Row;
}

TArray<FEffectVocabularyFitAuditRow> BuildEffectVocabularyFitRows(
	const TArray<UCardDefinition*>& Cards,
	const TArray<FCardElementCoverageAuditRow>& ElementCoverageRows)
{
	TArray<FEffectVocabularyFitAuditRow> Rows;
	TMap<FString, FString> StructuralCoverageByElement;
	for (const FCardElementCoverageAuditRow& CoverageRow : ElementCoverageRows)
	{
		StructuralCoverageByElement.Add(CoverageRow.Element, CoverageRow.CoverageStatus);
	}

	Rows.Add(BuildEffectVocabularyFitRow(TEXT("All Production"), Cards, StructuralCoverageByElement));
	for (const EJargonElementType ElementType : GetOrderedContentGapElements())
	{
		Rows.Add(BuildEffectVocabularyFitRow(GetElementTypeName(ElementType), Cards, StructuralCoverageByElement));
	}

	return Rows;
}

void BuildRecommendationDetails(
	const FString& ElementName,
	const FString& SuggestedCardType,
	const FString& RoleFilled,
	FString& OutOperation,
	FString& OutDelivery,
	FString& OutPayload,
	FString& OutRulesText)
{
	const FString StatusKeyword = GetElementStatusKeyword(ElementName);
	const FString ThemeNoun = GetElementThemeNoun(ElementName);

	if (SuggestedCardType == TEXT("Summon"))
	{
		OutOperation = TEXT("SummonUnit");
		OutDelivery = TEXT("ExplicitTile");
		OutPayload = FString::Printf(TEXT("%s summon definition + runtime summon BP class"), *ElementName);
		OutRulesText = FString::Printf(TEXT("Summon a %s ally."), *ThemeNoun);
		return;
	}

	if (SuggestedCardType == TEXT("Trap"))
	{
		OutOperation = TEXT("PlaceTileEffect");
		OutDelivery = TEXT("ExplicitTile");
		OutPayload = FString::Printf(TEXT("%s trap tile-effect definition + runtime trap BP class"), *ElementName);
		OutRulesText = FString::Printf(TEXT("Place a %s trap."), *ThemeNoun);
		return;
	}

	if (SuggestedCardType == TEXT("Aura"))
	{
		OutOperation = TEXT("PlaceTileEffect");
		OutDelivery = TEXT("ExplicitTile");
		OutPayload = FString::Printf(TEXT("%s aura tile-effect definition + runtime aura BP class"), *ElementName);
		OutRulesText = FString::Printf(TEXT("Place a %s aura."), *ThemeNoun);
		return;
	}

	if (RoleFilled == TEXT("Generator"))
	{
		OutOperation = TEXT("GainElementCharge");
		OutDelivery = TEXT("Source");
		OutPayload = FString::Printf(TEXT("Element=%s Value=1"), *ElementName);
		OutRulesText = FString::Printf(TEXT("Gain 1 %s charge."), *ElementName);
		return;
	}

	if (RoleFilled == TEXT("Element Payoff"))
	{
		OutOperation = ElementName == TEXT("Radiance") ? TEXT("ApplyShield") : TEXT("ApplyStatus");
		OutDelivery = ElementName == TEXT("Radiance") ? TEXT("Source or ExplicitFriendly") : TEXT("ExplicitUnit");
		OutPayload = FString::Printf(TEXT("Elemental bonus requires %s charges; payload follows %s lane"), *ElementName, *GetElementLaneDirection(ElementName));
		OutRulesText = ElementName == TEXT("Radiance")
			? FString::Printf(TEXT("Gain 1 %s charge. Optional: Spend 2 %s: Apply 3 Shield."), *ElementName, *ElementName)
			: FString::Printf(TEXT("Gain 1 %s charge. Optional: Spend 2 %s: Apply 2 %s."), *ElementName, *ElementName, *StatusKeyword);
		return;
	}

	if (RoleFilled == TEXT("Damage"))
	{
		OutOperation = ElementName == TEXT("Radiance") ? TEXT("DealDamage") : TEXT("DealDamage + ApplyStatus");
		OutDelivery = TEXT("ExplicitUnit");
		OutPayload = ElementName == TEXT("Radiance")
			? TEXT("Damage=2")
			: FString::Printf(TEXT("Damage=1 Status=%s Value=2"), *StatusKeyword);
		OutRulesText = ElementName == TEXT("Radiance")
			? TEXT("Deal 2 damage.")
			: FString::Printf(TEXT("Deal 1 damage. Apply 2 %s."), *StatusKeyword);
		return;
	}

	if (RoleFilled == TEXT("Defense/Utility"))
	{
		if (ElementName == TEXT("Fire"))
		{
			OutOperation = TEXT("GainElementCharge + ApplyShield");
			OutDelivery = TEXT("Source");
			OutPayload = TEXT("Fire=1 Shield=1");
			OutRulesText = TEXT("Gain 1 Fire charge. Apply 1 Shield.");
			return;
		}
		if (ElementName == TEXT("Frost") || ElementName == TEXT("Radiance") || ElementName == TEXT("Nature"))
		{
			OutOperation = ElementName == TEXT("Nature") ? TEXT("Heal + ApplyShield") : TEXT("ApplyShield");
			OutDelivery = TEXT("Source or ExplicitFriendly");
			OutPayload = ElementName == TEXT("Nature") ? TEXT("Heal=1 Shield=1") : TEXT("Shield=3");
			OutRulesText = ElementName == TEXT("Nature") ? TEXT("Heal 1 HP. Apply 1 Shield.") : TEXT("Apply 3 Shield.");
			return;
		}
		if (ElementName == TEXT("Storm"))
		{
			OutOperation = TEXT("DrawCards");
			OutDelivery = TEXT("Source");
			OutPayload = TEXT("Value=1");
			OutRulesText = TEXT("Draw 1 card.");
			return;
		}

		OutOperation = TEXT("DealDamage + Heal");
		OutDelivery = TEXT("ExplicitUnit + Source");
		OutPayload = TEXT("Damage=1 Heal=1");
		OutRulesText = TEXT("Deal 1 damage. Heal 1 HP.");
		return;
	}

	OutOperation = ElementName == TEXT("Storm") ? TEXT("PullTarget") : TEXT("ApplyStatus");
	OutDelivery = TEXT("ExplicitUnit");
	OutPayload = ElementName == TEXT("Storm")
		? TEXT("PullDistance=2")
		: FString::Printf(TEXT("Status=%s Value=1"), *StatusKeyword);
	OutRulesText = ElementName == TEXT("Storm")
		? TEXT("Pull the target 2 tiles.")
		: FString::Printf(TEXT("Apply 1 %s."), *StatusKeyword);
}

FString BuildRecommendationArtPrompt(
	const FString& ElementName,
	const FString& SuggestedCardType,
	const FString& RoleFilled,
	const FString& DraftRulesText)
{
	return FString::Printf(
		TEXT("Fantasy tactical card art for a %s %s card focused on %s. Show %s magic in a readable board-game composition. Rules inspiration: %s"),
		*ElementName,
		*SuggestedCardType,
		*RoleFilled,
		*GetElementLaneDirection(ElementName),
		*DraftRulesText);
}

void AddCardContentRecommendation(
	TArray<FCardContentRecommendationRow>& InOutRows,
	TSet<FString>& InOutKeys,
	const FString& Priority,
	const FString& ElementName,
	const FString& SuggestedCardType,
	const FString& RoleFilled,
	const FString& Reason,
	const FString& SourceReport)
{
	const FString Key = FString::Printf(TEXT("%s|%s|%s|%s"), *Priority, *ElementName, *SuggestedCardType, *RoleFilled);
	if (InOutKeys.Contains(Key))
	{
		return;
	}
	InOutKeys.Add(Key);

	FCardContentRecommendationRow Row;
	Row.Priority = Priority;
	Row.Element = ElementName;
	Row.SuggestedCardType = SuggestedCardType;
	Row.RoleFilled = RoleFilled;
	Row.Reason = Reason;
	Row.SourceReport = SourceReport;
	BuildRecommendationDetails(
		ElementName,
		SuggestedCardType,
		RoleFilled,
		Row.IntendedOperation,
		Row.Delivery,
		Row.Payload,
		Row.DraftRulesText);
	Row.CardArtPrompt = BuildRecommendationArtPrompt(ElementName, SuggestedCardType, RoleFilled, Row.DraftRulesText);
	InOutRows.Add(Row);
}

TArray<FCardElementCoverageAuditRow> BuildCardElementCoverageRows(const TArray<UCardDefinition*>& Cards)
{
	TArray<FCardElementCoverageAuditRow> Rows;
	const TArray<EJargonElementType> Elements = GetOrderedContentGapElements();
	Rows.Reserve(Elements.Num());

	for (const EJargonElementType ElementType : Elements)
	{
		FCardElementCoverageAuditRow Row;
		Row.Element = GetElementTypeName(ElementType);

		TArray<FString> CardNames;
		TArray<FString> Operations;
		TArray<FString> StatusTerms;

		for (const UCardDefinition* Card : Cards)
		{
			if (!IsProductionCard(Card) || Card->CardElement != ElementType)
			{
				continue;
			}

			Row.ProductionCards++;
			CardNames.Add(TextOrFallbackName(Card->DisplayName, Card));

			switch (Card->Category)
			{
			case ECardCategory::Spell:
				Row.SpellCards++;
				break;
			case ECardCategory::Summon:
				Row.SummonCards++;
				break;
			case ECardCategory::Trap:
				Row.TrapCards++;
				break;
			case ECardCategory::Aura:
				Row.AuraCards++;
				break;
			default:
				break;
			}

			const TArray<FString> RoleTags = BuildRoleTagsForCard(Card);
			if (RoleTags.Contains(TEXT("Damage")))
			{
				Row.DamageCards++;
			}
			if (RoleTags.Contains(TEXT("Defense")))
			{
				Row.DefenseCards++;
			}
			if (RoleTags.Contains(TEXT("Healing")))
			{
				Row.HealingCards++;
			}
			if (RoleTags.Contains(TEXT("Control")))
			{
				Row.ControlCards++;
			}
			if (RoleTags.Contains(TEXT("Mobility")))
			{
				Row.MobilityCards++;
			}
			if (RoleTags.Contains(TEXT("Draw")))
			{
				Row.DrawCards++;
			}
			if (RoleTags.Contains(TEXT("Energy")))
			{
				Row.EnergyCards++;
			}
			if (RoleTags.Contains(TEXT("Generator")))
			{
				Row.Generators++;
			}
			if (RoleTags.Contains(TEXT("Element Payoff")))
			{
				Row.Payoffs++;
			}
			const TArray<FString> TacticalRoleTags =
			{
				TEXT("Mobility"),
				TEXT("Control"),
				TEXT("Summon"),
				TEXT("Trap"),
				TEXT("Aura"),
				TEXT("Tile Effect"),
				TEXT("Draw"),
				TEXT("Energy")
			};
			if (StringListContainsAny(RoleTags, TacticalRoleTags))
			{
				Row.TacticalCards++;
			}

			TArray<FJargonEffectSpec> AuditEffects;
			TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
			BuildAuditEffectSpecs(Card, AuditEffects);
			BuildAuditElementalBonusGroups(Card, AuditBonusGroups);
			for (const FJargonEffectSpec& EffectSpec : AuditEffects)
			{
				AddUniqueString(Operations, GetCardEffectOperationName(EffectSpec.Operation));
				AddStatusTermForEffect(EffectSpec, StatusTerms);
			}
			for (const FCardAuditElementalBonusGroup& BonusGroup : AuditBonusGroups)
			{
				for (const FJargonEffectSpec& BonusEffect : BonusGroup.BonusEffects)
				{
					AddUniqueString(Operations, GetCardEffectOperationName(BonusEffect.Operation));
					AddStatusTermForEffect(BonusEffect, StatusTerms);
				}
			}
		}

		CardNames.Sort();
		Operations.Sort();
		StatusTerms.Sort();
		Row.UniqueOperations = Operations.Num();
		Row.StatusTerms = StatusTerms.Num() > 0 ? JoinStrings(StatusTerms, TEXT("; ")) : TEXT("None");
		Row.CardNames = JoinStrings(CardNames, TEXT("; "));

		TArray<FString> MissingCardTypes;
		if (Row.Element != TEXT("Neutral"))
		{
			if (Row.SpellCards <= 0)
			{
				MissingCardTypes.Add(TEXT("Spell"));
			}
			if (Row.SummonCards <= 0)
			{
				MissingCardTypes.Add(TEXT("Summon"));
			}
			if (Row.TrapCards <= 0)
			{
				MissingCardTypes.Add(TEXT("Trap"));
			}
			if (Row.AuraCards <= 0)
			{
				MissingCardTypes.Add(TEXT("Aura"));
			}
		}

		TArray<FString> MissingRoles;
		if (Row.Element == TEXT("Neutral"))
		{
			if (Row.ProductionCards < 4)
			{
				MissingRoles.Add(TEXT("support pool below four cards"));
			}
			if (Row.SpellCards < 4)
			{
				MissingRoles.Add(TEXT("needs more simple support spells"));
			}
		}
		else
		{
			if (Row.Generators <= 0)
			{
				MissingRoles.Add(TEXT("Generator"));
			}
			else if (Row.Generators < 2)
			{
				MissingRoles.Add(TEXT("Generator under target"));
			}
			if (Row.Payoffs <= 0)
			{
				MissingRoles.Add(TEXT("Element Payoff"));
			}
			else if (Row.Payoffs < 2)
			{
				MissingRoles.Add(TEXT("Element Payoff under target"));
			}
			if (Row.DamageCards <= 0)
			{
				MissingRoles.Add(TEXT("Damage"));
			}
			if ((Row.DefenseCards + Row.HealingCards + Row.ControlCards + Row.DrawCards + Row.EnergyCards) <= 0)
			{
				MissingRoles.Add(TEXT("Defense/Utility"));
			}
			if (Row.TacticalCards <= 0)
			{
				MissingRoles.Add(TEXT("Tactical"));
			}

			if (Row.Element == TEXT("Fire") && Row.ControlCards < 2)
			{
				MissingRoles.Add(TEXT("Fire burn/hazard pressure under target"));
			}
			else if (Row.Element == TEXT("Frost") && Row.DefenseCards < 2)
			{
				MissingRoles.Add(TEXT("Frost shielded setup under target"));
			}
			else if (Row.Element == TEXT("Storm") && (Row.MobilityCards + Row.DrawCards) < 4)
			{
				MissingRoles.Add(TEXT("Storm tempo tools under target"));
			}
			else if (Row.Element == TEXT("Nature") && Row.DefenseCards < 2)
			{
				MissingRoles.Add(TEXT("Nature growth shield under target"));
			}
			else if (Row.Element == TEXT("Radiance") && Row.HealingCards < 2)
			{
				MissingRoles.Add(TEXT("Radiance healing/protection under target"));
			}
		}

		Row.MissingCardTypes = MissingCardTypes.Num() > 0 ? JoinStrings(MissingCardTypes, TEXT("; ")) : TEXT("None");
		Row.MissingRoles = MissingRoles.Num() > 0 ? JoinStrings(MissingRoles, TEXT("; ")) : TEXT("None");

		if (MissingCardTypes.Num() > 0 || MissingRoles.Contains(TEXT("Generator")) || MissingRoles.Contains(TEXT("Element Payoff")) || MissingRoles.Contains(TEXT("Damage")) || MissingRoles.Contains(TEXT("Defense/Utility")) || MissingRoles.Contains(TEXT("Tactical")))
		{
			Row.CoverageStatus = TEXT("HighGap");
		}
		else if (MissingRoles.Num() > 0 || (Row.Element != TEXT("Neutral") && Row.ProductionCards < 8))
		{
			Row.CoverageStatus = TEXT("MediumGap");
		}
		else
		{
			Row.CoverageStatus = TEXT("Covered");
		}

		Row.RecommendedFocus = Row.CoverageStatus == TEXT("Covered")
			? FString::Printf(TEXT("%s has baseline coverage. Next additions can target taste and pack feel."), *Row.Element)
			: FString::Printf(TEXT("Focus %s on %s. Missing types: %s. Missing roles: %s."), *Row.Element, *GetElementLaneDirection(Row.Element), *Row.MissingCardTypes, *Row.MissingRoles);

		Rows.Add(Row);
	}

	return Rows;
}

TArray<FElementPackGapAuditRow> BuildElementPackGapRows(const TArray<UCardPackDefinition*>& Packs)
{
	TArray<FElementPackGapAuditRow> Rows;
	Rows.Reserve(Packs.Num());

	for (const UCardPackDefinition* Pack : Packs)
	{
		if (!Pack)
		{
			continue;
		}

		FElementPackGapAuditRow Row;
		Row.PackAssetPath = Pack->GetPathName();
		Row.PackName = TextOrFallbackName(Pack->DisplayName, Pack);

		TMap<FString, int32> ElementCounts;
		TMap<FString, int32> RoleCounts;
		TMap<FString, int32> FingerprintCounts;
		TSet<EJargonElementType> NonNeutralElements;
		TArray<FString> RepeatedFingerprints;

		for (const FWeightedCardPackEntry& Entry : Pack->CardPool)
		{
			const UCardDefinition* Card = Entry.CardDefinition.Get();
			if (!Card)
			{
				continue;
			}

			const bool bDebug = !IsProductionCard(Card);
			if (bDebug)
			{
				Row.DebugCardCount++;
				continue;
			}

			const FString ElementName = GetCardOwnedElementName(Card);
			ElementCounts.FindOrAdd(ElementName)++;
			FingerprintCounts.FindOrAdd(BuildCardSamenessFingerprint(Card))++;

			if (Card->CardElement == EJargonElementType::None)
			{
				Row.NeutralSupportCount++;
				Row.NeutralSupportWeight += Entry.Weight;
			}
			else
			{
				NonNeutralElements.Add(Card->CardElement);
				Row.ElementPoolCount++;
				Row.ElementPoolWeight += Entry.Weight;
			}

			switch (Card->Category)
			{
			case ECardCategory::Spell:
				Row.SpellCards++;
				break;
			case ECardCategory::Summon:
				Row.SummonCards++;
				break;
			case ECardCategory::Trap:
				Row.TrapCards++;
				break;
			case ECardCategory::Aura:
				Row.AuraCards++;
				break;
			default:
				break;
			}

			for (const FString& RoleTag : BuildRoleTagsForCard(Card))
			{
				RoleCounts.FindOrAdd(RoleTag)++;
			}
		}

		TArray<FString> NonNeutralElementNames;
		for (const EJargonElementType ElementType : NonNeutralElements)
		{
			NonNeutralElementNames.Add(GetElementTypeName(ElementType));
		}
		NonNeutralElementNames.Sort();
		if (NonNeutralElementNames.Num() == 1)
		{
			Row.DetectedElement = NonNeutralElementNames[0];
		}
		else if (NonNeutralElementNames.Num() > 1)
		{
			Row.DetectedElement = FString::Printf(TEXT("Mixed: %s"), *JoinStrings(NonNeutralElementNames, TEXT("; ")));
			Row.OtherElementCount = Row.ElementPoolCount;
		}
		else
		{
			Row.DetectedElement = Row.NeutralSupportCount > 0 ? TEXT("NeutralOnly") : TEXT("None");
		}

		if (NonNeutralElementNames.Num() == 1)
		{
			const FString MainElementName = NonNeutralElementNames[0];
			for (const FWeightedCardPackEntry& Entry : Pack->CardPool)
			{
				const UCardDefinition* Card = Entry.CardDefinition.Get();
				if (IsProductionCard(Card) && Card->CardElement != EJargonElementType::None && GetCardOwnedElementName(Card) != MainElementName)
				{
					Row.OtherElementCount++;
				}
			}
		}

		for (const TPair<FString, int32>& Pair : FingerprintCounts)
		{
			if (Pair.Value > 1)
			{
				RepeatedFingerprints.Add(FString::Printf(TEXT("%s=%d"), *Pair.Key, Pair.Value));
			}
		}
		RepeatedFingerprints.Sort();
		Row.RepeatedFingerprintGroups = RepeatedFingerprints.Num() > 0 ? JoinStrings(RepeatedFingerprints, TEXT("; ")) : TEXT("None");
		Row.RoleSpread = BuildStringCountSummary(RoleCounts);
		Row.ElementMix = BuildStringCountSummary(ElementCounts);

		TArray<FString> MissingCardTypes;
		if (NonNeutralElementNames.Num() == 1)
		{
			if (Row.SpellCards <= 0)
			{
				MissingCardTypes.Add(TEXT("Spell"));
			}
			if (Row.SummonCards <= 0)
			{
				MissingCardTypes.Add(TEXT("Summon"));
			}
			if (Row.TrapCards <= 0)
			{
				MissingCardTypes.Add(TEXT("Trap"));
			}
			if (Row.AuraCards <= 0)
			{
				MissingCardTypes.Add(TEXT("Aura"));
			}
		}

		TArray<FString> MissingRoles;
		if (NonNeutralElementNames.Num() == 1)
		{
			if (!RoleCounts.Contains(TEXT("Generator")))
			{
				MissingRoles.Add(TEXT("Generator"));
			}
			if (!RoleCounts.Contains(TEXT("Element Payoff")))
			{
				MissingRoles.Add(TEXT("Element Payoff"));
			}
			if (!RoleCounts.Contains(TEXT("Damage")))
			{
				MissingRoles.Add(TEXT("Damage"));
			}
			if (!RoleCounts.Contains(TEXT("Defense")) && !RoleCounts.Contains(TEXT("Healing")) && !RoleCounts.Contains(TEXT("Control")) && !RoleCounts.Contains(TEXT("Draw")) && !RoleCounts.Contains(TEXT("Energy")))
			{
				MissingRoles.Add(TEXT("Defense/Utility"));
			}
			if (!RoleCounts.Contains(TEXT("Mobility")) && !RoleCounts.Contains(TEXT("Control")) && !RoleCounts.Contains(TEXT("Summon")) && !RoleCounts.Contains(TEXT("Trap")) && !RoleCounts.Contains(TEXT("Aura")) && !RoleCounts.Contains(TEXT("Tile Effect")) && !RoleCounts.Contains(TEXT("Draw")))
			{
				MissingRoles.Add(TEXT("Tactical"));
			}
		}

		Row.MissingCardTypes = MissingCardTypes.Num() > 0 ? JoinStrings(MissingCardTypes, TEXT("; ")) : TEXT("None");
		Row.MissingRoles = MissingRoles.Num() > 0 ? JoinStrings(MissingRoles, TEXT("; ")) : TEXT("None");

		TArray<FString> Recommendations;
		if (Row.NeutralSupportCount <= 0)
		{
			Recommendations.Add(TEXT("Add Neutral support pool entries so each element pack has flexible deck-building glue."));
		}
		if (Row.ElementPoolCount <= 0)
		{
			Recommendations.Add(TEXT("Add production element cards or remove this pack from the shop until it has a real lane."));
		}
		if (NonNeutralElementNames.Num() > 1)
		{
			Recommendations.Add(TEXT("Element pack contains multiple non-neutral elements; split or remove off-element entries."));
		}
		if (MissingCardTypes.Num() > 0)
		{
			Recommendations.Add(FString::Printf(TEXT("Add missing card types: %s."), *Row.MissingCardTypes));
		}
		if (MissingRoles.Num() > 0)
		{
			Recommendations.Add(FString::Printf(TEXT("Add missing roles: %s."), *Row.MissingRoles));
		}
		if (RepeatedFingerprints.Num() > 0)
		{
			Recommendations.Add(TEXT("Rewrite or replace repeated tactical fingerprints."));
		}
		if (Row.DebugCardCount > 0)
		{
			Recommendations.Add(TEXT("Remove debug cards from production pack pools."));
		}

		Row.PackStatus = Recommendations.Num() > 0 ? TEXT("Review") : TEXT("Focused");
		Row.Recommendations = Recommendations.Num() > 0 ? JoinStrings(Recommendations, TEXT(" ")) : TEXT("Pack has focused element plus Neutral support coverage.");
		Rows.Add(Row);
	}

	Rows.Sort([](const FElementPackGapAuditRow& Left, const FElementPackGapAuditRow& Right)
	{
		return Left.PackName < Right.PackName;
	});

	return Rows;
}

TArray<FCardContentRecommendationRow> BuildCardContentRecommendationRows(
	const TArray<FCardElementCoverageAuditRow>& ElementCoverageRows,
	const TArray<FElementPackGapAuditRow>& ElementPackGapRows)
{
	TArray<FCardContentRecommendationRow> Rows;
	TSet<FString> RecommendationKeys;

	for (const FCardElementCoverageAuditRow& CoverageRow : ElementCoverageRows)
	{
		if (CoverageRow.Element == TEXT("Neutral"))
		{
			if (CoverageRow.ProductionCards < 4)
			{
				AddCardContentRecommendation(
					Rows,
					RecommendationKeys,
					TEXT("Medium"),
					CoverageRow.Element,
					TEXT("Spell"),
					TEXT("Defense/Utility"),
					TEXT("Neutral support pool is below the current four-card staple target."),
					TEXT("CardElementCoverageAudit"));
			}
			continue;
		}

		if (CoverageRow.SpellCards <= 0)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("High"), CoverageRow.Element, TEXT("Spell"), TEXT("Damage"), TEXT("Element has no production spell card."), TEXT("CardElementCoverageAudit"));
		}
		if (CoverageRow.SummonCards <= 0)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("High"), CoverageRow.Element, TEXT("Summon"), TEXT("Summon"), TEXT("Element has no production summon card."), TEXT("CardElementCoverageAudit"));
		}
		if (CoverageRow.TrapCards <= 0)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("High"), CoverageRow.Element, TEXT("Trap"), TEXT("Tactical"), TEXT("Element has no production trap card."), TEXT("CardElementCoverageAudit"));
		}
		if (CoverageRow.AuraCards <= 0)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("High"), CoverageRow.Element, TEXT("Aura"), TEXT("Tactical"), TEXT("Element has no production aura card."), TEXT("CardElementCoverageAudit"));
		}
		if (CoverageRow.Generators <= 0)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("High"), CoverageRow.Element, TEXT("Spell"), TEXT("Generator"), TEXT("Element has no production generator card."), TEXT("CardElementCoverageAudit"));
		}
		else if (CoverageRow.Generators < 2)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("Medium"), CoverageRow.Element, TEXT("Spell"), TEXT("Generator"), TEXT("Element has only one production generator card; target is at least two."), TEXT("CardElementCoverageAudit"));
		}
		if (CoverageRow.Payoffs <= 0)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("High"), CoverageRow.Element, TEXT("Spell"), TEXT("Element Payoff"), TEXT("Element has no production payoff card."), TEXT("CardElementCoverageAudit"));
		}
		else if (CoverageRow.Payoffs < 2)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("Medium"), CoverageRow.Element, TEXT("Spell"), TEXT("Element Payoff"), TEXT("Element has only one production payoff card; target is at least two."), TEXT("CardElementCoverageAudit"));
		}
		if (CoverageRow.DamageCards <= 0)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("High"), CoverageRow.Element, TEXT("Spell"), TEXT("Damage"), TEXT("Element has no production damage role."), TEXT("CardElementCoverageAudit"));
		}
		if ((CoverageRow.DefenseCards + CoverageRow.HealingCards + CoverageRow.ControlCards + CoverageRow.DrawCards + CoverageRow.EnergyCards) <= 0)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("High"), CoverageRow.Element, TEXT("Spell"), TEXT("Defense/Utility"), TEXT("Element has no production defense or utility role."), TEXT("CardElementCoverageAudit"));
		}
		if (CoverageRow.TacticalCards <= 0)
		{
			AddCardContentRecommendation(Rows, RecommendationKeys, TEXT("High"), CoverageRow.Element, TEXT("Spell"), TEXT("Tactical"), TEXT("Element has no production tactical role."), TEXT("CardElementCoverageAudit"));
		}
		else if (CoverageRow.ProductionCards < 8)
		{
			AddCardContentRecommendation(
				Rows,
				RecommendationKeys,
				TEXT("Low"),
				CoverageRow.Element,
				TEXT("Spell"),
				TEXT("Tactical"),
				TEXT("Element is below the long-term eight-card lane depth target."),
				TEXT("CardElementCoverageAudit"));
		}

		if (CoverageRow.Element == TEXT("Fire") && CoverageRow.ControlCards < 2)
		{
			AddCardContentRecommendation(
				Rows,
				RecommendationKeys,
				TEXT("Medium"),
				CoverageRow.Element,
				TEXT("Trap"),
				TEXT("Tactical"),
				TEXT("Fire has only one burn/control pressure card; add another hazard or pressure tool."),
				TEXT("CardElementCoverageAudit"));
		}
		else if (CoverageRow.Element == TEXT("Frost") && CoverageRow.DefenseCards < 2)
		{
			AddCardContentRecommendation(
				Rows,
				RecommendationKeys,
				TEXT("Medium"),
				CoverageRow.Element,
				TEXT("Spell"),
				TEXT("Defense/Utility"),
				TEXT("Frost shielded setup is thin; add another defensive control setup card."),
				TEXT("CardElementCoverageAudit"));
		}
		else if (CoverageRow.Element == TEXT("Storm") && CoverageRow.MobilityCards < 2)
		{
			AddCardContentRecommendation(
				Rows,
				RecommendationKeys,
				TEXT("Medium"),
				CoverageRow.Element,
				TEXT("Spell"),
				TEXT("Tactical"),
				TEXT("Storm mobility/pull-push tempo is thin; add another movement interaction card."),
				TEXT("CardElementCoverageAudit"));
		}
		else if (CoverageRow.Element == TEXT("Nature") && CoverageRow.DefenseCards < 2)
		{
			AddCardContentRecommendation(
				Rows,
				RecommendationKeys,
				TEXT("Low"),
				CoverageRow.Element,
				TEXT("Spell"),
				TEXT("Defense/Utility"),
				TEXT("Nature has healing and root coverage, but only one shield/growth setup card."),
				TEXT("CardElementCoverageAudit"));
		}
		else if (CoverageRow.Element == TEXT("Radiance") && CoverageRow.HealingCards < 2)
		{
			AddCardContentRecommendation(
				Rows,
				RecommendationKeys,
				TEXT("Medium"),
				CoverageRow.Element,
				TEXT("Spell"),
				TEXT("Defense/Utility"),
				TEXT("Radiance protection is strong, but direct healing has only one production card."),
				TEXT("CardElementCoverageAudit"));
		}
	}

	for (const FElementPackGapAuditRow& PackRow : ElementPackGapRows)
	{
		if (PackRow.PackStatus != TEXT("Review") || PackRow.DetectedElement.StartsWith(TEXT("Mixed")) || PackRow.DetectedElement == TEXT("NeutralOnly") || PackRow.DetectedElement == TEXT("None"))
		{
			continue;
		}

		if (PackRow.NeutralSupportCount <= 0)
		{
			AddCardContentRecommendation(
				Rows,
				RecommendationKeys,
				TEXT("Medium"),
				TEXT("Neutral"),
				TEXT("Spell"),
				TEXT("Defense/Utility"),
				FString::Printf(TEXT("%s has no Neutral support pool entries."), *PackRow.PackName),
				TEXT("ElementPackGapAudit"));
		}
	}

	const auto GetPriorityRank = [](const FString& Priority)
	{
		if (Priority == TEXT("High"))
		{
			return 0;
		}
		if (Priority == TEXT("Medium"))
		{
			return 1;
		}
		return 2;
	};

	Rows.Sort([&GetPriorityRank](const FCardContentRecommendationRow& Left, const FCardContentRecommendationRow& Right)
	{
		const int32 LeftRank = GetPriorityRank(Left.Priority);
		const int32 RightRank = GetPriorityRank(Right.Priority);
		if (LeftRank != RightRank)
		{
			return LeftRank < RightRank;
		}
		if (Left.Element != Right.Element)
		{
			return Left.Element < Right.Element;
		}
		if (Left.SuggestedCardType != Right.SuggestedCardType)
		{
			return Left.SuggestedCardType < Right.SuggestedCardType;
		}
		return Left.RoleFilled < Right.RoleFilled;
	});

	return Rows;
}

void AppendCardCsvLine(const FCardAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.AssetPath));
	Fields.Add(CsvEscape(Row.AssetName));
	Fields.Add(CsvEscape(Row.DisplayName));
	Fields.Add(CsvEscape(Row.Category));
	Fields.Add(CsvEscape(Row.CardElement));
	Fields.Add(CsvEscape(Row.TargetType));
	Fields.Add(CsvEscapeInt(Row.Cost));
	Fields.Add(CsvEscapeInt(Row.Range));
	Fields.Add(CsvEscapeBool(Row.bHasArt));
	Fields.Add(CsvEscapeBool(Row.bDescriptionEmpty));
	Fields.Add(CsvEscapeInt(Row.EffectsCount));
	Fields.Add(CsvEscape(Row.EffectsSummary));
	Fields.Add(CsvEscape(Row.PrimaryOperation));
	Fields.Add(CsvEscapeBool(Row.bUsedInPacks));
	Fields.Add(CsvEscape(Row.PackSummary));
	Fields.Add(CsvEscape(Row.ValidationStatus));
	Fields.Add(CsvEscape(JoinStrings(Row.Warnings)));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendCardBalanceCsvLine(const FCardBalanceAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.AssetPath));
	Fields.Add(CsvEscape(Row.AssetName));
	Fields.Add(CsvEscape(Row.DisplayName));
	Fields.Add(CsvEscape(Row.Category));
	Fields.Add(CsvEscape(Row.TargetType));
	Fields.Add(CsvEscapeInt(Row.Cost));
	Fields.Add(CsvEscapeInt(Row.BaseBalanceBudget));
	Fields.Add(CsvEscapeInt(Row.ElementalBonusBudget));
	Fields.Add(CsvEscapeInt(Row.ExpectedCostMin));
	Fields.Add(CsvEscapeInt(Row.ExpectedCostMax));
	Fields.Add(CsvEscapeInt(Row.SuggestedCost));
	Fields.Add(CsvEscape(Row.BalanceStatus));
	Fields.Add(CsvEscape(Row.RoleTags));
	Fields.Add(CsvEscape(Row.ElementTags));
	Fields.Add(CsvEscape(Row.ProductionStatus));
	Fields.Add(CsvEscape(Row.PackStatus));
	Fields.Add(CsvEscape(Row.EffectsSummary));
	Fields.Add(CsvEscape(JoinStrings(Row.Warnings)));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendDescriptionSuggestionCsvLine(const FCardDescriptionSuggestionRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.AssetPath));
	Fields.Add(CsvEscape(Row.AssetName));
	Fields.Add(CsvEscape(Row.DisplayName));
	Fields.Add(CsvEscape(Row.CurrentDescription));
	Fields.Add(CsvEscape(Row.SuggestedDescription));
	Fields.Add(CsvEscape(Row.Status));
	Fields.Add(CsvEscape(JoinStrings(Row.Warnings)));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendVarietyMatrixCsvLine(const FCardVarietyMatrixRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.Element));
	Fields.Add(CsvEscape(Row.Role));
	Fields.Add(CsvEscape(Row.Category));
	Fields.Add(CsvEscape(Row.ProductionStatus));
	Fields.Add(CsvEscape(Row.PackStatus));
	Fields.Add(CsvEscapeInt(Row.CardCount));
	Fields.Add(CsvEscape(JoinStrings(Row.CardNames, TEXT("; "))));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendCardDesignCsvLine(const FCardDesignAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.AssetPath));
	Fields.Add(CsvEscape(Row.AssetName));
	Fields.Add(CsvEscape(Row.DisplayName));
	Fields.Add(CsvEscape(Row.Category));
	Fields.Add(CsvEscapeInt(Row.Cost));
	Fields.Add(CsvEscape(Row.ElementTags));
	Fields.Add(CsvEscape(Row.RoleTags));
	Fields.Add(CsvEscape(Row.PrimaryKeyword));
	Fields.Add(CsvEscape(Row.SecondaryKeywords));
	Fields.Add(CsvEscape(Row.OperationSummary));
	Fields.Add(CsvEscape(Row.DeliverySummary));
	Fields.Add(CsvEscape(Row.TargetFilterSummary));
	Fields.Add(CsvEscape(Row.PayloadSummary));
	Fields.Add(CsvEscape(Row.ConditionSummary));
	Fields.Add(CsvEscape(Row.KeywordTerms));
	Fields.Add(CsvEscape(Row.SuspiciousPattern));
	Fields.Add(CsvEscape(Row.SamenessFingerprint));
	Fields.Add(CsvEscapeInt(Row.MatchingFingerprintCount));
	Fields.Add(CsvEscapeInt(Row.TacticalScore));
	Fields.Add(CsvEscape(Row.DesignStatus));
	Fields.Add(CsvEscape(JoinStrings(Row.Warnings)));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendElementIdentityCsvLine(const FElementIdentityAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.Element));
	Fields.Add(CsvEscapeInt(Row.ProductionCards));
	Fields.Add(CsvEscapeInt(Row.Generators));
	Fields.Add(CsvEscapeInt(Row.Payoffs));
	Fields.Add(CsvEscapeInt(Row.DamageCards));
	Fields.Add(CsvEscapeInt(Row.DefenseUtilityCards));
	Fields.Add(CsvEscapeInt(Row.TacticalCards));
	Fields.Add(CsvEscape(Row.UniquePrimaryKeywords));
	Fields.Add(CsvEscape(Row.MissingLanes));
	Fields.Add(CsvEscape(Row.RecommendedActions));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendPackExperienceCsvLine(const FPackExperienceAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.PackAssetPath));
	Fields.Add(CsvEscape(Row.PackName));
	Fields.Add(CsvEscapeInt(Row.CardCount));
	Fields.Add(CsvEscapeInt(Row.UniqueRoles));
	Fields.Add(CsvEscapeInt(Row.UniqueElements));
	Fields.Add(CsvEscapeInt(Row.UniquePrimaryKeywords));
	Fields.Add(CsvEscape(FString::Printf(TEXT("%.2f"), Row.AverageCost)));
	Fields.Add(CsvEscapeInt(Row.RepeatedFingerprintGroups));
	Fields.Add(CsvEscape(Row.RoleSpread));
	Fields.Add(CsvEscape(Row.ElementMix));
	Fields.Add(CsvEscape(Row.CostCurve));
	Fields.Add(CsvEscape(Row.ExperienceStatus));
	Fields.Add(CsvEscape(JoinStrings(Row.Warnings)));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendCardRewritePlanCsvLine(const FCardRewritePlanRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.AssetPath));
	Fields.Add(CsvEscape(Row.DisplayName));
	Fields.Add(CsvEscape(Row.CurrentIssue));
	Fields.Add(CsvEscape(Row.RecommendedAction));
	Fields.Add(CsvEscape(Row.ProposedRulesText));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendCardElementCoverageCsvLine(const FCardElementCoverageAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.Element));
	Fields.Add(CsvEscapeInt(Row.ProductionCards));
	Fields.Add(CsvEscapeInt(Row.SpellCards));
	Fields.Add(CsvEscapeInt(Row.SummonCards));
	Fields.Add(CsvEscapeInt(Row.TrapCards));
	Fields.Add(CsvEscapeInt(Row.AuraCards));
	Fields.Add(CsvEscapeInt(Row.DamageCards));
	Fields.Add(CsvEscapeInt(Row.DefenseCards));
	Fields.Add(CsvEscapeInt(Row.HealingCards));
	Fields.Add(CsvEscapeInt(Row.ControlCards));
	Fields.Add(CsvEscapeInt(Row.MobilityCards));
	Fields.Add(CsvEscapeInt(Row.DrawCards));
	Fields.Add(CsvEscapeInt(Row.EnergyCards));
	Fields.Add(CsvEscapeInt(Row.Generators));
	Fields.Add(CsvEscapeInt(Row.Payoffs));
	Fields.Add(CsvEscapeInt(Row.TacticalCards));
	Fields.Add(CsvEscapeInt(Row.UniqueOperations));
	Fields.Add(CsvEscape(Row.StatusTerms));
	Fields.Add(CsvEscape(Row.MissingCardTypes));
	Fields.Add(CsvEscape(Row.MissingRoles));
	Fields.Add(CsvEscape(Row.CoverageStatus));
	Fields.Add(CsvEscape(Row.RecommendedFocus));
	Fields.Add(CsvEscape(Row.CardNames));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendElementPackGapCsvLine(const FElementPackGapAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.PackAssetPath));
	Fields.Add(CsvEscape(Row.PackName));
	Fields.Add(CsvEscape(Row.DetectedElement));
	Fields.Add(CsvEscapeInt(Row.NeutralSupportCount));
	Fields.Add(CsvEscapeInt(Row.NeutralSupportWeight));
	Fields.Add(CsvEscapeInt(Row.ElementPoolCount));
	Fields.Add(CsvEscapeInt(Row.ElementPoolWeight));
	Fields.Add(CsvEscapeInt(Row.OtherElementCount));
	Fields.Add(CsvEscapeInt(Row.DebugCardCount));
	Fields.Add(CsvEscapeInt(Row.SpellCards));
	Fields.Add(CsvEscapeInt(Row.SummonCards));
	Fields.Add(CsvEscapeInt(Row.TrapCards));
	Fields.Add(CsvEscapeInt(Row.AuraCards));
	Fields.Add(CsvEscape(Row.RoleSpread));
	Fields.Add(CsvEscape(Row.ElementMix));
	Fields.Add(CsvEscape(Row.RepeatedFingerprintGroups));
	Fields.Add(CsvEscape(Row.MissingCardTypes));
	Fields.Add(CsvEscape(Row.MissingRoles));
	Fields.Add(CsvEscape(Row.PackStatus));
	Fields.Add(CsvEscape(Row.Recommendations));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendCardContentRecommendationCsvLine(const FCardContentRecommendationRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.Priority));
	Fields.Add(CsvEscape(Row.Element));
	Fields.Add(CsvEscape(Row.SuggestedCardType));
	Fields.Add(CsvEscape(Row.RoleFilled));
	Fields.Add(CsvEscape(Row.Reason));
	Fields.Add(CsvEscape(Row.IntendedOperation));
	Fields.Add(CsvEscape(Row.Delivery));
	Fields.Add(CsvEscape(Row.Payload));
	Fields.Add(CsvEscape(Row.DraftRulesText));
	Fields.Add(CsvEscape(Row.CardArtPrompt));
	Fields.Add(CsvEscape(Row.SourceReport));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendEffectVocabularyFitCsvLine(const FEffectVocabularyFitAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.Scope));
	Fields.Add(CsvEscapeInt(Row.ProductionCards));
	Fields.Add(CsvEscapeInt(Row.TotalEffectLines));
	Fields.Add(CsvEscapeInt(Row.UniqueOperations));
	Fields.Add(CsvEscapeInt(Row.UniqueDeliveries));
	Fields.Add(CsvEscape(Row.StructuralCoverageStatus));
	Fields.Add(CsvEscape(Row.MechanicalVarietyStatus));
	Fields.Add(CsvEscape(Row.OperationMix));
	Fields.Add(CsvEscape(Row.DeliveryMix));
	Fields.Add(CsvEscape(Row.StatusTerms));
	Fields.Add(CsvEscape(Row.DominantOperation));
	Fields.Add(CsvEscape(Row.RecommendedFirstBatch));
	Fields.Add(CsvEscape(Row.Notes));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendPackCsvLine(const FPackAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.AssetPath));
	Fields.Add(CsvEscape(Row.PackName));
	Fields.Add(CsvEscapeInt(Row.PriceCopper));
	Fields.Add(CsvEscapeInt(Row.AmountToGrant));
	Fields.Add(CsvEscapeInt(Row.CardPoolCount));
	Fields.Add(CsvEscapeInt(Row.ValidEntryCount));
	Fields.Add(CsvEscapeInt(Row.TotalValidWeight));
	Fields.Add(CsvEscapeBool(Row.bCanRollValidCard));
	Fields.Add(CsvEscape(JoinStrings(Row.Warnings)));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}

void AppendSummonCsvLine(const FSummonAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.AssetPath));
	Fields.Add(CsvEscape(Row.AssetName));
	Fields.Add(CsvEscape(Row.DisplayName));
	Fields.Add(CsvEscapeInt(Row.MaxHP));
	Fields.Add(CsvEscapeInt(Row.MoveRange));
	Fields.Add(CsvEscapeInt(Row.AttackRange));
	Fields.Add(CsvEscapeInt(Row.AttackDamage));
	Fields.Add(CsvEscape(Row.Team));
	Fields.Add(CsvEscapeInt(Row.OnSummonedEffectsCount));
	Fields.Add(CsvEscapeInt(Row.OnTurnStartEffectsCount));
	Fields.Add(CsvEscapeInt(Row.OnDeathEffectsCount));
	Fields.Add(CsvEscapeBool(Row.bIsValidDefinition));
	Fields.Add(CsvEscapeBool(Row.bScannedByPath));
	Fields.Add(CsvEscapeBool(Row.bReferencedByScannedCards));
	Fields.Add(CsvEscape(Row.Summary));
	Fields.Add(CsvEscape(JoinStrings(Row.Warnings)));
	Csv += FString::Join(Fields, TEXT(",")) + LINE_TERMINATOR;
}
#endif
}

UCardCatalogAuditTool::UCardCatalogAuditTool()
{
	CardScanPaths.Add(DefaultCardScanPath);
	CardScanPaths.Add(LegacyCardScanPath);
	PackScanPaths.Add(DefaultPackScanPath);
	PackScanPaths.Add(LegacyPackScanPath);
	SummonedUnitScanPaths.Add(DefaultSummonedUnitScanPath);
}

void UCardCatalogAuditTool::RunCardCatalogAudit()
{
#if WITH_EDITOR
	const TArray<FName> EffectiveCardScanPaths = GetEffectiveScanPaths(
		CardScanPaths,
		{ DefaultCardScanPath, LegacyCardScanPath });
	const TArray<FName> EffectivePackScanPaths = GetEffectiveScanPaths(
		PackScanPaths,
		{ DefaultPackScanPath, LegacyPackScanPath });
	const TArray<FName> EffectiveSummonedUnitScanPaths = GetEffectiveScanPaths(
		SummonedUnitScanPaths,
		{ DefaultSummonedUnitScanPath, LegacySummonedUnitScanPath });

	TArray<UCardDefinition*> Cards;
	TArray<UCardPackDefinition*> Packs;
	TArray<UJargonSummonedUnitDefinition*> SummonedUnitDefinitions;
	LoadAssetsFromPaths(EffectiveCardScanPaths, Cards);
	LoadAssetsFromPaths(EffectivePackScanPaths, Packs);
	LoadAssetsFromPaths(EffectiveSummonedUnitScanPaths, SummonedUnitDefinitions);

	TMap<FString, int32> DisplayNameCounts;
	for (const UCardDefinition* Card : Cards)
	{
		if (!Card)
		{
			continue;
		}

		const FString DisplayName = Card->DisplayName.ToString().TrimStartAndEnd();
		if (!DisplayName.IsEmpty())
		{
			DisplayNameCounts.FindOrAdd(DisplayName)++;
		}
	}

	TMap<const UCardDefinition*, TArray<FCardPackUsage>> PackUsagesByCard;
	for (const UCardPackDefinition* Pack : Packs)
	{
		if (!Pack)
		{
			continue;
		}

		const FString PackName = TextOrFallbackName(Pack->DisplayName, Pack);
		for (const FWeightedCardPackEntry& Entry : Pack->CardPool)
		{
			if (Entry.CardDefinition)
			{
				FCardPackUsage& PackUsage = PackUsagesByCard.FindOrAdd(Entry.CardDefinition).AddDefaulted_GetRef();
				PackUsage.PackName = PackName;
				PackUsage.Weight = Entry.Weight;
			}
		}
	}

	TSet<FString> ScannedSummonDefinitionPaths;
	TSet<FString> SeenSummonDefinitionPaths;
	TSet<FString> ReferencedSummonDefinitionPaths;
	for (const UJargonSummonedUnitDefinition* Definition : SummonedUnitDefinitions)
	{
		if (Definition)
		{
			ScannedSummonDefinitionPaths.Add(Definition->GetPathName());
			SeenSummonDefinitionPaths.Add(Definition->GetPathName());
		}
	}

	for (const UCardDefinition* Card : Cards)
	{
		if (!Card)
		{
			continue;
		}

		const auto AddReferencedSummonDefinition = [&SummonedUnitDefinitions, &SeenSummonDefinitionPaths, &ReferencedSummonDefinitionPaths](UJargonSummonedUnitDefinition* Definition)
		{
			if (!Definition)
			{
				return;
			}

			const FString DefinitionPath = Definition->GetPathName();
			ReferencedSummonDefinitionPaths.Add(DefinitionPath);
			if (!SeenSummonDefinitionPaths.Contains(DefinitionPath))
			{
				SeenSummonDefinitionPaths.Add(DefinitionPath);
				SummonedUnitDefinitions.Add(Definition);
			}
		};

		TArray<FJargonEffectSpec> AuditEffects;
		TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
		BuildAuditEffectSpecs(Card, AuditEffects);
		BuildAuditElementalBonusGroups(Card, AuditBonusGroups);

		for (const FJargonEffectSpec& EffectSpec : AuditEffects)
		{
			if (EffectSpec.Operation == EJargonEffectOperation::SummonUnit)
			{
				AddReferencedSummonDefinition(EffectSpec.SummonedUnitDefinition.Get());
			}
		}

		for (const FCardAuditElementalBonusGroup& BonusGroup : AuditBonusGroups)
		{
			for (const FJargonEffectSpec& BonusEffect : BonusGroup.BonusEffects)
			{
				if (BonusEffect.Operation == EJargonEffectOperation::SummonUnit)
				{
					AddReferencedSummonDefinition(BonusEffect.SummonedUnitDefinition.Get());
				}
			}
		}
	}

	TArray<FCardAuditRow> CardRows;
	CardRows.Reserve(Cards.Num());
	TArray<FCardBalanceAuditRow> BalanceRows;
	BalanceRows.Reserve(Cards.Num());
	TArray<FCardDescriptionSuggestionRow> DescriptionRows;
	DescriptionRows.Reserve(Cards.Num());
	TArray<FCardDesignAuditRow> DesignRows;
	DesignRows.Reserve(Cards.Num());
	TArray<FCardRewritePlanRow> RewriteRows;
	RewriteRows.Reserve(Cards.Num());
	TMap<FString, FCardVarietyMatrixRow> VarietyRowsByKey;
	TMap<const UCardDefinition*, bool> CardFatalStatusByCard;
	TMap<FString, int32> FingerprintCounts;
	int32 ElementGeneratorCardCount = 0;
	int32 ElementalBonusCardCount = 0;

	for (const UCardDefinition* Card : Cards)
	{
		if (Card && !IsLikelyDebugPathOrName(Card->GetPathName(), Card->GetName()))
		{
			FingerprintCounts.FindOrAdd(BuildCardSamenessFingerprint(Card))++;
		}
	}

	for (const UCardDefinition* Card : Cards)
	{
		if (!Card)
		{
			continue;
		}

		FCardAuditRow Row;
		Row.AssetPath = Card->GetPathName();
		Row.AssetName = Card->GetName();
		Row.DisplayName = TextOrFallbackName(Card->DisplayName, Card);
		Row.Category = GetCardCategoryName(Card->Category);
		Row.CardElement = GetElementTypeName(Card->CardElement);
		Row.TargetType = GetCardTargetTypeName(Card->TargetType);
		Row.Cost = Card->Cost;
		Row.Range = Card->Range;
		Row.bHasArt = Card->CardArt != nullptr;
		Row.bDescriptionEmpty = Card->Description.ToString().TrimStartAndEnd().IsEmpty();
		TArray<FJargonEffectSpec> AuditEffects;
		TArray<FCardAuditElementalBonusGroup> AuditBonusGroups;
		BuildAuditEffectSpecs(Card, AuditEffects);
		BuildAuditElementalBonusGroups(Card, AuditBonusGroups);
		Row.EffectsCount = AuditEffects.Num();
		Row.EffectsSummary = BuildEffectSummary(Card);
		Row.PrimaryOperation = AuditEffects.Num() > 0
			? GetCardEffectOperationName(AuditEffects[0].Operation)
			: TEXT("None");

		if (CardHasElementGenerator(Card))
		{
			ElementGeneratorCardCount++;
		}

		if (AuditBonusGroups.Num() > 0)
		{
			ElementalBonusCardCount++;
		}

		TArray<FString> FatalWarnings;
		TArray<FString> SoftWarnings;
		AddCardIntrinsicWarnings(Card, FatalWarnings, SoftWarnings);

		const FString RawDisplayName = Card->DisplayName.ToString().TrimStartAndEnd();
		if (!RawDisplayName.IsEmpty())
		{
			if (const int32* DisplayNameCount = DisplayNameCounts.Find(RawDisplayName))
			{
				if (*DisplayNameCount > 1)
				{
					SoftWarnings.Add(FString::Printf(TEXT("Warning: duplicate DisplayName '%s'."), *RawDisplayName));
				}
			}
		}

		const TArray<FCardPackUsage>* PackUsages = PackUsagesByCard.Find(Card);
		Row.bUsedInPacks = PackUsages && PackUsages->Num() > 0;
		Row.PackSummary = Row.bUsedInPacks ? BuildPackSummaryForCard(*PackUsages) : FString();
		const bool bLooksDebug = IsLikelyDebugPathOrName(Card->GetPathName(), Card->GetName());
		if (!Row.bUsedInPacks && !bLooksDebug)
		{
			SoftWarnings.Add(TEXT("Warning: card is not included in any scanned pack."));
		}

		BalanceRows.Add(BuildCardBalanceAuditRow(Card, Row.bUsedInPacks, Row.PackSummary));
		FCardDescriptionSuggestionRow DescriptionRow = BuildDescriptionSuggestionRow(Card);
		DescriptionRows.Add(DescriptionRow);
		if (!bLooksDebug && DescriptionRow.Status == TEXT("Missing"))
		{
			SoftWarnings.Add(TEXT("Warning: Description is missing; generated rules-first keyword text is available in CardDescriptionSuggestions.csv."));
		}
		else if (!bLooksDebug && DescriptionRow.Status == TEXT("Review"))
		{
			SoftWarnings.Add(TEXT("Review: authored Description differs from generated rules-first keyword text. See CardDescriptionSuggestions.csv."));
		}
		FCardDesignAuditRow DesignRow = BuildCardDesignAuditRow(Card, FingerprintCounts);
		DesignRows.Add(DesignRow);
		if (!IsLikelyDebugPathOrName(Card->GetPathName(), Card->GetName()) && DesignRow.Warnings.Num() > 0)
		{
			FCardRewritePlanRow RewriteRow;
			RewriteRow.AssetPath = Card->GetPathName();
			RewriteRow.DisplayName = Row.DisplayName;
			RewriteRow.CurrentIssue = JoinStrings(DesignRow.Warnings);
			RewriteRow.RecommendedAction = TEXT("Review for a clearer element lane, tactical rider, or rules-first wording.");
			RewriteRow.ProposedRulesText = BuildSuggestedRulesTextForCard(Card);
			RewriteRows.Add(RewriteRow);
		}
		AddVarietyMatrixEntriesForCard(Card, Row.bUsedInPacks, VarietyRowsByKey);

		Row.bHasFatalWarnings = FatalWarnings.Num() > 0;
		Row.Warnings.Append(FatalWarnings);
		Row.Warnings.Append(SoftWarnings);
		Row.ValidationStatus = Row.bHasFatalWarnings
			? TEXT("Invalid")
			: (Row.Warnings.Num() > 0 ? TEXT("Warning") : TEXT("Valid"));

		CardFatalStatusByCard.Add(Card, Row.bHasFatalWarnings);
		CardRows.Add(Row);
	}

	CardRows.Sort([](const FCardAuditRow& Left, const FCardAuditRow& Right)
	{
		if (Left.Category != Right.Category)
		{
			return Left.Category < Right.Category;
		}

		if (Left.Cost != Right.Cost)
		{
			return Left.Cost < Right.Cost;
		}

		return Left.DisplayName < Right.DisplayName;
	});

	BalanceRows.Sort([](const FCardBalanceAuditRow& Left, const FCardBalanceAuditRow& Right)
	{
		if (Left.BalanceStatus != Right.BalanceStatus)
		{
			return Left.BalanceStatus > Right.BalanceStatus;
		}

		if (Left.Category != Right.Category)
		{
			return Left.Category < Right.Category;
		}

		if (Left.Cost != Right.Cost)
		{
			return Left.Cost < Right.Cost;
		}

		return Left.DisplayName < Right.DisplayName;
	});

	DescriptionRows.Sort([](const FCardDescriptionSuggestionRow& Left, const FCardDescriptionSuggestionRow& Right)
	{
		if (Left.Status != Right.Status)
		{
			return Left.Status > Right.Status;
		}

		return Left.DisplayName < Right.DisplayName;
	});

	DesignRows.Sort([](const FCardDesignAuditRow& Left, const FCardDesignAuditRow& Right)
	{
		if (Left.DesignStatus != Right.DesignStatus)
		{
			return Left.DesignStatus > Right.DesignStatus;
		}
		if (Left.TacticalScore != Right.TacticalScore)
		{
			return Left.TacticalScore < Right.TacticalScore;
		}
		return Left.DisplayName < Right.DisplayName;
	});

	RewriteRows.Sort([](const FCardRewritePlanRow& Left, const FCardRewritePlanRow& Right)
	{
		return Left.DisplayName < Right.DisplayName;
	});

	TArray<FCardVarietyMatrixRow> VarietyRows;
	VarietyRowsByKey.GenerateValueArray(VarietyRows);
	VarietyRows.Sort([](const FCardVarietyMatrixRow& Left, const FCardVarietyMatrixRow& Right)
	{
		if (Left.Element != Right.Element)
		{
			return Left.Element < Right.Element;
		}
		if (Left.Role != Right.Role)
		{
			return Left.Role < Right.Role;
		}
		if (Left.Category != Right.Category)
		{
			return Left.Category < Right.Category;
		}
		if (Left.ProductionStatus != Right.ProductionStatus)
		{
			return Left.ProductionStatus < Right.ProductionStatus;
		}
		return Left.PackStatus < Right.PackStatus;
	});

	TArray<FString> VarietyGapWarnings;
	AddVarietyGapWarnings(BalanceRows, VarietyGapWarnings);

	const TArray<FElementIdentityAuditRow> ElementIdentityRows = BuildElementIdentityRows(BalanceRows, DesignRows);
	const TArray<FPackExperienceAuditRow> PackExperienceRows = BuildPackExperienceRows(Packs);
	const TArray<FCardElementCoverageAuditRow> ElementCoverageRows = BuildCardElementCoverageRows(Cards);
	const TArray<FElementPackGapAuditRow> ElementPackGapRows = BuildElementPackGapRows(Packs);
	const TArray<FCardContentRecommendationRow> ContentRecommendationRows = BuildCardContentRecommendationRows(ElementCoverageRows, ElementPackGapRows);
	const TArray<FEffectVocabularyFitAuditRow> EffectVocabularyFitRows = BuildEffectVocabularyFitRows(Cards, ElementCoverageRows);

	TArray<FPackAuditRow> PackRows;
	PackRows.Reserve(Packs.Num());

	for (const UCardPackDefinition* Pack : Packs)
	{
		if (!Pack)
		{
			continue;
		}

		FPackAuditRow Row;
		Row.AssetPath = Pack->GetPathName();
		Row.PackName = TextOrFallbackName(Pack->DisplayName, Pack);
		Row.PriceCopper = Pack->Price.GetTotalCopperValue();
		Row.AmountToGrant = Pack->AmountToGrant;
		Row.CardPoolCount = Pack->CardPool.Num();

		if (Pack->Price.Gold < 0 || Pack->Price.Silver < 0 || Pack->Price.Copper < 0)
		{
			Row.Warnings.Add(TEXT("Invalid: pack price has negative currency values."));
		}

		if (Pack->AmountToGrant <= 0)
		{
			Row.Warnings.Add(TEXT("Invalid: AmountToGrant must be greater than 0."));
		}

		if (Pack->CardPool.Num() == 0)
		{
			Row.Warnings.Add(TEXT("Invalid: card pool is empty."));
		}

		for (int32 EntryIndex = 0; EntryIndex < Pack->CardPool.Num(); ++EntryIndex)
		{
			const FWeightedCardPackEntry& Entry = Pack->CardPool[EntryIndex];
			if (!Entry.CardDefinition)
			{
				Row.Warnings.Add(FString::Printf(TEXT("Invalid: entry %d has no CardDefinition."), EntryIndex));
				continue;
			}

			const bool bPackLooksDebug = IsLikelyDebugPathOrName(Row.AssetPath, Row.PackName);
			const bool bCardLooksDebug = IsLikelyDebugPathOrName(Entry.CardDefinition->GetPathName(), Entry.CardDefinition->GetName());
			if (bCardLooksDebug && !bPackLooksDebug)
			{
				Row.Warnings.Add(FString::Printf(
					TEXT("Warning: entry %d card '%s' looks like debug/dev content but appears in non-debug pack '%s'."),
					EntryIndex,
					*GetNameSafe(Entry.CardDefinition),
					*Row.PackName));
			}

			if (Entry.Weight <= 0)
			{
				Row.Warnings.Add(FString::Printf(
					TEXT("Invalid: entry %d card '%s' has weight <= 0."),
					EntryIndex,
					*GetNameSafe(Entry.CardDefinition)));
				continue;
			}

			bool bCardHasFatalWarnings = false;
			if (const bool* KnownFatalStatus = CardFatalStatusByCard.Find(Entry.CardDefinition))
			{
				bCardHasFatalWarnings = *KnownFatalStatus;
			}
			else
			{
				bCardHasFatalWarnings = DoesCardHaveFatalIntrinsicWarnings(Entry.CardDefinition);
				Row.Warnings.Add(FString::Printf(
					TEXT("Warning: entry %d card '%s' is outside scanned card paths."),
					EntryIndex,
					*GetNameSafe(Entry.CardDefinition)));
			}

			if (bCardHasFatalWarnings)
			{
				Row.Warnings.Add(FString::Printf(
					TEXT("Invalid: entry %d card '%s' failed card validation."),
					EntryIndex,
					*GetNameSafe(Entry.CardDefinition)));
				continue;
			}

			Row.ValidEntryCount++;
			Row.TotalValidWeight += Entry.Weight;
		}

		if (!Pack->bAllowDuplicateCardsPerPurchase && Pack->AmountToGrant > Row.ValidEntryCount)
		{
			Row.Warnings.Add(FString::Printf(
				TEXT("Warning: AmountToGrant=%d but only %d valid unique cards are available."),
				Pack->AmountToGrant,
				Row.ValidEntryCount));
		}

		Row.bCanRollValidCard = Row.ValidEntryCount > 0 && Row.TotalValidWeight > 0;
		if (!Row.bCanRollValidCard)
		{
			Row.Warnings.Add(TEXT("Invalid: pack cannot roll any valid card."));
		}

		PackRows.Add(Row);
	}

	PackRows.Sort([](const FPackAuditRow& Left, const FPackAuditRow& Right)
	{
		return Left.PackName < Right.PackName;
	});

	TArray<FSummonAuditRow> SummonRows;
	SummonRows.Reserve(SummonedUnitDefinitions.Num());
	for (const UJargonSummonedUnitDefinition* Definition : SummonedUnitDefinitions)
	{
		if (!Definition)
		{
			continue;
		}

		FSummonAuditRow Row;
		Row.AssetPath = Definition->GetPathName();
		Row.AssetName = Definition->GetName();
		Row.DisplayName = TextOrFallbackName(Definition->DisplayName, Definition);
		Row.MaxHP = Definition->MaxHP;
		Row.MoveRange = Definition->MoveRange;
		Row.AttackRange = Definition->AttackRange;
		Row.AttackDamage = Definition->AttackDamage;
		Row.Team = GetTeamName(Definition->Team);
		Row.OnSummonedEffectsCount = Definition->OnSummonedEffects.Num();
		Row.OnTurnStartEffectsCount = Definition->OnTurnStartEffects.Num();
		Row.OnDeathEffectsCount = Definition->OnDeathEffects.Num();
		Row.bIsValidDefinition = Definition->IsValidDefinition();
		Row.bScannedByPath = ScannedSummonDefinitionPaths.Contains(Row.AssetPath);
		Row.bReferencedByScannedCards = ReferencedSummonDefinitionPaths.Contains(Row.AssetPath);
		Row.Summary = Definition->GetAuditSummary();
		AddSummonDefinitionWarnings(Definition, Row.bScannedByPath, Row.bReferencedByScannedCards, Row.Warnings);
		SummonRows.Add(Row);
	}

	SummonRows.Sort([](const FSummonAuditRow& Left, const FSummonAuditRow& Right)
	{
		return Left.DisplayName < Right.DisplayName;
	});

	int32 InvalidCardCount = 0;
	int32 WarningCardCount = 0;
	for (const FCardAuditRow& Row : CardRows)
	{
		if (Row.ValidationStatus == TEXT("Invalid"))
		{
			InvalidCardCount++;
		}
		else if (Row.ValidationStatus == TEXT("Warning"))
		{
			WarningCardCount++;
		}
	}

	int32 PackWarningCount = 0;
	for (const FPackAuditRow& Row : PackRows)
	{
		if (Row.Warnings.Num() > 0)
		{
			PackWarningCount++;
		}
	}

	int32 SummonInvalidCount = 0;
	int32 SummonWarningCount = 0;
	for (const FSummonAuditRow& Row : SummonRows)
	{
		if (!Row.bIsValidDefinition)
		{
			SummonInvalidCount++;
		}
		else if (Row.Warnings.Num() > 0)
		{
			SummonWarningCount++;
		}
	}

	int32 BalanceReviewCount = 0;
	for (const FCardBalanceAuditRow& Row : BalanceRows)
	{
		if (Row.BalanceStatus == TEXT("Review"))
		{
			BalanceReviewCount++;
		}
	}

	int32 DescriptionReviewCount = 0;
	int32 DescriptionMissingCount = 0;
	for (const FCardDescriptionSuggestionRow& Row : DescriptionRows)
	{
		if (Row.Status == TEXT("Missing"))
		{
			DescriptionMissingCount++;
		}
		else if (Row.Status == TEXT("Review"))
		{
			DescriptionReviewCount++;
		}
	}

	int32 DesignReviewCount = 0;
	for (const FCardDesignAuditRow& Row : DesignRows)
	{
		if (Row.DesignStatus == TEXT("Review"))
		{
			DesignReviewCount++;
		}
	}

	int32 PackExperienceReviewCount = 0;
	for (const FPackExperienceAuditRow& Row : PackExperienceRows)
	{
		if (Row.ExperienceStatus == TEXT("Review"))
		{
			PackExperienceReviewCount++;
		}
	}

	int32 ElementCoverageGapCount = 0;
	for (const FCardElementCoverageAuditRow& Row : ElementCoverageRows)
	{
		if (Row.CoverageStatus != TEXT("Covered"))
		{
			ElementCoverageGapCount++;
		}
	}

	int32 ElementPackGapReviewCount = 0;
	for (const FElementPackGapAuditRow& Row : ElementPackGapRows)
	{
		if (Row.PackStatus == TEXT("Review"))
		{
			ElementPackGapReviewCount++;
		}
	}

	int32 HighPriorityRecommendationCount = 0;
	for (const FCardContentRecommendationRow& Row : ContentRecommendationRows)
	{
		if (Row.Priority == TEXT("High"))
		{
			HighPriorityRecommendationCount++;
		}
	}

	UE_LOG(LogCardCatalogAudit, Display, TEXT("=== Card Catalog Audit ==="));
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Card paths: %s"), *JoinStrings([&EffectiveCardScanPaths]()
	{
		TArray<FString> PathStrings;
		for (const FName& Path : EffectiveCardScanPaths)
		{
			PathStrings.Add(Path.ToString());
		}
		return PathStrings;
	}(), TEXT(", ")));
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Pack paths: %s"), *JoinStrings([&EffectivePackScanPaths]()
	{
		TArray<FString> PathStrings;
		for (const FName& Path : EffectivePackScanPaths)
		{
			PathStrings.Add(Path.ToString());
		}
		return PathStrings;
	}(), TEXT(", ")));
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Summon definition paths: %s"), *JoinStrings([&EffectiveSummonedUnitScanPaths]()
	{
		TArray<FString> PathStrings;
		for (const FName& Path : EffectiveSummonedUnitScanPaths)
		{
			PathStrings.Add(Path.ToString());
		}
		return PathStrings;
	}(), TEXT(", ")));
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Cards found: %d | Invalid: %d | Warning: %d"), CardRows.Num(), InvalidCardCount, WarningCardCount);
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Packs found: %d | Packs with warnings: %d"), PackRows.Num(), PackWarningCount);
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Summon definitions found/referenced: %d | Invalid: %d | Warning: %d"), SummonRows.Num(), SummonInvalidCount, SummonWarningCount);
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Element content: Generators=%d | Bonus Cards=%d"), ElementGeneratorCardCount, ElementalBonusCardCount);
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Balance review: Cards=%d | Review=%d | VarietyRows=%d | VarietyGaps=%d"), BalanceRows.Num(), BalanceReviewCount, VarietyRows.Num(), VarietyGapWarnings.Num());
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Description review: Cards=%d | Review=%d | Missing=%d"), DescriptionRows.Num(), DescriptionReviewCount, DescriptionMissingCount);
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Design review: Cards=%d | Review=%d | ElementIdentityRows=%d | PackExperienceReview=%d | RewriteRows=%d"),
		DesignRows.Num(),
		DesignReviewCount,
		ElementIdentityRows.Num(),
		PackExperienceReviewCount,
		RewriteRows.Num());
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Content gaps: ElementCoverageRows=%d | GapRows=%d | ElementPackRows=%d | PackReviews=%d | Recommendations=%d | HighPriority=%d"),
		ElementCoverageRows.Num(),
		ElementCoverageGapCount,
		ElementPackGapRows.Num(),
		ElementPackGapReviewCount,
		ContentRecommendationRows.Num(),
		HighPriorityRecommendationCount);
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Effect vocabulary fit rows: %d | First expansion batch: Cleanse/Remove Status, Regen, Weak, Lifesteal"), EffectVocabularyFitRows.Num());
	if (Cards.Num() > 0 && ElementGeneratorCardCount == 0)
	{
		UE_LOG(LogCardCatalogAudit, Warning, TEXT("Element charge backend exists, but no scanned card currently gains element charges."));
	}
	if (Cards.Num() > 0 && ElementalBonusCardCount == 0)
	{
		UE_LOG(LogCardCatalogAudit, Warning, TEXT("ElementalBonusGroups exist, but no scanned card currently uses them."));
	}
	for (const FString& VarietyGapWarning : VarietyGapWarnings)
	{
		UE_LOG(LogCardCatalogAudit, Warning, TEXT("%s"), *VarietyGapWarning);
	}

	for (const FCardAuditRow& Row : CardRows)
	{
		UE_LOG(LogCardCatalogAudit, Display, TEXT("[%s] %s | %s | Cost=%d Range=%d Effects=%d Primary=%s Packs=%s"),
			*Row.ValidationStatus,
			*Row.DisplayName,
			*Row.Category,
			Row.Cost,
			Row.Range,
			Row.EffectsCount,
			*Row.PrimaryOperation,
			Row.bUsedInPacks ? *Row.PackSummary : TEXT("None"));

		if (Row.Warnings.Num() > 0)
		{
			UE_LOG(LogCardCatalogAudit, Warning, TEXT("  %s: %s"), *Row.AssetName, *JoinStrings(Row.Warnings));
		}
	}

	for (const FPackAuditRow& Row : PackRows)
	{
		UE_LOG(LogCardCatalogAudit, Display, TEXT("[Pack] %s | PriceCopper=%d Grants=%d Pool=%d ValidEntries=%d TotalWeight=%d CanRoll=%s"),
			*Row.PackName,
			Row.PriceCopper,
			Row.AmountToGrant,
			Row.CardPoolCount,
			Row.ValidEntryCount,
			Row.TotalValidWeight,
			Row.bCanRollValidCard ? TEXT("Yes") : TEXT("No"));

		if (Row.Warnings.Num() > 0)
		{
			UE_LOG(LogCardCatalogAudit, Warning, TEXT("  %s: %s"), *Row.PackName, *JoinStrings(Row.Warnings));
		}
	}

	for (const FSummonAuditRow& Row : SummonRows)
	{
		const FString Status = Row.bIsValidDefinition
			? (Row.Warnings.Num() > 0 ? TEXT("Warning") : TEXT("Valid"))
			: TEXT("Invalid");
		UE_LOG(LogCardCatalogAudit, Display, TEXT("[Summon][%s] %s | HP=%d Move=%d AttackRange=%d AttackDamage=%d Team=%s Referenced=%s"),
			*Status,
			*Row.DisplayName,
			Row.MaxHP,
			Row.MoveRange,
			Row.AttackRange,
			Row.AttackDamage,
			*Row.Team,
			Row.bReferencedByScannedCards ? TEXT("Yes") : TEXT("No"));

		if (Row.Warnings.Num() > 0)
		{
			UE_LOG(LogCardCatalogAudit, Warning, TEXT("  %s: %s"), *Row.AssetName, *JoinStrings(Row.Warnings));
		}
	}

	if (bExportCsvReports)
	{
		const FString SafeSubdirectory = OutputSubdirectory.IsEmpty() ? TEXT("CardCatalog") : OutputSubdirectory;
		const FString OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), SafeSubdirectory);
		IFileManager::Get().MakeDirectory(*OutputDirectory, true);

		FString CardCsv;
		CardCsv += TEXT("CardAssetPath,CardAssetName,DisplayName,Category,CardElement,TargetType,Cost,Range,HasArt,DescriptionEmpty,EffectsCount,EffectsSummary,PrimaryOperation,UsedInPacks,PackSummary,ValidationStatus,Warnings") LINE_TERMINATOR;
		for (const FCardAuditRow& Row : CardRows)
		{
			AppendCardCsvLine(Row, CardCsv);
		}

		FString PackCsv;
		PackCsv += TEXT("PackAssetPath,PackName,PriceCopper,AmountToGrant,CardPoolCount,ValidEntryCount,TotalValidWeight,CanRollValidCard,Warnings") LINE_TERMINATOR;
		for (const FPackAuditRow& Row : PackRows)
		{
			AppendPackCsvLine(Row, PackCsv);
		}

		FString SummonCsv;
		SummonCsv += TEXT("SummonAssetPath,SummonAssetName,DisplayName,MaxHP,MoveRange,AttackRange,AttackDamage,Team,OnSummonedEffectsCount,OnTurnStartEffectsCount,OnDeathEffectsCount,IsValidDefinition,ScannedByPath,ReferencedByScannedCards,Summary,Warnings") LINE_TERMINATOR;
		for (const FSummonAuditRow& Row : SummonRows)
		{
			AppendSummonCsvLine(Row, SummonCsv);
		}

		FString BalanceCsv;
		FString VarietyCsv;
		FString DescriptionCsv;
		FString DesignCsv;
		FString ElementIdentityCsv;
		FString PackExperienceCsv;
		FString RewritePlanCsv;
		FString ElementCoverageCsv;
		FString ElementPackGapCsv;
		FString ContentRecommendationsCsv;
		FString EffectVocabularyFitCsv;
		if (bExportBalanceReports)
		{
			BalanceCsv += TEXT("CardAssetPath,CardAssetName,DisplayName,Category,TargetType,Cost,BaseBalanceBudget,ElementalBonusBudget,ExpectedCostMin,ExpectedCostMax,SuggestedCost,BalanceStatus,RoleTags,ElementTags,ProductionStatus,PackStatus,EffectsSummary,Warnings") LINE_TERMINATOR;
			for (const FCardBalanceAuditRow& Row : BalanceRows)
			{
				AppendCardBalanceCsvLine(Row, BalanceCsv);
			}

			VarietyCsv += TEXT("Element,Role,Category,ProductionStatus,PackStatus,CardCount,Cards") LINE_TERMINATOR;
			for (const FCardVarietyMatrixRow& Row : VarietyRows)
			{
				AppendVarietyMatrixCsvLine(Row, VarietyCsv);
			}

			DescriptionCsv += TEXT("CardAssetPath,CardAssetName,DisplayName,CurrentDescription,SuggestedDescription,Status,Warnings") LINE_TERMINATOR;
			for (const FCardDescriptionSuggestionRow& Row : DescriptionRows)
			{
				AppendDescriptionSuggestionCsvLine(Row, DescriptionCsv);
			}

	DesignCsv += TEXT("CardAssetPath,CardAssetName,DisplayName,Category,Cost,ElementTags,RoleTags,PrimaryKeyword,SecondaryKeywords,OperationSummary,DeliverySummary,TargetFilterSummary,PayloadSummary,ConditionSummary,KeywordTerms,SuspiciousPattern,SamenessFingerprint,MatchingFingerprintCount,TacticalScore,DesignStatus,Warnings") LINE_TERMINATOR;
			for (const FCardDesignAuditRow& Row : DesignRows)
			{
				AppendCardDesignCsvLine(Row, DesignCsv);
			}

			ElementIdentityCsv += TEXT("Element,ProductionCards,Generators,Payoffs,DamageCards,DefenseUtilityCards,TacticalCards,UniquePrimaryKeywords,MissingLanes,RecommendedActions") LINE_TERMINATOR;
			for (const FElementIdentityAuditRow& Row : ElementIdentityRows)
			{
				AppendElementIdentityCsvLine(Row, ElementIdentityCsv);
			}

			PackExperienceCsv += TEXT("PackAssetPath,PackName,CardCount,UniqueRoles,UniqueElements,UniquePrimaryKeywords,AverageCost,RepeatedFingerprintGroups,RoleSpread,ElementMix,CostCurve,ExperienceStatus,Warnings") LINE_TERMINATOR;
			for (const FPackExperienceAuditRow& Row : PackExperienceRows)
			{
				AppendPackExperienceCsvLine(Row, PackExperienceCsv);
			}

			RewritePlanCsv += TEXT("CardAssetPath,DisplayName,CurrentIssue,RecommendedAction,ProposedRulesText") LINE_TERMINATOR;
			for (const FCardRewritePlanRow& Row : RewriteRows)
			{
				AppendCardRewritePlanCsvLine(Row, RewritePlanCsv);
			}

			ElementCoverageCsv += TEXT("Element,ProductionCards,SpellCards,SummonCards,TrapCards,AuraCards,DamageCards,DefenseCards,HealingCards,ControlCards,MobilityCards,DrawCards,EnergyCards,Generators,Payoffs,TacticalCards,UniqueOperations,StatusTerms,MissingCardTypes,MissingRoles,CoverageStatus,RecommendedFocus,Cards") LINE_TERMINATOR;
			for (const FCardElementCoverageAuditRow& Row : ElementCoverageRows)
			{
				AppendCardElementCoverageCsvLine(Row, ElementCoverageCsv);
			}

			ElementPackGapCsv += TEXT("PackAssetPath,PackName,DetectedElement,NeutralSupportCount,NeutralSupportWeight,ElementPoolCount,ElementPoolWeight,OtherElementCount,DebugCardCount,SpellCards,SummonCards,TrapCards,AuraCards,RoleSpread,ElementMix,RepeatedFingerprintGroups,MissingCardTypes,MissingRoles,PackStatus,Recommendations") LINE_TERMINATOR;
			for (const FElementPackGapAuditRow& Row : ElementPackGapRows)
			{
				AppendElementPackGapCsvLine(Row, ElementPackGapCsv);
			}

			ContentRecommendationsCsv += TEXT("Priority,Element,SuggestedCardType,RoleFilled,Reason,IntendedOperation,Delivery,Payload,DraftRulesText,CardArtPrompt,SourceReport") LINE_TERMINATOR;
			for (const FCardContentRecommendationRow& Row : ContentRecommendationRows)
			{
				AppendCardContentRecommendationCsvLine(Row, ContentRecommendationsCsv);
			}

			EffectVocabularyFitCsv += TEXT("Scope,ProductionCards,TotalEffectLines,UniqueOperations,UniqueDeliveries,StructuralCoverageStatus,MechanicalVarietyStatus,OperationMix,DeliveryMix,StatusTerms,DominantOperation,RecommendedFirstBatch,Notes") LINE_TERMINATOR;
			for (const FEffectVocabularyFitAuditRow& Row : EffectVocabularyFitRows)
			{
				AppendEffectVocabularyFitCsvLine(Row, EffectVocabularyFitCsv);
			}
		}

		const FString CardCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardCatalog.csv"));
		const FString PackCsvPath = FPaths::Combine(OutputDirectory, TEXT("PackAudit.csv"));
		const FString SummonCsvPath = FPaths::Combine(OutputDirectory, TEXT("SummonDefinitions.csv"));
		const FString BalanceCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardBalanceAudit.csv"));
		const FString VarietyCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardVarietyMatrix.csv"));
		const FString DescriptionCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardDescriptionSuggestions.csv"));
		const FString DesignCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardDesignAudit.csv"));
		const FString ElementIdentityCsvPath = FPaths::Combine(OutputDirectory, TEXT("ElementIdentityAudit.csv"));
		const FString PackExperienceCsvPath = FPaths::Combine(OutputDirectory, TEXT("PackExperienceAudit.csv"));
		const FString RewritePlanCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardRewritePlan.csv"));
		const FString ElementCoverageCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardElementCoverageAudit.csv"));
		const FString ElementPackGapCsvPath = FPaths::Combine(OutputDirectory, TEXT("ElementPackGapAudit.csv"));
		const FString ContentRecommendationsCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardContentRecommendations.csv"));
		const FString EffectVocabularyFitCsvPath = FPaths::Combine(OutputDirectory, TEXT("EffectVocabularyFitAudit.csv"));

		const bool bSavedCardCsv = FFileHelper::SaveStringToFile(CardCsv, *CardCsvPath);
		const bool bSavedPackCsv = FFileHelper::SaveStringToFile(PackCsv, *PackCsvPath);
		const bool bSavedSummonCsv = FFileHelper::SaveStringToFile(SummonCsv, *SummonCsvPath);
		bool bSavedBalanceCsv = false;
		bool bSavedVarietyCsv = false;
		bool bSavedDescriptionCsv = false;
		bool bSavedDesignCsv = false;
		bool bSavedElementIdentityCsv = false;
		bool bSavedPackExperienceCsv = false;
		bool bSavedRewritePlanCsv = false;
		bool bSavedElementCoverageCsv = false;
		bool bSavedElementPackGapCsv = false;
		bool bSavedContentRecommendationsCsv = false;
		bool bSavedEffectVocabularyFitCsv = false;
		if (bExportBalanceReports)
		{
			bSavedBalanceCsv = FFileHelper::SaveStringToFile(BalanceCsv, *BalanceCsvPath);
			bSavedVarietyCsv = FFileHelper::SaveStringToFile(VarietyCsv, *VarietyCsvPath);
			bSavedDescriptionCsv = FFileHelper::SaveStringToFile(DescriptionCsv, *DescriptionCsvPath);
			bSavedDesignCsv = FFileHelper::SaveStringToFile(DesignCsv, *DesignCsvPath);
			bSavedElementIdentityCsv = FFileHelper::SaveStringToFile(ElementIdentityCsv, *ElementIdentityCsvPath);
			bSavedPackExperienceCsv = FFileHelper::SaveStringToFile(PackExperienceCsv, *PackExperienceCsvPath);
			bSavedRewritePlanCsv = FFileHelper::SaveStringToFile(RewritePlanCsv, *RewritePlanCsvPath);
			bSavedElementCoverageCsv = FFileHelper::SaveStringToFile(ElementCoverageCsv, *ElementCoverageCsvPath);
			bSavedElementPackGapCsv = FFileHelper::SaveStringToFile(ElementPackGapCsv, *ElementPackGapCsvPath);
			bSavedContentRecommendationsCsv = FFileHelper::SaveStringToFile(ContentRecommendationsCsv, *ContentRecommendationsCsvPath);
			bSavedEffectVocabularyFitCsv = FFileHelper::SaveStringToFile(EffectVocabularyFitCsv, *EffectVocabularyFitCsvPath);
		}

		if (bSavedCardCsv)
		{
			UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote card catalog CSV: %s"), *CardCsvPath);
		}
		else
		{
			UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write card catalog CSV: %s"), *CardCsvPath);
		}

		if (bSavedPackCsv)
		{
			UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote pack audit CSV: %s"), *PackCsvPath);
		}
		else
		{
			UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write pack audit CSV: %s"), *PackCsvPath);
		}

		if (bSavedSummonCsv)
		{
			UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote summon definition audit CSV: %s"), *SummonCsvPath);
		}
		else
		{
			UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write summon definition audit CSV: %s"), *SummonCsvPath);
		}

		if (bExportBalanceReports)
		{
			if (bSavedBalanceCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote card balance audit CSV: %s"), *BalanceCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write card balance audit CSV: %s"), *BalanceCsvPath);
			}

			if (bSavedVarietyCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote card variety matrix CSV: %s"), *VarietyCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write card variety matrix CSV: %s"), *VarietyCsvPath);
			}

			if (bSavedDescriptionCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote card description suggestions CSV: %s"), *DescriptionCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write card description suggestions CSV: %s"), *DescriptionCsvPath);
			}

			if (bSavedDesignCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote card design audit CSV: %s"), *DesignCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write card design audit CSV: %s"), *DesignCsvPath);
			}

			if (bSavedElementIdentityCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote element identity audit CSV: %s"), *ElementIdentityCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write element identity audit CSV: %s"), *ElementIdentityCsvPath);
			}

			if (bSavedPackExperienceCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote pack experience audit CSV: %s"), *PackExperienceCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write pack experience audit CSV: %s"), *PackExperienceCsvPath);
			}

			if (bSavedRewritePlanCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote card rewrite plan CSV: %s"), *RewritePlanCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write card rewrite plan CSV: %s"), *RewritePlanCsvPath);
			}

			if (bSavedElementCoverageCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote card element coverage audit CSV: %s"), *ElementCoverageCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write card element coverage audit CSV: %s"), *ElementCoverageCsvPath);
			}

			if (bSavedElementPackGapCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote element pack gap audit CSV: %s"), *ElementPackGapCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write element pack gap audit CSV: %s"), *ElementPackGapCsvPath);
			}

			if (bSavedContentRecommendationsCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote card content recommendations CSV: %s"), *ContentRecommendationsCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write card content recommendations CSV: %s"), *ContentRecommendationsCsvPath);
			}

			if (bSavedEffectVocabularyFitCsv)
			{
				UE_LOG(LogCardCatalogAudit, Display, TEXT("Wrote effect vocabulary fit audit CSV: %s"), *EffectVocabularyFitCsvPath);
			}
			else
			{
				UE_LOG(LogCardCatalogAudit, Error, TEXT("Failed to write effect vocabulary fit audit CSV: %s"), *EffectVocabularyFitCsvPath);
			}
		}
	}
#else
	UE_LOG(LogCardCatalogAudit, Warning, TEXT("Card Catalog Audit is editor-only and cannot scan assets in this build."));
#endif
}
