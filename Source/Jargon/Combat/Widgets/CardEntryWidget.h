#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CardEntryWidget.generated.h"

class UButton;
class UCardDefinition;
class UCardDisplayWidget;
class UCardEntryWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCardEntryClicked, UCardDefinition*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCardEntryWidgetHovered, UCardEntryWidget*);

UCLASS()
class JARGON_API UCardEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnCardEntryClicked OnCardEntryClicked;
	FOnCardEntryWidgetHovered OnCardEntryHovered;
	FOnCardEntryWidgetHovered OnCardEntryUnhovered;

	UFUNCTION(BlueprintCallable, Category = "Card Entry")
	void InitializeFromCard(UCardDefinition* InCard);

	UFUNCTION(BlueprintPure, Category = "Card Entry")
	UCardDefinition* GetCardDefinition() const
	{
		return CardDefinition;
	}

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UFUNCTION()
	void HandleCardButtonClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Entry")
	void BP_OnCardInitialized();

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Entry", meta = (DeprecatedFunction, DeprecationMessage = "Card selection is owned by C++. This event is no longer called. Use BP_OnCardClickFeedback for visual-only feedback."))
	void BP_OnCardClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Entry")
	void BP_OnCardClickFeedback();

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Entry")
	void BP_OnCardHovered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Entry")
	void BP_OnCardUnhovered();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CardButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCardDisplayWidget> CardDisplay = nullptr;
};
