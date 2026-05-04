// JargonCombatPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "Combat/Widgets/ElementalBonusChoiceTypes.h"
#include "Combat/Widgets/JargonHoverInfoTypes.h"
#include "Core/JargonHeroTypes.h"
#include "Core/JargonRunStateTypes.h"
#include "Core/JargonTypes.h"
#include "GameFramework/PlayerController.h"
#include "JargonCombatPlayerController.generated.h"

class AGridTile;
class ABattleTileEffect;
class ABattleUnit;
class UCardDefinition;
class UCombatHUDWidget;
class UCombatHoverInfoWidget;
class UElementalBonusChoiceWidget;
class UPostMatchReportWidget;

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

	UFUNCTION(Exec)
	void JargonResetRunSave();

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

	UFUNCTION(BlueprintPure, Category = "Combat|Elemental Bonus")
	UElementalBonusChoiceWidget* GetElementalBonusChoiceWidget() const
	{
		return ElementalBonusChoiceWidget;
	}

	UFUNCTION(BlueprintPure, Category = "Combat|Post Match")
	UPostMatchReportWidget* GetPostMatchReportWidget() const
	{
		return PostMatchReportWidget;
	}

	UFUNCTION(BlueprintCallable, Category = "Combat|Post Match")
	void ShowPostMatchReport(const FJargonPostCombatReportData& ReportData);

	UFUNCTION(BlueprintCallable, Category = "Combat|Post Match")
	void ContinueFromPostMatchReport();

	UFUNCTION(BlueprintPure, Category = "Combat|Elemental Bonus")
	FJargonElementalBonusChoiceRequest GetPendingElementalBonusChoiceRequest() const
	{
		return PendingElementalBonusChoiceRequest;
	}

	UFUNCTION(BlueprintPure, Category = "Combat|Elemental Bonus")
	bool HasPendingElementalBonusChoiceRequest() const
	{
		return PendingElementalBonusChoiceRequest.bHasUsableOptions;
	}

	UFUNCTION(BlueprintCallable, Category = "Combat|Elemental Bonus")
	bool ConfirmPendingElementalBonusChoices(const TArray<int32>& SelectedBonusIndices);

	UFUNCTION(BlueprintCallable, Category = "Combat|Elemental Bonus")
	bool ConfirmFirstPendingElementalBonusChoice();

	UFUNCTION(BlueprintCallable, Category = "Combat|Elemental Bonus")
	bool SkipPendingElementalBonusChoices();

	UFUNCTION(BlueprintCallable, Category = "Combat|Elemental Bonus")
	void CancelPendingElementalBonusChoice();

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
	void InitializeElementalBonusChoiceWidget();
	void HidePostMatchReport();
	void UpdateCombatHoverInfo();
	void SetCurrentCombatHoverInfo(const FJargonCombatHoverInfo& NewHoverInfo);
	FJargonCombatHoverInfo BuildCombatHoverInfoFromHit(const FHitResult& HitResult) const;
	FJargonCombatHoverInfo MakeCombatHoverInfoFromUnit(ABattleUnit* Unit) const;
	FJargonCombatHoverInfo MakeCombatHoverInfoFromTileEffect(ABattleTileEffect* TileEffect) const;
	FJargonCombatHoverInfo MakeCombatHoverInfoFromTile(AGridTile* Tile) const;
	bool TryBeginElementalBonusChoice(UCardDefinition* Card, AGridTile* TileTarget, bool bSelfTarget);
	FJargonElementalBonusChoiceRequest BuildElementalBonusChoiceRequest(UCardDefinition* Card) const;
	bool ExecutePendingElementalBonusCardPlay(const TArray<int32>& SelectedBonusIndices);
	void ClearPendingElementalBonusChoiceRequest(bool bNotifyWidget = true);

	UFUNCTION()
	void HandleCombatPhaseChanged(ECombatPhase NewPhase);

	UFUNCTION()
	void HandlePostMatchContinueRequested();

	UFUNCTION()
	void HandleCombatEnergyChanged(int32 NewEnergy);

	UFUNCTION()
	void HandleElementChargesChanged();

	UFUNCTION()
	void HandleHeroRuntimeStateChanged(const FJargonHeroRuntimeState& NewHeroRuntimeState);

	UFUNCTION()
	void HandlePlayerActionAvailabilityChanged(bool bCanMove, bool bCanAttack);

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|UI")
	TObjectPtr<UCombatHUDWidget> CombatHUD = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hover", meta = (AllowPrivateAccess = "true", ToolTip = "Optional Blueprint child of CombatHoverInfoWidget. When assigned, the controller creates it and feeds description-only hover info into it."))
	TSubclassOf<UCombatHoverInfoWidget> CombatHoverInfoWidgetClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Hover", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatHoverInfoWidget> CombatHoverInfoWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hover", meta = (AllowPrivateAccess = "true"))
	int32 CombatHoverInfoWidgetZOrder = 20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Elemental Bonus", meta = (AllowPrivateAccess = "true", ToolTip = "Optional Blueprint child of ElementalBonusChoiceWidget. When assigned, eligible elemental bonus groups prompt through this widget after target selection. When unassigned, bonuses are skipped and cards play base effects only."))
	TSubclassOf<UElementalBonusChoiceWidget> ElementalBonusChoiceWidgetClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Elemental Bonus", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UElementalBonusChoiceWidget> ElementalBonusChoiceWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Elemental Bonus", meta = (AllowPrivateAccess = "true"))
	int32 ElementalBonusChoiceWidgetZOrder = 25;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Post Match", meta = (AllowPrivateAccess = "true", ToolTip = "Blueprint child of PostMatchReportWidget shown in combat after victory or defeat. Continue returns to the saved exploration location."))
	TSubclassOf<UPostMatchReportWidget> PostMatchReportWidgetClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Post Match", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPostMatchReportWidget> PostMatchReportWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Post Match", meta = (AllowPrivateAccess = "true"))
	int32 PostMatchReportWidgetZOrder = 30;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Elemental Bonus", meta = (AllowPrivateAccess = "true"))
	FJargonElementalBonusChoiceRequest PendingElementalBonusChoiceRequest;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Elemental Bonus")
	TObjectPtr<UCardDefinition> PendingElementalBonusCard = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Elemental Bonus")
	TObjectPtr<AGridTile> PendingElementalBonusTileTarget = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Elemental Bonus")
	bool bPendingElementalBonusSelfTarget = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Elemental Bonus")
	bool bLoggedMissingElementalBonusPromptThisCombat = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Post Match")
	bool bPostMatchContinueRequested = false;
};
