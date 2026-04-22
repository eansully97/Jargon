// CardEntryWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CardEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UCardDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCardEntryClicked, UCardDefinition*);

UCLASS()
class JARGON_API UCardEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void InitializeFromCard(UCardDefinition* InCard);

	UCardDefinition* GetCardDefinition() const
	{
		return CardDefinition;
	}

	FOnCardEntryClicked& OnCardClicked()
	{
		return CardClickedDelegate;
	}

protected:
	UFUNCTION()
	void HandleCardButtonClicked();

	void RefreshDisplay();

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UButton> CardButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> CardNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> CostText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> DescriptionText = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	FOnCardEntryClicked CardClickedDelegate;
};