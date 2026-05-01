// CombatHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/JargonHeroTypes.h"
#include "Core/JargonTypes.h"
#include "CombatHUDWidget.generated.h"

class UHorizontalBox;
class UImage;
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

	void SetElementChargeValues(const TMap<EJargonElementType, int32>& NewElementCharges);

	UFUNCTION(BlueprintCallable, Category = "Combat HUD|Hero")
	void RefreshHeroIdentity(const FJargonHeroClassInfo& HeroClassInfo, const FJargonHeroAspectInfo& HeroAspectInfo);

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
	FText GetElementDisplayText(EJargonElementType ElementType) const;

	UFUNCTION()
	void HandleCardEntryClicked(UCardDefinition* ClickedCard);

	UFUNCTION()
	void HandleEndTurnButtonClicked();

	void RefreshPhaseText(ECombatPhase NewPhase);
	void RefreshEnergyText(int32 NewEnergy, int32 NewMaxEnergy);
	void RefreshElementCharges(int32 Fire, int32 Frost, int32 Storm, int32 Nature, int32 Radiance, int32 Quietus);
	void RefreshActionAvailabilityText(ECombatPhase CurrentPhase, bool bCanMove, bool bCanAttack);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat HUD|Hero")
	void BP_OnHeroIdentityRefreshed(const FJargonHeroClassInfo& HeroClassInfo, const FJargonHeroAspectInfo& HeroAspectInfo);

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
	TObjectPtr<UTextBlock> FireChargeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> FrostChargeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> StormChargeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> NatureChargeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> RadianceChargeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> QuietusChargeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> HeroClassText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> HeroClassDescriptionText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> DominantElementText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> ActiveAspectText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> ActiveAspectDescriptionText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> ActiveAspectPassiveText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> AspectProgressText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> HeroIdentitySummaryText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> FireChargeIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> FrostChargeIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> StormChargeIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> NatureChargeIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> RadianceChargeIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> QuietusChargeIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> ActionAvailabilityText = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat HUD")
	TSubclassOf<UCardEntryWidget> CardEntryWidgetClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat HUD")
	TArray<TObjectPtr<UCardEntryWidget>> SpawnedCardWidgets;

	FOnHandCardClicked HandCardClickedDelegate;
	FOnEndTurnClicked EndTurnClickedDelegate;
};
