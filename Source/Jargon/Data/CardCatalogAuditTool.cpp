#include "Data/CardCatalogAuditTool.h"

#include "Core/JargonRunStateTypes.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/CardDefinition.h"
#include "Data/CardPackDefinition.h"

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

FString GetClassDisplayName(const UClass* Class)
{
	return Class ? Class->GetName() : TEXT("None");
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
	case ECardEffectOperation::DrawCards:
	case ECardEffectOperation::GainEnergy:
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
		return;
	}

	for (int32 EffectIndex = 0; EffectIndex < Card->Effects.Num(); ++EffectIndex)
	{
		const FCardEffectSpec& EffectSpec = Card->Effects[EffectIndex];
		const FString EffectLabel = FString::Printf(TEXT("Effect %d (%s)"), EffectIndex, *GetCardEffectOperationName(EffectSpec.Operation));

		if (EffectSpec.Operation == ECardEffectOperation::None)
		{
			OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has operation None."), *EffectLabel));
			continue;
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
			if (!EffectSpec.UnitClass)
			{
				OutFatalWarnings.Add(FString::Printf(TEXT("Invalid: %s has no UnitClass."), *EffectLabel));
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

		default:
			break;
		}
	}
}

bool DoesCardHaveFatalIntrinsicWarnings(const UCardDefinition* Card)
{
	TArray<FString> FatalWarnings;
	TArray<FString> SoftWarnings;
	AddCardIntrinsicWarnings(Card, FatalWarnings, SoftWarnings);
	return FatalWarnings.Num() > 0;
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
		TArray<FString> Fields;

		if (OperationUsesValue(EffectSpec.Operation) || EffectSpec.Operation == ECardEffectOperation::PlaceTileEffect)
		{
			Fields.Add(FString::Printf(TEXT("Value=%d"), EffectSpec.Value));
		}

		if (EffectSpec.EffectRadius > 0)
		{
			Fields.Add(FString::Printf(TEXT("Radius=%d"), EffectSpec.EffectRadius));
		}

		if (IsChainOperation(EffectSpec.Operation))
		{
			Fields.Add(FString::Printf(TEXT("Chain=%d"), EffectSpec.ChainCount));
		}

		if (EffectSpec.Operation == ECardEffectOperation::MoveSelf)
		{
			Fields.Add(FString::Printf(TEXT("Move=%d"), EffectSpec.MoveDistance));
		}

		if (EffectSpec.Operation == ECardEffectOperation::PushTarget)
		{
			Fields.Add(FString::Printf(TEXT("Push=%d"), EffectSpec.PushDistance));
			Fields.Add(FString::Printf(TEXT("Collision=%d"), EffectSpec.CollisionDamage));
		}

		if (EffectSpec.Operation == ECardEffectOperation::PullTarget)
		{
			Fields.Add(FString::Printf(TEXT("Pull=%d"), EffectSpec.PullDistance));
		}

		if (EffectSpec.Operation == ECardEffectOperation::SummonUnit)
		{
			Fields.Add(FString::Printf(TEXT("Unit=%s"), *GetClassDisplayName(EffectSpec.UnitClass.Get())));
			Fields.Add(FString::Printf(TEXT("AttackExhausted=%s"), EffectSpec.bSummonEntersWithAttackExhausted ? TEXT("true") : TEXT("false")));
		}

		if (EffectSpec.Operation == ECardEffectOperation::PlaceTileEffect)
		{
			Fields.Add(FString::Printf(TEXT("TileEffect=%s"), *GetClassDisplayName(EffectSpec.TileEffectClass.Get())));
			Fields.Add(FString::Printf(TEXT("Duration=%d"), EffectSpec.TileEffectDuration));
		}

		const FString FieldSummary = Fields.Num() > 0
			? FString::Printf(TEXT("(%s)"), *JoinStrings(Fields, TEXT(", ")))
			: FString();

		EffectSummaries.Add(FString::Printf(
			TEXT("%d:%s%s"),
			EffectIndex,
			*GetCardEffectOperationName(EffectSpec.Operation),
			*FieldSummary));
	}

	return JoinStrings(EffectSummaries, TEXT("; "));
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
#endif
}

UCardCatalogAuditTool::UCardCatalogAuditTool()
{
	CardScanPaths.Add(DefaultCardScanPath);
	CardScanPaths.Add(LegacyCardScanPath);
	PackScanPaths.Add(DefaultPackScanPath);
	PackScanPaths.Add(LegacyPackScanPath);
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

	TArray<UCardDefinition*> Cards;
	TArray<UCardPackDefinition*> Packs;
	LoadAssetsFromPaths(EffectiveCardScanPaths, Cards);
	LoadAssetsFromPaths(EffectivePackScanPaths, Packs);

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

	TArray<FCardAuditRow> CardRows;
	CardRows.Reserve(Cards.Num());
	TMap<const UCardDefinition*, bool> CardFatalStatusByCard;

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
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Cards found: %d | Invalid: %d | Warning: %d"), CardRows.Num(), InvalidCardCount, WarningCardCount);
	UE_LOG(LogCardCatalogAudit, Display, TEXT("Packs found: %d | Packs with warnings: %d"), PackRows.Num(), PackWarningCount);

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

		const FString CardCsvPath = FPaths::Combine(OutputDirectory, TEXT("CardCatalog.csv"));
		const FString PackCsvPath = FPaths::Combine(OutputDirectory, TEXT("PackAudit.csv"));

		const bool bSavedCardCsv = FFileHelper::SaveStringToFile(CardCsv, *CardCsvPath);
		const bool bSavedPackCsv = FFileHelper::SaveStringToFile(PackCsv, *PackCsvPath);

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
	}
#else
	UE_LOG(LogCardCatalogAudit, Warning, TEXT("Card Catalog Audit is editor-only and cannot scan assets in this build."));
#endif
}
