#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
	const TArray<FDeckEditLibraryCardEntry>& GetLibraryEntriesForCurrentPage() const
	{
		return CurrentLibraryPageEntries;
	}

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

protected:
	UJargonGameInstance* ResolveRunState(UJargonGameInstance* ExplicitRunState) const;
	void RebuildViewData();
	void RebuildCurrentLibraryPageEntries();
	void ClampLibraryPageIndex();
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
	TArray<FDeckEditLibraryCardEntry> CurrentLibraryPageEntries;

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
