// BattleUnit.h

#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "Core/JargonTypes.h"
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
class UJargonSummonedUnitDefinition;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBattleUnitMovementCompletedSignature, ABattleUnit*);

UCLASS()
class JARGON_API ABattleUnit : public AActor
{
	GENERATED_BODY()

public:
	ABattleUnit();

	virtual void Tick(float DeltaSeconds) override;

	void FaceDirection(const FVector& WorldDirection);
	void FaceLocation(const FVector& WorldLocation);

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
		return !bMoveActionUsedThisTurn;
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

	void ApplyDamageFromSource(int32 Amount, ABattleUnit* DamageSourceUnit);
	void ApplyDamageFromEffectContext(int32 Amount, const FJargonEffectContext& EffectContext);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void ApplyHeal(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void IncreaseAttack(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void IncreaseMaxHealth(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void SetBaseCombatStats(int32 NewMaxHP, int32 NewMoveRange, int32 NewAttackRange, int32 NewAttackDamage, bool bRestoreToFullHealth = true);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Summon")
	void ApplySummonedUnitDefinition(UJargonSummonedUnitDefinition* Definition);

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	bool CanAttackTarget(const ABattleUnit* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	bool PerformBasicAttack(ABattleUnit* Target);

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

	const TArray<FJargonEffectSpec>& GetOnSummonedEffects() const
	{
		return OnSummonedEffects;
	}

	const TArray<FJargonEffectSpec>& GetOnTurnStartEffects() const
	{
		return OnTurnStartEffects;
	}

	const TArray<FJargonEffectSpec>& GetOnDeathEffects() const
	{
		return OnDeathEffects;
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
	
	void InitializeStatusWidget();
	void RefreshStatusWidget();

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

	void PlayDeathPresentation();
	void FinalizeDeathAndDestroy();
	void ApplyDamageInternal(int32 Amount, const FJargonCombatCueEvent* DamageCueSource);
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
	
protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> UnitMesh = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterialInstances;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> StatusWidgetComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "UI")
	TSubclassOf<UBattleUnitStatusWidget> StatusWidgetClass = nullptr;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle Unit|Effects", meta = (AllowPrivateAccess = "true"))
	TArray<FJargonEffectSpec> OnSummonedEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle Unit|Effects", meta = (AllowPrivateAccess = "true", ToolTip = "Shared effects resolved at the start of this unit's side turn. Use self and radius effects here; targeted activated abilities are not supported yet."))
	TArray<FJargonEffectSpec> OnTurnStartEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle Unit|Effects", meta = (AllowPrivateAccess = "true", ToolTip = "Shared effects resolved once when this unit dies. The death tile is captured before occupancy is cleared."))
	TArray<FJargonEffectSpec> OnDeathEffects;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	TObjectPtr<AGridTile> CurrentTile = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit | Status")
	bool bIsDead = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit | Status")
	bool bHasExecutedDeathEffects = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Unit | Status", meta = (AllowPrivateAccess = "true"))
	int32 StunTurnsRemaining = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle Unit | Status", meta = (AllowPrivateAccess = "true"))
	int32 FreezeTurnsRemaining = 0;

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
