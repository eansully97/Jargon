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

	/** Spawns the authored board tiles from editor/runtime settings. Safe for Blueprint calls during setup. */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void GenerateBoard();

	/** Editor button for rebuilding spawned tile actors while authoring a board. */
	UFUNCTION(CallInEditor, Category = "Grid")
	void RebuildGridInEditor();

	/** Clears and regenerates the runtime grid, including random resolved blocked coordinates. */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void RebuildGrid();

	/** Editor button that removes generated tile actors owned by this board. */
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

	/** Builds a shortest walkable path for unit movement; occupied/blocked tiles are treated as path blockers. */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<AGridTile*> BuildPath(AGridTile* StartTile, AGridTile* EndTile) const;

	/** Builds a path for pull-style effects where the endpoint may be occupied by the pull source. */
	TArray<AGridTile*> BuildPathAllowingOccupiedEndTile(AGridTile* StartTile, AGridTile* EndTile) const;

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

	/** Tile actor class designers assign in the board Blueprint/defaults before generation. */
	UPROPERTY(EditAnywhere, Category = "Grid")
	TSubclassOf<AGridTile> TileClass;

	/** Authored player spawn coordinate used by combat startup. */
	UPROPERTY(EditAnywhere, Category = "Grid")
	FHexCoord PlayerSpawnCoord;

	/** Authored default enemy spawn coordinate used when encounter data does not supply a tile. */
	UPROPERTY(EditAnywhere, Category = "Grid")
	FHexCoord EnemySpawnCoord;

	/** Designer-authored blocked coordinates that are always blocked before random obstacles are added. */
	UPROPERTY(EditAnywhere, Category = "Grid")
	TArray<FHexCoord> BlockedCoords;

	UPROPERTY(EditAnywhere, Category = "Grid|Obstacles", meta = (ClampMin = "0"))
	int32 RandomBlockedTileMin = 3;

	UPROPERTY(EditAnywhere, Category = "Grid|Obstacles", meta = (ClampMin = "0"))
	int32 RandomBlockedTileMax = 6;

	UPROPERTY(EditAnywhere, Category = "Grid|Obstacles")
	bool bProtectSpawnNeighborsFromRandomBlocking = true;

	/** Runtime lookup owned by the board; tiles are spawned actors and registered by hex coordinate. */
	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	TMap<FHexCoord, TObjectPtr<AGridTile>> TileMap;

	/** Final blocked coordinate set after combining authored and random obstacle choices. */
	UPROPERTY(VisibleInstanceOnly, Category = "Grid|Obstacles")
	TArray<FHexCoord> ResolvedBlockedCoords;

	FVector GetTileWorldLocation(const FHexCoord& Coord) const;
	TArray<AGridTile*> BuildPathInternal(AGridTile* StartTile, AGridTile* EndTile, bool bAllowOccupiedEndTile) const;
	bool IsCoordBlocked(const FHexCoord& Coord) const;
	FHexCoord GetPushDirectionOffset(const AGridTile* SourceTile, const AGridTile* TargetTile) const;
	void BuildResolvedBlockedCoords();
	bool ShouldProtectCoordFromRandomBlocking(const FHexCoord& Coord) const;
	void DestroyExistingTiles();
};
