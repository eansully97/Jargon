#include "DeckEditWidget.h"

#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"
#include "Data/CardScriptDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "Data/JargonTileEffectDefinition.h"
#include "Engine/World.h"
#include "Town/JargonTownPlayerController.h"

namespace
{
constexpr int32 FallbackMaxCopiesPerCardInDeck = 3;
constexpr int32 FallbackMaxRunDeckSize = 30;

struct FDeckEditCardCounts
{
	int32 DeckCount = 0;
	int32 OwnedCount = 0;
};

int32 CountCopiesOfCard(const TArray<TObjectPtr<UCardDefinition>>& Cards, const UCardDefinition* Card)
{
	if (!Card)
	{
		return 0;
	}

	int32 CopyCount = 0;
	for (const TObjectPtr<UCardDefinition>& CandidateCard : Cards)
	{
		if (CandidateCard.Get() == Card)
		{
			++CopyCount;
		}
	}

	return CopyCount;
}

FString GetCardDisplayNameForSort(const UCardDefinition* Card)
{
	return Card ? Card->DisplayName.ToString() : FString();
}

int32 GetCardCostForSort(const UCardDefinition* Card)
{
	return Card ? Card->Cost : MAX_int32;
}

FText GetCardElementTextForDeckEntry(const UCardDefinition* Card)
{
	return Card ? Card->GetCardElementDisplayText() : FText::GetEmpty();
}

FText GetElementFilterText(EJargonElementType Element)
{
	if (Element == EJargonElementType::None)
	{
		return NSLOCTEXT("DeckEdit", "LibraryFilterNeutralElement", "Neutral");
	}

	const UEnum* ElementEnum = StaticEnum<EJargonElementType>();
	return ElementEnum
		? ElementEnum->GetDisplayNameTextByValue(static_cast<int64>(Element))
		: FText::AsNumber(static_cast<int32>(Element));
}

FText GetCategoryFilterText(ECardCategory Category)
{
	const UEnum* CategoryEnum = StaticEnum<ECardCategory>();
	return CategoryEnum
		? CategoryEnum->GetDisplayNameTextByValue(static_cast<int64>(Category))
		: FText::AsNumber(static_cast<int32>(Category));
}

void NormalizeElementFilters(FDeckEditLibraryFilter& Filter)
{
	if (Filter.Elements.Num() > 0)
	{
		Filter.bFilterByElement = true;
	}

	if (Filter.bFilterByElement && Filter.Elements.Num() <= 0)
	{
		Filter.Elements.Add(Filter.Element);
	}

	if (!Filter.bFilterByElement)
	{
		Filter.Elements.Empty();
		return;
	}

	TArray<EJargonElementType> UniqueElements;
	for (const EJargonElementType Element : Filter.Elements)
	{
		UniqueElements.AddUnique(Element);
	}

	Filter.Elements = MoveTemp(UniqueElements);
	if (Filter.Elements.Num() > 0)
	{
		Filter.Element = Filter.Elements[0];
	}
}

void NormalizeCategoryFilters(FDeckEditLibraryFilter& Filter)
{
	if (Filter.Categories.Num() > 0)
	{
		Filter.bFilterByCategory = true;
	}

	if (Filter.bFilterByCategory && Filter.Categories.Num() <= 0)
	{
		Filter.Categories.Add(Filter.Category);
	}

	if (!Filter.bFilterByCategory)
	{
		Filter.Categories.Empty();
		return;
	}

	TArray<ECardCategory> UniqueCategories;
	for (const ECardCategory Category : Filter.Categories)
	{
		UniqueCategories.AddUnique(Category);
	}

	Filter.Categories = MoveTemp(UniqueCategories);
	if (Filter.Categories.Num() > 0)
	{
		Filter.Category = Filter.Categories[0];
	}
}

void NormalizeLibraryFilter(FDeckEditLibraryFilter& Filter)
{
	NormalizeElementFilters(Filter);
	NormalizeCategoryFilters(Filter);
}

FText JoinFilterTextList(const TArray<FText>& Values)
{
	FString JoinedText;
	for (int32 Index = 0; Index < Values.Num(); ++Index)
	{
		if (Index > 0)
		{
			JoinedText += TEXT(", ");
		}
		JoinedText += Values[Index].ToString();
	}

	return FText::FromString(JoinedText);
}

FText BuildBlockedReason(const FDeckEditLibraryCardEntry& Entry)
{
	if (Entry.bCanAddToDeck)
	{
		return FText::GetEmpty();
	}

	if (Entry.bAddBlockedByOwnership)
	{
		return NSLOCTEXT("DeckEdit", "AddBlockedByOwnership", "No owned reserve copies available.");
	}

	if (Entry.bAddBlockedByMaxCopies)
	{
		return NSLOCTEXT("DeckEdit", "AddBlockedByMaxCopies", "This card is already at the deck copy limit.");
	}

	if (Entry.bAddBlockedByDeckSize)
	{
		return NSLOCTEXT("DeckEdit", "AddBlockedByDeckSize", "The run deck is full.");
	}

	if (Entry.bAddBlockedByElementLimit)
	{
		return NSLOCTEXT("DeckEdit", "AddBlockedByElementLimit", "Adding this card would exceed the deck element limit.");
	}

	return FText::GetEmpty();
}

FText GetTileEffectDefinitionHoverDescription(const UJargonTileEffectDefinition* TileEffectDefinition)
{
	if (!TileEffectDefinition)
	{
		return FText::GetEmpty();
	}

	if (!TileEffectDefinition->Description.IsEmpty())
	{
		return TileEffectDefinition->Description;
	}

	return TileEffectDefinition->DisplayName;
}

FText GetSummonDefinitionHoverDescription(const UJargonSummonedUnitDefinition* SummonedUnitDefinition)
{
	return SummonedUnitDefinition ? SummonedUnitDefinition->Description : FText::GetEmpty();
}

const UJargonCardPlaceTileEffectAction* FindFirstTileEffectActionWithDescription(const UJargonCardScript* CardScript)
{
	if (!CardScript)
	{
		return nullptr;
	}

	const auto TryAction = [](const UJargonCardAction* Action) -> const UJargonCardPlaceTileEffectAction*
	{
		const UJargonCardPlaceTileEffectAction* TileEffectAction = Cast<UJargonCardPlaceTileEffectAction>(Action);
		if (!TileEffectAction || GetTileEffectDefinitionHoverDescription(TileEffectAction->TileEffectDefinition).IsEmpty())
		{
			return nullptr;
		}

		return TileEffectAction;
	};

	for (const TObjectPtr<UJargonCardAction>& Action : CardScript->Actions)
	{
		if (const UJargonCardPlaceTileEffectAction* TileEffectAction = TryAction(Action.Get()))
		{
			return TileEffectAction;
		}
	}

	for (const FJargonCardElementalBonusScript& BonusScript : CardScript->ElementalBonuses)
	{
		for (const TObjectPtr<UJargonCardAction>& Action : BonusScript.Actions)
		{
			if (const UJargonCardPlaceTileEffectAction* TileEffectAction = TryAction(Action.Get()))
			{
				return TileEffectAction;
			}
		}
	}

	return nullptr;
}

const UJargonCardSummonAction* FindFirstSummonActionWithDescription(const UJargonCardScript* CardScript)
{
	if (!CardScript)
	{
		return nullptr;
	}

	const auto TryAction = [](const UJargonCardAction* Action) -> const UJargonCardSummonAction*
	{
		const UJargonCardSummonAction* SummonAction = Cast<UJargonCardSummonAction>(Action);
		if (!SummonAction || GetSummonDefinitionHoverDescription(SummonAction->SummonedUnitDefinition).IsEmpty())
		{
			return nullptr;
		}

		return SummonAction;
	};

	for (const TObjectPtr<UJargonCardAction>& Action : CardScript->Actions)
	{
		if (const UJargonCardSummonAction* SummonAction = TryAction(Action.Get()))
		{
			return SummonAction;
		}
	}

	for (const FJargonCardElementalBonusScript& BonusScript : CardScript->ElementalBonuses)
	{
		for (const TObjectPtr<UJargonCardAction>& Action : BonusScript.Actions)
		{
			if (const UJargonCardSummonAction* SummonAction = TryAction(Action.Get()))
			{
				return SummonAction;
			}
		}
	}

	return nullptr;
}

AJargonTownPlayerController* ResolveTownPlayerController(const UUserWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}

	if (AJargonTownPlayerController* TownController = Widget->GetOwningPlayer<AJargonTownPlayerController>())
	{
		return TownController;
	}

	const UWorld* World = Widget->GetWorld();
	return World ? Cast<AJargonTownPlayerController>(World->GetFirstPlayerController()) : nullptr;
}
}

void UDeckEditWidget::RefreshFromRunState(UJargonGameInstance* JargonGameInstance)
{
	CachedRunState = ResolveRunState(JargonGameInstance);

	RunDeckCards.Empty();
	RunOwnedCards.Empty();
	RunReserveCards.Empty();

	if (CachedRunState)
	{
		CachedRunState->RefreshRunReserveCardsFromAvailableShopPacks();
		RunDeckCards = CachedRunState->GetRunDeckCards();
		RunOwnedCards = CachedRunState->GetOwnedRunCards();
		RunReserveCards = CachedRunState->GetRunReserveCards();
	}

	RebuildViewData();

	BP_OnDeckDataRefreshed();
	BP_OnLibraryPageChanged();
}

void UDeckEditWidget::RefreshFromCachedRunState()
{
	RefreshFromRunState(CachedRunState.Get());
}

bool UDeckEditWidget::MoveCardFromReserveToDeck(UJargonGameInstance* JargonGameInstance, UCardDefinition* Card)
{
	CachedRunState = ResolveRunState(JargonGameInstance);
	return AddOneCopyToDeck(Card);
}

bool UDeckEditWidget::MoveCardFromDeckToReserve(UJargonGameInstance* JargonGameInstance, UCardDefinition* Card)
{
	CachedRunState = ResolveRunState(JargonGameInstance);
	return RemoveOneCopyFromDeck(Card);
}

bool UDeckEditWidget::AddOneCopyToDeck(UCardDefinition* Card)
{
	UJargonGameInstance* RunState = ResolveRunState(CachedRunState.Get());
	if (!RunState || !Card)
	{
		return false;
	}

	const int32 MaxDeckSize = RunState->GetMaxRunDeckSize();
	if (RunDeckCards.Num() >= MaxDeckSize)
	{
		return false;
	}

	const int32 DeckCount = CountCopiesOfCard(RunDeckCards, Card);
	if (DeckCount >= RunState->GetMaxCopiesPerDeckCard())
	{
		return false;
	}

	const int32 OwnedCount = CountCopiesOfCard(RunOwnedCards, Card);
	if (OwnedCount - DeckCount <= 0)
	{
		return false;
	}

	const bool bMovedSuccessfully = RunState->MoveCardFromReserveToDeck(Card);
	if (bMovedSuccessfully)
	{
		RefreshFromRunState(RunState);
	}

	return bMovedSuccessfully;
}

bool UDeckEditWidget::RemoveOneCopyFromDeck(UCardDefinition* Card)
{
	UJargonGameInstance* RunState = ResolveRunState(CachedRunState.Get());
	if (!RunState || !Card)
	{
		return false;
	}

	const bool bMovedSuccessfully = RunState->MoveCardFromDeckToReserve(Card);
	if (bMovedSuccessfully)
	{
		RefreshFromRunState(RunState);
	}

	return bMovedSuccessfully;
}

bool UDeckEditWidget::RecycleAllExtraReserveCards()
{
	UJargonGameInstance* RunState = ResolveRunState(CachedRunState.Get());
	if (!RunState)
	{
		return false;
	}

	int32 CardsRecycled = 0;
	FJargonCurrencyAmount CurrencyAwarded;
	FText FailureReason;
	const bool bRecycledSuccessfully = RunState->RecycleAllExtraReserveCards(CardsRecycled, CurrencyAwarded, FailureReason);
	if (bRecycledSuccessfully)
	{
		RefreshFromRunState(RunState);
	}

	return bRecycledSuccessfully;
}

void UDeckEditWidget::ShowLibraryCardHoverInfo(UCardDefinition* Card, UObject* SourceObject)
{
	AJargonTownPlayerController* TownController = ResolveTownPlayerController(this);
	if (!TownController)
	{
		return;
	}

	TownController->ShowTownHoverInfo(BuildLibraryCardHoverInfo(Card, SourceObject ? SourceObject : Card));
}

void UDeckEditWidget::ClearLibraryCardHoverInfo(UObject* SourceObject)
{
	if (AJargonTownPlayerController* TownController = ResolveTownPlayerController(this))
	{
		TownController->ClearTownHoverInfo(SourceObject);
	}
}

FJargonCombatHoverInfo UDeckEditWidget::BuildLibraryCardHoverInfo(UCardDefinition* Card, UObject* SourceObject) const
{
	if (!Card || !Card->CardScript)
	{
		return FJargonCombatHoverInfo();
	}

	if (const UJargonCardPlaceTileEffectAction* TileEffectAction = FindFirstTileEffectActionWithDescription(Card->CardScript))
	{
		FJargonCombatHoverInfo HoverInfo;
		HoverInfo.bHasInfo = true;
		HoverInfo.InfoType = EJargonCombatHoverInfoType::TileEffect;
		HoverInfo.DescriptionText = GetTileEffectDefinitionHoverDescription(TileEffectAction->TileEffectDefinition);
		HoverInfo.SourceObject = SourceObject ? SourceObject : Card;
		return HoverInfo;
	}

	if (const UJargonCardSummonAction* SummonAction = FindFirstSummonActionWithDescription(Card->CardScript))
	{
		FJargonCombatHoverInfo HoverInfo;
		HoverInfo.bHasInfo = true;
		HoverInfo.InfoType = EJargonCombatHoverInfoType::Unit;
		HoverInfo.DescriptionText = GetSummonDefinitionHoverDescription(SummonAction->SummonedUnitDefinition);
		HoverInfo.SourceObject = SourceObject ? SourceObject : Card;
		return HoverInfo;
	}

	return FJargonCombatHoverInfo();
}

void UDeckEditWidget::SetLibraryCardsPerPage(int32 InCardsPerPage)
{
	LibraryCardsPerPage = FMath::Max(1, InCardsPerPage);
	ClampLibraryPageIndex();
	RebuildCurrentLibraryPageEntries();
	BP_OnLibraryPageChanged();
}

void UDeckEditWidget::SetLibraryFilter(FDeckEditLibraryFilter InFilter)
{
	NormalizeLibraryFilter(InFilter);
	LibraryFilter = InFilter;
	CurrentLibraryPageIndex = 0;
	RebuildFilteredLibraryEntries();
	ClampLibraryPageIndex();
	RebuildCurrentLibraryPageEntries();
	BP_OnLibraryFilterChanged();
	BP_OnLibraryPageChanged();
}

void UDeckEditWidget::ClearLibraryFilter()
{
	SetLibraryFilter(FDeckEditLibraryFilter());
}

void UDeckEditWidget::SetLibraryElementFilter(bool bEnabled, EJargonElementType Element)
{
	FDeckEditLibraryFilter NewFilter = LibraryFilter;
	NewFilter.bFilterByElement = bEnabled;
	NewFilter.Element = Element;
	NewFilter.Elements.Empty();
	if (bEnabled)
	{
		NewFilter.Elements.Add(Element);
	}
	SetLibraryFilter(NewFilter);
}

void UDeckEditWidget::SetLibraryElementFilterEnabled(EJargonElementType Element, bool bEnabled)
{
	FDeckEditLibraryFilter NewFilter = LibraryFilter;
	NormalizeLibraryFilter(NewFilter);

	if (bEnabled)
	{
		NewFilter.Elements.AddUnique(Element);
	}
	else
	{
		NewFilter.Elements.Remove(Element);
	}

	NewFilter.bFilterByElement = NewFilter.Elements.Num() > 0;
	if (NewFilter.Elements.Num() > 0)
	{
		NewFilter.Element = NewFilter.Elements[0];
	}

	SetLibraryFilter(NewFilter);
}

void UDeckEditWidget::SetLibraryElementFilters(const TArray<EJargonElementType>& Elements)
{
	FDeckEditLibraryFilter NewFilter = LibraryFilter;
	NewFilter.Elements = Elements;
	NewFilter.bFilterByElement = Elements.Num() > 0;
	if (Elements.Num() > 0)
	{
		NewFilter.Element = Elements[0];
	}
	SetLibraryFilter(NewFilter);
}

void UDeckEditWidget::SetLibraryCategoryFilter(bool bEnabled, ECardCategory Category)
{
	FDeckEditLibraryFilter NewFilter = LibraryFilter;
	NewFilter.bFilterByCategory = bEnabled;
	NewFilter.Category = Category;
	NewFilter.Categories.Empty();
	if (bEnabled)
	{
		NewFilter.Categories.Add(Category);
	}
	SetLibraryFilter(NewFilter);
}

void UDeckEditWidget::SetLibraryCategoryFilterEnabled(ECardCategory Category, bool bEnabled)
{
	FDeckEditLibraryFilter NewFilter = LibraryFilter;
	NormalizeLibraryFilter(NewFilter);

	if (bEnabled)
	{
		NewFilter.Categories.AddUnique(Category);
	}
	else
	{
		NewFilter.Categories.Remove(Category);
	}

	NewFilter.bFilterByCategory = NewFilter.Categories.Num() > 0;
	if (NewFilter.Categories.Num() > 0)
	{
		NewFilter.Category = NewFilter.Categories[0];
	}

	SetLibraryFilter(NewFilter);
}

void UDeckEditWidget::SetLibraryCategoryFilters(const TArray<ECardCategory>& Categories)
{
	FDeckEditLibraryFilter NewFilter = LibraryFilter;
	NewFilter.Categories = Categories;
	NewFilter.bFilterByCategory = Categories.Num() > 0;
	if (Categories.Num() > 0)
	{
		NewFilter.Category = Categories[0];
	}
	SetLibraryFilter(NewFilter);
}

void UDeckEditWidget::SetShowOnlyOwned(bool bEnabled)
{
	FDeckEditLibraryFilter NewFilter = LibraryFilter;
	NewFilter.bShowOnlyOwned = bEnabled;
	SetLibraryFilter(NewFilter);
}

void UDeckEditWidget::SetShowOnlyAddable(bool bEnabled)
{
	FDeckEditLibraryFilter NewFilter = LibraryFilter;
	NewFilter.bShowOnlyAddable = bEnabled;
	SetLibraryFilter(NewFilter);
}

FText UDeckEditWidget::GetLibraryFilterSummaryText() const
{
	TArray<FText> ActiveFilterParts;

	if (LibraryFilter.bFilterByElement)
	{
		TArray<FText> ElementTexts;
		for (const EJargonElementType Element : LibraryFilter.Elements)
		{
			ElementTexts.Add(GetElementFilterText(Element));
		}

		ActiveFilterParts.Add(FText::Format(
			LibraryFilter.Elements.Num() == 1
				? NSLOCTEXT("DeckEdit", "LibraryFilterElementSummary", "Element: {0}")
				: NSLOCTEXT("DeckEdit", "LibraryFilterElementsSummary", "Elements: {0}"),
			JoinFilterTextList(ElementTexts)));
	}

	if (LibraryFilter.bFilterByCategory)
	{
		TArray<FText> CategoryTexts;
		for (const ECardCategory Category : LibraryFilter.Categories)
		{
			CategoryTexts.Add(GetCategoryFilterText(Category));
		}

		ActiveFilterParts.Add(FText::Format(
			LibraryFilter.Categories.Num() == 1
				? NSLOCTEXT("DeckEdit", "LibraryFilterCategorySummary", "Type: {0}")
				: NSLOCTEXT("DeckEdit", "LibraryFilterCategoriesSummary", "Types: {0}"),
			JoinFilterTextList(CategoryTexts)));
	}

	if (LibraryFilter.bShowOnlyOwned)
	{
		ActiveFilterParts.Add(NSLOCTEXT("DeckEdit", "LibraryFilterOwnedSummary", "Owned"));
	}

	if (LibraryFilter.bShowOnlyAddable)
	{
		ActiveFilterParts.Add(NSLOCTEXT("DeckEdit", "LibraryFilterAddableSummary", "Addable"));
	}

	if (ActiveFilterParts.Num() <= 0)
	{
		return NSLOCTEXT("DeckEdit", "LibraryFilterAllCardsSummary", "All cards");
	}

	FString Summary;
	for (int32 Index = 0; Index < ActiveFilterParts.Num(); ++Index)
	{
		if (Index > 0)
		{
			Summary += TEXT(" | ");
		}
		Summary += ActiveFilterParts[Index].ToString();
	}

	return FText::FromString(Summary);
}

bool UDeckEditWidget::SetLibraryPageIndex(int32 InPageIndex)
{
	const int32 OldPageIndex = CurrentLibraryPageIndex;

	CurrentLibraryPageIndex = InPageIndex;
	ClampLibraryPageIndex();

	if (CurrentLibraryPageIndex == OldPageIndex)
	{
		return false;
	}

	RebuildCurrentLibraryPageEntries();
	BP_OnLibraryPageChanged();
	return true;
}

bool UDeckEditWidget::GoToNextLibraryPage()
{
	return SetLibraryPageIndex(CurrentLibraryPageIndex + 1);
}

bool UDeckEditWidget::GoToPreviousLibraryPage()
{
	return SetLibraryPageIndex(CurrentLibraryPageIndex - 1);
}

int32 UDeckEditWidget::GetLibraryPageCount() const
{
	if (FilteredLibraryEntries.Num() == 0)
	{
		return 0;
	}

	const int32 SafeCardsPerPage = FMath::Max(1, LibraryCardsPerPage);
	return FMath::DivideAndRoundUp(FilteredLibraryEntries.Num(), SafeCardsPerPage);
}

FText UDeckEditWidget::GetLibraryPageText() const
{
	const int32 PageCount = GetLibraryPageCount();
	if (PageCount <= 0)
	{
		return FText::FromString(TEXT("0 / 0"));
	}

	return FText::Format(
		NSLOCTEXT("DeckEdit", "LibraryPageText", "{0} / {1}"),
		FText::AsNumber(CurrentLibraryPageIndex + 1),
		FText::AsNumber(PageCount)
	);
}

UJargonGameInstance* UDeckEditWidget::ResolveRunState(UJargonGameInstance* ExplicitRunState) const
{
	if (ExplicitRunState)
	{
		return ExplicitRunState;
	}

	const UWorld* World = GetWorld();
	return World ? World->GetGameInstance<UJargonGameInstance>() : nullptr;
}

void UDeckEditWidget::RebuildViewData()
{
	StackedDeckEntries.Empty();
	LibraryEntries.Empty();
	FilteredLibraryEntries.Empty();
	DeckElementSummary = CachedRunState ? CachedRunState->GetRunDeckElementSummary() : FJargonDeckElementSummary();
	bCanRecycleAllExtraReserveCards = false;
	RecycleAllExtraReserveCardCount = 0;
	RecycleAllExtraReserveCurrencyValue = FJargonCurrencyAmount();
	RecycleAllExtraReserveBlockedReason = FText::GetEmpty();

	if (CachedRunState)
	{
		RecycleAllExtraReserveCardCount = CachedRunState->GetRecycleAllExtraReserveCardCopyCount();
		RecycleAllExtraReserveCurrencyValue = CachedRunState->GetRecycleAllExtraReserveCardsValue();
		bCanRecycleAllExtraReserveCards = CachedRunState->CanRecycleAllExtraReserveCards(RecycleAllExtraReserveBlockedReason);
	}

	TMap<UCardDefinition*, FDeckEditCardCounts> CountsByCard;
	const int32 MaxCopiesPerDeckCard = CachedRunState
		? CachedRunState->GetMaxCopiesPerDeckCard()
		: FallbackMaxCopiesPerCardInDeck;
	const int32 MaxRunDeckSize = CachedRunState
		? CachedRunState->GetMaxRunDeckSize()
		: FallbackMaxRunDeckSize;

	for (UCardDefinition* Card : RunDeckCards)
	{
		if (Card)
		{
			CountsByCard.FindOrAdd(Card).DeckCount++;
		}
	}

	for (UCardDefinition* Card : RunOwnedCards)
	{
		if (Card)
		{
			CountsByCard.FindOrAdd(Card).OwnedCount++;
		}
	}

	for (UCardDefinition* Card : RunReserveCards)
	{
		if (Card)
		{
			CountsByCard.FindOrAdd(Card);
		}
	}

	TArray<UCardDefinition*> UniqueCards;
	CountsByCard.GetKeys(UniqueCards);
	UniqueCards.Sort([](const UCardDefinition& LeftCard, const UCardDefinition& RightCard)
	{
		return UDeckEditWidget::SortCardsByCostThenName(LeftCard, RightCard);
	});

	for (UCardDefinition* Card : UniqueCards)
	{
		if (!Card)
		{
			continue;
		}

		const FDeckEditCardCounts& Counts = CountsByCard.FindChecked(Card);

		if (Counts.DeckCount > 0)
		{
			FDeckEditStackedDeckEntry DeckEntry;
			DeckEntry.Card = Card;
			DeckEntry.DeckCount = Counts.DeckCount;
			DeckEntry.CardElement = Card->CardElement;
			DeckEntry.CardElementText = GetCardElementTextForDeckEntry(Card);
			StackedDeckEntries.Add(DeckEntry);
		}

		FDeckEditLibraryCardEntry LibraryEntry;
		LibraryEntry.Card = Card;
		LibraryEntry.DeckCount = Counts.DeckCount;
		LibraryEntry.OwnedCount = Counts.OwnedCount;
		LibraryEntry.ReserveCount = FMath::Max(0, Counts.OwnedCount - Counts.DeckCount);
		LibraryEntry.CardElement = Card->CardElement;
		LibraryEntry.CardElementText = GetCardElementTextForDeckEntry(Card);
		LibraryEntry.bAddBlockedByOwnership = LibraryEntry.ReserveCount <= 0;
		LibraryEntry.bAddBlockedByMaxCopies = Counts.DeckCount >= MaxCopiesPerDeckCard;
		LibraryEntry.bAddBlockedByDeckSize = RunDeckCards.Num() >= MaxRunDeckSize;
		LibraryEntry.bAddBlockedByElementLimit = CachedRunState
			? !CachedRunState->WouldRunDeckRespectElementLimitWithCard(Card)
			: false;
		LibraryEntry.bCanAddToDeck =
			!LibraryEntry.bAddBlockedByOwnership &&
			!LibraryEntry.bAddBlockedByMaxCopies &&
			!LibraryEntry.bAddBlockedByDeckSize &&
			!LibraryEntry.bAddBlockedByElementLimit;
		LibraryEntry.AddToDeckBlockedReason = BuildBlockedReason(LibraryEntry);
		LibraryEntry.bCanRemoveFromDeck = Counts.DeckCount > 0;
		LibraryEntries.Add(LibraryEntry);
	}

	RebuildFilteredLibraryEntries();
	ClampLibraryPageIndex();
	RebuildCurrentLibraryPageEntries();
}

void UDeckEditWidget::RebuildFilteredLibraryEntries()
{
	FilteredLibraryEntries.Empty();

	for (const FDeckEditLibraryCardEntry& Entry : LibraryEntries)
	{
		if (DoesLibraryEntryPassFilter(Entry))
		{
			FilteredLibraryEntries.Add(Entry);
		}
	}
}

void UDeckEditWidget::RebuildCurrentLibraryPageEntries()
{
	CurrentLibraryPageEntries.Empty();

	const int32 PageCount = GetLibraryPageCount();
	if (PageCount <= 0)
	{
		CurrentLibraryPageIndex = 0;
		return;
	}

	const int32 SafeCardsPerPage = FMath::Max(1, LibraryCardsPerPage);
	const int32 StartIndex = CurrentLibraryPageIndex * SafeCardsPerPage;
	const int32 EndIndexExclusive = FMath::Min(StartIndex + SafeCardsPerPage, FilteredLibraryEntries.Num());

	for (int32 Index = StartIndex; Index < EndIndexExclusive; ++Index)
	{
		CurrentLibraryPageEntries.Add(FilteredLibraryEntries[Index]);
	}
}

void UDeckEditWidget::ClampLibraryPageIndex()
{
	const int32 PageCount = GetLibraryPageCount();
	if (PageCount <= 0)
	{
		CurrentLibraryPageIndex = 0;
		return;
	}

	CurrentLibraryPageIndex = FMath::Clamp(CurrentLibraryPageIndex, 0, PageCount - 1);
}

bool UDeckEditWidget::DoesLibraryEntryPassFilter(const FDeckEditLibraryCardEntry& Entry) const
{
	const UCardDefinition* Card = Entry.Card.Get();
	if (!Card)
	{
		return false;
	}

	if (LibraryFilter.bFilterByElement && !LibraryFilter.Elements.Contains(Entry.CardElement))
	{
		return false;
	}

	if (LibraryFilter.bFilterByCategory && !LibraryFilter.Categories.Contains(Card->Category))
	{
		return false;
	}

	if (LibraryFilter.bShowOnlyOwned && Entry.OwnedCount <= 0)
	{
		return false;
	}

	if (LibraryFilter.bShowOnlyAddable && !Entry.bCanAddToDeck)
	{
		return false;
	}

	return true;
}

bool UDeckEditWidget::SortCardsByCostThenName(const UCardDefinition& LeftCard, const UCardDefinition& RightCard)
{
	const int32 LeftCost = GetCardCostForSort(&LeftCard);
	const int32 RightCost = GetCardCostForSort(&RightCard);

	if (LeftCost != RightCost)
	{
		return LeftCost < RightCost;
	}

	return GetCardDisplayNameForSort(&LeftCard) < GetCardDisplayNameForSort(&RightCard);
}
