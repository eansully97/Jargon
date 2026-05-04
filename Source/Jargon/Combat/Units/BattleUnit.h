// BattleUnit.h

#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "Core/JargonTypes.h"
#include "Data/JargonStatusEffectDefinition.h"
#include "GameFramework/Actor.h"
#include "BattleUnit.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class AGridTile;
class UWidgetComponent;
class UBattleUnitStatusWidget;
class UAnimationAsset;
class UMaterialInstanceDynamic;
class ABattleUnit;
class UJargonAbilityDefinition;
class UJargonSummonedUnitDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBattleUnitMovementCompletedSignature, ABattleUnit*);

UCLASS()
class JARGON_API ABattleUnit : public AActor
{
	GENERATED_BODY()

public:
	ABattleUnit();

	virtual void Tick(float DeltaSeconds) override;

	/** Rotates the unit toward a world-space direction for board-facing and attack presentation. */
	void FaceDirection(const FVector& WorldDirection);

	/** Rotates the unit toward a world-space location without moving it. */
	void FaceLocation(const FVector& WorldLocation);

	/** Enables/disables native material highlight feedback; safe for Blueprint presentation calls. */
	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Visual")
	void SetHighlightEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Visual")
	void SetHighlightColor(const FLinearColor& InColor);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Visual")
	void PlayHitFlash();

	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Visual")
	void ClearHitFlash();

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	int32 GetMaxHP() const
	{
		return MaxHP;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	int32 GetCurrentHP() const
	{
		return CurrentHP;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	int32 GetTemporaryShield() const
	{
		return TemporaryShield;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	int32 GetMoveRange() const
	{
		return MoveRange;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	int32 GetAttackRange() const
	{
		return AttackRange;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	int32 GetAttackDamage() const
	{
		return AttackDamage;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	ETeam GetTeam() const
	{
		return Team;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	AGridTile* GetCurrentTile() const
	{
		return CurrentTile;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	bool IsDead() const
	{
		return bIsDead;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Turn")
	void ResetTurnActions();

	UFUNCTION(BlueprintPure, Category = "Battle Unit|Turn")
	bool HasMoveActionRemaining() const
	{
		return !bMoveActionUsedThisTurn && !IsRooted() && !bMovementBlockedByRootThisTurn;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit|Turn")
	bool HasAttackActionRemaining() const
	{
		return !bAttackActionUsedThisTurn;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Turn")
	bool ConsumeMoveAction();

	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Turn")
	bool ConsumeAttackAction();

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void PlaceOnTile(AGridTile* Tile);

	/** Clears this unit from its current tile without choosing a new tile. Used during movement/death cleanup. */
	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void ClearCurrentTileOccupancy();

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void SetCurrentTile(AGridTile* Tile);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	bool MoveAlongPath(const TArray<AGridTile*>& Path);

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	bool IsMovingAlongPath() const
	{
		return bIsMovingAlongPath;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void ApplyDamage(int32 Amount);

	/** Damage helpers that preserve source/context for cues, pacing metrics, lifesteal, and status rules. */
	void ApplyDamageFromSource(int32 Amount, ABattleUnit* DamageSourceUnit);
	void ApplyDamageFromEffectContext(int32 Amount, const FJargonEffectContext& EffectContext);
	int32 ApplyDamageFromSourceAndGetHealthDamage(int32 Amount, ABattleUnit* DamageSourceUnit);
	int32 ApplyDamageFromEffectContextAndGetHealthDamage(int32 Amount, const FJargonEffectContext& EffectContext);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void ApplyHeal(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void IncreaseAttack(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void IncreaseMaxHealth(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void SetBaseCombatStats(int32 NewMaxHP, int32 NewMoveRange, int32 NewAttackRange, int32 NewAttackDamage, bool bRestoreToFullHealth = true);

	/** Copies static summon Data Asset stats, animations, and hooks onto this runtime unit. */
	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Summon")
	void ApplySummonedUnitDefinition(UJargonSummonedUnitDefinition* Definition);

	UFUNCTION(BlueprintPure, Category = "Battle Unit|Summon")
	UJargonSummonedUnitDefinition* GetAppliedSummonedUnitDefinition() const
	{
		return AppliedSummonedUnitDefinition;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	bool CanAttackTarget(const ABattleUnit* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	bool PerformBasicAttack(ABattleUnit* Target);

	/** Plays native/Blueprint attack presentation; damage timing remains coordinated by GameMode. */
	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Presentation")
	void PlayBasicAttackPresentation(ABattleUnit* Target);

	UFUNCTION(BlueprintPure, Category = "Battle Unit|Presentation")
	float GetBasicAttackDamageDelay() const
	{
		return BasicAttackDamageDelay;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit|Presentation")
	float GetBasicAttackPresentationDuration() const;

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void AddTemporaryShield(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void ClearTemporaryShield();

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	void ApplyStun(int32 Turns);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	bool ConsumeStunTurn();

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	bool IsStunned() const
	{
		return StunTurnsRemaining > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	int32 GetStunTurnsRemaining() const
	{
		return StunTurnsRemaining;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	void ApplyFreeze(int32 Turns);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	bool ConsumeFreezeTurn();

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	bool IsFrozen() const
	{
		return FreezeTurnsRemaining > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	int32 GetFreezeTurnsRemaining() const
	{
		return FreezeTurnsRemaining;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	void ApplyBurn(int32 Stacks);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	bool ConsumeBurnTurn();

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	bool IsBurning() const
	{
		return BurnStacks > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	int32 GetBurnStacks() const
	{
		return BurnStacks;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	void ApplyRoot(int32 Turns);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	bool ConsumeRootTurn();

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	bool IsRooted() const
	{
		return RootTurnsRemaining > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	int32 GetRootTurnsRemaining() const
	{
		return RootTurnsRemaining;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	void ApplyVulnerable(int32 BonusDamage);

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	bool IsVulnerable() const
	{
		return VulnerableDamageBonus > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	int32 GetVulnerableDamageBonus() const
	{
		return VulnerableDamageBonus;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	void ApplyRegen(int32 Stacks);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	bool ConsumeRegenTurn();

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	bool IsRegenerating() const
	{
		return RegenStacks > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	int32 GetRegenStacks() const
	{
		return RegenStacks;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	void ApplyWeak(int32 DamageReduction);

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	bool IsWeakened() const
	{
		return WeakDamageReduction > 0;
	}

	UFUNCTION(BlueprintPure, Category = "Battle Unit | Status")
	int32 GetWeakDamageReduction() const
	{
		return WeakDamageReduction;
	}

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	bool CleanseStatus(EJargonStatusEffectKind StatusKind);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit | Status")
	bool CleanseAllNegativeStatuses();

	/** Presentation highlight for the enemy currently taking an AI action. */
	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Presentation")
	void SetActingHighlight(bool bInActingHighlight);

	UFUNCTION(BlueprintPure, Category = "Battle Unit|Presentation")
	bool IsActingHighlighted() const
	{
		return bActingHighlight;
	}

	FOnBattleUnitMovementCompletedSignature& OnMovementCompleted()
	{
		return MovementCompletedDelegate;
	}

	/** Reusable ability hooks copied from unit definitions or authored on specialized unit Blueprints. */
	UJargonAbilityDefinition* GetOnSummonedAbility() const
	{
		return OnSummonedAbility;
	}

	UJargonAbilityDefinition* GetOnTurnStartAbility() const
	{
		return OnTurnStartAbility;
	}

	UJargonAbilityDefinition* GetOnDeathAbility() const
	{
		return OnDeathAbility;
	}

	bool HasExecutedDeathEffects() const
	{
		return bHasExecutedDeathEffects;
	}

	void MarkDeathEffectsExecuted()
	{
		bHasExecutedDeathEffects = true;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	/** Creates and binds the status widget component instance if a widget class is assigned. */
	void InitializeStatusWidget();

	/** Pushes runtime HP/shield/status values to the status widget and Blueprint presentation hooks. */
	void RefreshStatusWidget();

	/** Creates dynamic material instances so highlight and hit-flash feedback does not mutate shared materials. */
	void InitializeDynamicMaterials();
	void RefreshMaterialFeedback();
	void SetFlashColor(const FLinearColor& InColor);

	UFUNCTION()
	void HandleHitFlashTimerElapsed();
	
	UFUNCTION()
	void ReturnToIdleAfterBasicAttack();
	FTimerHandle BasicAttackTimerHandle;

	void AdvanceMovementSegment();
	void HandlePathSegmentArrival(AGridTile* ReachedTile, bool bIsFinalTile);
	void EnterTileDuringPathMovement(AGridTile* Tile, bool bKeepOccupancyAfterEntry);
	void FinishPathMovement();
	void StopPathMovement();

	void PlayIdleAnimation();
	void InitializeIdlePresentation();
	void PlayDeathPresentation();
	void FinalizeDeathAndDestroy();
	int32 ApplyDamageInternal(int32 Amount, const FJargonCombatCueEvent* DamageCueSource);
	int32 ConsumeWeakDamageReductionForOutgoingDamage(int32 Amount);
	void EmitDamageCue(int32 Value, AGridTile* CueTile, const FJargonCombatCueEvent* DamageCueSource);
	void EmitUnitCue(EJargonCombatCueType CueType, int32 Value = 0, AGridTile* CueTile = nullptr);

	UFUNCTION()
	void HandleDeathTimerElapsed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle Unit|Presentation")
	void BP_OnStatusChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle Unit|Presentation")
	void BP_OnShieldChanged(int32 NewShieldValue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle Unit|Presentation")
	void BP_OnStunChanged(int32 NewStunTurnsRemaining);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle Unit|Presentation")
	void BP_OnFreezeChanged(int32 NewFreezeTurnsRemaining);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle Unit|Presentation")
	void BP_OnBurnChanged(int32 NewBurnStacks);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle Unit|Presentation")
	void BP_OnRootChanged(int32 NewRootTurnsRemaining);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle Unit|Presentation")
	void BP_OnVulnerableChanged(int32 NewVulnerableDamageBonus);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle Unit|Presentation")
	void BP_OnRegenChanged(int32 NewRegenStacks);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle Unit|Presentation")
	void BP_OnWeakChanged(int32 NewWeakDamageReduction);
	
protected:
	/** Runtime component hierarchy root owned by the battle unit actor. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	/** Mesh component configured by the unit Blueprint; C++ drives animations and material feedback when assigned. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> UnitMesh = nullptr;

	/** Dynamic material instances created at runtime for highlight and hit-flash parameters. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterialInstances;

	/** Widget component that owns the native battle unit status widget instance. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> StatusWidgetComponent = nullptr;

	/** Status widget Blueprint class displayed above this unit; optional for purely Blueprint-driven presentation. */
	UPROPERTY(VisibleDefaultsOnly, Category = "UI")
	TSubclassOf<UBattleUnitStatusWidget> StatusWidgetClass = nullptr;

	/** Combat team used by targeting filters and AI decisions. */
	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	ETeam Team = ETeam::Enemy;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Stats", meta = (ClampMin = "1", ToolTip = "Base max HP for enemies and summons. Player hero units are overridden by HeroDefinition.", EditCondition = "Team != ETeam::Player", EditConditionHides))
	int32 MaxHP = 5;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	int32 CurrentHP = 5;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	int32 TemporaryShield = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Stats", meta = (ClampMin = "0", ToolTip = "Base move range for enemies and summons. Player hero units are overridden by HeroDefinition.", EditCondition = "Team != ETeam::Player", EditConditionHides))
	int32 MoveRange = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Stats", meta = (ClampMin = "1", ToolTip = "Base basic attack range for enemies and summons. Player hero units are overridden by HeroDefinition.", EditCondition = "Team != ETeam::Player", EditConditionHides))
	int32 AttackRange = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Stats", meta = (ClampMin = "0", ToolTip = "Base basic attack damage for enemies and summons. Player hero units are overridden by HeroDefinition.", EditCondition = "Team != ETeam::Player", EditConditionHides))
	int32 AttackDamage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle Unit|Abilities", meta = (AllowPrivateAccess = "true", ToolTip = "Reusable ability definition resolved when this unit is summoned. Data-driven summoned units receive this from their Summoned Unit Definition."))
	TObjectPtr<UJargonAbilityDefinition> OnSummonedAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle Unit|Abilities", meta = (AllowPrivateAccess = "true", ToolTip = "Reusable ability definition resolved at the start of this unit's side turn."))
	TObjectPtr<UJargonAbilityDefinition> OnTurnStartAbility = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle Unit|Abilities", meta = (AllowPrivateAccess = "true", ToolTip = "Reusable ability definition resolved once when this unit dies. The death tile is captured before occupancy is cleared."))
	TObjectPtr<UJargonAbilityDefinition> OnDeathAbility = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battle Unit|Summon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UJargonSummonedUnitDefinition> AppliedSummonedUnitDefinition = nullptr;

	/** Tile currently occupied by this runtime unit. The tile stores the reciprocal occupancy pointer. */
	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	TObjectPtr<AGridTile> CurrentTile = nullptr;

	/** Runtime death flag; Data Assets never store current HP/death state. */
	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit | Status")
	bool bIsDead = false;

	/** Guards death hooks so OnDeath abilities resolve once even if multiple damage paths report death. */
	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit | Status")
	bool bHasExecutedDeathEffects = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Unit | Status", meta = (AllowPrivateAccess = "true"))
	int32 StunTurnsRemaining = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Unit | Status", meta = (AllowPrivateAccess = "true"))
	int32 FreezeTurnsRemaining = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Unit | Status", meta = (AllowPrivateAccess = "true"))
	int32 BurnStacks = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Unit | Status", meta = (AllowPrivateAccess = "true"))
	int32 RootTurnsRemaining = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Unit | Status", meta = (AllowPrivateAccess = "true"))
	int32 VulnerableDamageBonus = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Unit | Status", meta = (AllowPrivateAccess = "true"))
	int32 RegenStacks = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Unit | Status", meta = (AllowPrivateAccess = "true"))
	int32 WeakDamageReduction = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Status")
	TObjectPtr<UAnimationAsset> DeathAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Status", meta = (ClampMin = "0.0"))
	float DeathDestroyDelay = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Presentation")
	TObjectPtr<UAnimationAsset> BasicAttackAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Presentation")
	TObjectPtr<UAnimationAsset> IdleAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Presentation", meta = (ClampMin = "0.0"))
	float BasicAttackDamageDelay = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Presentation")
	bool bFaceTargetOnBasicAttack = true;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Turn")
	bool bMoveActionUsedThisTurn = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Turn")
	bool bAttackActionUsedThisTurn = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Turn")
	bool bMovementBlockedByRootThisTurn = false;

	/** Runtime path data for async tile-by-tile movement presentation. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGridTile>> ActiveMovePath;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Movement", meta = (ClampMin = "0.01"))
	float MovementSecondsPerTile = 0.18f;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Movement")
	bool bIsMovingAlongPath = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Movement")
	int32 ActiveMoveSegmentIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Movement")
	float ActiveMoveSegmentElapsed = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Movement")
	float ActiveMoveSegmentDuration = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Movement")
	FVector ActiveMoveSegmentStart = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Movement")
	FVector ActiveMoveSegmentEnd = FVector::ZeroVector;

	/** Runtime visual flags that feed dynamic material parameters. */
	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Visual")
	bool bActingHighlight = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Visual")
	bool bHighlightEnabled = false;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FLinearColor HighlightColor = FLinearColor(1.f, 1.f, 0.f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HighlightStrengthWhenEnabled = 1.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Visual")
	bool bHitFlashActive = false;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FLinearColor HitFlashColor = FLinearColor(1.f, 0.f, 0.f, 0.5f);

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitFlashStrengthWhenActive = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual", meta = (ClampMin = "0.0"))
	float HitFlashDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FName HighlightStrengthParameterName = TEXT("HighlightStrength");

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FName HighlightColorParameterName = TEXT("HighlightColor");

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FName FlashStrengthParameterName = TEXT("FlashStrength");

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FName FlashColorParameterName = TEXT("FlashColor");

	FOnBattleUnitMovementCompletedSignature MovementCompletedDelegate;
	
	FTimerHandle HitFlashTimerHandle;
	FTimerHandle DeathTimerHandle;
};
