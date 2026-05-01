#include "Data/CardCatalogAuditTool.h"

#include "Core/JargonRunStateTypes.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/CardDefinition.h"
#include "Data/CardPackDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"

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
const FName DefaultSummonedUnitScanPath(TEXT("/Game/Jargon/Data/SummonedUnits"));

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
	FString OptionalUnitClassOverride;
	int32 OnSummonedEffectsCount = 0;
	int32 OnTurnStartEffectsCount = 0;
	int32 OnDeathEffectsCount = 0;
	bool bIsValidDefinition = false;
	bool bScannedByPath = false;
	bool bReferencedByScannedCards = false;
	FString Summary;
	TArray<FString> Warnings;
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

FString GetEnumDisplayName(const UEnum* Enum, int64 Value)
{
	if (!Enum)
	{
		return FString::FromInt(static_cast<int32>(Value));
	}

	return Enum->GetDisplayNameTextByValue(Value).ToString();
}

FString GetCardCategoryName(ECardCategory Category)
{
	return GetEnumDisplayName(StaticEnum<ECardCategory>(), static_cast<int64>(Category));
}

FString GetCardTargetTypeName(ECardTargetType TargetType)
{
	return GetEnumDisplayName(StaticEnum<ECardTargetType>(), static_cast<int64>(TargetType));
}

FString GetCardEffectOperationName(ECardEffectOperation Operation)
{
	return GetEnumDisplayName(StaticEnum<ECardEffectOperation>(), static_cast<int64>(Operation));
}

FString GetElementTypeName(EJargonElementType ElementType)
{
	return GetEnumDisplayName(StaticEnum<EJargonElementType>(), static_cast<int64>(ElementType));
}

FString GetTeamName(ETeam Team)
{
	return GetEnumDisplayName(StaticEnum<ETeam>(), static_cast<int64>(Team));
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

bool OperationUsesValue(ECardEffectOperation Operation)
{
	switch (Operation)
	{
	case ECardEffectOperation::DealDamage:
	case ECardEffectOperation::Heal:
	case ECardEffectOperation::ApplyShield:
	case ECardEffectOperation::ApplyStun:
	case ECardEffectOperation::ApplyFreeze:
	case ECardEffectOperation::DrawCards:
	case ECardEffectOperation::GainEnergy:
	case ECardEffectOperation::GainElementCharge:
	case ECardEffectOperation::ChainDamage:
	case ECardEffectOperation::ChainHeal:
	case ECardEffectOperation::ChainStun:
		return true;

	default:
		return false;
	}
}

bool IsChainOperation(ECardEffectOperation Operation)
{
	return Operation == ECardEffectOperation::ChainDamage
		|| Operation == ECardEffectOperation::ChainHeal
		|| Operation == ECardEffectOperation::ChainStun;
}

bool IsFriendlyTargetingOperation(ECardEffectOperation Operation)
{
	return Operation == ECardEffectOperation::Heal
		|| Operation == ECardEffectOperation::ApplyShield
		|| Operation == ECardEffectOperation::ChainHeal;
}

bool IsHostileTargetingOperation(ECardEffectOperation Operation)
{
	return Operation == ECardEffectOperation::DealDamage
		|| Operation == ECardEffectOperation::ApplyStun
		|| Operation == ECardEffectOperation::ApplyFreeze
		|| Operation == ECardEffectOperation::PushTarget
		|| Operation == ECardEffectOperation::PullTarget
		|| Operation == ECardEffectOperation::ChainDamage
		|| Operation == ECardEffectOperation::ChainStun;
}

bool BonusGroupMixesFriendlyAndHostileEffects(const FCardElementalBonusGroup& BonusGroup)
{
	bool bHasFriendlyOperation = false;
	bool bHasHostileOperation = false;
	for (const FCardEffectSpec& BonusEffect : BonusGroup.BonusEffects)
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

	for (int32 EffectIndex = 0; EffectIndex < Card->Effects.Num(); ++EffectIndex)
	{
		const FCardEffectSpec& EffectSpec = Card->Effects[EffectIndex];
		if (EffectSpec.Operation != ECardEffectOperation::MoveSelf)
		{
			continue;
		}

		if (EffectIndex < Card->Effects.Num() - 1)
		{
			OutSoftWarnings.Add(FString::Printf(
				TEXT("Warning: Effect %d MoveSelf appears before later base effects. MoveSelf can resolve asynchronously, so later base effects may be skipped for that resolve pass."),
				EffectIndex));
		}

		if (Card->ElementalBonusGroups.Num() > 0)
		{
			OutSoftWarnings.Add(FString::Printf(
				TEXT("Warning: Effect %d MoveSelf appears before ElementalBonusGroups. MoveSelf can resolve asynchronously, so elemental bonuses may be skipped for that resolve pass."),
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

	if (!Definition->OptionalUnitClassOverride)
	{
		OutWarnings.Add(TEXT("Notice: OptionalUnitClassOverride is empty; runtime must use CombatGameMode DefaultSummonedUnitClass."));
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
	const FCardEffectSpec& EffectSpec,
	const FString& EffectLabel,
	TArray<FString>& OutFatalWarnings,
	TArray<FString>& OutSoftWarnings)
{
	if (EffectSpec.Operation == ECardEffectOperation::None)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has operation None."), *EffectLabel));
		return;
	}

	if (OperationUsesValue(EffectSpec.Operation) && EffectSpec.Value <= 0)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires Value > 0."), *EffectLabel));
	}

	if (EffectSpec.EffectRadius < 0)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has negative EffectRadius."), *EffectLabel));
	}

	if (IsChainOperation(EffectSpec.Operation) && EffectSpec.ChainCount <= 0)
	{
		OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires ChainCount > 0."), *EffectLabel));
	}

	switch (EffectSpec.Operation)
	{
	case ECardEffectOperation::MoveSelf:
		if (EffectSpec.MoveDistance <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires MoveDistance > 0."), *EffectLabel));
		}
		break;

	case ECardEffectOperation::PushTarget:
		if (EffectSpec.PushDistance <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires PushDistance > 0."), *EffectLabel));
		}

		if (EffectSpec.CollisionDamage < 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has negative CollisionDamage."), *EffectLabel));
		}
		break;

	case ECardEffectOperation::PullTarget:
		if (EffectSpec.PullDistance <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires PullDistance > 0."), *EffectLabel));
		}
		OutSoftWarnings.Add(FString::Printf(TEXT("Warning: %s uses PullTarget, which is currently deferred."), *EffectLabel));
		break;

	case ECardEffectOperation::SummonUnit:
		if (!EffectSpec.SummonedUnitDefinition && !EffectSpec.UnitClass)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has neither SummonedUnitDefinition nor UnitClass."), *EffectLabel));
		}
		else if (EffectSpec.SummonedUnitDefinition && EffectSpec.UnitClass)
		{
			OutSoftWarnings.Add(FString::Printf(TEXT("Notice: %s has both SummonedUnitDefinition and UnitClass. The definition wins; UnitClass fallback remains for migration."), *EffectLabel));
		}
		else if (EffectSpec.SummonedUnitDefinition)
		{
			OutSoftWarnings.Add(FString::Printf(TEXT("Info: %s uses the data-driven summon definition path."), *EffectLabel));
		}
		else
		{
			OutSoftWarnings.Add(FString::Printf(TEXT("Notice: %s uses the legacy UnitClass summon path."), *EffectLabel));
		}
		break;

	case ECardEffectOperation::PlaceTileEffect:
		if (!EffectSpec.TileEffectClass)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has no TileEffectClass."), *EffectLabel));
		}

		if (EffectSpec.TileEffectDuration < 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has negative TileEffectDuration."), *EffectLabel));
		}
		break;

	case ECardEffectOperation::GainElementCharge:
		if (EffectSpec.ElementType == EJargonElementType::None)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires ElementType other than None."), *EffectLabel));
		}
		break;

	default:
		break;
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

	if (Card->Effects.Num() == 0)
	{
		OutFatalWarnings.Add(TEXT("Invalid: no Effects authored."));
		if (Card->ElementalBonusGroups.Num() > 0)
		{
			OutSoftWarnings.Add(TEXT("Warning: ElementalBonusGroups are optional bonuses and cannot replace base Effects[]."));
		}
	}
	else
	{
		for (int32 EffectIndex = 0; EffectIndex < Card->Effects.Num(); ++EffectIndex)
		{
			const FCardEffectSpec& EffectSpec = Card->Effects[EffectIndex];
			const FString EffectLabel = FString::Printf(TEXT("Effect %d (%s)"), EffectIndex, *GetCardEffectOperationName(EffectSpec.Operation));
			AddCardEffectSpecWarnings(EffectSpec, EffectLabel, OutFatalWarnings, OutSoftWarnings);
		}
	}

	for (int32 BonusIndex = 0; BonusIndex < Card->ElementalBonusGroups.Num(); ++BonusIndex)
	{
		const FCardElementalBonusGroup& BonusGroup = Card->ElementalBonusGroups[BonusIndex];
		const FString BonusMode = BonusGroup.bSpendCharges ? TEXT("Spend") : TEXT("Check");
		const FString BonusLabel = FString::Printf(TEXT("ElementalBonus %d (%s)"), BonusIndex, *BonusMode);

		if (BonusGroup.ElementType == EJargonElementType::None)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires ElementType other than None."), *BonusLabel));
		}

		if (BonusGroup.RequiredCharges <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s requires RequiredCharges > 0. Element bonuses are optional, but their authored requirement must still be meaningful."), *BonusLabel));
		}

		if (BonusGroup.BonusEffects.Num() <= 0)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has no BonusEffects."), *BonusLabel));
			continue;
		}

		for (int32 BonusEffectIndex = 0; BonusEffectIndex < BonusGroup.BonusEffects.Num(); ++BonusEffectIndex)
		{
			const FCardEffectSpec& BonusEffectSpec = BonusGroup.BonusEffects[BonusEffectIndex];
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

	for (const FCardEffectSpec& EffectSpec : Card->Effects)
	{
		if (EffectSpec.Operation == ECardEffectOperation::GainElementCharge)
		{
			return true;
		}
	}

	for (const FCardElementalBonusGroup& BonusGroup : Card->ElementalBonusGroups)
	{
		for (const FCardEffectSpec& BonusEffectSpec : BonusGroup.BonusEffects)
		{
			if (BonusEffectSpec.Operation == ECardEffectOperation::GainElementCharge)
			{
				return true;
			}
		}
	}

	return false;
}

FString BuildEffectSummary(const UCardDefinition* Card)
{
	if (!Card || Card->Effects.Num() == 0)
	{
		return TEXT("None");
	}

	TArray<FString> EffectSummaries;
	EffectSummaries.Reserve(Card->Effects.Num());

	for (int32 EffectIndex = 0; EffectIndex < Card->Effects.Num(); ++EffectIndex)
	{
		const FCardEffectSpec& EffectSpec = Card->Effects[EffectIndex];
		EffectSummaries.Add(FString::Printf(
			TEXT("Effect %d: %s"),
			EffectIndex,
			*Card->GetEffectAuditSummary(EffectSpec)));
	}

	const FString BaseEffectSummary = JoinStrings(EffectSummaries, TEXT("; "));
	if (Card->ElementalBonusGroups.Num() <= 0)
	{
		return BaseEffectSummary;
	}

	TArray<FString> BonusSummaries;
	BonusSummaries.Reserve(Card->ElementalBonusGroups.Num());
	for (int32 BonusIndex = 0; BonusIndex < Card->ElementalBonusGroups.Num(); ++BonusIndex)
	{
		const FCardElementalBonusGroup& BonusGroup = Card->ElementalBonusGroups[BonusIndex];
		BonusSummaries.Add(FString::Printf(
			TEXT("Bonus %d: %s"),
			BonusIndex,
			*Card->GetElementalBonusAuditSummary(BonusGroup)));
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

void AppendCardCsvLine(const FCardAuditRow& Row, FString& Csv)
{
	TArray<FString> Fields;
	Fields.Add(CsvEscape(Row.AssetPath));
	Fields.Add(CsvEscape(Row.AssetName));
	Fields.Add(CsvEscape(Row.DisplayName));
	Fields.Add(CsvEscape(Row.Category));
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
	Fields.Add(CsvEscape(Row.OptionalUnitClassOverride));
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
		{ DefaultSummonedUnitScanPath });

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

		for (const FCardEffectSpec& EffectSpec : Card->Effects)
		{
			if (EffectSpec.Operation == ECardEffectOperation::SummonUnit)
			{
				AddReferencedSummonDefinition(EffectSpec.SummonedUnitDefinition.Get());
			}
		}

		for (const FCardElementalBonusGroup& BonusGroup : Card->ElementalBonusGroups)
		{
			for (const FCardEffectSpec& BonusEffect : BonusGroup.BonusEffects)
			{
				if (BonusEffect.Operation == ECardEffectOperation::SummonUnit)
				{
					AddReferencedSummonDefinition(BonusEffect.SummonedUnitDefinition.Get());
				}
			}
		}
	}

	TArray<FCardAuditRow> CardRows;
	CardRows.Reserve(Cards.Num());
	TMap<const UCardDefinition*, bool> CardFatalStatusByCard;
	int32 ElementGeneratorCardCount = 0;
	int32 ElementalBonusCardCount = 0;

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
		Row.TargetType = GetCardTargetTypeName(Card->TargetType);
		Row.Cost = Card->Cost;
		Row.Range = Card->Range;
		Row.bHasArt = Card->CardArt != nullptr;
		Row.bDescriptionEmpty = Card->Description.ToString().TrimStartAndEnd().IsEmpty();
		Row.EffectsCount = Card->Effects.Num();
		Row.EffectsSummary = BuildEffectSummary(Card);
		Row.PrimaryOperation = Card->Effects.Num() > 0
			? GetCardEffectOperationName(Card->Effects[0].Operation)
			: TEXT("None");

		if (CardHasElementGenerator(Card))
		{
			ElementGeneratorCardCount++;
		}

		if (Card->ElementalBonusGroups.Num() > 0)
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
		if (!Row.bUsedInPacks)
		{
			SoftWarnings.Add(TEXT("Warning: card is not included in any scanned pack."));
		}

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
		Row.OptionalUnitClassOverride = GetClassDisplayName(Definition->OptionalUnitClassOverride.Get());
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
	if (Cards.Num() > 0 && ElementGeneratorCardCount == 0)
	{
		UE_LOG(LogCardCatalogAudit, Warning, TEXT("Element charge backend exists, but no scanned card currently gains element charges."));
	}
	if (Cards.Num() > 0 && ElementalBonusCardCount == 0)
	{
		UE_LOG(LogCardCatalogAudit, Warning, TEXT("ElementalBonusGroups exist, but no scanned card currently uses them."));
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
		UE_LOG(LogCardCatalogAudit, Display, TEXT("[Summon][%s] %s | HP=%d Move=%d AttackRange=%d AttackDamage=%d Team=%s OptionalClass=%s Referenced=%s"),
			*Status,
			*Row.DisplayName,
			Row.MaxHP,
			Row.MoveRange,
			Row.AttackRange,
			Row.AttackDamage,
			*Row.Team,
			*Row.OptionalUnitClassOverride,
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
		CardCsv += TEXT("CardAssetPath,CardAssetName,DisplayName,Category,TargetType,Cost,Range,HasArt,DescriptionEmpty,EffectsCount,EffectsSummary,PrimaryOperation,UsedInPacks,PackSummary,ValidationStatus,Warnings") LINE_TERMINATOR;
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
		SummonCsv += TEXT("SummonAssetPath,SummonAssetName,DisplayName,MaxHP,MoveRange,AttackRange,AttackDamage,Team,OptionalUnitClassOverride,OnSummonedEffectsCount,OnTurnStartEffectsCount,OnDeathEffectsCount,IsValidDefinition,ScannedByPath,ReferencedByScannedCards,Summary,Warnings") LINE_TERMINATOR;
		for (const FSummonAuditRow& Row : SummonRows)
		{
			AppendSummonCsvLine(Row, SummonCsv);
		}

		const FString CardCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardCatalog.csv"));
		const FString PackCsvPath = FPaths::Combine(OutputDirectory, TEXT("PackAudit.csv"));
		const FString SummonCsvPath = FPaths::Combine(OutputDirectory, TEXT("SummonDefinitions.csv"));

		const bool bSavedCardCsv = FFileHelper::SaveStringToFile(CardCsv, *CardCsvPath);
		const bool bSavedPackCsv = FFileHelper::SaveStringToFile(PackCsv, *PackCsvPath);
		const bool bSavedSummonCsv = FFileHelper::SaveStringToFile(SummonCsv, *SummonCsvPath);

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
	}
#else
	UE_LOG(LogCardCatalogAudit, Warning, TEXT("Card Catalog Audit is editor-only and cannot scan assets in this build."));
#endif
}
