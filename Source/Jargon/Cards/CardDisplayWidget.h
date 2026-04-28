#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CardDisplayWidget.generated.h"

class UBorder;
class UCardDefinition;
class UImage;
class UTextBlock;

UCLASS()
class JARGON_API UCardDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Card Display")
	void SetCardDefinition(UCardDefinition* InCardDefinition);

	UFUNCTION(BlueprintCallable, Category = "Card Display")
	void RefreshFromCardDefinition();

	UFUNCTION(BlueprintPure, Category = "Card Display")
	UCardDefinition* GetCardDefinition() const
	{
		return CardDefinition;
	}

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Display")
	void BP_OnCardDisplayRefreshed();

	FText GetCardCategoryText() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Display", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CardNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TypeText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardArt = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardFrame = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card Display")
	FText EmptyCardNameText = FText::FromString(TEXT("Card"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card Display")
	FText EmptyDescriptionText = FText::GetEmpty();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card Display")
	FLinearColor EmptyCardAccentColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.f);
};