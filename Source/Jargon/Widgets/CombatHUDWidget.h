// CombatHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatHUDWidget.generated.h"

class UHorizontalBox;
class UTextBlock;
class UCardDefinition;
class UCardEntryWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHandCardClicked, UCardDefinition*);

UCLASS()
class JARGON_API UCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void RefreshHand(const TArray<TObjectPtr<UCardDefinition>>& HandCards);
	void SetSelectedCard(UCardDefinition* SelectedCard);

	FOnHandCardClicked& OnHandCardClicked()
	{
		return HandCardClickedDelegate;
	}

protected:
	void RefreshSelectedCardText(UCardDefinition* SelectedCard);

	UFUNCTION()
	void HandleCardEntryClicked(UCardDefinition* ClickedCard);

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UHorizontalBox> HandContainer = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> SelectedCardText = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat HUD")
	TSubclassOf<UCardEntryWidget> CardEntryWidgetClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat HUD")
	TArray<TObjectPtr<UCardEntryWidget>> SpawnedCardWidgets;

	FOnHandCardClicked HandCardClickedDelegate;
};