// JargonCombatPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonHeroTypes.h"
#include "Core/JargonTypes.h"
#include "GameFramework/PlayerController.h"
#include "JargonCombatPlayerController.generated.h"

class AGridTile;
class ABattleUnit;
class UCardDefinition;
class UCombatHUDWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeckChangedSignature, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandChangedSignature, int32, NewCount);

UCLASS()
class JARGON_API AJargonCombatPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AJargonCombatPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "Cards")
	void ShuffleDrawPile();

	UFUNCTION(BlueprintCallable, Category = "Cards")
	void SelectCard(UCardDefinition* Card);

	void InitializeCombatUI();
	void InitializeStartingDeck();
	void DrawCards(int32 Count);

	void ClearSelectedCard();
	void RemoveCardFromHand(UCardDefinition* Card);
	void RequestSelectFriendlyUnit(ABattleUnit* Unit);
	void RequestMoveToTile(AGridTile* Tile);
	bool RequestBasicAttackOnUnit(ABattleUnit* Unit);
	void RequestPlayCardOnUnit(ABattleUnit* Unit);
	void RequestPlayCardOnTile(AGridTile* Tile);
	void RequestEndTurn();

	UFUNCTION(Exec)
	void JargonLogNextCardEffectTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Cards")
	int32 GetDeckCount() const
	{
		return DrawPile.Num();
	}

	UFUNCTION(BlueprintCallable, Category = "Combat|Cards")
	int32 GetHandCount() const
	{
		return Hand.Num();
	}

	UPROPERTY(BlueprintAssignable, Category = "Combat|Cards")
	FOnDeckChangedSignature OnDeckChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Cards")
	FOnHandChangedSignature OnHandChanged;

protected:
	void HandleLeftClick();
	void HandleRightClick();
	void HandleCancelSelection();
	void HandleEndTurnInput();
	void RefreshHUD();
	void RefreshCombatStateHUD();
	void BroadcastCardCounts();

	UFUNCTION()
	void HandleCombatPhaseChanged(ECombatPhase NewPhase);

	UFUNCTION()
	void HandleCombatEnergyChanged(int32 NewEnergy);

	UFUNCTION()
	void HandleElementChargesChanged();

	UFUNCTION()
	void HandleHeroRuntimeStateChanged(const FJargonHeroRuntimeState& NewHeroRuntimeState);

	UFUNCTION()
	void HandlePlayerActionAvailabilityChanged(bool bCanMove, bool bCanAttack);

protected:
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|UI")
	TObjectPtr<UCombatHUDWidget> CombatHUD = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Cards")
	TObjectPtr<UCardDefinition> SelectedCard = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Cards")
	TArray<TObjectPtr<UCardDefinition>> DrawPile;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Cards")
	TArray<TObjectPtr<UCardDefinition>> Hand;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Cards")
	TArray<TObjectPtr<UCardDefinition>> DiscardPile;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Cards")
	bool bCardTargetingMode = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Cards")
	bool bStartingDeckInitialized = false;
};
