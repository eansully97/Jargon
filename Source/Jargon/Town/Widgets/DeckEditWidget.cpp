#include "DeckEditWidget.h"

#include "Core/JargonGameInstance.h"
#include "Data/CardDefinition.h"
#include "Engine/World.h"

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

void UDeckEditWidget::SetLibraryCardsPerPage(int32 InCardsPerPage)
{
	LibraryCardsPerPage = FMath::Max(1, InCardsPerPage);
	ClampLibraryPageIndex();
	RebuildCurrentLibraryPageEntries();
	BP_OnLibraryPageChanged();
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
	if (LibraryEntries.Num() == 0)
	{
		return 0;
	}

	const int32 SafeCardsPerPage = FMath::Max(1, LibraryCardsPerPage);
	return FMath::DivideAndRoundUp(LibraryEntries.Num(), SafeCardsPerPage);
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
		LibraryEntry.bAddBlockedByElementLimit = CachedRunState && !CachedRunState->WouldRunDeckRespectElementLimitWithCard(Card);
		LibraryEntry.bCanAddToDeck =
			!LibraryEntry.bAddBlockedByOwnership &&
			!LibraryEntry.bAddBlockedByMaxCopies &&
			!LibraryEntry.bAddBlockedByDeckSize &&
			!LibraryEntry.bAddBlockedByElementLimit;
		LibraryEntry.AddToDeckBlockedReason = BuildBlockedReason(LibraryEntry);
		LibraryEntry.bCanRemoveFromDeck = Counts.DeckCount > 0;
		LibraryEntries.Add(LibraryEntry);
	}

	ClampLibraryPageIndex();
	RebuildCurrentLibraryPageEntries();
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
	const int32 EndIndexExclusive = FMath::Min(StartIndex + SafeCardsPerPage, LibraryEntries.Num());

	for (int32 Index = StartIndex; Index < EndIndexExclusive; ++Index)
	{
		CurrentLibraryPageEntries.Add(LibraryEntries[Index]);
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
