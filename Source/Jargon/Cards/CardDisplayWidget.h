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
	/** Assigns the card Data Asset this widget should render. Safe for Blueprint setup and spawned widget initialization. */
	UFUNCTION(BlueprintCallable, Category = "Card Display")
	void SetCardDefinition(UCardDefinition* InCardDefinition);

	/** Pulls display text, cost, type, and art from the assigned card definition into optional widget bindings. */
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
	/** Card Data Asset currently rendered by this widget; can be supplied on spawn for reusable card UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card Display", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	/** Optional TextBlock binding named CostText. When absent, card display still refreshes the remaining bindings. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText = nullptr;

	/** Optional TextBlock binding named CardNameText for the card display name. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CardNameText = nullptr;

	/** Optional TextBlock binding named TypeText for the card category label. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TypeText = nullptr;

	/** Optional TextBlock binding named DescriptionText for player-facing card rules/flavor text. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText = nullptr;

	/** Optional Image binding named CardArt for manually assigned card artwork. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardArt = nullptr;

	/** Optional Image binding named CardFrame for Blueprint-owned frame/accent styling. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardFrame = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card Display")
	FText EmptyCardNameText = FText::FromString(TEXT("Card"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card Display")
	FText EmptyDescriptionText = FText::GetEmpty();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card Display")
	FLinearColor EmptyCardAccentColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.f);
};
