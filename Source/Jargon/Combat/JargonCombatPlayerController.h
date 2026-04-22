// JargonCombatPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JargonCombatPlayerController.generated.h"

class AGridTile;
class ABattleUnit;
class UCardDefinition;
class UCombatHUDWidget;

UCLASS()
class JARGON_API AJargonCombatPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AJargonCombatPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void InitializeCombatUI();
	void InitializeStartingDeck();
	void DrawCards(int32 Count);
	void SelectCard(UCardDefinition* Card);
	void ClearSelectedCard();
	void RemoveCardFromHand(UCardDefinition* Card);
	void RequestMoveToTile(AGridTile* Tile);
	void RequestPlayCardOnUnit(ABattleUnit* Unit);
	void RequestEndTurn();

protected:
	void HandleLeftClick();
	void HandleRightClick();
	void HandleCancelSelection();
	void HandleEndTurnInput();
	void RefreshHUD();

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
};