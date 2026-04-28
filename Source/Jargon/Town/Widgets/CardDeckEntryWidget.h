#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CardDeckEntryWidget.generated.h"

class UButton;
class UCardDefinition;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCardDeckEntryClicked, UCardDefinition*);

UCLASS()
class JARGON_API UCardDeckEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnCardDeckEntryClicked OnDeckEntryClicked;

	UFUNCTION(BlueprintCallable, Category = "Deck Entry")
	void InitializeFromStackedDeckEntry(UCardDefinition* InCardDefinition, int32 InDeckCount);

	UFUNCTION(BlueprintPure, Category = "Deck Entry")
	UCardDefinition* GetCardDefinition() const
	{
		return CardDefinition;
	}

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;

	UFUNCTION()
	void HandleCardButtonClicked();

	void RefreshVisuals();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Entry")
	void BP_OnDeckEntryRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Deck Entry")
	void BP_OnDeckEntryClicked();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Entry", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck Entry", meta = (ExposeOnSpawn = "true"))
	int32 DeckCount = 0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CardButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CardNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeckCountText = nullptr;
};