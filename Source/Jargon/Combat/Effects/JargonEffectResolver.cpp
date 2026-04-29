#include "Combat/Effects/JargonEffectResolver.h"

#include "Combat/JargonCombatGameMode.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Grid/GridBoard.h"
#include "Combat/Grid/GridTile.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/CardDefinition.h"

namespace
{
	void AppendValidUnitsFromArray(
		const TArray<TObjectPtr<ABattleUnit>>& SourceArray,
		TArray<ABattleUnit*>& OutUnits)
	{
		for (const TObjectPtr<ABattleUnit>& UnitPtr : SourceArray)
		{
			ABattleUnit* Unit = UnitPtr.Get();
			if (IsValid(Unit) && !Unit->IsDead())
			{
				OutUnits.Add(Unit);
			}
		}
	}

	TArray<ABattleUnit*> GetAllLivingCombatUnits(const AJargonCombatGameMode* GameMode)
	{
		TArray<ABattleUnit*> Units;

		if (!GameMode)
		{
			return Units;
		}

		AppendValidUnitsFromArray(GameMode->GetFriendlyUnits(), Units);
		AppendValidUnitsFromArray(GameMode->GetEnemyUnits(), Units);

		return Units;
	}

	bool IsUnitPayloadOperation(EJargonEffectOperation Operation)
	{
		return Operation == EJargonEffectOperation::DealDamage
			|| Operation == EJargonEffectOperation::Heal
			|| Operation == EJargonEffectOperation::ApplyShield
			|| Operation == EJargonEffectOperation::ApplyStun;
	}
}

bool FJargonEffectResolver::ResolveEffects(
	const TArray<FJargonEffectSpec>& Effects,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (Effects.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("JargonEffectResolver received no effects to resolve."));
		return false;
	}

	if (!Context.GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("JargonEffectResolver cannot resolve effects without a combat game mode."));
		return false;
	}

	bool bResolvedAnyEffect = false;

	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		FJargonEffectResult EffectResult;
		if (!ResolveEffect(Effects[EffectIndex], Context, EffectResult))
		{
			UE_LOG(LogTemp, Warning, TEXT("Jargon effect spec at index %d failed to resolve."), EffectIndex);
			continue;
		}

		bResolvedAnyEffect = true;
		OutResult.bResolvedAnyEffect = true;
		OutResult.bConsumePlayerMove |= EffectResult.bConsumePlayerMove;
		OutResult.bContinuesAsynchronously |= EffectResult.bContinuesAsynchronously;
		OutResult.EnergyGainAfterCost += EffectResult.EnergyGainAfterCost;

		if (EffectResult.SpawnedUnit)
		{
			OutResult.SpawnedUnit = EffectResult.SpawnedUnit;
		}

		if (EffectResult.SpawnedTileEffect)
		{
			OutResult.SpawnedTileEffect = EffectResult.SpawnedTileEffect;
		}

		if (EffectResult.bContinuesAsynchronously)
		{
			if (EffectIndex < Effects.Num() - 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("Async Jargon effect at index %d started before later effect specs. Later effects are skipped for this resolve pass."), EffectIndex);
			}
			break;
		}
	}

	return bResolvedAnyEffect;
}

bool FJargonEffectResolver::ResolveEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (!Context.GameMode)
	{
		return false;
	}

	if (IsUnitPayloadOperation(EffectSpec.Operation))
	{
		return ResolveUnitPayloadEffect(EffectSpec, Context, OutResult);
	}

	switch (EffectSpec.Operation)
	{
	case EJargonEffectOperation::MoveSource:
		return ResolveMoveSourceEffect(EffectSpec, Context, OutResult);

	case EJargonEffectOperation::PushTarget:
		return ResolvePushTargetEffect(EffectSpec, Context, OutResult);

	case EJargonEffectOperation::PullTarget:
		UE_LOG(LogTemp, Warning, TEXT("PullTarget is intentionally deferred in the generic Jargon effect resolver."));
		return false;

	case EJargonEffectOperation::SummonUnit:
		return ResolveSummonUnitEffect(EffectSpec, Context, OutResult);

	case EJargonEffectOperation::PlaceTileEffect:
		return ResolvePlaceTileEffect(EffectSpec, Context, OutResult);

	case EJargonEffectOperation::DestroyTileEffect:
		return ResolveDestroyTileEffect(EffectSpec, Context, OutResult);

	case EJargonEffectOperation::DrawCards:
		return ResolveDrawCardsEffect(EffectSpec, Context, OutResult);

	case EJargonEffectOperation::GainEnergy:
		return ResolveGainEnergyEffect(EffectSpec, Context, OutResult);

	case EJargonEffectOperation::None:
	default:
		UE_LOG(LogTemp, Warning, TEXT("Unsupported or empty Jargon effect operation: %d."), static_cast<int32>(EffectSpec.Operation));
		return false;
	}
}

bool FJargonEffectResolver::ResolveMoveSourceEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (!Context.GameMode || !Context.SourceUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveSource requires GameMode and SourceUnit."));
		return false;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	AGridTile* StartTile = GetResolvedSourceTile(Context);
	AGridTile* DestinationTile = GetResolvedTargetTile(Context);

	if (!GridBoard || !StartTile || !DestinationTile || DestinationTile == StartTile || !DestinationTile->IsWalkable())
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveSource could not resolve a valid destination tile."));
		return false;
	}

	const TArray<AGridTile*> Path = GridBoard->BuildPath(StartTile, DestinationTile);
	if (Path.Num() < 2)
	{
		return false;
	}

	const int32 StepsRequired = Path.Num() - 1;
	const int32 MaxSteps = FMath::Max(0, EffectSpec.MoveDistance);
	if (StepsRequired > MaxSteps)
	{
		UE_LOG(LogTemp, Log, TEXT("MoveSource target is out of effect range. Required <= %d, actual %d."), MaxSteps, StepsRequired);
		return false;
	}

	if (!Context.GameMode->StartPlayerControlledMoveSequence(Context.SourceUnit, Path, false))
	{
		return false;
	}

	OutResult.bResolvedAnyEffect = true;
	OutResult.bContinuesAsynchronously = true;
	return true;
}

bool FJargonEffectResolver::ResolvePushTargetEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (!Context.GameMode || !Context.SourceUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("PushTarget requires GameMode and SourceUnit."));
		return false;
	}

	const TArray<ABattleUnit*> TargetUnits = GatherTargetUnits(EffectSpec, Context);
	if (TargetUnits.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PushTarget found no valid target units."));
		return false;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	AGridTile* SourceTile = GetResolvedSourceTile(Context);
	if (!GridBoard || !SourceTile)
	{
		return false;
	}

	const int32 PushDistance = FMath::Max(1, EffectSpec.PushDistance);
	const int32 CollisionDamage = FMath::Max(0, EffectSpec.CollisionDamage);
	bool bResolvedAnyPushEffect = false;

	for (ABattleUnit* TargetUnit : TargetUnits)
	{
		if (!IsValid(TargetUnit) || TargetUnit->IsDead())
		{
			continue;
		}

		AGridTile* TargetTile = TargetUnit->GetCurrentTile();
		if (!TargetTile)
		{
			continue;
		}

		AGridTile* BestDestination = nullptr;
		bool bCollidedWithObstruction = false;

		for (int32 Step = 1; Step <= PushDistance; ++Step)
		{
			AGridTile* CandidateTile = GridBoard->GetTileInPushDirection(SourceTile, TargetTile, Step);
			if (!CandidateTile || !CandidateTile->IsWalkable())
			{
				bCollidedWithObstruction = true;
				break;
			}

			BestDestination = CandidateTile;
		}

		if (BestDestination)
		{
			TargetUnit->PlaceOnTile(BestDestination);
			bResolvedAnyPushEffect = true;
		}

		if (bCollidedWithObstruction && CollisionDamage > 0 && !TargetUnit->IsDead())
		{
			TargetUnit->ApplyDamage(CollisionDamage);
			bResolvedAnyPushEffect = true;
		}
	}

	OutResult.bResolvedAnyEffect = bResolvedAnyPushEffect;
	return bResolvedAnyPushEffect;
}

bool FJargonEffectResolver::ResolveSummonUnitEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (!Context.GameMode || !Context.SourceUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonUnit requires GameMode and SourceUnit."));
		return false;
	}

	AGridTile* TargetTile = GetResolvedTargetTile(Context);
	if (!TargetTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonUnit requires a target tile."));
		return false;
	}

	if (!EffectSpec.UnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonUnit effect has no UnitClass."));
		return false;
	}

	ABattleUnit* SpawnedUnit = Context.GameMode->SpawnSummonedUnitFromClass(
		EffectSpec.UnitClass,
		Context.SourceUnit,
		TargetTile,
		EffectSpec.bSummonEntersWithAttackExhausted);

	if (!SpawnedUnit)
	{
		return false;
	}

	OutResult.bResolvedAnyEffect = true;
	OutResult.SpawnedUnit = SpawnedUnit;
	return true;
}

bool FJargonEffectResolver::ResolvePlaceTileEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (!Context.GameMode || !Context.SourceUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect requires GameMode and SourceUnit in the current compatibility path."));
		return false;
	}

	AGridTile* TargetTile = GetResolvedTargetTile(Context);
	if (!TargetTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect requires a target tile."));
		return false;
	}

	if (!EffectSpec.TileEffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect has no TileEffectClass."));
		return false;
	}

	const ECardCategory TileEffectCategory = Context.SourceCard
		? Context.SourceCard->Category
		: EffectSpec.TileEffectCategory;

	ABattleTileEffect* SpawnedEffect = Context.GameMode->SpawnPersistentTileEffectFromClass(
		EffectSpec.TileEffectClass,
		Context.SourceCard,
		Context.SourceUnit,
		TargetTile,
		TileEffectCategory,
		FMath::Max(0, EffectSpec.Value),
		FMath::Max(0, EffectSpec.Radius),
		FMath::Max(0, EffectSpec.TileEffectDuration));

	if (!SpawnedEffect)
	{
		return false;
	}

	OutResult.bResolvedAnyEffect = true;
	OutResult.SpawnedTileEffect = SpawnedEffect;
	return true;
}

bool FJargonEffectResolver::ResolveDestroyTileEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (!Context.GameMode)
	{
		return false;
	}

	const TArray<AGridTile*> TargetTiles = GatherTargetTiles(EffectSpec, Context);
	if (TargetTiles.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("DestroyTileEffect found no target tiles."));
		return false;
	}

	bool bDestroyedAny = false;

	for (AGridTile* Tile : TargetTiles)
	{
		if (!Tile)
		{
			continue;
		}

		TArray<TObjectPtr<ABattleTileEffect>> TileEffects = Tile->GetTileEffects();
		for (const TObjectPtr<ABattleTileEffect>& TileEffectPtr : TileEffects)
		{
			ABattleTileEffect* TileEffect = TileEffectPtr.Get();
			if (!IsValid(TileEffect))
			{
				continue;
			}

			if (EffectSpec.TargetFilter != EJargonEffectTargetFilter::Any &&
				TileEffect->GetSourceTeam() == GetResolvedSourceTeam(Context))
			{
				continue;
			}

			TileEffect->Destroy();
			bDestroyedAny = true;
		}
	}

	OutResult.bResolvedAnyEffect = bDestroyedAny;
	return bDestroyedAny;
}

bool FJargonEffectResolver::ResolveDrawCardsEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (!Context.GameMode)
	{
		return false;
	}

	const int32 CardCount = FMath::Max(0, EffectSpec.Value);
	if (CardCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("DrawCards effect requires Value > 0."));
		return false;
	}

	const bool bDrewCards = Context.GameMode->DrawCardsForPlayer(CardCount);
	OutResult.bResolvedAnyEffect = bDrewCards;
	return bDrewCards;
}

bool FJargonEffectResolver::ResolveGainEnergyEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	const int32 EnergyAmount = FMath::Max(0, EffectSpec.Value);
	if (EnergyAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GainEnergy effect requires Value > 0."));
		return false;
	}

	OutResult.bResolvedAnyEffect = true;
	OutResult.EnergyGainAfterCost += EnergyAmount;
	return true;
}

bool FJargonEffectResolver::ResolveUnitPayloadEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	const int32 Amount = FMath::Max(0, EffectSpec.Value);
	if (Amount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unit payload effect %d requires Value > 0."), static_cast<int32>(EffectSpec.Operation));
		return false;
	}

	const TArray<ABattleUnit*> TargetUnits = GatherTargetUnits(EffectSpec, Context);
	if (TargetUnits.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unit payload effect %d found no valid target units."), static_cast<int32>(EffectSpec.Operation));
		return false;
	}

	bool bAppliedAny = false;
	for (ABattleUnit* TargetUnit : TargetUnits)
	{
		if (!ApplyUnitPayload(TargetUnit, EffectSpec, Context))
		{
			continue;
		}

		bAppliedAny = true;

		if (!IsValid(Context.GameMode) ||
			Context.GameMode->GetCurrentCombatPhase() == ECombatPhase::Victory ||
			Context.GameMode->GetCurrentCombatPhase() == ECombatPhase::Defeat)
		{
			break;
		}
	}

	OutResult.bResolvedAnyEffect = bAppliedAny;
	return bAppliedAny;
}

TArray<ABattleUnit*> FJargonEffectResolver::GatherTargetUnits(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context)
{
	TArray<ABattleUnit*> TargetUnits;

	switch (EffectSpec.Delivery)
	{
	case EJargonEffectDelivery::Self:
		if (Context.SourceUnit && DoesUnitPassTargetFilter(Context.SourceUnit, EffectSpec, Context))
		{
			TargetUnits.Add(Context.SourceUnit);
		}
		break;

	case EJargonEffectDelivery::ExplicitUnit:
		if (ABattleUnit* TargetUnit = GetResolvedTargetUnit(Context))
		{
			if (DoesUnitPassTargetFilter(TargetUnit, EffectSpec, Context))
			{
				TargetUnits.Add(TargetUnit);
			}
		}
		else if ((EffectSpec.Operation == EJargonEffectOperation::Heal ||
			EffectSpec.Operation == EJargonEffectOperation::ApplyShield) &&
			Context.SourceUnit &&
			DoesUnitPassTargetFilter(Context.SourceUnit, EffectSpec, Context))
		{
			TargetUnits.Add(Context.SourceUnit);
		}
		break;

	case EJargonEffectDelivery::UnitsInRadius:
	{
		if (!Context.GameMode)
		{
			break;
		}

		AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
		AGridTile* CenterTile = GetResolvedTargetTile(Context);
		if (!CenterTile)
		{
			CenterTile = GetResolvedSourceTile(Context);
		}

		if (!GridBoard || !CenterTile)
		{
			break;
		}

		const int32 Radius = FMath::Max(0, EffectSpec.Radius);
		const TArray<ABattleUnit*> AllUnits = GetAllLivingCombatUnits(Context.GameMode);
		for (ABattleUnit* CandidateUnit : AllUnits)
		{
			if (!DoesUnitPassTargetFilter(CandidateUnit, EffectSpec, Context))
			{
				continue;
			}

			AGridTile* CandidateTile = CandidateUnit->GetCurrentTile();
			if (CandidateTile && GridBoard->AreTilesWithinRange(CandidateTile, CenterTile, Radius))
			{
				TargetUnits.Add(CandidateUnit);
			}
		}
		break;
	}

	case EJargonEffectDelivery::ChainUnits:
		TargetUnits = GatherChainTargetUnits(EffectSpec, Context);
		break;

	case EJargonEffectDelivery::ExplicitTile:
	case EJargonEffectDelivery::TilesInRadius:
	default:
		UE_LOG(LogTemp, Warning, TEXT("Delivery %d does not gather unit targets for operation %d."),
			static_cast<int32>(EffectSpec.Delivery),
			static_cast<int32>(EffectSpec.Operation));
		break;
	}

	return TargetUnits;
}

TArray<AGridTile*> FJargonEffectResolver::GatherTargetTiles(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context)
{
	TArray<AGridTile*> TargetTiles;

	AGridTile* CenterTile = GetResolvedTargetTile(Context);
	if (!CenterTile)
	{
		CenterTile = GetResolvedSourceTile(Context);
	}

	switch (EffectSpec.Delivery)
	{
	case EJargonEffectDelivery::ExplicitTile:
	case EJargonEffectDelivery::Self:
		if (CenterTile)
		{
			TargetTiles.Add(CenterTile);
		}
		break;

	case EJargonEffectDelivery::TilesInRadius:
	{
		AGridBoard* GridBoard = Context.GameMode ? Context.GameMode->GetGridBoard() : nullptr;
		if (!GridBoard || !CenterTile)
		{
			break;
		}

		TargetTiles = GridBoard->GetTilesWithinRadius(CenterTile, FMath::Max(0, EffectSpec.Radius));
		break;
	}

	default:
		UE_LOG(LogTemp, Warning, TEXT("Delivery %d does not gather tile targets for operation %d."),
			static_cast<int32>(EffectSpec.Delivery),
			static_cast<int32>(EffectSpec.Operation));
		break;
	}

	return TargetTiles;
}

TArray<ABattleUnit*> FJargonEffectResolver::GatherChainTargetUnits(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context)
{
	TArray<ABattleUnit*> HitUnits;

	if (!Context.GameMode)
	{
		return HitUnits;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	if (!GridBoard)
	{
		return HitUnits;
	}

	ABattleUnit* CurrentTarget = GetResolvedTargetUnit(Context);
	if (!CurrentTarget &&
		(EffectSpec.Operation == EJargonEffectOperation::Heal ||
			EffectSpec.Operation == EJargonEffectOperation::ApplyShield))
	{
		CurrentTarget = Context.SourceUnit.Get();
	}

	if (!CurrentTarget || !DoesUnitPassTargetFilter(CurrentTarget, EffectSpec, Context))
	{
		UE_LOG(LogTemp, Warning, TEXT("ChainUnits requires an initial target that passes its target filter."));
		return HitUnits;
	}

	const int32 ChainCount = FMath::Max(1, EffectSpec.ChainCount);
	const int32 ChainSearchRadius = FMath::Max(1, EffectSpec.Radius);
	TArray<ABattleUnit*> AllUnits = GetAllLivingCombatUnits(Context.GameMode);

	for (int32 ChainIndex = 0; ChainIndex < ChainCount; ++ChainIndex)
	{
		if (!IsValid(CurrentTarget) || CurrentTarget->IsDead() ||
			!DoesUnitPassTargetFilter(CurrentTarget, EffectSpec, Context))
		{
			break;
		}

		AGridTile* ChainOriginTile = CurrentTarget->GetCurrentTile();
		if (!ChainOriginTile)
		{
			break;
		}

		HitUnits.Add(CurrentTarget);

		TArray<ABattleUnit*> CandidateUnits;
		for (ABattleUnit* CandidateUnit : AllUnits)
		{
			if (!IsValid(CandidateUnit) || CandidateUnit->IsDead())
			{
				continue;
			}

			if (HitUnits.Contains(CandidateUnit) ||
				!DoesUnitPassTargetFilter(CandidateUnit, EffectSpec, Context))
			{
				continue;
			}

			AGridTile* CandidateTile = CandidateUnit->GetCurrentTile();
			if (CandidateTile && GridBoard->AreTilesWithinRange(ChainOriginTile, CandidateTile, ChainSearchRadius))
			{
				CandidateUnits.Add(CandidateUnit);
			}
		}

		if (CandidateUnits.Num() <= 0)
		{
			break;
		}

		CurrentTarget = CandidateUnits[FMath::RandRange(0, CandidateUnits.Num() - 1)];
	}

	return HitUnits;
}

bool FJargonEffectResolver::DoesUnitPassTargetFilter(
	const ABattleUnit* Unit,
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context)
{
	if (!Unit || Unit->IsDead())
	{
		return false;
	}

	const ETeam SourceTeam = GetResolvedSourceTeam(Context);

	switch (EffectSpec.TargetFilter)
	{
	case EJargonEffectTargetFilter::None:
	case EJargonEffectTargetFilter::Any:
		return true;

	case EJargonEffectTargetFilter::FriendlyToSource:
		return Unit->GetTeam() == SourceTeam;

	case EJargonEffectTargetFilter::EnemyToSource:
		return Unit->GetTeam() != SourceTeam;

	case EJargonEffectTargetFilter::SourceOnly:
		return Unit == Context.SourceUnit.Get();

	default:
		return false;
	}
}

bool FJargonEffectResolver::ApplyUnitPayload(
	ABattleUnit* TargetUnit,
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context)
{
	if (!TargetUnit || TargetUnit->IsDead())
	{
		return false;
	}

	const int32 Amount = FMath::Max(0, EffectSpec.Value);
	if (Amount <= 0)
	{
		return false;
	}

	switch (EffectSpec.Operation)
	{
	case EJargonEffectOperation::DealDamage:
		TargetUnit->ApplyDamage(Amount);
		return true;

	case EJargonEffectOperation::Heal:
		TargetUnit->ApplyHeal(Amount);
		return true;

	case EJargonEffectOperation::ApplyShield:
		TargetUnit->AddTemporaryShield(Amount);
		return true;

	case EJargonEffectOperation::ApplyStun:
		TargetUnit->ApplyStun(Amount);
		return true;

	default:
		return false;
	}
}

AGridTile* FJargonEffectResolver::GetResolvedTargetTile(const FJargonEffectContext& Context)
{
	if (Context.PrimaryTileTarget)
	{
		return Context.PrimaryTileTarget.Get();
	}

	if (Context.PrimaryUnitTarget)
	{
		return Context.PrimaryUnitTarget->GetCurrentTile();
	}

	if (Context.TriggeringUnit)
	{
		return Context.TriggeringUnit->GetCurrentTile();
	}

	return nullptr;
}

ABattleUnit* FJargonEffectResolver::GetResolvedTargetUnit(const FJargonEffectContext& Context)
{
	if (Context.PrimaryUnitTarget)
	{
		return Context.PrimaryUnitTarget.Get();
	}

	if (Context.TriggeringUnit)
	{
		return Context.TriggeringUnit.Get();
	}

	AGridTile* TargetTile = GetResolvedTargetTile(Context);
	return TargetTile ? TargetTile->GetOccupyingUnit() : nullptr;
}

AGridTile* FJargonEffectResolver::GetResolvedSourceTile(const FJargonEffectContext& Context)
{
	if (Context.SourceTile)
	{
		return Context.SourceTile.Get();
	}

	return Context.SourceUnit ? Context.SourceUnit->GetCurrentTile() : nullptr;
}

ETeam FJargonEffectResolver::GetResolvedSourceTeam(const FJargonEffectContext& Context)
{
	return Context.SourceUnit ? Context.SourceUnit->GetTeam() : Context.SourceTeam;
}
