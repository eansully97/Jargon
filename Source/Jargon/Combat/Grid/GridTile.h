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
class AGridBoard;

UCLASS()
class JARGON_API AGridTile : public AActor
{
	GENERATED_BODY()

public:
	AGridTile();

	/** Hex coordinate assigned by the owning GridBoard during generation. */
	UFUNCTION(BlueprintPure, Category = "Grid")
	FHexCoord GetCoord() const
	{
		return Coord;
	}

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetCoord(const FHexCoord& InCoord);

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

	UFUNCTION(BlueprintPure, Category = "Grid")
	AGridBoard* GetOwningGridBoard() const;

	UFUNCTION(BlueprintCallable, Category = "Grid|Effects")
	void AddTileEffect(ABattleTileEffect* TileEffect);

	UFUNCTION(BlueprintCallable, Category = "Grid|Effects")
	void RemoveTileEffect(ABattleTileEffect* TileEffect);

	/** Runtime tile effects currently registered on this tile; actors remain owned by the world/GameMode. */
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

	/** Hex board coordinate; visible at runtime and assigned by GridBoard generation. */
	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	FHexCoord Coord;

	/** Instance-authored blocker flag for fixed map obstacles. */
	UPROPERTY(EditInstanceOnly, Category = "Grid")
	bool bBlocked = false;

	/** Runtime occupancy pointer; ABattleUnit is responsible for updating it when moving tiles. */
	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	TObjectPtr<ABattleUnit> OccupyingUnit = nullptr;

	/** Presentation state set by movement, targeting, and selection highlight code. */
	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	ETileHighlightState HighlightState = ETileHighlightState::None;

	/** Runtime trap/aura actors registered on this tile. */
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
