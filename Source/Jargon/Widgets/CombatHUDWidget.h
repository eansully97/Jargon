// CombatHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/JargonTypes.h"
#include "CombatHUDWidget.generated.h"

class UHorizontalBox;
class UTextBlock;
class UCardDefinition;
class UCardEntryWidget;
class UButton;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHandCardClicked, UCardDefinition*);
DECLARE_MULTICAST_DELEGATE(FOnEndTurnClicked);

UCLASS()
class JARGON_API UCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void RefreshHand(const TArray<TObjectPtr<UCardDefinition>>& HandCards);
	void SetSelectedCard(UCardDefinition* SelectedCard);
	
	UFUNCTION(BlueprintCallable)
	void SetPhaseText(ECombatPhase NewPhase);
	
	UFUNCTION(BlueprintCallable)
	void SetEnergyText(int32 NewEnergy);

	UFUNCTION(BlueprintCallable)
	void SetEnergyValues(int32 NewEnergy, int32 NewMaxEnergy);

	UFUNCTION(BlueprintCallable)
	void SetActionAvailability(ECombatPhase CurrentPhase, bool bCanMove, bool bCanAttack);

	FOnHandCardClicked& OnHandCardClicked()
	{
		return HandCardClickedDelegate;
	}

	FOnEndTurnClicked& OnEndTurnClicked()
	{
		return EndTurnClickedDelegate;
	}

protected:
	void RefreshSelectedCardText(UCardDefinition* SelectedCard);

	UFUNCTION()
	void HandleCardEntryClicked(UCardDefinition* ClickedCard);

	UFUNCTION()
	void HandleEndTurnButtonClicked();

	void RefreshPhaseText(ECombatPhase NewPhase);
	void RefreshEnergyText(int32 NewEnergy, int32 NewMaxEnergy);
	void RefreshActionAvailabilityText(ECombatPhase CurrentPhase, bool bCanMove, bool bCanAttack);

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UHorizontalBox> HandContainer = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> SelectedCardText = nullptr;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	TObjectPtr<UButton> EndTurnButton = nullptr;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	TObjectPtr<UTextBlock> PhaseText = nullptr;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	TObjectPtr<UTextBlock> EnergyText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> ActionAvailabilityText = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat HUD")
	TSubclassOf<UCardEntryWidget> CardEntryWidgetClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat HUD")
	TArray<TObjectPtr<UCardEntryWidget>> SpawnedCardWidgets;

	FOnHandCardClicked HandCardClickedDelegate;
	FOnEndTurnClicked EndTurnClickedDelegate;
};
