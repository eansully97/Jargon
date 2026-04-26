#include "BattleTileEffect.h"

#include "Combat/JargonCombatGameMode.h"
#include "Components/SceneComponent.h"
#include "Combat/Grid/GridBoard.h"
#include "Combat/Grid/GridTile.h"
#include "Combat/Units/BattleUnit.h"

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

TArray<ABattleUnit*> ABattleTileEffect::GetLivingUnitsInEffectRadius(const AJargonCombatGameMode* CombatGameMode) const
{
	TArray<ABattleUnit*> UnitsInRadius;

	const TArray<AGridTile*> TilesInRadius = GetTilesInEffectRadius(CombatGameMode);
	for (AGridTile* Tile : TilesInRadius)
	{
		if (!Tile)
		{
			continue;
		}

		ABattleUnit* OccupyingUnit = Tile->GetOccupyingUnit();
		if (!OccupyingUnit || OccupyingUnit->IsDead())
		{
			continue;
		}

		UnitsInRadius.AddUnique(OccupyingUnit);
	}

	return UnitsInRadius;
}

bool ABattleTileEffect::DoesUnitPassTargetFilter(const ABattleUnit* Unit, EJargonTileEffectTargetFilter TargetFilter) const
{
	if (!Unit || Unit->IsDead())
	{
		return false;
	}

	switch (TargetFilter)
	{
	case EJargonTileEffectTargetFilter::FriendlyToSource:
		return Unit->GetTeam() == SourceTeam;

	case EJargonTileEffectTargetFilter::EnemyToSource:
		return Unit->GetTeam() != SourceTeam;

	case EJargonTileEffectTargetFilter::Any:
		return true;

	default:
		return false;
	}
}

int32 ABattleTileEffect::ResolveEffectAmount(int32 DefaultEffectValue) const
{
	return EffectValue > 0
		? EffectValue
		: FMath::Max(0, DefaultEffectValue);
}

bool ABattleTileEffect::ApplyConfiguredOperationToUnit(
	ABattleUnit* TargetUnit,
	EJargonTileEffectOperation Operation,
	int32 Amount) const
{
	if (!TargetUnit || TargetUnit->IsDead() || Amount <= 0)
	{
		return false;
	}

	switch (Operation)
	{
	case EJargonTileEffectOperation::DealDamage:
		TargetUnit->ApplyDamage(Amount);
		return true;

	case EJargonTileEffectOperation::Heal:
		TargetUnit->ApplyHeal(Amount);
		return true;

	case EJargonTileEffectOperation::ApplyShield:
		TargetUnit->AddTemporaryShield(Amount);
		return true;

	case EJargonTileEffectOperation::ApplyStun:
		TargetUnit->ApplyStun(Amount);
		return true;

	case EJargonTileEffectOperation::None:
	default:
		return false;
	}
}