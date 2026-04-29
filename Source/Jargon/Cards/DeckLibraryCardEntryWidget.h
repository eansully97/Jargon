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
	FOnDeckLibraryCardEntryClicked OnLibraryCardClicked;

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

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;

	UFUNCTION()
	void HandleCardButtonClicked();

	bool CanAddDisplayedCardToDeck() const;
	void RefreshVisuals();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Library")
	void BP_OnLibraryCardEntryRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Library")
	void BP_OnLibraryCardClicked();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	int32 OwnedCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	int32 DeckCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	int32 AvailableCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	bool bCanAddToDeck = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Library", meta = (ExposeOnSpawn = "true"))
	bool bCanRemoveFromDeck = false;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CardButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCardDisplayWidget> CardDisplay = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OwnedCountText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeckCountText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AvailableCountText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> UnavailableOverlay = nullptr;
};
