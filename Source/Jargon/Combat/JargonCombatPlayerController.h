// JargonCombatPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "Combat/Widgets/JargonHoverInfoTypes.h"
#include "Core/JargonHeroTypes.h"
#include "Core/JargonTypes.h"
#include "GameFramework/PlayerController.h"
#include "JargonCombatPlayerController.generated.h"

class AGridTile;
class ABattleTileEffect;
class ABattleUnit;
class UCardDefinition;
class UCombatHUDWidget;
class UCombatHoverInfoWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeckChangedSignature, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandChangedSignature, int32, NewCount);

UCLASS()
class JARGON_API AJargonCombatPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AJargonCombatPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
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

	UFUNCTION(BlueprintPure, Category = "Combat|Hover")
	FJargonCombatHoverInfo GetCurrentCombatHoverInfo() const
	{
		return CurrentCombatHoverInfo;
	}

	UFUNCTION(BlueprintPure, Category = "Combat|Hover")
	UCombatHoverInfoWidget* GetCombatHoverInfoWidget() const
	{
		return CombatHoverInfoWidget;
	}

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

	UFUNCTION(BlueprintCallable, Category = "Combat|Cards")
	int32 GetDiscardCount() const
	{
		return DiscardPile.Num();
	}

	UPROPERTY(BlueprintAssignable, Category = "Combat|Cards")
	FOnDeckChangedSignature OnDeckChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Cards")
	FOnHandChangedSignature OnHandChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Hover")
	FOnCombatHoverInfoChangedSignature OnCombatHoverInfoChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Hover")
	bool bEnableCombatHoverInfo = true;

protected:
	void HandleLeftClick();
	void HandleRightClick();
	void HandleCancelSelection();
	void HandleEndTurnInput();
	void RefreshHUD();
	void RefreshCombatStateHUD();
	void BroadcastCardCounts();
	void InitializeCombatHoverInfoWidget();
	void UpdateCombatHoverInfo();
	void SetCurrentCombatHoverInfo(const FJargonCombatHoverInfo& NewHoverInfo);
	FJargonCombatHoverInfo BuildCombatHoverInfoFromHit(const FHitResult& HitResult) const;
	FJargonCombatHoverInfo MakeCombatHoverInfoFromUnit(ABattleUnit* Unit) const;
	FJargonCombatHoverInfo MakeCombatHoverInfoFromTileEffect(ABattleTileEffect* TileEffect) const;
	FJargonCombatHoverInfo MakeCombatHoverInfoFromTile(AGridTile* Tile) const;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hover", meta = (AllowPrivateAccess = "true", ToolTip = "Optional Blueprint child of CombatHoverInfoWidget. When assigned, the controller creates it and feeds description-only hover info into it."))
	TSubclassOf<UCombatHoverInfoWidget> CombatHoverInfoWidgetClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Hover", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatHoverInfoWidget> CombatHoverInfoWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hover", meta = (AllowPrivateAccess = "true"))
	int32 CombatHoverInfoWidgetZOrder = 20;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Hover", meta = (AllowPrivateAccess = "true"))
	FJargonCombatHoverInfo CurrentCombatHoverInfo;
};
