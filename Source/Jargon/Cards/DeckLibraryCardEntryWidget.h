#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeckLibraryCardEntryWidget.generated.h"

class UButton;
class UCardDefinition;
class UCardDisplayWidget;
class UTextBlock;
class UWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDeckLibraryCardEntryClicked, UCardDefinition*);

UCLASS()
class JARGON_API UDeckLibraryCardEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Native click delegate used by deck editing code; Blueprint events remain presentation hooks. */
	FOnDeckLibraryCardEntryClicked OnLibraryCardClicked;

	/** Initializes the entry from deck-editor view data. Safe for runtime UI refreshes; does not mutate the run deck by itself. */
	UFUNCTION(BlueprintCallable, Category = "Deck Library")
	void InitializeFromCardLibraryEntry(
		UCardDefinition* InCardDefinition,
		int32 InOwnedCount,
		int32 InDeckCount,
		int32 InAvailableCount,
		bool bInCanAddToDeck,
		bool bInCanRemoveFromDeck);

	UFUNCTION(BlueprintPure, Category = "Deck Library")
	UCardDefinition* GetCardDefinition() const
	{
		return CardDefinition;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Library")
	bool IsClickBlocked() const
	{
		return bIsClickBlocked;
	}

	UFUNCTION(BlueprintPure, Category = "Deck Library")
	bool CanExecuteLibraryCardClick() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UFUNCTION()
	void HandleCardButtonClicked();

	void HandleCardHovered();
	void HandleCardUnhovered();
	class UDeckEditWidget* ResolveOwningDeckEditWidget() const;

	bool CanAddDisplayedCardToDeck() const;

	/** Rebuilds the card display, count labels, and unavailable overlay from the cached entry data. */
	void RefreshVisuals();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Library")
	void BP_OnLibraryCardEntryRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Library")
	void BP_OnLibraryCardClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Library")
	void BP_OnLibraryCardClickBlocked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Library")
	void BP_OnLibraryCardHovered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Library")
	void BP_OnLibraryCardUnhovered();

protected:
	/** Card Data Asset represented by this library row or tile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	/** Total owned copies in the run, including copies already placed in the active deck. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	int32 OwnedCount = 0;

	/** Copies of this card currently in the active run deck. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	int32 DeckCount = 0;

	/** Owned copies that remain in reserve and can potentially move into the deck. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	int32 AvailableCount = 0;

	/** Cached deck-edit availability; click handling rechecks this before broadcasting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	bool bCanAddToDeck = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	bool bCanRemoveFromDeck = false;

	UPROPERTY(BlueprintReadOnly, Category = "Deck Library")
	bool bIsClickBlocked = true;

	/** Optional Button binding named CardButton. If absent, mouse hover still works but button clicks are unavailable. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CardButton = nullptr;

	/** Optional CardDisplay binding named CardDisplay for nested card visuals. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCardDisplayWidget> CardDisplay = nullptr;

	/** Optional TextBlock binding named OwnedCountText. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OwnedCountText = nullptr;

	/** Optional TextBlock binding named DeckCountText. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeckCountText = nullptr;

	/** Optional TextBlock binding named AvailableCountText. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AvailableCountText = nullptr;

	/** Optional overlay widget shown when the card cannot currently be added to the deck. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> UnavailableOverlay = nullptr;
};
