// BattleTileEffect.cpp

#include "Grid/BattleTileEffect.h"

#include "Combat/JargonCombatGameMode.h"
#include "Components/SceneComponent.h"
#include "Data/CardDefinition.h"
#include "Grid/GridBoard.h"
#include "Grid/GridTile.h"

ABattleTileEffect::ABattleTileEffect()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CurrentTile = nullptr;
	SourceCard = nullptr;
	SourceTeam = ETeam::Player;
	CardCategory = ECardCategory::Spell;
	EffectRadius = 0;
	EffectValue = 0;
	TileEffectZOffset = 15.f;
}

void ABattleTileEffect::Destroyed()
{
	if (CurrentTile)
	{
		CurrentTile->RemoveTileEffect(this);
		CurrentTile = nullptr;
	}

	Super::Destroyed();
}

void ABattleTileEffect::InitializeFromCard(
	UCardDefinition* InSourceCard,
	ETeam InSourceTeam,
	ECardCategory InCardCategory,
	int32 InEffectValue,
	int32 InEffectRadius)
{
	SourceCard = InSourceCard;
	SourceTeam = InSourceTeam;
	CardCategory = InCardCategory;
	EffectValue = FMath::Max(0, InEffectValue);
	EffectRadius = FMath::Max(0, InEffectRadius);
}

void ABattleTileEffect::PlaceOnTile(AGridTile* Tile)
{
	if (!Tile)
	{
		return;
	}

	if (CurrentTile == Tile)
	{
		SetActorLocation(Tile->GetActorLocation() + FVector(0.f, 0.f, TileEffectZOffset));
		return;
	}

	if (CurrentTile)
	{
		CurrentTile->RemoveTileEffect(this);
	}

	CurrentTile = Tile;
	CurrentTile->AddTileEffect(this);
	SetActorLocation(Tile->GetActorLocation() + FVector(0.f, 0.f, TileEffectZOffset));
}

void ABattleTileEffect::HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode)
{
}

void ABattleTileEffect::HandleUnitEnteredTile(AJargonCombatGameMode* CombatGameMode, ABattleUnit* EnteringUnit)
{
}

TArray<AGridTile*> ABattleTileEffect::GetTilesInEffectRadius(const AJargonCombatGameMode* CombatGameMode) const
{
	TArray<AGridTile*> TilesInRadius;

	if (!CurrentTile)
	{
		return TilesInRadius;
	}

	if (!CombatGameMode)
	{
		TilesInRadius.Add(CurrentTile);
		return TilesInRadius;
	}

	AGridBoard* GridBoard = CombatGameMode->GetGridBoard();
	if (!GridBoard)
	{
		TilesInRadius.Add(CurrentTile);
		return TilesInRadius;
	}

	return GridBoard->GetTilesWithinRadius(CurrentTile, EffectRadius);
}
