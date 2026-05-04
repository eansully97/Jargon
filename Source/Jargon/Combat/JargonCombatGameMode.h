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

	/** Snapshot metrics collected during one combat and handed to post-match reporting/audits. */
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

	/** Runtime combat bootstrap: board, units, deck/hand, hero state, artifacts, and initial turn flow. */
	void InitializeCombat();

	/** Applies initial facing after combatants are spawned and placed on the board. */
	void InitializeCombatantFacing();

	/** Selects a friendly unit for player actions when the unit is valid for the current combat phase. */
	bool TrySelectFriendlyUnit(ABattleUnit* FriendlyUnit);

	/** Attempts a player-controlled move and starts movement presentation when pathing succeeds. */
	bool TryMovePlayerUnitToTile(AGridTile* DestinationTile);

	/** Attempts a basic attack with the selected/player unit and routes through presentation timing. */
	bool TryBasicAttackWithPlayerUnit(ABattleUnit* Target);

	/** Card-play entry points used by the combat controller after target selection and optional bonus choice. */
	bool TryPlayCardOnTarget(UCardDefinition* Card, ABattleUnit* Target);
	bool TryPlayCardOnTarget(UCardDefinition* Card, ABattleUnit* Target, const TArray<int32>& SelectedElementalBonusIndices);
	bool TryPlayCardOnTile(UCardDefinition* Card, AGridTile* TileTarget);
	bool TryPlayCardOnTile(UCardDefinition* Card, AGridTile* TileTarget, const TArray<int32>& SelectedElementalBonusIndices);
	bool TryPlayCardOnSelf(UCardDefinition* Card);
	bool TryPlayCardOnSelf(UCardDefinition* Card, const TArray<int32>& SelectedElementalBonusIndices);
	bool StartPlayerControlledMoveSequence(ABattleUnit* MovingUnit, const TArray<AGridTile*>& Path, bool bConsumeMoveAction);

	/** Executes a unit death hook once, preserving the death tile for effects that need placement/target context. */
	void ExecuteOnDeathAbility(ABattleUnit* DeadUnit, AGridTile* DeathTile);

	/** Spawns a persistent trap/aura runtime actor from a Data Asset and registers it with combat state. */
	ABattleTileEffect* SpawnPersistentTileEffectFromDefinition(
		UJargonTileEffectDefinition* Definition,
		TSubclassOf<ABattleTileEffect> RuntimeTileEffectClass,
		const UCardDefinition* Card,
		const ABattleUnit* SourceUnit,
		AGridTile* TargetTile);

	/** Spawns a persistent tile effect when only source team is known, such as non-unit ability hooks. */
	ABattleTileEffect* SpawnPersistentTileEffectFromDefinitionForTeam(
		UJargonTileEffectDefinition* Definition,
		TSubclassOf<ABattleTileEffect> RuntimeTileEffectClass,
		const UCardDefinition* Card,
		ETeam SourceTeam,
		AGridTile* TargetTile);

	/** Spawns a summoned combat unit from a Data Asset and runtime Blueprint shell. */
	ABattleUnit* SpawnSummonedUnitFromDefinition(
		UJargonSummonedUnitDefinition* Definition,
		TSubclassOf<ABattleUnit> RuntimeSummonedUnitClass,
		const ABattleUnit* SourceUnit,
		AGridTile* TargetTile,
		bool bAttackExhaustedOverride,
		bool bUseAttackExhaustedOverride);

	/** Removes a tile effect from GameMode tracking after destruction or expiry. */
	void UnregisterPersistentTileEffect(ABattleTileEffect* TileEffect);

	/** Broadcasts unit-entry triggers to active tile effects after movement enters a tile. */
	void NotifyTileEffectsUnitEntered(ABattleUnit* EnteringUnit, AGridTile* EnteredTile);
	
	/** Updates target highlights for a selected card; presentation only, no card resolution. */
	void RefreshCardTargetHighlights(ABattleUnit* SourceUnit, const UCardDefinition* Card);

	/** Updates movement highlights for the currently selected friendly unit. */
	void RefreshPlayerMovementHighlights();
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Preview")
	void PreviewUnitMovementRange(ABattleUnit* UnitToPreview);

	void HandleUnitDied(ABattleUnit* DeadUnit, AGridTile* DeathTile = nullptr);
	void HandleVictory();
	void HandleDefeat();
	void ReturnToExploration();

	/** Player-facing turn request; validates phase before ending the turn. */
	void RequestEndPlayerTurn();

	/** Debug helper: asks the next card resolve to emit a full effect trace to logs. */
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

	/** Returns combat-local element charges owned by this GameMode. Element charges are not Energy. */
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

	/** Clears combat-local element charges, normally at combat setup/teardown boundaries. */
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

	/** Blueprint-assignable delegate fired after C++ updates hero aspect/class runtime state. */
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
	/** Finds or spawns combat-owned actors and wires runtime state before the first player turn. */
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
	/** Runtime board actor owned by the level; cached by GameMode for pathing, spawning, and targeting. */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<AGridBoard> GridBoard = nullptr;

	/** Player hero battle unit spawned/controlled for this combat only. */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<APlayerBattleUnit> PlayerUnit = nullptr;

	/** Static hero Data Asset selected by run state; copied into runtime unit/aspect state at combat start. */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Hero")
	TObjectPtr<UJargonHeroDefinition> ActiveHeroDefinition = nullptr;

	/** Combat-local hero class/aspect state derived from element charges and active hero definition. */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Hero")
	FJargonHeroRuntimeState HeroRuntimeState;

	/** Runtime friendly units currently participating in combat, including player summons. */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TArray<TObjectPtr<ABattleUnit>> FriendlyUnits;

	/** Runtime enemy units currently participating in combat. */
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

	/** Friendly unit selected for player movement, attacks, and card source context. */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<ABattleUnit> SelectedFriendlyUnit = nullptr;

	/** Persistent tile-effect actors owned by the world and tracked by GameMode for turn/entry hooks. */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Cards")
	TArray<TObjectPtr<ABattleTileEffect>> ActiveTileEffects;

	/** Camera pawn spawned by combat when a map does not already provide one. */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	TObjectPtr<ATacticsCameraPawn> SpawnedCameraPawn = nullptr;

	/** Runtime presentation actor receiving gameplay cue events. */
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

	/** Pending movement/attack state used while async movement or basic attack presentation is running. */
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
