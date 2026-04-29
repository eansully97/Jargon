// JargonCombatGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonRunStateTypes.h"
#include "Core/JargonTypes.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "JargonCombatGameMode.generated.h"

class AGridBoard;
class AGridTile;
class ABattleUnit;
class ABattleTileEffect;
class APlayerBattleUnit;
class AEnemyBattleUnit;
class ATacticsCameraPawn;
class UCardDefinition;
class UCombatHUDWidget;
class AJargonCombatPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatPhaseChangedSignature, ECombatPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatEnergyChangedSignature, int32, NewEnergy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentActingEnemyChangedSignature, ABattleUnit*, NewActingEnemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerActionAvailabilityChangedSignature, bool, bCanMove, bool, bCanAttack);

UCLASS()
class JARGON_API AJargonCombatGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJargonCombatGameMode();

	virtual void BeginPlay() override;

	void InitializeCombat();
	void InitializeCombatantFacing();

	bool TrySelectFriendlyUnit(ABattleUnit* FriendlyUnit);
	bool TryMovePlayerUnitToTile(AGridTile* DestinationTile);
	bool TryBasicAttackWithPlayerUnit(ABattleUnit* Target);
	bool TryPlayCardOnTarget(UCardDefinition* Card, ABattleUnit* Target);
	bool TryPlayCardOnTile(UCardDefinition* Card, AGridTile* TileTarget);
	bool TryPlayCardOnSelf(UCardDefinition* Card);
	bool StartPlayerControlledMoveSequence(ABattleUnit* MovingUnit, const TArray<AGridTile*>& Path, bool bConsumeMoveAction);
	void ExecuteOnDeathEffects(ABattleUnit* DeadUnit, AGridTile* DeathTile);
	ABattleTileEffect* SpawnPersistentTileEffectFromClass(
		TSubclassOf<ABattleTileEffect> TileEffectClass,
		const UCardDefinition* Card,
		const ABattleUnit* SourceUnit,
		AGridTile* TargetTile,
		ECardCategory EffectCategory,
		int32 EffectValue,
		int32 EffectRadius,
		int32 EffectDuration);
		ABattleUnit* SpawnSummonedUnitFromClass(
		TSubclassOf<ABattleUnit> UnitClass,
		const ABattleUnit* SourceUnit,
		AGridTile* TargetTile,
		bool bAttackExhaustedOnSpawn);
	void NotifyTileEffectsUnitEntered(ABattleUnit* EnteringUnit, AGridTile* EnteredTile);
	
	void RefreshCardTargetHighlights(ABattleUnit* SourceUnit, const UCardDefinition* Card);
	void RefreshPlayerMovementHighlights();
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Preview")
	void PreviewUnitMovementRange(ABattleUnit* UnitToPreview);

	void HandleUnitDied(ABattleUnit* DeadUnit, AGridTile* DeathTile = nullptr);
	void HandleVictory();
	void HandleDefeat();
	void ReturnToExploration();

	void RequestEndPlayerTurn();

	APlayerBattleUnit* GetPlayerUnit() const
	{
		return PlayerUnit;
	}

	const TArray<TObjectPtr<ABattleUnit>>& GetEnemyUnits() const
	{
		return EnemyUnits;
	}

	const TArray<TObjectPtr<ABattleUnit>>& GetFriendlyUnits() const
	{
		return FriendlyUnits;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	ABattleUnit* GetCurrentActingEnemy() const
	{
		return CurrentActingEnemy;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	ABattleUnit* GetSelectedFriendlyUnit() const
	{
		return SelectedFriendlyUnit;
	}

	AGridBoard* GetGridBoard() const
	{
		return GridBoard;
	}

	TSubclassOf<UCombatHUDWidget> GetCombatHUDClass() const
	{
		return CombatHUDClass;
	}

	const TArray<TObjectPtr<UCardDefinition>>& GetStartingDeckDefinitions() const
	{
		return StartingDeckDefinitions;
	}

	int32 GetStartingHandSize() const
	{
		return StartingHandSize;
	}

	UFUNCTION(BlueprintCallable, Category = "Combat")
	ECombatPhase GetCurrentCombatPhase() const
	{
		return CombatPhase;
	}

	UFUNCTION(BlueprintCallable, Category = "Combat")
	int32 GetCurrentRound() const
	{
		return CurrentRound;
	}

	UFUNCTION(BlueprintCallable, Category = "Combat")
	int32 GetCurrentEnergy() const
	{
		return CurrentEnergy;
	}

	UFUNCTION(BlueprintCallable, Category = "Combat")
	int32 GetCurrentMaxEnergy() const
	{
		return CurrentMaxEnergy;
	}

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void AddCurrentEnergy(int32 Amount);

	bool DrawCardsForPlayer(int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool HasPlayerMoveRemaining() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool HasPlayerAttackRemaining() const;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatPhaseChangedSignature OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatEnergyChangedSignature OnEnergyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnPlayerActionAvailabilityChangedSignature OnPlayerActionAvailabilityChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCurrentActingEnemyChangedSignature OnCurrentActingEnemyChanged;

protected:
	void InitializeCameraPawn();
	void FindGridBoard();
	void SpawnCombatants();
	void SpawnEnemiesFromPendingEncounter();
	void SpawnFallbackEnemy();
	AGridTile* ResolveEnemySpawnTile(const FHexCoord& PreferredCoord) const;
	ABattleUnit* SpawnEnemyUnitAtTile(TSubclassOf<ABattleUnit> UnitClass, AGridTile* SpawnTile);
	bool AreAllEnemiesDefeated() const;
	void RegisterBattleUnitCallbacks(ABattleUnit* Unit);
	void ResetCombatRewardState();
	void AccumulateEnemyKillReward(ABattleUnit* DeadEnemy);
	FJargonCurrencyAmount GetEnemyKillCurrencyReward(const ABattleUnit* DeadEnemy) const;

	void StartBattleFlow();
	void StartPlayerTurn();
	void EndPlayerTurn();
	void StartEnemyTurn();
	void ResolveEnemyTurn();
	void ResolveNextEnemyAction();
	bool ResolveSingleEnemyAction(ABattleUnit* EnemyUnit);
	void EndEnemyTurn();
	void ExecuteOnSummonedEffects(ABattleUnit* SummonedUnit);
	void ExecuteOnTurnStartEffects(ABattleUnit* SourceUnit);
	void NotifyPlayerTurnStartTileEffects();
	void RefreshSelectedFriendlyUnitPresentation();
	void SetSelectedFriendlyUnit(ABattleUnit* NewSelectedFriendlyUnit);
	void ClearPendingMovementSequence();
	bool StartEnemyMoveSequence(ABattleUnit* EnemyUnit, const TArray<AGridTile*>& Path);
	void HandleBattleUnitMovementCompleted(ABattleUnit* MovedUnit);
	void HandlePlayerControlledMoveCompleted(ABattleUnit* MovedUnit);
	void HandleEnemyMoveCompleted(ABattleUnit* MovedUnit);

	AJargonCombatPlayerController* GetCombatPlayerController() const;
	int32 CalculateMaxEnergyForRound(int32 RoundNumber) const;
	bool IsFriendlyUnitSelectable(const ABattleUnit* Unit) const;
	ABattleUnit* FindFallbackSelectedFriendlyUnit() const;
	ABattleUnit* FindPreferredEnemyTarget(ABattleUnit* EnemyUnit) const;
	bool TryPlayCardWithResolvedTile(UCardDefinition* Card, AGridTile* TileTarget, ABattleUnit* ExplicitUnitTarget, bool bSkipRangeValidation);
	AGridTile* FindBestEnemyMoveDestination(ABattleUnit* EnemyUnit, ABattleUnit* TargetUnit) const;
	int32 GetPreferredEnemyDistance(const ABattleUnit* EnemyUnit) const;
	bool CanUnitAttackFromTile(const ABattleUnit* EnemyUnit, const AGridTile* FromTile, const ABattleUnit* TargetUnit) const;
	int32 GetEnemyTileScore(const ABattleUnit* EnemyUnit, const AGridTile* CandidateTile, const ABattleUnit* TargetUnit) const;

	void SetCombatPhase(ECombatPhase NewPhase);
	void SetCurrentEnergy(int32 NewEnergy);
	void SetCurrentActingEnemy(ABattleUnit* NewActingEnemy);
	void BroadcastPlayerActionAvailabilityChanged();
	bool StartPresentedBasicAttack(
		ABattleUnit* Attacker,
		ABattleUnit* Target,
		bool bReturnToPlayerTurnAfterSequence,
		bool bContinueEnemyTurnAfterSequence);
	void ApplyPresentedBasicAttackDamage();
	void HandlePresentedBasicAttackCompleted();
	void ScheduleNextEnemyAction(float DelaySeconds);
	void ScheduleEndEnemyTurn(float DelaySeconds);
	void ClearEnemyTurnTimer();
	void ClearBasicAttackTimer();

	enum class EPendingMovementContext : uint8
	{
		None,
		PlayerControlled,
		Enemy
	};

protected:
	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<AGridBoard> GridBoard = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<APlayerBattleUnit> PlayerUnit = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TArray<TObjectPtr<ABattleUnit>> FriendlyUnits;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TArray<TObjectPtr<ABattleUnit>> EnemyUnits;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Rewards")
	int32 DefeatedEnemyCount = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Rewards")
	FJargonCurrencyAmount AccumulatedEnemyKillCurrency;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<ABattleUnit> CurrentActingEnemy = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<ABattleUnit> SelectedFriendlyUnit = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Cards")
	TArray<TObjectPtr<ABattleTileEffect>> ActiveTileEffects;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<ATacticsCameraPawn> SpawnedCameraPawn = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	ECombatPhase CombatPhase = ECombatPhase::BattleStart;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<ATacticsCameraPawn> CameraPawnClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<APlayerBattleUnit> PlayerUnitClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Enemy", meta = (DisplayName = "Fallback Enemy Unit Class"))
	TSubclassOf<AEnemyBattleUnit> EnemyUnitClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|UI")
	TSubclassOf<UCombatHUDWidget> CombatHUDClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Cards")
	TArray<TObjectPtr<UCardDefinition>> StartingDeckDefinitions;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Cards", meta = (ClampMin = "1"))
	int32 StartingHandSize = 3;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	int32 CurrentRound = 0;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	int32 CurrentEnergy = 0;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	int32 CurrentMaxEnergy = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn", meta = (ClampMin = "0", DisplayName = "Starting Max Energy"))
	int32 EnergyPerTurn = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn", meta = (ClampMin = "0"))
	int32 MaxEnergyIncreasePerRound = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn", meta = (ClampMin = "0"))
	int32 MaxEnergyCap = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn", meta = (ClampMin = "0"))
	int32 CardsDrawnPerTurn = 1;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	int32 EnemyTurnActionIndex = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn", meta = (ClampMin = "0.0"))
	float EnemyTurnStartDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn", meta = (ClampMin = "0.0"))
	float EnemyActionDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn", meta = (ClampMin = "0.0"))
	float EnemyTurnEndDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Turn|Presentation")
	FLinearColor SelectedFriendlyUnitHighlightColor = FLinearColor(0.85f, 0.85f, 0.1f, 0.2f);

	EPendingMovementContext PendingMovementContext = EPendingMovementContext::None;
	TWeakObjectPtr<ABattleUnit> PendingMovementUnit;
	TWeakObjectPtr<ABattleUnit> PendingAttackAttacker;
	TWeakObjectPtr<ABattleUnit> PendingAttackTarget;
	bool bReturnToPlayerTurnAfterAttackSequence = false;
	bool bContinueEnemyTurnAfterAttackSequence = false;

	FTimerHandle EnemyTurnTimerHandle;
	FTimerHandle BasicAttackDamageTimerHandle;
	FTimerHandle BasicAttackCompletionTimerHandle;
};
