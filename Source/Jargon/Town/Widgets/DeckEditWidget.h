#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/Widgets/JargonHoverInfoTypes.h"
#include "Core/JargonRunStateTypes.h"
#include "DeckEditWidget.generated.h"

class UJargonGameInstance;
class UCardDefinition;

USTRUCT(BlueprintType)
struct FDeckEditStackedDeckEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TObjectPtr<UCardDefinition> Card = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	int32 DeckCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Element")
	EJargonElementType CardElement = EJargonElementType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Element")
	FText CardElementText;
};

USTRUCT(BlueprintType)
struct FDeckEditLibraryCardEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TObjectPtr<UCardDefinition> Card = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	int32 OwnedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	int32 DeckCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	int32 ReserveCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Element")
	EJargonElementType CardElement = EJargonElementType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Element")
	FText CardElementText;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	bool bCanAddToDeck = false;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	bool bCanRemoveFromDeck = false;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Availability")
	bool bAddBlockedByOwnership = false;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Availability")
	bool bAddBlockedByDeckSize = false;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Availability")
	bool bAddBlockedByMaxCopies = false;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Availability")
	bool bAddBlockedByElementLimit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Availability")
	FText AddToDeckBlockedReason;
};

USTRUCT(BlueprintType)
struct FDeckEditLibraryFilter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck Edit|Filter")
	bool bFilterByElement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck Edit|Filter")
	EJargonElementType Element = EJargonElementType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck Edit|Filter", meta = (ToolTip = "Optional multi-select element filter. When set, entries pass if their element matches any selected value. None means Neutral. The single Element field remains for one-value filter compatibility."))
	TArray<EJargonElementType> Elements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck Edit|Filter")
	bool bFilterByCategory = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck Edit|Filter")
	ECardCategory Category = ECardCategory::Spell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck Edit|Filter", meta = (ToolTip = "Optional multi-select card type filter. When set, entries pass if their card type matches any selected value. The single Category field remains for one-value filter compatibility."))
	TArray<ECardCategory> Categories;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck Edit|Filter")
	bool bShowOnlyOwned = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck Edit|Filter")
	bool bShowOnlyAddable = false;
};

UCLASS(Abstract, Blueprintable)
class JARGON_API UDeckEditWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Deck Edit")
	virtual void RefreshFromRunState(UJargonGameInstance* JargonGameInstance);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit")
	virtual void RefreshFromCachedRunState();

	UFUNCTION(BlueprintCallable, Category = "Deck Edit")
	virtual bool MoveCardFromReserveToDeck(UJargonGameInstance* JargonGameInstance, UCardDefinition* Card);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit")
	virtual bool MoveCardFromDeckToReserve(UJargonGameInstance* JargonGameInstance, UCardDefinition* Card);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit")
	virtual bool AddOneCopyToDeck(UCardDefinition* Card);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit")
	virtual bool RemoveOneCopyFromDeck(UCardDefinition* Card);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Recycle")
	virtual bool RecycleAllExtraReserveCards();

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Hover")
	void ShowLibraryCardHoverInfo(UCardDefinition* Card, UObject* SourceObject);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Hover")
	void ClearLibraryCardHoverInfo(UObject* SourceObject);

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Hover")
	FJargonCombatHoverInfo BuildLibraryCardHoverInfo(UCardDefinition* Card, UObject* SourceObject) const;

	UFUNCTION(BlueprintPure, Category = "Deck Edit")
	const TArray<FDeckEditStackedDeckEntry>& GetStackedDeckEntries() const
	{
		return StackedDeckEntries;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit")
	const TArray<FDeckEditLibraryCardEntry>& GetAllLibraryEntries() const
	{
		return LibraryEntries;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit")
	const TArray<FDeckEditLibraryCardEntry>& GetFilteredLibraryEntries() const
	{
		return FilteredLibraryEntries;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit")
	const TArray<FDeckEditLibraryCardEntry>& GetLibraryEntriesForCurrentPage() const
	{
		return CurrentLibraryPageEntries;
	}

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void SetLibraryFilter(FDeckEditLibraryFilter InFilter);

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Filter")
	FDeckEditLibraryFilter GetLibraryFilter() const
	{
		return LibraryFilter;
	}

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void ClearLibraryFilter();

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void SetLibraryElementFilter(bool bEnabled, EJargonElementType Element);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void SetLibraryElementFilterEnabled(EJargonElementType Element, bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void SetLibraryElementFilters(const TArray<EJargonElementType>& Elements);

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Filter")
	TArray<EJargonElementType> GetLibraryElementFilters() const
	{
		return LibraryFilter.Elements;
	}

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void SetLibraryCategoryFilter(bool bEnabled, ECardCategory Category);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void SetLibraryCategoryFilterEnabled(ECardCategory Category, bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void SetLibraryCategoryFilters(const TArray<ECardCategory>& Categories);

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Filter")
	TArray<ECardCategory> GetLibraryCategoryFilters() const
	{
		return LibraryFilter.Categories;
	}

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void SetShowOnlyOwned(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Filter")
	void SetShowOnlyAddable(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Filter")
	int32 GetFilteredLibraryCardCount() const
	{
		return FilteredLibraryEntries.Num();
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Filter")
	int32 GetTotalLibraryCardCount() const
	{
		return LibraryEntries.Num();
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Filter")
	FText GetLibraryFilterSummaryText() const;

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Library")
	void SetLibraryCardsPerPage(int32 InCardsPerPage);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Library")
	bool SetLibraryPageIndex(int32 InPageIndex);

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Library")
	bool GoToNextLibraryPage();

	UFUNCTION(BlueprintCallable, Category = "Deck Edit|Library")
	bool GoToPreviousLibraryPage();

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Library")
	int32 GetLibraryCardsPerPage() const
	{
		return LibraryCardsPerPage;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Library")
	int32 GetCurrentLibraryPageIndex() const
	{
		return CurrentLibraryPageIndex;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Library")
	int32 GetLibraryPageCount() const;

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Library")
	FText GetLibraryPageText() const;

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Elements")
	FJargonDeckElementSummary GetDeckElementSummary() const
	{
		return DeckElementSummary;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Recycle")
	bool CanRecycleAllExtraReserveCards() const
	{
		return bCanRecycleAllExtraReserveCards;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Recycle")
	int32 GetRecycleAllExtraReserveCardCount() const
	{
		return RecycleAllExtraReserveCardCount;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Recycle")
	FJargonCurrencyAmount GetRecycleAllExtraReserveCurrencyValue() const
	{
		return RecycleAllExtraReserveCurrencyValue;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Edit|Recycle")
	FText GetRecycleAllExtraReserveBlockedReason() const
	{
		return RecycleAllExtraReserveBlockedReason;
	}

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Edit")
	void BP_OnDeckDataRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Edit")
	void BP_OnLibraryPageChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Edit|Filter")
	void BP_OnLibraryFilterChanged();

protected:
	UJargonGameInstance* ResolveRunState(UJargonGameInstance* ExplicitRunState) const;
	void RebuildViewData();
	void RebuildFilteredLibraryEntries();
	void RebuildCurrentLibraryPageEntries();
	void ClampLibraryPageIndex();
	bool DoesLibraryEntryPassFilter(const FDeckEditLibraryCardEntry& Entry) const;
	static bool SortCardsByCostThenName(const UCardDefinition& LeftCard, const UCardDefinition& RightCard);

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TArray<TObjectPtr<UCardDefinition>> RunDeckCards;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TArray<TObjectPtr<UCardDefinition>> RunOwnedCards;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TArray<TObjectPtr<UCardDefinition>> RunReserveCards;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TArray<FDeckEditStackedDeckEntry> StackedDeckEntries;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TArray<FDeckEditLibraryCardEntry> LibraryEntries;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TArray<FDeckEditLibraryCardEntry> FilteredLibraryEntries;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit")
	TArray<FDeckEditLibraryCardEntry> CurrentLibraryPageEntries;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Filter")
	FDeckEditLibraryFilter LibraryFilter;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Elements")
	FJargonDeckElementSummary DeckElementSummary;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Recycle")
	bool bCanRecycleAllExtraReserveCards = false;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Recycle")
	int32 RecycleAllExtraReserveCardCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Recycle")
	FJargonCurrencyAmount RecycleAllExtraReserveCurrencyValue;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Recycle")
	FText RecycleAllExtraReserveBlockedReason;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck Edit|Library", meta = (ClampMin = "1"))
	int32 LibraryCardsPerPage = 12;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Edit|Library")
	int32 CurrentLibraryPageIndex = 0;

	UPROPERTY(Transient)
	TObjectPtr<UJargonGameInstance> CachedRunState = nullptr;
};
