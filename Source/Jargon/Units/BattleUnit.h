// BattleUnit.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "GameFramework/Actor.h"
#include "BattleUnit.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class AGridTile;

UCLASS()
class JARGON_API ABattleUnit : public AActor
{
	GENERATED_BODY()

public:
	ABattleUnit();

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

	UFUNCTION(BlueprintPure, Category = "Battle Unit")
	bool CanAttackTarget(const ABattleUnit* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Battle Unit")
	bool PerformBasicAttack(ABattleUnit* Target);

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> UnitMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	int32 MaxHP = 5;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	int32 CurrentHP = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	int32 MoveRange = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	int32 AttackRange = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	int32 AttackDamage = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Battle Unit")
	ETeam Team = ETeam::Enemy;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	TObjectPtr<AGridTile> CurrentTile = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Battle Unit")
	bool bIsDead = false;
};