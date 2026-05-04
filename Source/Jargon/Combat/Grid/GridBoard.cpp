// GridBoard.cpp

#include "Combat/Grid/GridBoard.h"

#include "Containers/Queue.h"
#include "Components/SceneComponent.h"
#include "Combat/Grid/GridTile.h"

namespace
{
	const TArray<FHexCoord>& GetHexNeighborOffsets()
	{
		static const TArray<FHexCoord> NeighborOffsets =
		{
			FHexCoord(1, 0),
			FHexCoord(1, -1),
			FHexCoord(0, -1),
			FHexCoord(-1, 0),
			FHexCoord(-1, 1),
			FHexCoord(0, 1)
		};

		return NeighborOffsets;
	}

	FIntVector AxialToCube(const FHexCoord& Coord)
	{
		return FIntVector(Coord.Q, Coord.GetS(), Coord.R);
	}

	int32 CubeDot(const FIntVector& A, const FIntVector& B)
	{
		return (A.X * B.X) + (A.Y * B.Y) + (A.Z * B.Z);
	}
}

AGridBoard::AGridBoard()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Width = 8;
	Height = 6;
	TileSize = 200.f;
	PlayerSpawnCoord = FHexCoord(1, 1);
	EnemySpawnCoord = FHexCoord(6, 4);
	RandomBlockedTileMin = 3;
	RandomBlockedTileMax = 6;
	bProtectSpawnNeighborsFromRandomBlocking = true;
}

void AGridBoard::BeginPlay()
{
	Super::BeginPlay();

	GenerateBoard();
}

void AGridBoard::RebuildGridInEditor()
{
	RebuildGrid();
}

void AGridBoard::RebuildGrid()
{
	ClearSpawnedTiles();
	GenerateBoard();
}

void AGridBoard::ClearSpawnedTiles()
{
	for (TPair<FHexCoord, TObjectPtr<AGridTile>>& Pair : TileMap)
	{
		if (AGridTile* Tile = Pair.Value.Get())
		{
			Tile->Destroy();
		}
	}
	TileMap.Empty();
}

void AGridBoard::GenerateBoard()
{
	DestroyExistingTiles();
	TileMap.Empty();
	BuildResolvedBlockedCoords();

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

	for (int32 Q = 0; Q < Width; ++Q)
	{
		for (int32 R = 0; R < Height; ++R)
		{
			const FHexCoord Coord(Q, R);
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

AGridTile* AGridBoard::GetTile(const FHexCoord& Coord) const
{
	if (const TObjectPtr<AGridTile>* FoundTile = TileMap.Find(Coord))
	{
		return FoundTile->Get();
	}

	return nullptr;
}

bool AGridBoard::IsCoordValid(const FHexCoord& Coord) const
{
	return Coord.Q >= 0 && Coord.Q < Width && Coord.R >= 0 && Coord.R < Height;
}

TArray<AGridTile*> AGridBoard::GetNeighbors(AGridTile* Tile) const
{
	TArray<AGridTile*> Neighbors;

	if (!Tile)
	{
		return Neighbors;
	}

	const FHexCoord Coord = Tile->GetCoord();

	for (const FHexCoord& NeighborOffset : GetHexNeighborOffsets())
	{
		const FHexCoord NeighborCoord = Coord + NeighborOffset;
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

int32 AGridBoard::GetTileDistance(const AGridTile* TileA, const AGridTile* TileB) const
{
	if (!TileA || !TileB)
	{
		return MAX_int32;
	}

	return GetCoordDistance(TileA->GetCoord(), TileB->GetCoord());
}

int32 AGridBoard::GetCoordDistance(const FHexCoord& CoordA, const FHexCoord& CoordB) const
{
	const FHexCoord Delta = CoordA - CoordB;
	return (FMath::Abs(Delta.Q) + FMath::Abs(Delta.R) + FMath::Abs(Delta.GetS())) / 2;
}

bool AGridBoard::AreTilesWithinRange(const AGridTile* TileA, const AGridTile* TileB, int32 Range) const
{
	if (Range < 0)
	{
		return false;
	}

	return GetTileDistance(TileA, TileB) <= Range;
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
	return BuildPathInternal(StartTile, EndTile, false);
}

TArray<AGridTile*> AGridBoard::BuildPathAllowingOccupiedEndTile(AGridTile* StartTile, AGridTile* EndTile) const
{
	return BuildPathInternal(StartTile, EndTile, true);
}

TArray<AGridTile*> AGridBoard::BuildPathInternal(AGridTile* StartTile, AGridTile* EndTile, bool bAllowOccupiedEndTile) const
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

	if (EndTile->IsBlocked() || (!bAllowOccupiedEndTile && EndTile->IsOccupied()))
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

	for (const TPair<FHexCoord, TObjectPtr<AGridTile>>& Pair : TileMap)
	{
		AGridTile* CandidateTile = Pair.Value.Get();
		if (!CandidateTile)
		{
			continue;
		}

		if (GetTileDistance(CenterTile, CandidateTile) <= Radius)
		{
			TilesInRadius.Add(CandidateTile);
		}
	}

	return TilesInRadius;
}

AGridTile* AGridBoard::GetTileInPushDirection(
	const AGridTile* SourceTile,
	const AGridTile* TargetTile,
	int32 StepDistance) const
{
	if (StepDistance <= 0 || !TargetTile)
	{
		return nullptr;
	}

	const FHexCoord DirectionOffset = GetPushDirectionOffset(SourceTile, TargetTile);
	if (DirectionOffset.IsZero())
	{
		return nullptr;
	}

	const FHexCoord CandidateCoord = TargetTile->GetCoord() + (DirectionOffset * StepDistance);
	return IsCoordValid(CandidateCoord) ? GetTile(CandidateCoord) : nullptr;
}

void AGridBoard::ClearHighlights()
{
	for (TPair<FHexCoord, TObjectPtr<AGridTile>>& Pair : TileMap)
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

void AGridBoard::HighlightTilesInRangeFrom(AGridTile* StartTile, int32 Range)
{
	ClearHighlights();

	if (!StartTile)
	{
		return;
	}

	const TArray<AGridTile*> TilesInRange = GetTilesWithinRadius(StartTile, Range);
	for (AGridTile* Tile : TilesInRange)
	{
		if (!Tile)
		{
			continue;
		}

		Tile->SetHighlightState(ETileHighlightState::Reachable);
	}

	StartTile->SetHighlightState(ETileHighlightState::Selected);
}

AGridTile* AGridBoard::GetPlayerSpawnTile() const
{
	return FindNearestWalkableTile(PlayerSpawnCoord);
}

AGridTile* AGridBoard::GetEnemySpawnTile() const
{
	return FindNearestWalkableTile(EnemySpawnCoord);
}

AGridTile* AGridBoard::FindNearestWalkableTile(const FHexCoord& PreferredCoord) const
{
	AGridTile* BestTile = nullptr;
	int32 BestDistance = MAX_int32;
	FHexCoord BestCoord;
	bool bHasBestCoord = false;

	for (const TPair<FHexCoord, TObjectPtr<AGridTile>>& Pair : TileMap)
	{
		AGridTile* CandidateTile = Pair.Value.Get();
		if (!CandidateTile || !CandidateTile->IsWalkable())
		{
			continue;
		}

		const FHexCoord CandidateCoord = CandidateTile->GetCoord();
		const int32 CandidateDistance = GetCoordDistance(PreferredCoord, CandidateCoord);
		const bool bIsBetterDistance = CandidateDistance < BestDistance;
		const bool bIsSameDistanceButEarlierCoord =
			CandidateDistance == BestDistance &&
			(!bHasBestCoord ||
			 CandidateCoord.Q < BestCoord.Q ||
			 (CandidateCoord.Q == BestCoord.Q && CandidateCoord.R < BestCoord.R));

		if (!BestTile || bIsBetterDistance || bIsSameDistanceButEarlierCoord)
		{
			BestTile = CandidateTile;
			BestDistance = CandidateDistance;
			BestCoord = CandidateCoord;
			bHasBestCoord = true;
		}
	}

	return BestTile;
}

FVector AGridBoard::GetTileWorldLocation(const FHexCoord& Coord) const
{
	constexpr float SqrtThree = 1.7320508075688772f;

	const FVector BoardOrigin = GetActorLocation();
	const float WorldX = TileSize * SqrtThree * (static_cast<float>(Coord.Q) + (static_cast<float>(Coord.R) * 0.5f));
	const float WorldY = TileSize * 1.5f * static_cast<float>(Coord.R);
	return BoardOrigin + FVector(WorldX, WorldY, 0.f);
}

bool AGridBoard::IsCoordBlocked(const FHexCoord& Coord) const
{
	return ResolvedBlockedCoords.Contains(Coord);
}

FHexCoord AGridBoard::GetPushDirectionOffset(const AGridTile* SourceTile, const AGridTile* TargetTile) const
{
	if (!SourceTile || !TargetTile)
	{
		return FHexCoord();
	}

	const FHexCoord Delta = TargetTile->GetCoord() - SourceTile->GetCoord();
	if (Delta.IsZero())
	{
		return FHexCoord();
	}

	const FIntVector DeltaCube = AxialToCube(Delta);

	FHexCoord BestDirection;
	int32 BestScore = MIN_int32;
	bool bFoundDirection = false;

	for (const FHexCoord& Direction : GetHexNeighborOffsets())
	{
		const int32 DirectionScore = CubeDot(DeltaCube, AxialToCube(Direction));
		if (!bFoundDirection || DirectionScore > BestScore)
		{
			BestScore = DirectionScore;
			BestDirection = Direction;
			bFoundDirection = true;
		}
	}

	return bFoundDirection ? BestDirection : FHexCoord();
}

void AGridBoard::DestroyExistingTiles()
{
	for (TPair<FHexCoord, TObjectPtr<AGridTile>>& Pair : TileMap)
	{
		if (AGridTile* Tile = Pair.Value.Get())
		{
			Tile->Destroy();
		}
	}
}

void AGridBoard::BuildResolvedBlockedCoords()
{
	ResolvedBlockedCoords = BlockedCoords;

	const int32 MinimumRandomBlockedTiles = FMath::Max(0, RandomBlockedTileMin);
	const int32 MaximumRandomBlockedTiles = FMath::Max(MinimumRandomBlockedTiles, RandomBlockedTileMax);
	if (MaximumRandomBlockedTiles <= 0)
	{
		return;
	}

	TArray<FHexCoord> CandidateCoords;
	CandidateCoords.Reserve(Width * Height);

	for (int32 Q = 0; Q < Width; ++Q)
	{
		for (int32 R = 0; R < Height; ++R)
		{
			const FHexCoord CandidateCoord(Q, R);
			if (ResolvedBlockedCoords.Contains(CandidateCoord))
			{
				continue;
			}

			if (ShouldProtectCoordFromRandomBlocking(CandidateCoord))
			{
				continue;
			}

			CandidateCoords.Add(CandidateCoord);
		}
	}

	if (CandidateCoords.Num() == 0)
	{
		return;
	}

	for (int32 Index = CandidateCoords.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		if (SwapIndex != Index)
		{
			CandidateCoords.Swap(Index, SwapIndex);
		}
	}

	const int32 RandomBlockedTileCount = FMath::Clamp(
		FMath::RandRange(MinimumRandomBlockedTiles, MaximumRandomBlockedTiles),
		0,
		CandidateCoords.Num());

	for (int32 Index = 0; Index < RandomBlockedTileCount; ++Index)
	{
		ResolvedBlockedCoords.Add(CandidateCoords[Index]);
	}
}

bool AGridBoard::ShouldProtectCoordFromRandomBlocking(const FHexCoord& Coord) const
{
	if (Coord == PlayerSpawnCoord || Coord == EnemySpawnCoord)
	{
		return true;
	}

	if (!bProtectSpawnNeighborsFromRandomBlocking)
	{
		return false;
	}

	return GetCoordDistance(Coord, PlayerSpawnCoord) <= 1 ||
		GetCoordDistance(Coord, EnemySpawnCoord) <= 1;
}
