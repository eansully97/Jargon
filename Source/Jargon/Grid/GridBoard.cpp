// GridBoard.cpp

#include "Grid/GridBoard.h"

#include "Containers/Queue.h"
#include "Components/SceneComponent.h"
#include "Grid/GridTile.h"

AGridBoard::AGridBoard()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Width = 8;
	Height = 6;
	TileSize = 200.f;
	PlayerSpawnCoord = FIntPoint(1, 1);
	EnemySpawnCoord = FIntPoint(6, 4);
}

void AGridBoard::BeginPlay()
{
	Super::BeginPlay();

	GenerateBoard();
}

void AGridBoard::GenerateBoard()
{
	DestroyExistingTiles();
	TileMap.Empty();

	if (!TileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridBoard '%s' has no TileClass assigned."), *GetName());
		return;
	}

	if (Width <= 0 || Height <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridBoard '%s' has invalid dimensions Width=%d Height=%d."), *GetName(), Width, Height);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 X = 0; X < Width; ++X)
	{
		for (int32 Y = 0; Y < Height; ++Y)
		{
			const FIntPoint Coord(X, Y);
			const FVector SpawnLocation = GetTileWorldLocation(Coord);
			const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

			AGridTile* SpawnedTile = World->SpawnActorDeferred<AGridTile>(
				TileClass,
				SpawnTransform,
				this,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn
			);

			if (!SpawnedTile)
			{
				continue;
			}

			SpawnedTile->SetCoord(Coord);
			SpawnedTile->SetBlocked(IsCoordBlocked(Coord));

			SpawnedTile->FinishSpawning(SpawnTransform);
			TileMap.Add(Coord, SpawnedTile);
		}
	}

	ClearHighlights();
}

AGridTile* AGridBoard::GetTile(const FIntPoint& Coord) const
{
	if (const TObjectPtr<AGridTile>* FoundTile = TileMap.Find(Coord))
	{
		return FoundTile->Get();
	}

	return nullptr;
}

bool AGridBoard::IsCoordValid(const FIntPoint& Coord) const
{
	return Coord.X >= 0 && Coord.X < Width && Coord.Y >= 0 && Coord.Y < Height;
}

TArray<AGridTile*> AGridBoard::GetNeighbors(AGridTile* Tile) const
{
	TArray<AGridTile*> Neighbors;

	if (!Tile)
	{
		return Neighbors;
	}

	const FIntPoint Coord = Tile->GetCoord();

	const TArray<FIntPoint> NeighborCoords =
	{
		FIntPoint(Coord.X + 1, Coord.Y),
		FIntPoint(Coord.X - 1, Coord.Y),
		FIntPoint(Coord.X, Coord.Y + 1),
		FIntPoint(Coord.X, Coord.Y - 1)
	};

	for (const FIntPoint& NeighborCoord : NeighborCoords)
	{
		if (!IsCoordValid(NeighborCoord))
		{
			continue;
		}

		if (AGridTile* NeighborTile = GetTile(NeighborCoord))
		{
			Neighbors.Add(NeighborTile);
		}
	}

	return Neighbors;
}

TArray<AGridTile*> AGridBoard::FindReachableTiles(AGridTile* StartTile, int32 MoveRange) const
{
	TArray<AGridTile*> ReachableTiles;

	if (!StartTile || MoveRange < 0)
	{
		return ReachableTiles;
	}

	TQueue<AGridTile*> Frontier;
	TMap<AGridTile*, int32> DistanceMap;

	Frontier.Enqueue(StartTile);
	DistanceMap.Add(StartTile, 0);

	while (!Frontier.IsEmpty())
	{
		AGridTile* CurrentTile = nullptr;
		Frontier.Dequeue(CurrentTile);

		if (!CurrentTile)
		{
			continue;
		}

		const int32* CurrentDistancePtr = DistanceMap.Find(CurrentTile);
		if (!CurrentDistancePtr)
		{
			continue;
		}

		const int32 CurrentDistance = *CurrentDistancePtr;

		if (CurrentDistance > 0)
		{
			ReachableTiles.Add(CurrentTile);
		}

		if (CurrentDistance >= MoveRange)
		{
			continue;
		}

		for (AGridTile* Neighbor : GetNeighbors(CurrentTile))
		{
			if (!Neighbor)
			{
				continue;
			}

			if (DistanceMap.Contains(Neighbor))
			{
				continue;
			}

			if (!Neighbor->IsWalkable())
			{
				continue;
			}

			DistanceMap.Add(Neighbor, CurrentDistance + 1);
			Frontier.Enqueue(Neighbor);
		}
	}

	return ReachableTiles;
}

TArray<AGridTile*> AGridBoard::BuildPath(AGridTile* StartTile, AGridTile* EndTile) const
{
	TArray<AGridTile*> Path;

	if (!StartTile || !EndTile)
	{
		return Path;
	}

	if (StartTile == EndTile)
	{
		Path.Add(StartTile);
		return Path;
	}

	if (!EndTile->IsWalkable())
	{
		return Path;
	}

	TQueue<AGridTile*> Frontier;
	TSet<AGridTile*> Visited;
	TMap<AGridTile*, AGridTile*> ParentMap;

	Frontier.Enqueue(StartTile);
	Visited.Add(StartTile);

	bool bFoundPath = false;

	while (!Frontier.IsEmpty())
	{
		AGridTile* CurrentTile = nullptr;
		Frontier.Dequeue(CurrentTile);

		if (!CurrentTile)
		{
			continue;
		}

		if (CurrentTile == EndTile)
		{
			bFoundPath = true;
			break;
		}

		for (AGridTile* Neighbor : GetNeighbors(CurrentTile))
		{
			if (!Neighbor)
			{
				continue;
			}

			if (Visited.Contains(Neighbor))
			{
				continue;
			}

			if (Neighbor != EndTile && !Neighbor->IsWalkable())
			{
				continue;
			}

			Visited.Add(Neighbor);
			ParentMap.Add(Neighbor, CurrentTile);
			Frontier.Enqueue(Neighbor);
		}
	}

	if (!bFoundPath)
	{
		return Path;
	}

	AGridTile* CurrentStep = EndTile;
	while (CurrentStep)
	{
		Path.Insert(CurrentStep, 0);

		if (CurrentStep == StartTile)
		{
			break;
		}

		AGridTile* const* ParentPtr = ParentMap.Find(CurrentStep);
		CurrentStep = ParentPtr ? *ParentPtr : nullptr;
	}

	if (Path.Num() == 0 || Path[0] != StartTile)
	{
		Path.Empty();
	}

	return Path;
}

TArray<AGridTile*> AGridBoard::GetTilesWithinRadius(AGridTile* CenterTile, int32 Radius) const
{
	TArray<AGridTile*> TilesInRadius;

	if (!CenterTile || Radius < 0)
	{
		return TilesInRadius;
	}

	const FIntPoint CenterCoord = CenterTile->GetCoord();

	for (int32 X = CenterCoord.X - Radius; X <= CenterCoord.X + Radius; ++X)
	{
		for (int32 Y = CenterCoord.Y - Radius; Y <= CenterCoord.Y + Radius; ++Y)
		{
			const FIntPoint CandidateCoord(X, Y);
			if (!IsCoordValid(CandidateCoord))
			{
				continue;
			}

			const int32 ManhattanDistance =
				FMath::Abs(CandidateCoord.X - CenterCoord.X) +
				FMath::Abs(CandidateCoord.Y - CenterCoord.Y);

			if (ManhattanDistance > Radius)
			{
				continue;
			}

			if (AGridTile* CandidateTile = GetTile(CandidateCoord))
			{
				TilesInRadius.Add(CandidateTile);
			}
		}
	}

	return TilesInRadius;
}

void AGridBoard::ClearHighlights()
{
	for (TPair<FIntPoint, TObjectPtr<AGridTile>>& Pair : TileMap)
	{
		AGridTile* Tile = Pair.Value.Get();
		if (!Tile)
		{
			continue;
		}

		if (Tile->IsBlocked())
		{
			Tile->SetHighlightState(ETileHighlightState::Blocked);
		}
		else if (Tile->IsOccupied())
		{
			Tile->SetHighlightState(ETileHighlightState::Occupied);
		}
		else
		{
			Tile->SetHighlightState(ETileHighlightState::None);
		}
	}
}

void AGridBoard::HighlightReachableTilesFrom(AGridTile* StartTile, int32 MoveRange)
{
	ClearHighlights();

	if (!StartTile)
	{
		return;
	}

	const TArray<AGridTile*> ReachableTiles = FindReachableTiles(StartTile, MoveRange);
	for (AGridTile* Tile : ReachableTiles)
	{
		if (Tile)
		{
			Tile->SetHighlightState(ETileHighlightState::Reachable);
		}
	}

	StartTile->SetHighlightState(ETileHighlightState::Selected);
}

AGridTile* AGridBoard::GetPlayerSpawnTile() const
{
	return GetTile(PlayerSpawnCoord);
}

AGridTile* AGridBoard::GetEnemySpawnTile() const
{
	return GetTile(EnemySpawnCoord);
}

FVector AGridBoard::GetTileWorldLocation(const FIntPoint& Coord) const
{
	const FVector BoardOrigin = GetActorLocation();
	return BoardOrigin + FVector(Coord.X * TileSize, Coord.Y * TileSize, 0.f);
}

bool AGridBoard::IsCoordBlocked(const FIntPoint& Coord) const
{
	return BlockedCoords.Contains(Coord);
}

void AGridBoard::DestroyExistingTiles()
{
	for (TPair<FIntPoint, TObjectPtr<AGridTile>>& Pair : TileMap)
	{
		if (AGridTile* Tile = Pair.Value.Get())
		{
			Tile->Destroy();
		}
	}
}
