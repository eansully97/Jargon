// GridBoard.h

#pragma once

#include "CoreMinimal.h"
#include "Core/JargonTypes.h"
#include "GameFramework/Actor.h"
#include "GridBoard.generated.h"

class USceneComponent;
class AGridTile;

UCLASS()
class JARGON_API AGridBoard : public AActor
{
	GENERATED_BODY()

public:
	AGridBoard();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void GenerateBoard();

	UFUNCTION(CallInEditor, Category = "Grid")
	void RebuildGridInEditor();

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void RebuildGrid();

	UFUNCTION(CallInEditor, Category = "Grid")
	void ClearSpawnedTiles();

	UFUNCTION(BlueprintPure, Category = "Grid")
	AGridTile* GetTile(const FHexCoord& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsCoordValid(const FHexCoord& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	TArray<AGridTile*> GetNeighbors(AGridTile* Tile) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	int32 GetCoordDistance(const FHexCoord& CoordA, const FHexCoord& CoordB) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	int32 GetTileDistance(const AGridTile* TileA, const AGridTile* TileB) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool AreTilesWithinRange(const AGridTile* TileA, const AGridTile* TileB, int32 Range) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<AGridTile*> FindReachableTiles(AGridTile* StartTile, int32 MoveRange) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<AGridTile*> BuildPath(AGridTile* StartTile, AGridTile* EndTile) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<AGridTile*> GetTilesWithinRadius(AGridTile* CenterTile, int32 Radius) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	AGridTile* GetTileInPushDirection(const AGridTile* SourceTile, const AGridTile* TargetTile, int32 StepDistance) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ClearHighlights();

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void HighlightReachableTilesFrom(AGridTile* StartTile, int32 MoveRange);

	void HighlightTilesInRangeFrom(AGridTile* StartTile, int32 Range);

	UFUNCTION(BlueprintPure, Category = "Grid")
	AGridTile* GetPlayerSpawnTile() const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	AGridTile* GetEnemySpawnTile() const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	AGridTile* FindNearestWalkableTile(const FHexCoord& PreferredCoord) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, Category = "Grid")
	int32 Width = 8;

	UPROPERTY(EditAnywhere, Category = "Grid")
	int32 Height = 6;

	UPROPERTY(EditAnywhere, Category = "Grid")
	float TileSize = 200.f;

	UPROPERTY(EditAnywhere, Category = "Grid")
	TSubclassOf<AGridTile> TileClass;

	UPROPERTY(EditAnywhere, Category = "Grid")
	FHexCoord PlayerSpawnCoord;

	UPROPERTY(EditAnywhere, Category = "Grid")
	FHexCoord EnemySpawnCoord;

	UPROPERTY(EditAnywhere, Category = "Grid")
	TArray<FHexCoord> BlockedCoords;

	UPROPERTY(EditAnywhere, Category = "Grid|Obstacles", meta = (ClampMin = "0"))
	int32 RandomBlockedTileMin = 3;

	UPROPERTY(EditAnywhere, Category = "Grid|Obstacles", meta = (ClampMin = "0"))
	int32 RandomBlockedTileMax = 6;

	UPROPERTY(EditAnywhere, Category = "Grid|Obstacles")
	bool bProtectSpawnNeighborsFromRandomBlocking = true;

	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	TMap<FHexCoord, TObjectPtr<AGridTile>> TileMap;

	UPROPERTY(VisibleInstanceOnly, Category = "Grid|Obstacles")
	TArray<FHexCoord> ResolvedBlockedCoords;

	FVector GetTileWorldLocation(const FHexCoord& Coord) const;
	bool IsCoordBlocked(const FHexCoord& Coord) const;
	FHexCoord GetPushDirectionOffset(const AGridTile* SourceTile, const AGridTile* TargetTile) const;
	void BuildResolvedBlockedCoords();
	bool ShouldProtectCoordFromRandomBlocking(const FHexCoord& Coord) const;
	void DestroyExistingTiles();
};
