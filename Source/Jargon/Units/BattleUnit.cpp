// BattleUnit.cpp

#include "Units/BattleUnit.h"

#include "Combat/JargonCombatGameMode.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Grid/GridTile.h"

ABattleUnit::ABattleUnit()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	UnitMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("UnitMesh"));
	UnitMesh->SetupAttachment(SceneRoot);
	UnitMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	UnitMesh->SetCollisionObjectType(ECC_Pawn);
	UnitMesh->SetCollisionResponseToAllChannels(ECR_Block);

	MaxHP = 5;
	CurrentHP = MaxHP;
	MoveRange = 3;
	AttackRange = 1;
	AttackDamage = 1;
	Team = ETeam::Enemy;
	CurrentTile = nullptr;
	bIsDead = false;
}

void ABattleUnit::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = FMath::Clamp(CurrentHP, 0, MaxHP);

	if (CurrentHP <= 0)
	{
		CurrentHP = MaxHP;
	}
}

void ABattleUnit::PlaceOnTile(AGridTile* Tile)
{
	if (!Tile || bIsDead)
	{
		return;
	}

	if (CurrentTile == Tile)
	{
		SetActorLocation(Tile->GetUnitStandLocation());
		return;
	}

	if (Tile->IsOccupied() && Tile->GetOccupyingUnit() != this)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleUnit '%s' cannot be placed on occupied tile."), *GetName());
		return;
	}

	if (Tile->IsBlocked())
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleUnit '%s' cannot be placed on blocked tile."), *GetName());
		return;
	}

	ClearCurrentTileOccupancy();
	SetCurrentTile(Tile);
	SetActorLocation(Tile->GetUnitStandLocation());
}

void ABattleUnit::ClearCurrentTileOccupancy()
{
	if (CurrentTile && CurrentTile->GetOccupyingUnit() == this)
	{
		CurrentTile->SetOccupyingUnit(nullptr);
	}
}

void ABattleUnit::SetCurrentTile(AGridTile* Tile)
{
	CurrentTile = Tile;

	if (CurrentTile)
	{
		CurrentTile->SetOccupyingUnit(this);
	}
}

void ABattleUnit::MoveAlongPath(const TArray<AGridTile*>& Path)
{
	if (bIsDead || Path.Num() == 0)
	{
		return;
	}

	AGridTile* DestinationTile = Path.Last();
	if (!DestinationTile)
	{
		return;
	}

	PlaceOnTile(DestinationTile);
}

void ABattleUnit::ApplyDamage(int32 Amount)
{
	if (bIsDead || Amount <= 0)
	{
		return;
	}

	CurrentHP = FMath::Max(0, CurrentHP - Amount);

	if (CurrentHP > 0)
	{
		return;
	}

	bIsDead = true;
	ClearCurrentTileOccupancy();
	CurrentTile = nullptr;

	AJargonCombatGameMode* CombatGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AJargonCombatGameMode>() : nullptr;
	if (CombatGameMode)
	{
		CombatGameMode->HandleUnitDied(this);
	}

	Destroy();
}

bool ABattleUnit::CanAttackTarget(const ABattleUnit* Target) const
{
	if (!Target || bIsDead || Target->IsDead())
	{
		return false;
	}

	if (Target == this)
	{
		return false;
	}

	if (Target->GetTeam() == Team)
	{
		return false;
	}

	if (!CurrentTile || !Target->GetCurrentTile())
	{
		return false;
	}

	const FIntPoint MyCoord = CurrentTile->GetCoord();
	const FIntPoint TargetCoord = Target->GetCurrentTile()->GetCoord();

	const int32 ManhattanDistance =
		FMath::Abs(MyCoord.X - TargetCoord.X) +
		FMath::Abs(MyCoord.Y - TargetCoord.Y);

	return ManhattanDistance <= AttackRange;
}

bool ABattleUnit::PerformBasicAttack(ABattleUnit* Target)
{
	if (!CanAttackTarget(Target))
	{
		return false;
	}

	Target->ApplyDamage(AttackDamage);
	return true;
}