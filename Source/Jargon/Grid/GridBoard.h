// GridBoard.h

#pragma once

#include "CoreMinimal.h"
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

	UFUNCTION(BlueprintPure, Category = "Grid")
	AGridTile* GetTile(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsCoordValid(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	TArray<AGridTile*> GetNeighbors(AGridTile* Tile) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<AGridTile*> FindReachableTiles(AGridTile* StartTile, int32 MoveRange) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<AGridTile*> BuildPath(AGridTile* StartTile, AGridTile* EndTile) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	TArray<AGridTile*> GetTilesWithinRadius(AGridTile* CenterTile, int32 Radius) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ClearHighlights();

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void HighlightReachableTilesFrom(AGridTile* StartTile, int32 MoveRange);

	UFUNCTION(BlueprintPure, Category = "Grid")
	AGridTile* GetPlayerSpawnTile() const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	AGridTile* GetEnemySpawnTile() const;

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
	FIntPoint PlayerSpawnCoord = FIntPoint(1, 1);

	UPROPERTY(EditAnywhere, Category = "Grid")
	FIntPoint EnemySpawnCoord = FIntPoint(6, 4);

	UPROPERTY(EditAnywhere, Category = "Grid")
	TArray<FIntPoint> BlockedCoords;

	UPROPERTY(VisibleInstanceOnly, Category = "Grid")
	TMap<FIntPoint, TObjectPtr<AGridTile>> TileMap;

	FVector GetTileWorldLocation(const FIntPoint& Coord) const;
	bool IsCoordBlocked(const FIntPoint& Coord) const;
	void DestroyExistingTiles();
};
