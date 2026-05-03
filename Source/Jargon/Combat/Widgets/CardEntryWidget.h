#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CardEntryWidget.generated.h"

class UButton;
class UCardDefinition;
class UCardDisplayWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCardEntryClicked, UCardDefinition*);

UCLASS()
class JARGON_API UCardEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnCardEntryClicked OnCardEntryClicked;

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

	UFUNCTION()
	void HandleCardButtonClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Entry")
	void BP_OnCardInitialized();

	UFUNCTION(BlueprintImplementableEvent, Category = "Card Entry")
	void BP_OnCardClicked();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CardButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCardDisplayWidget> CardDisplay = nullptr;
};
