#include "BattleTileEffect.h"

#include "Combat/JargonCombatGameMode.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Combat/Grid/GridBoard.h"
#include "Combat/Grid/GridTile.h"
#include "Combat/Units/BattleUnit.h"
#include "Engine/World.h"

ABattleTileEffect::ABattleTileEffect()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EffectMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EffectMesh"));
	EffectMesh->SetupAttachment(SceneRoot);
	EffectMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EffectMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	EffectMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	EffectMesh->SetGenerateOverlapEvents(false);

	CurrentTile = nullptr;
	SourceCard = nullptr;
	SourceTeam = ETeam::Player;
	CardCategory = ECardCategory::Spell;
	EffectRadius = 0;
	EffectValue = 0;
	RemainingDuration = 0;
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
	int32 InEffectRadius,
	int32 InDuration)
{
	SourceCard = InSourceCard;
	SourceTeam = InSourceTeam;
	CardCategory = InCardCategory;
	EffectValue = FMath::Max(0, InEffectValue);
	EffectRadius = FMath::Max(0, InEffectRadius);
	RemainingDuration = FMath::Max(0, InDuration);

	BP_OnInitializedFromCard();
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
		BP_OnPlacedOnTile(Tile);
		return;
	}

	if (CurrentTile)
	{
		CurrentTile->RemoveTileEffect(this);
	}

	CurrentTile = Tile;
	CurrentTile->AddTileEffect(this);
	SetActorLocation(Tile->GetActorLocation() + FVector(0.f, 0.f, TileEffectZOffset));

	BP_OnPlacedOnTile(Tile);
}

void ABattleTileEffect::HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode)
{
	BP_OnPlayerTurnStart(CombatGameMode);
}

void ABattleTileEffect::HandleUnitEnteredTile(AJargonCombatGameMode* CombatGameMode, ABattleUnit* EnteringUnit)
{
}

FJargonEffectContext ABattleTileEffect::BuildEffectContext(AJargonCombatGameMode* CombatGameMode, ABattleUnit* TriggeringUnit) const
{
	FJargonEffectContext Context;
	Context.GameMode = CombatGameMode;
	Context.SourceObject = const_cast<ABattleTileEffect*>(this);
	Context.SourceUnit = nullptr;
	Context.SourceTeam = SourceTeam;
	Context.SourceTile = CurrentTile;
	Context.PrimaryUnitTarget = TriggeringUnit;
	Context.PrimaryTileTarget = CurrentTile;
	Context.TriggeringUnit = TriggeringUnit;
	Context.OwningTileEffect = const_cast<ABattleTileEffect*>(this);
	Context.SourceCard = SourceCard;
	return Context;
}

void ABattleTileEffect::SetRemainingDuration(int32 NewDuration)
{
	RemainingDuration = FMath::Max(0, NewDuration);
}

void ABattleTileEffect::ConsumeDurationTick()
{
	if (RemainingDuration <= 0)
	{
		// 0 means infinite.
		return;
	}

	RemainingDuration = FMath::Max(0, RemainingDuration - 1);

	if (RemainingDuration <= 0)
	{
		Destroy();
	}
}

ABattleTileEffect* ABattleTileEffect::SpawnTileEffectOnTile(
	TSubclassOf<ABattleTileEffect> TileEffectClass,
	AGridTile* TargetTile,
	int32 InEffectValue,
	int32 InEffectRadius,
	int32 InDuration)
{
	if (!TileEffectClass || !TargetTile)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (!TargetTile->IsWalkable())
	{
		return nullptr;
	}

	ABattleTileEffect* SpawnedEffect = World->SpawnActor<ABattleTileEffect>(
		TileEffectClass,
		TargetTile->GetActorLocation(),
		FRotator::ZeroRotator
	);

	if (!SpawnedEffect)
	{
		return nullptr;
	}

	SpawnedEffect->InitializeFromCard(
		SourceCard,
		SourceTeam,
		CardCategory,
		InEffectValue,
		InEffectRadius,
		InDuration);

	SpawnedEffect->PlaceOnTile(TargetTile);

	return SpawnedEffect;
}

ABattleTileEffect* ABattleTileEffect::SpawnCopyOnTile(AGridTile* TargetTile)
{
	return SpawnTileEffectOnTile(
		GetClass(),
		TargetTile,
		EffectValue,
		EffectRadius,
		RemainingDuration);
}

TArray<AGridTile*> ABattleTileEffect::GetCandidateTilesInRadius(AJargonCombatGameMode* CombatGameMode, int32 Radius) const
{
	TArray<AGridTile*> CandidateTiles;

	if (!CurrentTile)
	{
		return CandidateTiles;
	}

	if (!CombatGameMode)
	{
		CandidateTiles.Add(CurrentTile);
		return CandidateTiles;
	}

	AGridBoard* GridBoard = CombatGameMode->GetGridBoard();
	if (!GridBoard)
	{
		CandidateTiles.Add(CurrentTile);
		return CandidateTiles;
	}

	return GridBoard->GetTilesWithinRadius(CurrentTile, FMath::Max(0, Radius));
}

TArray<AGridTile*> ABattleTileEffect::GetEmptyWalkableTilesInRadius(AJargonCombatGameMode* CombatGameMode, int32 Radius) const
{
	TArray<AGridTile*> EmptyWalkableTiles;

	const TArray<AGridTile*> CandidateTiles = GetCandidateTilesInRadius(CombatGameMode, Radius);
	for (AGridTile* Tile : CandidateTiles)
	{
		if (!Tile || Tile == CurrentTile)
		{
			continue;
		}

		if (!Tile->IsWalkable())
		{
			continue;
		}

		if (Tile->GetTileEffects().Num() > 0)
		{
			continue;
		}

		EmptyWalkableTiles.Add(Tile);
	}

	return EmptyWalkableTiles;
}

TArray<AGridTile*> ABattleTileEffect::GetTilesInEffectRadius(const AJargonCombatGameMode* CombatGameMode) const
{
	TArray<AGridTile*> TilesInRadius;

	if (!CurrentTile)
	{
		return TilesInRadius;
	}

	if (EffectRadius <= 0)
	{
		TilesInRadius.Add(CurrentTile);
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

	case EJargonTileEffectOperation::IncreaseAttack:
		TargetUnit->IncreaseAttack(Amount);
		return true;

	case EJargonTileEffectOperation::IncreaseMaxHealth:
		TargetUnit->IncreaseMaxHealth(Amount);
		return true;

	case EJargonTileEffectOperation::None:
	default:
		return false;
	}
}

void ABattleTileEffect::ShowAffectedTiles()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AJargonCombatGameMode* CombatGameMode = World->GetAuthGameMode<AJargonCombatGameMode>();
	if (!CombatGameMode)
	{
		return;
	}

	AGridBoard* GridBoard = CombatGameMode->GetGridBoard();
	if (!GridBoard || !CurrentTile)
	{
		return;
	}

	GridBoard->ClearHighlights();

	const TArray<AGridTile*> TilesInRadius = GetTilesInEffectRadius(CombatGameMode);
	for (AGridTile* Tile : TilesInRadius)
	{
		if (Tile)
		{
			Tile->SetHighlightState(ETileHighlightState::Reachable);
		}
	}

	CurrentTile->SetHighlightState(ETileHighlightState::Selected);
}
