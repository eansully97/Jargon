// GridTile.cpp

#include "Grid/GridTile.h"


#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Grid/BattleTileEffect.h"
#include "Grid/GridBoard.h"
#include "Materials/MaterialInterface.h"

AGridTile::AGridTile()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	TileMesh->SetupAttachment(SceneRoot);
	TileMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TileMesh->SetCollisionObjectType(ECC_WorldStatic);
	TileMesh->SetCollisionResponseToAllChannels(ECR_Block);
	TileMesh->SetMobility(EComponentMobility::Movable);

	bBlocked = false;
	HighlightState = ETileHighlightState::None;
	OccupyingUnit = nullptr;
	UnitStandZOffset = 100.f;
}

void AGridTile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshVisualState();
}

void AGridTile::BeginPlay()
{
	Super::BeginPlay();

	RefreshVisualState();
}

void AGridTile::SetCoord(const FHexCoord& InCoord)
{
	Coord = InCoord;
}

void AGridTile::SetBlocked(bool bInBlocked)
{
	bBlocked = bInBlocked;

	if (bBlocked)
	{
		HighlightState = ETileHighlightState::Blocked;
	}
	else if (HighlightState == ETileHighlightState::Blocked)
	{
		HighlightState = ETileHighlightState::None;
	}

	RefreshVisualState();
}

bool AGridTile::IsOccupied() const
{
	return OccupyingUnit != nullptr;
}

bool AGridTile::IsWalkable() const
{
	return !bBlocked && !IsOccupied();
}

void AGridTile::SetOccupyingUnit(ABattleUnit* NewUnit)
{
	OccupyingUnit = NewUnit;

	if (bBlocked)
	{
		HighlightState = ETileHighlightState::Blocked;
	}
	else if (OccupyingUnit)
	{
		HighlightState = ETileHighlightState::Occupied;
	}
	else if (HighlightState == ETileHighlightState::Occupied)
	{
		HighlightState = ETileHighlightState::None;
	}

	RefreshVisualState();
}

void AGridTile::SetHighlightState(ETileHighlightState NewState)
{
	HighlightState = NewState;
	RefreshVisualState();
}

FVector AGridTile::GetUnitStandLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, UnitStandZOffset);
}

AGridBoard* AGridTile::GetOwningGridBoard() const
{
	return Cast<AGridBoard>(GetOwner());
}

void AGridTile::AddTileEffect(ABattleTileEffect* TileEffect)
{
	if (!TileEffect)
	{
		return;
	}

	TileEffects.AddUnique(TileEffect);
}

void AGridTile::RemoveTileEffect(ABattleTileEffect* TileEffect)
{
	if (!TileEffect)
	{
		return;
	}

	TileEffects.RemoveSingleSwap(TileEffect);
}

void AGridTile::RefreshVisualState()
{
	if (!TileMesh)
	{
		return;
	}

	UMaterialInterface* MaterialToApply = DefaultMaterial;

	if (bBlocked && BlockedMaterial)
	{
		MaterialToApply = BlockedMaterial;
	}
	else
	{
		switch (HighlightState)
		{
		case ETileHighlightState::Reachable:
			if (ReachableMaterial)
			{
				MaterialToApply = ReachableMaterial;
			}
			break;

		case ETileHighlightState::Blocked:
			if (BlockedMaterial)
			{
				MaterialToApply = BlockedMaterial;
			}
			break;

		case ETileHighlightState::Occupied:
			if (OccupiedMaterial)
			{
				MaterialToApply = OccupiedMaterial;
			}
			break;

		case ETileHighlightState::Selected:
			if (SelectedMaterial)
			{
				MaterialToApply = SelectedMaterial;
			}
			break;

		case ETileHighlightState::None:
		default:
			break;
		}
	}

	if (MaterialToApply)
	{
		TileMesh->SetMaterial(0, MaterialToApply);
	}
}
