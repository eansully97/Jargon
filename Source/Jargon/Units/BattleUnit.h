// BattleUnit.h

#pragma once

#include "CoreMinimal.h"
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

UCLASS()
class JARGON_API ABattleUnit : public AActor
{
	GENERATED_BODY()

public:
	ABattleUnit();

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
	void MoveAlongPath(const TArray<AGridTile*>& Path);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void ApplyDamage(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void ApplyHeal(int32 Amount);

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

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void AddTemporaryShield(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	void ClearTemporaryShield();

	UFUNCTION(BlueprintCallable, Category = "Battle Unit|Presentation")
	void SetActingHighlight(bool bInActingHighlight);

	UFUNCTION(BlueprintPure, Category = "Battle Unit|Presentation")
	bool IsActingHighlighted() const
	{
		return bActingHighlight;
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

	void PlayDeathPresentation();
	void FinalizeDeathAndDestroy();

	UFUNCTION()
	void HandleDeathTimerElapsed();
	
protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> UnitMesh = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterialInstances;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> StatusWidgetComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UBattleUnitStatusWidget> StatusWidgetClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	int32 MaxHP = 5;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	int32 CurrentHP = 5;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	int32 TemporaryShield = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	int32 MoveRange = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	int32 AttackRange = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	int32 AttackDamage = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	ETeam Team = ETeam::Enemy;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Presentation")
	TObjectPtr<UAnimationAsset> BasicAttackAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Presentation")
	TObjectPtr<UAnimationAsset> IdleAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Presentation", meta = (ClampMin = "0.0"))
	float BasicAttackDamageDelay = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Presentation")
	bool bFaceTargetOnBasicAttack = true;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	TObjectPtr<AGridTile> CurrentTile = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	bool bIsDead = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Turn")
	bool bMoveActionUsedThisTurn = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Turn")
	bool bAttackActionUsedThisTurn = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit|Presentation")
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
	FLinearColor ActingHighlightColor = FLinearColor(1.f, 0.5f, 0.f, 0.2f);

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FName HighlightStrengthParameterName = TEXT("HighlightStrength");

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FName HighlightColorParameterName = TEXT("HighlightColor");

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FName FlashStrengthParameterName = TEXT("FlashStrength");

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Visual")
	FName FlashColorParameterName = TEXT("FlashColor");
	
	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Death")
	TObjectPtr<UAnimationAsset> DeathAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit|Death", meta = (ClampMin = "0.0"))
	float DeathDestroyDelay = 0.75f;

	FTimerHandle HitFlashTimerHandle;
	FTimerHandle DeathTimerHandle;
};
