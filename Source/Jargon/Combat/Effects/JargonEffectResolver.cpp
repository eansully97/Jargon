#include "Combat/Effects/JargonEffectResolver.h"

#include "Combat/JargonCombatGameMode.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Grid/GridBoard.h"
#include "Combat/Grid/GridTile.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/CardDefinition.h"
#include "Data/JargonRelicDefinition.h"

namespace
{
	void AppendValidEffectResolverUnitsFromArray(
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

	TArray<ABattleUnit*> GetAllLivingEffectResolverCombatUnits(const AJargonCombatGameMode* GameMode)
	{
		TArray<ABattleUnit*> Units;

		if (!GameMode)
		{
			return Units;
		}

		AppendValidEffectResolverUnitsFromArray(GameMode->GetFriendlyUnits(), Units);
		AppendValidEffectResolverUnitsFromArray(GameMode->GetEnemyUnits(), Units);

		return Units;
	}

	bool IsUnitPayloadOperation(EJargonEffectOperation Operation)
	{
		return Operation == EJargonEffectOperation::DealDamage
			|| Operation == EJargonEffectOperation::Heal
			|| Operation == EJargonEffectOperation::ApplyShield
			|| Operation == EJargonEffectOperation::ApplyStun;
	}

	bool IsLiveSourceUnit(const FJargonEffectContext& Context)
	{
		return IsValid(Context.SourceUnit.Get()) && !Context.SourceUnit->IsDead();
	}

	void EmitEffectResolverCue(
		const FJargonEffectContext& Context,
		const FJargonEffectSpec& EffectSpec,
		EJargonCombatCueType CueType,
		ABattleUnit* TargetUnit,
		AGridTile* TargetTile,
		int32 Value)
	{
		if (!Context.GameMode)
		{
			return;
		}

		FJargonCombatCueEvent Cue;
		Cue.CueType = CueType;
		Cue.Operation = EffectSpec.Operation;
		Cue.Trigger = Context.Trigger;
		Cue.SourceObject = Context.SourceObject;
		Cue.SourceUnit = Context.SourceUnit;
		Cue.TargetUnit = TargetUnit;
		Cue.SourceTile = Context.SourceTile;
		Cue.TargetTile = TargetTile ? TargetTile : (TargetUnit ? TargetUnit->GetCurrentTile() : Context.PrimaryTileTarget.Get());
		Cue.OwningTileEffect = Context.OwningTileEffect;
		Cue.SourceCard = Context.SourceCard;
		Cue.SourceRelic = Cast<UJargonRelicDefinition>(Context.SourceObject.Get());
		Cue.Value = Value;
		Cue.Radius = EffectSpec.Radius;

		if (Cue.TargetUnit)
		{
			Cue.WorldLocation = Cue.TargetUnit->GetActorLocation();
			Cue.bHasWorldLocation = true;
		}
		else if (Cue.TargetTile)
		{
			Cue.WorldLocation = Cue.TargetTile->GetActorLocation();
			Cue.bHasWorldLocation = true;
		}
		else if (Cue.SourceUnit)
		{
			Cue.WorldLocation = Cue.SourceUnit->GetActorLocation();
			Cue.bHasWorldLocation = true;
		}

		Context.GameMode->EmitCombatCue(Cue);
	}

	const TCHAR* GetEffectOperationName(EJargonEffectOperation Operation)
	{
		switch (Operation)
		{
		case EJargonEffectOperation::None:
			return TEXT("None");
		case EJargonEffectOperation::DealDamage:
			return TEXT("DealDamage");
		case EJargonEffectOperation::Heal:
			return TEXT("Heal");
		case EJargonEffectOperation::ApplyShield:
			return TEXT("ApplyShield");
		case EJargonEffectOperation::ApplyStun:
			return TEXT("ApplyStun");
		case EJargonEffectOperation::MoveSource:
			return TEXT("MoveSource");
		case EJargonEffectOperation::PushTarget:
			return TEXT("PushTarget");
		case EJargonEffectOperation::PullTarget:
			return TEXT("PullTarget");
		case EJargonEffectOperation::SummonUnit:
			return TEXT("SummonUnit");
		case EJargonEffectOperation::PlaceTileEffect:
			return TEXT("PlaceTileEffect");
		case EJargonEffectOperation::DestroyTileEffect:
			return TEXT("DestroyTileEffect");
		case EJargonEffectOperation::DrawCards:
			return TEXT("DrawCards");
		case EJargonEffectOperation::GainEnergy:
			return TEXT("GainEnergy");
		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* GetEffectDeliveryName(EJargonEffectDelivery Delivery)
	{
		switch (Delivery)
		{
		case EJargonEffectDelivery::Self:
			return TEXT("Self");
		case EJargonEffectDelivery::ExplicitUnit:
			return TEXT("ExplicitUnit");
		case EJargonEffectDelivery::ExplicitTile:
			return TEXT("ExplicitTile");
		case EJargonEffectDelivery::UnitsInRadius:
			return TEXT("UnitsInRadius");
		case EJargonEffectDelivery::TilesInRadius:
			return TEXT("TilesInRadius");
		case EJargonEffectDelivery::ChainUnits:
			return TEXT("ChainUnits");
		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* GetEffectTriggerName(EJargonEffectTrigger Trigger)
	{
		switch (Trigger)
		{
		case EJargonEffectTrigger::OnPlayed:
			return TEXT("OnPlayed");
		case EJargonEffectTrigger::OnSummoned:
			return TEXT("OnSummoned");
		case EJargonEffectTrigger::OnEnterTile:
			return TEXT("OnEnterTile");
		case EJargonEffectTrigger::OnTurnStart:
			return TEXT("OnTurnStart");
		case EJargonEffectTrigger::Activated:
			return TEXT("Activated");
		case EJargonEffectTrigger::OnDeath:
			return TEXT("OnDeath");
		case EJargonEffectTrigger::OnCombatStart:
			return TEXT("OnCombatStart");
		case EJargonEffectTrigger::OnEnemyDeath:
			return TEXT("OnEnemyDeath");
		default:
			return TEXT("Unknown");
		}
	}

}

bool FJargonEffectResolver::ValidateEffectForContext(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context)
{
	if (!Context.GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("Jargon effect %s cannot resolve without a combat game mode."),
			GetEffectOperationName(EffectSpec.Operation));
		return false;
	}

	const TCHAR* OperationName = GetEffectOperationName(EffectSpec.Operation);
	const TCHAR* DeliveryName = GetEffectDeliveryName(EffectSpec.Delivery);
	const TCHAR* TriggerName = GetEffectTriggerName(Context.Trigger);

	if (EffectSpec.Operation == EJargonEffectOperation::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("Jargon effect with Delivery %s in %s context has no operation."),
			DeliveryName,
			TriggerName);
		return false;
	}

	if (Context.Trigger == EJargonEffectTrigger::OnDeath)
	{
		if (EffectSpec.Operation == EJargonEffectOperation::MoveSource)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery %s is not supported for OnDeath context."),
				OperationName,
				DeliveryName);
			return false;
		}

		if (EffectSpec.Delivery == EJargonEffectDelivery::ChainUnits)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery ChainUnits is not supported for OnDeath context without a living explicit initial target."),
				OperationName);
			return false;
		}
	}

	if (Context.Trigger == EJargonEffectTrigger::OnTurnStart &&
		EffectSpec.Operation == EJargonEffectOperation::MoveSource)
	{
		UE_LOG(LogTemp, Warning, TEXT("Effect MoveSource with Delivery %s is not supported for OnTurnStart context because async unit turn-start movement is not sequenced yet."),
			DeliveryName);
		return false;
	}

	if (IsUnitPayloadOperation(EffectSpec.Operation))
	{
		if (EffectSpec.Value <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s requires Value > 0."),
				OperationName);
			return false;
		}

		if (EffectSpec.Delivery == EJargonEffectDelivery::ExplicitTile ||
			EffectSpec.Delivery == EJargonEffectDelivery::TilesInRadius)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery %s does not gather unit targets. Use ExplicitUnit, Self, UnitsInRadius, or ChainUnits."),
				OperationName,
				DeliveryName);
			return false;
		}
	}

	if (EffectSpec.Delivery == EJargonEffectDelivery::Self &&
		IsUnitPayloadOperation(EffectSpec.Operation) &&
		!IsLiveSourceUnit(Context))
	{
		UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery Self in %s context requires a living SourceUnit."),
			OperationName,
			TriggerName);
		return false;
	}

	if (EffectSpec.Delivery == EJargonEffectDelivery::ExplicitUnit &&
		(IsUnitPayloadOperation(EffectSpec.Operation) ||
			EffectSpec.Operation == EJargonEffectOperation::PushTarget ||
			EffectSpec.Operation == EJargonEffectOperation::PullTarget))
	{
		ABattleUnit* TargetUnit = GetResolvedTargetUnit(Context);
		if (!IsValid(TargetUnit) || TargetUnit->IsDead())
		{
			const bool bCanFallbackToSource =
				(EffectSpec.Operation == EJargonEffectOperation::Heal ||
					EffectSpec.Operation == EJargonEffectOperation::ApplyShield) &&
				IsLiveSourceUnit(Context);

			if (!bCanFallbackToSource)
			{
				UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery ExplicitUnit in %s context requires a living PrimaryUnitTarget or TriggeringUnit."),
					OperationName,
					TriggerName);
				return false;
			}
		}
	}

	if (EffectSpec.Delivery == EJargonEffectDelivery::UnitsInRadius)
	{
		AGridTile* CenterTile = GetResolvedTargetTile(Context);
		if (!CenterTile)
		{
			CenterTile = GetResolvedSourceTile(Context);
		}

		if (!CenterTile)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery UnitsInRadius in %s context requires a source or target tile."),
				OperationName,
				TriggerName);
			return false;
		}
	}

	if (EffectSpec.Delivery == EJargonEffectDelivery::TilesInRadius)
	{
		AGridTile* CenterTile = GetResolvedTargetTile(Context);
		if (!CenterTile)
		{
			CenterTile = GetResolvedSourceTile(Context);
		}

		if (!CenterTile)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery TilesInRadius in %s context requires a source or target tile."),
				OperationName,
				TriggerName);
			return false;
		}
	}

	if (EffectSpec.Delivery == EJargonEffectDelivery::ChainUnits)
	{
		if (EffectSpec.ChainCount <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery ChainUnits requires ChainCount > 0."),
				OperationName);
			return false;
		}

		if (EffectSpec.Radius <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery ChainUnits has Radius <= 0. Resolver will use a minimum chain search radius of 1."),
				OperationName);
		}

		ABattleUnit* InitialTarget = GetResolvedTargetUnit(Context);
		const bool bCanFallbackToSource =
			(EffectSpec.Operation == EJargonEffectOperation::Heal ||
				EffectSpec.Operation == EJargonEffectOperation::ApplyShield) &&
			IsLiveSourceUnit(Context);

		if ((!IsValid(InitialTarget) || InitialTarget->IsDead()) && !bCanFallbackToSource)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s with Delivery ChainUnits in %s context requires a living initial target."),
				OperationName,
				TriggerName);
			return false;
		}
	}

	switch (EffectSpec.Operation)
	{
	case EJargonEffectOperation::MoveSource:
		if (!IsLiveSourceUnit(Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("MoveSource requires a living SourceUnit."));
			return false;
		}
		if (!GetResolvedTargetTile(Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("MoveSource requires a target tile."));
			return false;
		}
		if (EffectSpec.MoveDistance <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("MoveSource requires MoveDistance > 0."));
			return false;
		}
		break;

	case EJargonEffectOperation::PushTarget:
		if (!IsLiveSourceUnit(Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("PushTarget requires a living SourceUnit."));
			return false;
		}
		if (EffectSpec.PushDistance <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("PushTarget requires PushDistance > 0."));
			return false;
		}
		break;

	case EJargonEffectOperation::PullTarget:
		UE_LOG(LogTemp, Warning, TEXT("PullTarget is intentionally deferred in the generic Jargon effect resolver."));
		return false;

	case EJargonEffectOperation::SummonUnit:
		if (!IsLiveSourceUnit(Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("SummonUnit requires a living SourceUnit."));
			return false;
		}
		if (!EffectSpec.UnitClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("SummonUnit effect has no UnitClass."));
			return false;
		}
		if (!GetResolvedTargetTile(Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("SummonUnit requires a target tile."));
			return false;
		}
		break;

	case EJargonEffectOperation::PlaceTileEffect:
		if (!EffectSpec.TileEffectClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect has no TileEffectClass."));
			return false;
		}
		if (!GetResolvedTargetTile(Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect requires a source or target tile."));
			return false;
		}
		if (EffectSpec.Delivery != EJargonEffectDelivery::ExplicitTile &&
			EffectSpec.Delivery != EJargonEffectDelivery::Self)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect currently supports Delivery ExplicitTile or Self, not %s."),
				DeliveryName);
			return false;
		}
		break;

	case EJargonEffectOperation::DestroyTileEffect:
		if (!GetResolvedTargetTile(Context) && !GetResolvedSourceTile(Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("DestroyTileEffect requires a source or target tile."));
			return false;
		}
		break;

	case EJargonEffectOperation::DrawCards:
	case EJargonEffectOperation::GainEnergy:
		if (Context.Trigger != EJargonEffectTrigger::OnPlayed)
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %s is only supported for OnPlayed/card contexts right now, not %s."),
				OperationName,
				TriggerName);
			return false;
		}
		break;

	default:
		break;
	}

	return true;
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

	if (!ValidateEffectForContext(EffectSpec, Context))
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
	if (!Context.GameMode || !IsLiveSourceUnit(Context))
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveSource requires GameMode and a living SourceUnit."));
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
	if (!Context.GameMode || !IsLiveSourceUnit(Context))
	{
		UE_LOG(LogTemp, Warning, TEXT("PushTarget requires GameMode and a living SourceUnit."));
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
			EmitEffectResolverCue(Context, EffectSpec, EJargonCombatCueType::Push, TargetUnit, BestDestination, PushDistance);
			TargetUnit->PlaceOnTile(BestDestination);
			bResolvedAnyPushEffect = true;
		}

		if (bCollidedWithObstruction && CollisionDamage > 0 && !TargetUnit->IsDead())
		{
			EmitEffectResolverCue(Context, EffectSpec, EJargonCombatCueType::PushCollision, TargetUnit, TargetTile, CollisionDamage);
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
	if (!Context.GameMode || !IsLiveSourceUnit(Context))
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonUnit requires GameMode and a living SourceUnit."));
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
	EmitEffectResolverCue(Context, EffectSpec, EJargonCombatCueType::UnitSummoned, SpawnedUnit, TargetTile, 0);
	return true;
}

bool FJargonEffectResolver::ResolvePlaceTileEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (!Context.GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect requires GameMode."));
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

	const int32 TileEffectValue = FMath::Max(0, EffectSpec.Value);
	const int32 TileEffectRadius = FMath::Max(0, EffectSpec.Radius);
	const int32 TileEffectDuration = FMath::Max(0, EffectSpec.TileEffectDuration);

	ABattleTileEffect* SpawnedEffect = nullptr;
	if (IsLiveSourceUnit(Context))
	{
		SpawnedEffect = Context.GameMode->SpawnPersistentTileEffectFromClass(
			EffectSpec.TileEffectClass,
			Context.SourceCard,
			Context.SourceUnit.Get(),
			TargetTile,
			TileEffectCategory,
			TileEffectValue,
			TileEffectRadius,
			TileEffectDuration);
	}
	else
	{
		SpawnedEffect = Context.GameMode->SpawnPersistentTileEffectFromClassForTeam(
			EffectSpec.TileEffectClass,
			Context.SourceCard,
			GetResolvedSourceTeam(Context),
			TargetTile,
			TileEffectCategory,
			TileEffectValue,
			TileEffectRadius,
			TileEffectDuration);
	}

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

			if (!DoesTileEffectPassTargetFilter(TileEffect, EffectSpec, Context))
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

	if (EffectSpec.Delivery == EJargonEffectDelivery::UnitsInRadius)
	{
		AGridTile* PulseTile = GetResolvedTargetTile(Context);
		if (!PulseTile)
		{
			PulseTile = GetResolvedSourceTile(Context);
		}

		EmitEffectResolverCue(Context, EffectSpec, EJargonCombatCueType::AoEPulse, nullptr, PulseTile, Amount);
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
		const TArray<ABattleUnit*> AllUnits = GetAllLivingEffectResolverCombatUnits(Context.GameMode);
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
	TArray<ABattleUnit*> AllUnits = GetAllLivingEffectResolverCombatUnits(Context.GameMode);

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

		ABattleUnit* NextTarget = CandidateUnits[FMath::RandRange(0, CandidateUnits.Num() - 1)];
		EmitEffectResolverCue(Context, EffectSpec, EJargonCombatCueType::ChainJump, NextTarget, NextTarget ? NextTarget->GetCurrentTile() : nullptr, EffectSpec.Value);
		CurrentTarget = NextTarget;
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

bool FJargonEffectResolver::DoesTileEffectPassTargetFilter(
	const ABattleTileEffect* TileEffect,
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context)
{
	if (!TileEffect)
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
		return TileEffect->GetSourceTeam() == SourceTeam;

	case EJargonEffectTargetFilter::EnemyToSource:
		return TileEffect->GetSourceTeam() != SourceTeam;

	case EJargonEffectTargetFilter::SourceOnly:
		return TileEffect == Context.OwningTileEffect.Get() || TileEffect == Context.SourceObject.Get();

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
