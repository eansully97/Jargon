// CardResolver.cpp

#include "Combat/CardResolver.h"

#include "Combat/JargonCombatGameMode.h"
#include "Data/CardDefinition.h"
#include "Grid/GridBoard.h"
#include "Grid/GridTile.h"
#include "Units/BattleUnit.h"

bool FCardResolver::ResolveCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult
)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit)
	{
		return false;
	}

	switch (Card->Category)
	{
	case ECardCategory::Spell:
		return ResolveSpellCard(Card, Context, OutResult);

	case ECardCategory::Trap:
	case ECardCategory::Aura:
		return ResolvePersistentTileCard(Card, Context, OutResult);

	case ECardCategory::Summon:
		return ResolveSummonCard(Card, Context, OutResult);

	default:
		UE_LOG(LogTemp, Warning, TEXT("FCardResolver::ResolveCard encountered unsupported card category."));
		return false;
	}
}

bool FCardResolver::ResolveSpellCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card)
	{
		return false;
	}

	switch (Card->EffectType)
	{
	case ECardEffectType::Damage:
		return ResolveDamage(Card, Context, OutResult);

	case ECardEffectType::Heal:
		return ResolveHeal(Card, Context, OutResult);

	case ECardEffectType::AOE_Damage:
		return ResolveAOEDamage(Card, Context, OutResult);

	case ECardEffectType::Push:
		return ResolvePush(Card, Context, OutResult);

	case ECardEffectType::MoveSelf:
		return ResolveMoveSelf(Card, Context, OutResult);

	case ECardEffectType::Guard:
		return ResolveGuard(Card, Context, OutResult);

	case ECardEffectType::None:
		UE_LOG(LogTemp, Warning, TEXT("Spell card '%s' has no immediate EffectType assigned."),
			*Card->DisplayName.ToString());
		return false;

	default:
		UE_LOG(LogTemp, Warning, TEXT("FCardResolver::ResolveCard encountered unsupported card effect."));
		return false;
	}
}

bool FCardResolver::ResolvePersistentTileCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit)
	{
		return false;
	}

	AGridTile* TargetTile = GetResolvedTargetTile(Context);
	if (!TargetTile)
	{
		return false;
	}

	if (!Card->PersistentTileEffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Persistent tile card '%s' has no PersistentTileEffectClass assigned."),
			*Card->DisplayName.ToString());
		return false;
	}

	return Context.GameMode->SpawnPersistentTileEffect(Card, Context.SourceUnit, TargetTile) != nullptr;
}

bool FCardResolver::ResolveSummonCard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit)
	{
		return false;
	}

	AGridTile* TargetTile = GetResolvedTargetTile(Context);
	if (!TargetTile)
	{
		return false;
	}

	if (!Card->SummonedUnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Summon card '%s' has no SummonedUnitClass assigned."),
			*Card->DisplayName.ToString());
		return false;
	}

	return Context.GameMode->SpawnSummonedUnitFromCard(Card, Context.SourceUnit, TargetTile) != nullptr;
}

bool FCardResolver::ResolveDamage(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult
)
{
	if (!Card || !Context.SourceUnit)
	{
		return false;
	}

	ABattleUnit* TargetUnit = GetResolvedTargetUnit(Context);
	if (!TargetUnit || TargetUnit->IsDead())
	{
		return false;
	}

	if (TargetUnit->GetTeam() == Context.SourceUnit->GetTeam())
	{
		return false;
	}

	TargetUnit->ApplyDamage(Card->Value);
	return true;
}

bool FCardResolver::ResolveGuard(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult
)
{
	if (!Card || !Context.SourceUnit)
	{
		return false;
	}

	Context.SourceUnit->AddTemporaryShield(Card->Value);
	return true;
}

bool FCardResolver::ResolveHeal(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult
)
{
	if (!Card || !Context.SourceUnit)
	{
		return false;
	}

	ABattleUnit* ActualTarget = GetResolvedTargetUnit(Context);
	if (!ActualTarget)
	{
		ActualTarget = Context.SourceUnit.Get();
	}

	if (!ActualTarget || ActualTarget->IsDead())
	{
		return false;
	}

	if (ActualTarget->GetTeam() != Context.SourceUnit->GetTeam())
	{
		return false;
	}

	ActualTarget->ApplyHeal(Card->Value);
	return true;
}

bool FCardResolver::ResolveAOEDamage(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult
)
{
	if (!Card || !Context.SourceUnit || !Context.GameMode)
	{
		return false;
	}

	AGridTile* CenterTile = GetResolvedTargetTile(Context);
	if (!CenterTile)
	{
		return false;
	}

	const int32 EffectRadius = (Card->Radius > 0) ? Card->Radius : Card->Range;

	TArray<TWeakObjectPtr<ABattleUnit>> TargetsToDamage;

	const TArray<TObjectPtr<ABattleUnit>>& EnemyUnits = Context.GameMode->GetEnemyUnits();
	TargetsToDamage.Reserve(EnemyUnits.Num());

	for (const TObjectPtr<ABattleUnit>& EnemyUnit : EnemyUnits)
	{
		if (!IsValid(EnemyUnit) || EnemyUnit->IsDead())
		{
			continue;
		}

		if (EnemyUnit->GetTeam() == Context.SourceUnit->GetTeam())
		{
			continue;
		}

		if (!EnemyUnit->GetCurrentTile())
		{
			continue;
		}

		if (GetTileDistance(EnemyUnit->GetCurrentTile(), CenterTile) <= EffectRadius)
		{
			TargetsToDamage.Add(EnemyUnit);
		}
	}

	for (const TWeakObjectPtr<ABattleUnit>& TargetPtr : TargetsToDamage)
	{
		ABattleUnit* TargetUnit = TargetPtr.Get();
		if (!IsValid(TargetUnit) || TargetUnit->IsDead())
		{
			continue;
		}

		TargetUnit->ApplyDamage(Card->Value);

		if (!IsValid(Context.GameMode) ||
			Context.GameMode->GetCurrentCombatPhase() == ECombatPhase::Victory ||
			Context.GameMode->GetCurrentCombatPhase() == ECombatPhase::Defeat)
		{
			break;
		}
	}

	return true;
}

bool FCardResolver::ResolvePush(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult
)
{
	if (!Card || !Context.SourceUnit || !Context.GameMode)
	{
		return false;
	}

	ABattleUnit* TargetUnit = GetResolvedTargetUnit(Context);
	if (!TargetUnit || TargetUnit->IsDead())
	{
		return false;
	}

	if (TargetUnit->GetTeam() == Context.SourceUnit->GetTeam())
	{
		return false;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	AGridTile* SourceTile = Context.SourceUnit->GetCurrentTile();
	AGridTile* TargetTile = GetResolvedTargetTile(Context);

	if (!GridBoard || !SourceTile || !TargetTile)
	{
		return false;
	}

	const FIntPoint SourceCoord = SourceTile->GetCoord();
	const FIntPoint TargetCoord = TargetTile->GetCoord();
	const FIntPoint Delta = TargetCoord - SourceCoord;

	FIntPoint Direction = FIntPoint::ZeroValue;

	if (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
	{
		Direction.X = FMath::Clamp(Delta.X, -1, 1);
	}
	else
	{
		Direction.Y = FMath::Clamp(Delta.Y, -1, 1);
	}

	if (Direction == FIntPoint::ZeroValue)
	{
		return false;
	}

	const int32 PushDistance = FMath::Max(1, Card->Value);

	AGridTile* BestDestination = nullptr;

	for (int32 Step = 1; Step <= PushDistance; ++Step)
	{
		const FIntPoint CandidateCoord = TargetCoord + FIntPoint(Direction.X * Step, Direction.Y * Step);
		if (!GridBoard->IsCoordValid(CandidateCoord))
		{
			break;
		}

		AGridTile* CandidateTile = GridBoard->GetTile(CandidateCoord);
		if (!CandidateTile || !CandidateTile->IsWalkable())
		{
			break;
		}

		BestDestination = CandidateTile;
	}

	if (!BestDestination)
	{
		return false;
	}

	TargetUnit->PlaceOnTile(BestDestination);
	return true;
}

bool FCardResolver::ResolveMoveSelf(
	const UCardDefinition* Card,
	const FCardResolveContext& Context,
	FCardResolveResult& OutResult
)
{
	if (!Card || !Context.GameMode || !Context.SourceUnit || !Context.TileTarget)
	{
		return false;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	AGridTile* StartTile = Context.SourceUnit->GetCurrentTile();
	AGridTile* DestinationTile = Context.TileTarget.Get();

	if (!GridBoard || !StartTile || !DestinationTile)
	{
		return false;
	}

	if (DestinationTile == StartTile)
	{
		return false;
	}

	if (!DestinationTile->IsWalkable())
	{
		return false;
	}

	const TArray<AGridTile*> Path = GridBoard->BuildPath(StartTile, DestinationTile);
	if (Path.Num() < 2)
	{
		return false;
	}

	const int32 StepsRequired = Path.Num() - 1;
	if (StepsRequired > Card->Range)
	{
		return false;
	}

	Context.SourceUnit->MoveAlongPath(Path);

	return true;
}

AGridTile* FCardResolver::GetResolvedTargetTile(const FCardResolveContext& Context)
{
	if (Context.TileTarget)
	{
		return Context.TileTarget.Get();
	}

	if (Context.UnitTarget)
	{
		return Context.UnitTarget->GetCurrentTile();
	}

	return nullptr;
}

ABattleUnit* FCardResolver::GetResolvedTargetUnit(const FCardResolveContext& Context)
{
	if (Context.UnitTarget)
	{
		return Context.UnitTarget.Get();
	}

	AGridTile* TargetTile = GetResolvedTargetTile(Context);
	return TargetTile ? TargetTile->GetOccupyingUnit() : nullptr;
}

int32 FCardResolver::GetTileDistance(const AGridTile* TileA, const AGridTile* TileB)
{
	if (!TileA || !TileB)
	{
		return MAX_int32;
	}

	const FIntPoint CoordA = TileA->GetCoord();
	const FIntPoint CoordB = TileB->GetCoord();

	return FMath::Abs(CoordA.X - CoordB.X) + FMath::Abs(CoordA.Y - CoordB.Y);
}
