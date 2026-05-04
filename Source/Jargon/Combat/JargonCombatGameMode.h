// JargonCombatGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "Core/JargonHeroTypes.h"
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
class UJargonArtifactDefinition;
class AJargonCombatPresentationManager;
class UJargonCombatPresentationSettings;
class UJargonDeckDefinition;
class UJargonHeroDefinition;
class UJargonSummonedUnitDefinition;
class UJargonTileEffectDefinition;
class UJargonAbilityDefinition;
struct FJargonHeroAspectDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatPhaseChangedSignature, ECombatPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatEnergyChangedSignature, int32, NewEnergy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnElementChargesChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeroRuntimeStateChangedSignature, const FJargonHeroRuntimeState&, NewHeroRuntimeState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentActingEnemyChangedSignature, ABattleUnit*, NewActingEnemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerActionAvailabilityChangedSignature, bool, bCanMove, bool, bCanAttack);

USTRUCT(BlueprintType)
struct JARGON_API FJargonCombatPacingSummary
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 InitialEnemyCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 PlayerTurnsTaken = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 EnemyTurnsTaken = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 CardsPlayed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 EnergySpentOnCards = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 PlayerDamageDealt = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 PlayerDamageTaken = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 EnemyDamageDealt = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 EnemyDamageTaken = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 PlayerHealingReceived = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 EnemyHealingReceived = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 PlayerShieldGained = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 EnemyShieldGained = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 PlayerSummonsCreated = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 EnemySummonsCreated = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 EnemiesKilled = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 FriendlyUnitsLost = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	bool bHeroDied = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 HeroStartingHP = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Pacing")
	int32 HeroStartingMaxHP = 0;
};

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
	bool TryPlayCardOnTarget(UCardDefinition* Card, ABattleUnit* Target, const TArray<int32>& SelectedElementalBonusIndices);
	bool TryPlayCardOnTile(UCardDefinition* Card, AGridTile* TileTarget);
	bool TryPlayCardOnTile(UCardDefinition* Card, AGridTile* TileTarget, const TArray<int32>& SelectedElementalBonusIndices);
	bool TryPlayCardOnSelf(UCardDefinition* Card);
	bool TryPlayCardOnSelf(UCardDefinition* Card, const TArray<int32>& SelectedElementalBonusIndices);
	bool StartPlayerControlledMoveSequence(ABattleUnit* MovingUnit, const TArray<AGridTile*>& Path, bool bConsumeMoveAction);
	void ExecuteOnDeathAbility(ABattleUnit* DeadUnit, AGridTile* DeathTile);
	ABattleTileEffect* SpawnPersistentTileEffectFromDefinition(
		UJargonTileEffectDefinition* Definition,
		TSubclassOf<ABattleTileEffect> RuntimeTileEffectClass,
		const UCardDefinition* Card,
		const ABattleUnit* SourceUnit,
		AGridTile* TargetTile);
	ABattleTileEffect* SpawnPersistentTileEffectFromDefinitionForTeam(
		UJargonTileEffectDefinition* Definition,
		TSubclassOf<ABattleTileEffect> RuntimeTileEffectClass,
		const UCardDefinition* Card,
		ETeam SourceTeam,
		AGridTile* TargetTile);
	ABattleUnit* SpawnSummonedUnitFromDefinition(
		UJargonSummonedUnitDefinition* Definition,
		TSubclassOf<ABattleUnit> RuntimeSummonedUnitClass,
		const ABattleUnit* SourceUnit,
		AGridTile* TargetTile,
		bool bAttackExhaustedOverride,
		bool bUseAttackExhaustedOverride);
	void UnregisterPersistentTileEffect(ABattleTileEffect* TileEffect);
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
	void RequestLogNextCardEffectTrace();

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

	TArray<UCardDefinition*> GetEmergencyStartingDeckCards() const;

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

	UFUNCTION(BlueprintPure, Category = "Combat|Elements")
	int32 GetElementCharges(EJargonElementType Element) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Elements")
	int32 GetElementChargeCap() const
	{
		return 10;
	}

	UFUNCTION(BlueprintCallable, Category = "Combat|Elements")
	void GainElementCharges(EJargonElementType Element, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Combat|Elements")
	bool HasElementCharges(EJargonElementType Element, int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Elements")
	bool TrySpendElementCharges(EJargonElementType Element, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Elements")
	void ClearElementCharges();

	const TMap<EJargonElementType, int32>& GetAllElementCharges() const
	{
		return ElementCharges;
	}

	UFUNCTION(BlueprintPure, Category = "Combat|Hero")
	UJargonHeroDefinition* GetActiveHeroDefinition() const
	{
		return ActiveHeroDefinition;
	}

	UFUNCTION(BlueprintPure, Category = "Combat|Hero")
	FJargonHeroRuntimeState GetHeroRuntimeState() const
	{
		return HeroRuntimeState;
	}

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Class")
	bool GetActiveHeroClassInfo(FJargonHeroClassInfo& OutClassInfo) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Class")
	bool GetHeroClassInfo(EJargonHeroClass HeroClass, FJargonHeroClassInfo& OutClassInfo) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Class")
	TArray<FJargonHeroClassInfo> GetConfiguredHeroClassInfos() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Class")
	FText GetActiveHeroClassDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Class")
	FText GetActiveHeroClassDescription() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Aspect")
	bool GetActiveHeroAspectInfo(FJargonHeroAspectInfo& OutAspectInfo) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Aspect")
	bool GetHeroAspectInfo(EJargonHeroClass HeroClass, EJargonElementType ElementType, FJargonHeroAspectInfo& OutAspectInfo) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Aspect")
	bool GetHeroAspectInfoForElement(EJargonHeroClass HeroClass, EJargonElementType Element, FJargonHeroAspectInfo& OutAspectInfo) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Aspect")
	TArray<FJargonHeroAspectInfo> GetConfiguredHeroAspectInfos() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Aspect")
	FText GetActiveHeroAspectDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Aspect")
	FText GetActiveHeroAspectDescription() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Aspect")
	FText GetActiveHeroAspectPassiveName() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Aspect")
	FText GetActiveHeroDominantElementDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Hero|Aspect")
	int32 GetHeroAspectActivationChargeThreshold() const
	{
		return GetElementChargeCap();
	}

	bool DrawCardsForPlayer(int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Combat|Presentation")
	void EmitCombatCue(const FJargonCombatCueEvent& Cue);

	UFUNCTION(BlueprintPure, Category = "Combat|Pacing")
	FJargonCombatPacingSummary GetCombatPacingSummary() const
	{
		return CombatPacingSummary;
	}

	void RecordCombatDamageApplied(const ABattleUnit* SourceUnit, const ABattleUnit* TargetUnit, int32 DamageAmount);
	void RecordCombatHealingApplied(const ABattleUnit* TargetUnit, int32 HealAmount);
	void RecordCombatShieldGained(const ABattleUnit* TargetUnit, int32 ShieldAmount);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool HasPlayerMoveRemaining() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool HasPlayerAttackRemaining() const;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatPhaseChangedSignature OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatEnergyChangedSignature OnEnergyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Elements")
	FOnElementChargesChangedSignature OnElementChargesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Hero")
	FOnHeroRuntimeStateChangedSignature OnHeroRuntimeStateChanged;

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Hero")
	void BP_OnHeroRuntimeStateChanged(const FJargonHeroRuntimeState& NewHeroRuntimeState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Hero")
	void BP_OnHeroClassPassiveTriggered(EJargonHeroClass HeroClass, EJargonEffectTrigger Trigger, const FText& PassiveName);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Hero")
	void BP_OnHeroAspectPassiveTriggered(EJargonHeroAspect HeroAspect, EJargonEffectTrigger Trigger, const FText& PassiveName);

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnPlayerActionAvailabilityChangedSignature OnPlayerActionAvailabilityChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCurrentActingEnemyChangedSignature OnCurrentActingEnemyChanged;

protected:
	void InitializeCameraPawn();
	void InitializePresentationManager();
	void FindGridBoard();
	void SpawnCombatants();
	void SpawnEnemiesFromPendingEncounter();
	TSubclassOf<APlayerBattleUnit> ResolvePlayerUnitClass() const;
	void ApplyActiveHeroToPlayerUnit();
	void SpawnFallbackEnemy();
	AGridTile* ResolveEnemySpawnTile(const FHexCoord& PreferredCoord) const;
	ABattleUnit* SpawnEnemyUnitAtTile(TSubclassOf<ABattleUnit> UnitClass, AGridTile* SpawnTile);
	bool AreAllEnemiesDefeated() const;
	void RegisterBattleUnitCallbacks(ABattleUnit* Unit);
	void ResetCombatRewardState();
	void AccumulateEnemyKillReward(ABattleUnit* DeadEnemy);
	FJargonCurrencyAmount GetEnemyKillCurrencyReward(const ABattleUnit* DeadEnemy) const;
	void ResetCombatPacingSummary();
	void InitializeCombatPacingSummary();
	void RecordCombatCardPlayed(const UCardDefinition* Card, int32 EnergyCost);
	void RecordCombatUnitSummoned(const ABattleUnit* SummonedUnit);
	void RecordCombatUnitDied(const ABattleUnit* DeadUnit);
	void LogCombatPacingSummary(bool bVictory);

	void StartBattleFlow();
	void StartPlayerTurn();
	void EndPlayerTurn();
	void StartEnemyTurn();
	void ResolveEnemyTurn();
	void ResolveNextEnemyAction();
	bool ResolveSingleEnemyAction(ABattleUnit* EnemyUnit);
	void EndEnemyTurn();
	void ExecuteRunArtifactOnCombatStartAbilities();
	void ExecuteRunArtifactOnPlayerTurnStartAbilities();
	void ExecuteRunArtifactOnEnemyDeathAbilities(ABattleUnit* DeadEnemy, AGridTile* DeathTile);
	void ExecuteHeroAspectPlayerTurnStartPassive();
	void ExecuteHeroAspectEnemyDeathPassive(ABattleUnit* DeadEnemy, AGridTile* DeathTile);
	void ResolveHeroAspectPassiveEffects(
		EJargonEffectTrigger Trigger,
		const FText& PassiveName,
		const TArray<FJargonEffectSpec>& Effects,
		const UJargonAbilityDefinition* AbilityDefinition,
		ABattleUnit* PrimaryUnitTarget,
		AGridTile* PrimaryTileTarget,
		ABattleUnit* TriggeringUnit);
	void ExecuteOnSummonedAbility(ABattleUnit* SummonedUnit);
	void ExecuteOnTurnStartAbility(ABattleUnit* SourceUnit);
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
	ABattleUnit* SpawnSummonedUnitActor(TSubclassOf<ABattleUnit> UnitClass, AGridTile* TargetTile);
	ABattleUnit* FinalizeSpawnedSummonedUnit(
		ABattleUnit* SpawnedUnit,
		const ABattleUnit* SourceUnit,
		AGridTile* TargetTile,
		bool bAttackExhaustedOnSpawn,
		bool bRegisterEnemyTeam);
	bool TryPlayCardWithResolvedTile(
		UCardDefinition* Card,
		AGridTile* TileTarget,
		ABattleUnit* ExplicitUnitTarget,
		bool bSkipRangeValidation,
		const TArray<int32>& SelectedElementalBonusIndices);
	AGridTile* FindBestEnemyMoveDestination(ABattleUnit* EnemyUnit, ABattleUnit* TargetUnit) const;
	int32 GetPreferredEnemyDistance(const ABattleUnit* EnemyUnit) const;
	bool CanUnitAttackFromTile(const ABattleUnit* EnemyUnit, const AGridTile* FromTile, const ABattleUnit* TargetUnit) const;
	int32 GetEnemyTileScore(const ABattleUnit* EnemyUnit, const AGridTile* CandidateTile, const ABattleUnit* TargetUnit) const;

	void SetCombatPhase(ECombatPhase NewPhase);
	void SetCurrentEnergy(int32 NewEnergy);
	void RefreshHeroRuntimeStateFromElements();
	EJargonElementType ResolveDominantElementFromCharges(EJargonElementType PreviousDominantElement) const;
	bool TryLockHeroAspectTransformation(EJargonElementType Element, int32 OldCharges, int32 NewCharges);
	void EmitHeroAspectTransformationCue(const FJargonHeroAspectDefinition* AspectDefinition);
	void ResolveHeroAspectTransformationEffects(const FJargonHeroAspectDefinition& AspectDefinition);
	bool IsHeroRuntimeStateDifferent(const FJargonHeroRuntimeState& First, const FJargonHeroRuntimeState& Second) const;
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

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Hero")
	TObjectPtr<UJargonHeroDefinition> ActiveHeroDefinition = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Hero")
	FJargonHeroRuntimeState HeroRuntimeState;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TArray<TObjectPtr<ABattleUnit>> FriendlyUnits;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TArray<TObjectPtr<ABattleUnit>> EnemyUnits;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Rewards")
	int32 DefeatedEnemyCount = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Rewards")
	FJargonCurrencyAmount AccumulatedEnemyKillCurrency;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Pacing", meta = (AllowPrivateAccess = "true"))
	FJargonCombatPacingSummary CombatPacingSummary;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Pacing")
	bool bLogCombatPacingSummary = true;

	bool bHasLoggedCombatPacingSummary = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<ABattleUnit> CurrentActingEnemy = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<ABattleUnit> SelectedFriendlyUnit = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Cards")
	TArray<TObjectPtr<ABattleTileEffect>> ActiveTileEffects;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<ATacticsCameraPawn> SpawnedCameraPawn = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Presentation")
	TObjectPtr<AJargonCombatPresentationManager> PresentationManager = nullptr;

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

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Presentation")
	TSubclassOf<AJargonCombatPresentationManager> PresentationManagerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Presentation")
	TObjectPtr<UJargonCombatPresentationSettings> PresentationSettings = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Cards", meta = (ToolTip = "Preferred deck Data Asset used only when combat is direct-loaded without an active run."))
	TObjectPtr<UJargonDeckDefinition> EmergencyStartingDeckDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Cards", meta = (ClampMin = "1"))
	int32 StartingHandSize = 3;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	int32 CurrentRound = 0;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	int32 CurrentEnergy = 0;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Turn")
	int32 CurrentMaxEnergy = 0;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Elements")
	TMap<EJargonElementType, int32> ElementCharges;

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

private:
	bool bLogNextCardEffectTrace = false;
};
