// GridTile.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "GameFramework/Actor.h"
#include "GridTile.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UMaterialInterface;
class ABattleUnit;
class ABattleTileEffect;

UCLASS()
class JARGON_API AGridTile : public AActor
{
	GENERATED_BODY()

public:
	AGridTile();

	UFUNCTION(BlueprintPure, Category = "Grid")
	FIntPoint GetCoord() const
	{
		return Coord;
	}

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetCoord(const FIntPoint& InCoord);

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsBlocked() const
	{
		return bBlocked;
	}

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetBlocked(bool bInBlocked);

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsOccupied() const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsWalkable() const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	ABattleUnit* GetOccupyingUnit() const
	{
		return OccupyingUnit;
	}

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetOccupyingUnit(ABattleUnit* NewUnit);

	UFUNCTION(BlueprintPure, Category = "Grid")
	ETileHighlightState GetHighlightState() const
	{
		return HighlightState;
	}

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetHighlightState(ETileHighlightState NewState);

	UFUNCTION(BlueprintPure, Category = "Grid")
	FVector GetUnitStandLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Grid|Effects")
	void AddTileEffect(ABattleTileEffect* TileEffect);

	UFUNCTION(BlueprintCallable, Category = "Grid|Effects")
	void RemoveTileEffect(ABattleTileEffect* TileEffect);

	const TArray<TObjectPtr<ABattleTileEffect>>& GetTileEffects() const
	{
		return TileEffects;
	}

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void RefreshVisualState();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TileMesh;

	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(EditInstanceOnly, Category = "Grid")
	bool bBlocked = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	TObjectPtr<ABattleUnit> OccupyingUnit = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	ETileHighlightState HighlightState = ETileHighlightState::None;

	UPROPERTY(VisibleInstanceOnly, Category = "Grid|Effects")
	TArray<TObjectPtr<ABattleTileEffect>> TileEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> DefaultMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> ReachableMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> BlockedMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> OccupiedMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> SelectedMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	float UnitStandZOffset = 100.f;
};
