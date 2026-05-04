#include "Combat/Effects/JargonEffectResolver.h"

#include "Combat/JargonCombatGameMode.h"
#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Grid/GridBoard.h"
#include "Combat/Grid/GridTile.h"
#include "Combat/Presentation/JargonCombatCueTypes.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/CardDefinition.h"
#include "Data/JargonArtifactDefinition.h"
#include "Data/JargonStatusEffectDefinition.h"
#include "Data/JargonSummonedUnitDefinition.h"
#include "Data/JargonTileEffectDefinition.h"

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
			|| Operation == EJargonEffectOperation::ApplyStun
			|| Operation == EJargonEffectOperation::ApplyFreeze
			|| Operation == EJargonEffectOperation::ApplyBurn
			|| Operation == EJargonEffectOperation::ApplyRoot
			|| Operation == EJargonEffectOperation::ApplyVulnerable
			|| Operation == EJargonEffectOperation::ApplyStatus
			|| Operation == EJargonEffectOperation::CleanseStatus;
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
		Cue.SourceArtifact = Cast<UJargonArtifactDefinition>(Context.SourceObject.Get());
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
		case EJargonEffectOperation::ApplyFreeze:
			return TEXT("ApplyFreeze");
		case EJargonEffectOperation::ApplyBurn:
			return TEXT("ApplyBurn");
		case EJargonEffectOperation::ApplyRoot:
			return TEXT("ApplyRoot");
		case EJargonEffectOperation::ApplyVulnerable:
			return TEXT("ApplyVulnerable");
		case EJargonEffectOperation::ApplyStatus:
			return TEXT("ApplyStatus");
		case EJargonEffectOperation::CleanseStatus:
			return TEXT("CleanseStatus");
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
		case EJargonEffectOperation::GainElementCharge:
			return TEXT("GainElementCharge");
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

	AGridTile* GetTraceSourceTile(const FJargonEffectContext& Context)
	{
		if (Context.SourceTile)
		{
			return Context.SourceTile.Get();
		}

		return Context.SourceUnit ? Context.SourceUnit->GetCurrentTile() : nullptr;
	}

	void PopulateTraceEventFromContext(
		FJargonEffectTraceEvent& Event,
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		EJargonEffectTraceEventType EventType,
		int32 EffectIndex,
		const TCHAR* Reason = TEXT(""),
		const TCHAR* Warning = TEXT(""))
	{
		Event.EffectIndex = EffectIndex;
		Event.EventType = EventType;
		Event.Operation = EffectSpec.Operation;
		Event.Delivery = EffectSpec.Delivery;
		Event.TargetFilter = EffectSpec.TargetFilter;
		Event.SourceObject = Context.SourceObject;
		Event.SourceCard = Context.SourceCard;
		Event.SourceUnit = Context.SourceUnit;
		Event.SourceTile = GetTraceSourceTile(Context);
		Event.ExplicitUnitTarget = Context.PrimaryUnitTarget;
		Event.ExplicitTileTarget = Context.PrimaryTileTarget;
		Event.Value = EffectSpec.Value;
		Event.ElementType = EffectSpec.ElementType;
		Event.PayloadSummary = JargonEffectContracts::BuildPayloadSummary(EffectSpec);
		Event.Reason = Reason;
		Event.Warning = Warning;
	}

	void AddTraceEvent(
		FJargonEffectTrace* OutTrace,
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		EJargonEffectTraceEventType EventType,
		int32 EffectIndex,
		const TCHAR* Reason = TEXT(""),
		const TCHAR* Warning = TEXT(""))
	{
		if (!OutTrace)
		{
			return;
		}

		FJargonEffectTraceEvent Event;
		PopulateTraceEventFromContext(Event, EffectSpec, Context, EventType, EffectIndex, Reason, Warning);
		OutTrace->AddEvent(Event);
	}

	void AddTraceOperationResult(
		FJargonEffectTrace* OutTrace,
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		int32 EffectIndex,
		bool bResolved,
		const FJargonEffectResult& EffectResult,
		const TCHAR* Reason = TEXT(""))
	{
		if (!OutTrace)
		{
			return;
		}

		FJargonEffectTraceEvent Event;
		PopulateTraceEventFromContext(
			Event,
			EffectSpec,
			Context,
			bResolved ? EJargonEffectTraceEventType::OperationApplied : EJargonEffectTraceEventType::OperationFailed,
			EffectIndex,
			bResolved ? Reason : (FCString::Strlen(Reason) > 0 ? Reason : TEXT("OperationFailed")),
			bResolved ? TEXT("") : TEXT("Operation failed to resolve."));
		Event.bOperationSucceeded = bResolved;
		Event.bResolvedAnyEffect = EffectResult.bResolvedAnyEffect;
		Event.bContinuesAsynchronously = EffectResult.bContinuesAsynchronously;
		Event.SpawnedUnit = EffectResult.SpawnedUnit;
		Event.SpawnedTileEffect = EffectResult.SpawnedTileEffect;
		Event.EnergyGain = EffectResult.EnergyGainAfterCost;
		if (bResolved && EffectSpec.Operation == EJargonEffectOperation::GainElementCharge)
		{
			Event.ElementChargeDelta = FMath::Max(0, EffectSpec.Value);
		}
		OutTrace->AddEvent(Event);
	}

	void AddTraceUnitsGathered(
		FJargonEffectTrace* OutTrace,
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		int32 EffectIndex,
		const TArray<ABattleUnit*>& TargetUnits)
	{
		if (!OutTrace)
		{
			return;
		}

		FJargonEffectTraceEvent Event;
		PopulateTraceEventFromContext(Event, EffectSpec, Context, EJargonEffectTraceEventType::TargetsGathered, EffectIndex, TEXT("TargetsGathered"));
		for (ABattleUnit* TargetUnit : TargetUnits)
		{
			Event.ResolvedUnitTargets.Add(TargetUnit);
		}
		OutTrace->AddEvent(Event);
	}

	void AddTraceTilesGathered(
		FJargonEffectTrace* OutTrace,
		const FJargonEffectSpec& EffectSpec,
		const FJargonEffectContext& Context,
		int32 EffectIndex,
		const TArray<AGridTile*>& TargetTiles)
	{
		if (!OutTrace)
		{
			return;
		}

		FJargonEffectTraceEvent Event;
		PopulateTraceEventFromContext(Event, EffectSpec, Context, EJargonEffectTraceEventType::TargetsGathered, EffectIndex, TEXT("TargetsGathered"));
		for (AGridTile* TargetTile : TargetTiles)
		{
			Event.ResolvedTileTargets.Add(TargetTile);
		}
		OutTrace->AddEvent(Event);
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
		if (JargonEffectContracts::RequiresValue(EffectSpec.Operation) && EffectSpec.Value <= 0)
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
					EffectSpec.Operation == EJargonEffectOperation::ApplyShield ||
					EffectSpec.Operation == EJargonEffectOperation::CleanseStatus) &&
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
				EffectSpec.Operation == EJargonEffectOperation::ApplyShield ||
				EffectSpec.Operation == EJargonEffectOperation::CleanseStatus) &&
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
	case EJargonEffectOperation::ApplyStatus:
		if (!EffectSpec.StatusEffectDefinition)
		{
			UE_LOG(LogTemp, Warning, TEXT("ApplyStatus requires StatusEffectDefinition."));
			return false;
		}
		if (!EffectSpec.StatusEffectDefinition->IsValidDefinition())
		{
			UE_LOG(LogTemp, Warning, TEXT("ApplyStatus requires a valid StatusEffectDefinition. Definition='%s'."),
				*GetPathNameSafe(EffectSpec.StatusEffectDefinition.Get()));
			return false;
		}
		break;

	case EJargonEffectOperation::CleanseStatus:
		if (EffectSpec.StatusEffectDefinition && !EffectSpec.StatusEffectDefinition->IsValidDefinition())
		{
			UE_LOG(LogTemp, Warning, TEXT("CleanseStatus references an invalid StatusEffectDefinition. Definition='%s'."),
				*GetPathNameSafe(EffectSpec.StatusEffectDefinition.Get()));
			return false;
		}
		break;

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
		if (!IsLiveSourceUnit(Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("PullTarget requires a living SourceUnit."));
			return false;
		}
		if (EffectSpec.PullDistance <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("PullTarget requires PullDistance > 0."));
			return false;
		}
		break;

	case EJargonEffectOperation::SummonUnit:
		if (!EffectSpec.SummonedUnitDefinition)
		{
			UE_LOG(LogTemp, Warning, TEXT("SummonUnit effect requires SummonedUnitDefinition."));
			return false;
		}
		if (!EffectSpec.RuntimeSummonedUnitClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("SummonUnit effect requires RuntimeSummonedUnitClass."));
			return false;
		}
		if (!ResolvePlacementTile(EffectSpec, Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("SummonUnit could not resolve placement tile. Placement=%s."),
				*JargonEffectContracts::GetPlacementAnchorName(EffectSpec.PlacementAnchor));
			return false;
		}
		break;

	case EJargonEffectOperation::PlaceTileEffect:
		if (!EffectSpec.TileEffectDefinition)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect requires TileEffectDefinition."));
			return false;
		}
		if (!EffectSpec.RuntimeTileEffectClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect requires RuntimeTileEffectClass."));
			return false;
		}
		if (!ResolvePlacementTile(EffectSpec, Context))
		{
			UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect could not resolve placement tile. Placement=%s."),
				*JargonEffectContracts::GetPlacementAnchorName(EffectSpec.PlacementAnchor));
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

	case EJargonEffectOperation::GainElementCharge:
		if (EffectSpec.ElementType == EJargonElementType::None)
		{
			UE_LOG(LogTemp, Warning, TEXT("GainElementCharge requires an ElementType other than None."));
			return false;
		}
		if (EffectSpec.Value <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("GainElementCharge requires Value > 0."));
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
	return ResolveEffects(Effects, Context, OutResult, nullptr);
}

bool FJargonEffectResolver::ResolveEffects(
	const TArray<FJargonEffectSpec>& Effects,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult,
	FJargonEffectTrace* OutTrace)
{
	if (Effects.Num() <= 0)
	{
		if (OutTrace)
		{
			OutTrace->ResetForContext(Context);
			AddTraceEvent(OutTrace, FJargonEffectSpec(), Context, EJargonEffectTraceEventType::ResolveStarted, INDEX_NONE, TEXT("NoEffects"));
			AddTraceEvent(OutTrace, FJargonEffectSpec(), Context, EJargonEffectTraceEventType::ValidationFailed, INDEX_NONE, TEXT("NoEffects"), TEXT("JargonEffectResolver received no effects to resolve."));
			AddTraceEvent(OutTrace, FJargonEffectSpec(), Context, EJargonEffectTraceEventType::ResolveFinished, INDEX_NONE, TEXT("NoEffects"));
			OutTrace->bResolved = false;
			OutTrace->Summary = OutTrace->ToCompactString();
		}
		UE_LOG(LogTemp, Warning, TEXT("JargonEffectResolver received no effects to resolve."));
		return false;
	}

	if (OutTrace)
	{
		OutTrace->ResetForContext(Context);
		AddTraceEvent(OutTrace, Effects[0], Context, EJargonEffectTraceEventType::ResolveStarted, INDEX_NONE, TEXT("ResolveStarted"));
	}

	if (!Context.GameMode)
	{
		if (OutTrace)
		{
			AddTraceEvent(OutTrace, Effects[0], Context, EJargonEffectTraceEventType::ValidationFailed, INDEX_NONE, TEXT("MissingGameMode"), TEXT("JargonEffectResolver cannot resolve effects without a combat game mode."));
			AddTraceEvent(OutTrace, Effects[0], Context, EJargonEffectTraceEventType::ResolveFinished, INDEX_NONE, TEXT("MissingGameMode"));
			OutTrace->bResolved = false;
			OutTrace->Summary = OutTrace->ToCompactString();
		}
		UE_LOG(LogTemp, Warning, TEXT("JargonEffectResolver cannot resolve effects without a combat game mode."));
		return false;
	}

	bool bResolvedAnyEffect = false;
	bool bResolvedAnySpecOrNoOp = false;

	for (int32 EffectIndex = 0; EffectIndex < Effects.Num(); ++EffectIndex)
	{
		FJargonEffectResult EffectResult;
		if (!ResolveEffect(Effects[EffectIndex], Context, EffectResult, OutTrace, EffectIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("Jargon effect spec at index %d failed to resolve."), EffectIndex);
			continue;
		}

		bResolvedAnySpecOrNoOp = true;
		if (EffectResult.bResolvedAnyEffect)
		{
			bResolvedAnyEffect = true;
			OutResult.bResolvedAnyEffect = true;
		}
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
				AddTraceEvent(OutTrace, Effects[EffectIndex], Context, EJargonEffectTraceEventType::EffectsSkipped, EffectIndex, TEXT("AsyncSkippedLaterEffects"), TEXT("Async effect skipped later effect specs for this resolve pass."));
				UE_LOG(LogTemp, Warning, TEXT("Async Jargon effect at index %d started before later effect specs. Later effects are skipped for this resolve pass."), EffectIndex);
			}
			break;
		}
	}

	const bool bResolved = bResolvedAnyEffect || bResolvedAnySpecOrNoOp;
	if (OutTrace)
	{
		OutTrace->bResolved = bResolved;
		OutTrace->bResolvedAnyEffect = bResolvedAnyEffect;
		OutTrace->bContinuedAsynchronously = OutResult.bContinuesAsynchronously;
		AddTraceEvent(OutTrace, Effects[0], Context, EJargonEffectTraceEventType::ResolveFinished, INDEX_NONE, bResolved ? TEXT("Resolved") : TEXT("OperationFailed"));
		OutTrace->Summary = OutTrace->ToCompactString();
	}
	return bResolved;
}

bool FJargonEffectResolver::ResolveEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	return ResolveEffect(EffectSpec, Context, OutResult, nullptr, INDEX_NONE);
}

bool FJargonEffectResolver::ResolveEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult,
	FJargonEffectTrace* OutTrace,
	int32 EffectIndex)
{
	AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::ResolveStarted, EffectIndex, TEXT("ResolveStarted"));

	if (!Context.GameMode)
	{
		AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::ValidationFailed, EffectIndex, TEXT("MissingGameMode"), TEXT("Effect cannot resolve without a combat game mode."));
		AddTraceOperationResult(OutTrace, EffectSpec, Context, EffectIndex, false, OutResult, TEXT("MissingGameMode"));
		return false;
	}

	if (!ValidateEffectForContext(EffectSpec, Context))
	{
		AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::ValidationFailed, EffectIndex, TEXT("ValidationFailed"), TEXT("ValidateEffectForContext returned false."));
		AddTraceOperationResult(OutTrace, EffectSpec, Context, EffectIndex, false, OutResult, TEXT("ValidationFailed"));
		return false;
	}

	bool bResolved = false;
	if (IsUnitPayloadOperation(EffectSpec.Operation))
	{
		bResolved = ResolveUnitPayloadEffect(EffectSpec, Context, OutResult, OutTrace, EffectIndex);
		AddTraceOperationResult(OutTrace, EffectSpec, Context, EffectIndex, bResolved, OutResult);
		if (bResolved && OutResult.bContinuesAsynchronously)
		{
			AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::AsyncStarted, EffectIndex, TEXT("AsyncStarted"));
		}
		return bResolved;
	}

	switch (EffectSpec.Operation)
	{
	case EJargonEffectOperation::MoveSource:
		bResolved = ResolveMoveSourceEffect(EffectSpec, Context, OutResult);
		break;

	case EJargonEffectOperation::PushTarget:
		bResolved = ResolvePushTargetEffect(EffectSpec, Context, OutResult, OutTrace, EffectIndex);
		break;

	case EJargonEffectOperation::PullTarget:
		bResolved = ResolvePullTargetEffect(EffectSpec, Context, OutResult, OutTrace, EffectIndex);
		break;

	case EJargonEffectOperation::SummonUnit:
		bResolved = ResolveSummonUnitEffect(EffectSpec, Context, OutResult, OutTrace, EffectIndex);
		break;

	case EJargonEffectOperation::PlaceTileEffect:
		bResolved = ResolvePlaceTileEffect(EffectSpec, Context, OutResult);
		break;

	case EJargonEffectOperation::DestroyTileEffect:
		bResolved = ResolveDestroyTileEffect(EffectSpec, Context, OutResult, OutTrace, EffectIndex);
		break;

	case EJargonEffectOperation::DrawCards:
		bResolved = ResolveDrawCardsEffect(EffectSpec, Context, OutResult);
		break;

	case EJargonEffectOperation::GainEnergy:
		bResolved = ResolveGainEnergyEffect(EffectSpec, Context, OutResult);
		break;

	case EJargonEffectOperation::GainElementCharge:
		bResolved = ResolveGainElementChargeEffect(EffectSpec, Context, OutResult);
		break;

	case EJargonEffectOperation::None:
	default:
		UE_LOG(LogTemp, Warning, TEXT("Unsupported or empty Jargon effect operation: %d."), static_cast<int32>(EffectSpec.Operation));
		bResolved = false;
		break;
	}

	AddTraceOperationResult(OutTrace, EffectSpec, Context, EffectIndex, bResolved, OutResult);
	if (bResolved && OutResult.bContinuesAsynchronously)
	{
		AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::AsyncStarted, EffectIndex, TEXT("AsyncStarted"));
	}
	return bResolved;
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
	FJargonEffectResult& OutResult,
	FJargonEffectTrace* OutTrace,
	int32 EffectIndex)
{
	if (!Context.GameMode || !IsLiveSourceUnit(Context))
	{
		UE_LOG(LogTemp, Warning, TEXT("PushTarget requires GameMode and a living SourceUnit."));
		return false;
	}

	const TArray<ABattleUnit*> TargetUnits = GatherTargetUnits(EffectSpec, Context, OutTrace, EffectIndex);
	AddTraceUnitsGathered(OutTrace, EffectSpec, Context, EffectIndex, TargetUnits);
	if (TargetUnits.Num() <= 0)
	{
		if (CanTreatNoTargetsAsNoOp(Context))
		{
			AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::NoTargetsNoOp, EffectIndex, TEXT("NoTargetsCleanNoOp"));
			UE_LOG(LogTemp, Verbose, TEXT("PushTarget found no valid target units in %s context; treating as a clean no-op."),
				GetEffectTriggerName(Context.Trigger));
			OutResult.bResolvedAnyEffect = false;
			return true;
		}

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
			TargetUnit->ApplyDamageFromEffectContext(CollisionDamage, Context);
			bResolvedAnyPushEffect = true;
		}
	}

	OutResult.bResolvedAnyEffect = bResolvedAnyPushEffect;
	if (!bResolvedAnyPushEffect && CanTreatNoTargetsAsNoOp(Context))
	{
		return true;
	}
	return bResolvedAnyPushEffect;
}

bool FJargonEffectResolver::ResolvePullTargetEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult,
	FJargonEffectTrace* OutTrace,
	int32 EffectIndex)
{
	if (!Context.GameMode || !IsLiveSourceUnit(Context))
	{
		UE_LOG(LogTemp, Warning, TEXT("PullTarget requires GameMode and a living SourceUnit."));
		return false;
	}

	const TArray<ABattleUnit*> TargetUnits = GatherTargetUnits(EffectSpec, Context, OutTrace, EffectIndex);
	AddTraceUnitsGathered(OutTrace, EffectSpec, Context, EffectIndex, TargetUnits);
	if (TargetUnits.Num() <= 0)
	{
		if (CanTreatNoTargetsAsNoOp(Context))
		{
			AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::NoTargetsNoOp, EffectIndex, TEXT("NoTargetsCleanNoOp"));
			UE_LOG(LogTemp, Verbose, TEXT("PullTarget found no valid target units in %s context; treating as a clean no-op."),
				GetEffectTriggerName(Context.Trigger));
			OutResult.bResolvedAnyEffect = false;
			return true;
		}

		UE_LOG(LogTemp, Warning, TEXT("PullTarget found no valid target units."));
		return false;
	}

	AGridBoard* GridBoard = Context.GameMode->GetGridBoard();
	AGridTile* SourceTile = GetResolvedSourceTile(Context);
	if (!GridBoard || !SourceTile)
	{
		return false;
	}

	const int32 PullDistance = FMath::Max(1, EffectSpec.PullDistance);
	bool bResolvedAnyPullEffect = false;

	for (ABattleUnit* TargetUnit : TargetUnits)
	{
		if (!IsValid(TargetUnit) || TargetUnit->IsDead())
		{
			continue;
		}

		AGridTile* TargetTile = TargetUnit->GetCurrentTile();
		if (!TargetTile || TargetTile == SourceTile)
		{
			continue;
		}

		const TArray<AGridTile*> PathToSource = GridBoard->BuildPath(TargetTile, SourceTile);
		if (PathToSource.Num() < 3)
		{
			continue;
		}

		const int32 LastLegalPathIndex = PathToSource.Num() - 2;
		const int32 DestinationIndex = FMath::Clamp(PullDistance, 1, LastLegalPathIndex);
		AGridTile* BestDestination = PathToSource.IsValidIndex(DestinationIndex)
			? PathToSource[DestinationIndex]
			: nullptr;

		if (!BestDestination || !BestDestination->IsWalkable() || BestDestination == TargetTile || BestDestination == SourceTile)
		{
			continue;
		}

		EmitEffectResolverCue(Context, EffectSpec, EJargonCombatCueType::Pull, TargetUnit, BestDestination, PullDistance);
		TargetUnit->PlaceOnTile(BestDestination);
		bResolvedAnyPullEffect = true;
	}

	OutResult.bResolvedAnyEffect = bResolvedAnyPullEffect;
	if (!bResolvedAnyPullEffect && CanTreatNoTargetsAsNoOp(Context))
	{
		return true;
	}
	return bResolvedAnyPullEffect;
}

bool FJargonEffectResolver::ResolveSummonUnitEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult,
	FJargonEffectTrace* OutTrace,
	int32 EffectIndex)
{
	if (!Context.GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonUnit requires GameMode."));
		return false;
	}

	AGridTile* TargetTile = ResolvePlacementTile(EffectSpec, Context);
	if (!TargetTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonUnit could not resolve placement tile. Placement=%s."),
			*JargonEffectContracts::GetPlacementAnchorName(EffectSpec.PlacementAnchor));
		return false;
	}

	if (!EffectSpec.SummonedUnitDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonUnit effect requires SummonedUnitDefinition."));
		return false;
	}

	if (!EffectSpec.RuntimeSummonedUnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SummonUnit effect requires RuntimeSummonedUnitClass."));
		return false;
	}

	ABattleUnit* SpawnedUnit = Context.GameMode->SpawnSummonedUnitFromDefinition(
		EffectSpec.SummonedUnitDefinition.Get(),
		EffectSpec.RuntimeSummonedUnitClass,
		Context.SourceUnit.Get(),
		TargetTile,
		EffectSpec.bSummonEntersWithAttackExhausted,
		true);

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

	AGridTile* TargetTile = ResolvePlacementTile(EffectSpec, Context);
	if (!TargetTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect could not resolve placement tile. Placement=%s."),
			*JargonEffectContracts::GetPlacementAnchorName(EffectSpec.PlacementAnchor));
		return false;
	}

	if (!EffectSpec.TileEffectDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect requires TileEffectDefinition."));
		return false;
	}

	if (!EffectSpec.RuntimeTileEffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlaceTileEffect requires RuntimeTileEffectClass."));
		return false;
	}

	ABattleTileEffect* SpawnedEffect = nullptr;
	if (IsLiveSourceUnit(Context))
	{
		SpawnedEffect = Context.GameMode->SpawnPersistentTileEffectFromDefinition(
			EffectSpec.TileEffectDefinition.Get(),
			EffectSpec.RuntimeTileEffectClass,
			Context.SourceCard,
			Context.SourceUnit.Get(),
			TargetTile);
	}
	else
	{
		SpawnedEffect = Context.GameMode->SpawnPersistentTileEffectFromDefinitionForTeam(
			EffectSpec.TileEffectDefinition.Get(),
			EffectSpec.RuntimeTileEffectClass,
			Context.SourceCard,
			GetResolvedSourceTeam(Context),
			TargetTile);
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
	FJargonEffectResult& OutResult,
	FJargonEffectTrace* OutTrace,
	int32 EffectIndex)
{
	if (!Context.GameMode)
	{
		return false;
	}

	const TArray<AGridTile*> TargetTiles = GatherTargetTiles(EffectSpec, Context);
	AddTraceTilesGathered(OutTrace, EffectSpec, Context, EffectIndex, TargetTiles);
	if (TargetTiles.Num() <= 0)
	{
		if (CanTreatNoTargetsAsNoOp(Context))
		{
			AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::NoTargetsNoOp, EffectIndex, TEXT("NoTargetsCleanNoOp"));
			UE_LOG(LogTemp, Verbose, TEXT("DestroyTileEffect found no target tiles in %s context; treating as a clean no-op."),
				GetEffectTriggerName(Context.Trigger));
			OutResult.bResolvedAnyEffect = false;
			return true;
		}

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
	if (!bDestroyedAny && CanTreatNoTargetsAsNoOp(Context))
	{
		return true;
	}
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

bool FJargonEffectResolver::ResolveGainElementChargeEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult)
{
	if (!Context.GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("GainElementCharge requires a combat game mode."));
		return false;
	}

	if (EffectSpec.ElementType == EJargonElementType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GainElementCharge requires an ElementType other than None."));
		return false;
	}

	const int32 ChargeAmount = FMath::Max(0, EffectSpec.Value);
	if (ChargeAmount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GainElementCharge requires Value > 0."));
		return false;
	}

	Context.GameMode->GainElementCharges(EffectSpec.ElementType, ChargeAmount);
	OutResult.bResolvedAnyEffect = true;
	return true;
}

bool FJargonEffectResolver::ResolveUnitPayloadEffect(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectResult& OutResult,
	FJargonEffectTrace* OutTrace,
	int32 EffectIndex)
{
	const int32 Amount = FMath::Max(0, EffectSpec.Value);
	if (JargonEffectContracts::RequiresValue(EffectSpec.Operation) && Amount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unit payload effect %d requires Value > 0."), static_cast<int32>(EffectSpec.Operation));
		return false;
	}

	const TArray<ABattleUnit*> TargetUnits = GatherTargetUnits(EffectSpec, Context, OutTrace, EffectIndex);
	AddTraceUnitsGathered(OutTrace, EffectSpec, Context, EffectIndex, TargetUnits);
	if (TargetUnits.Num() <= 0)
	{
		if (CanTreatNoTargetsAsNoOp(Context))
		{
			AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::NoTargetsNoOp, EffectIndex, TEXT("NoTargetsCleanNoOp"));
			UE_LOG(LogTemp, Verbose, TEXT("Unit payload effect %s found no valid target units in %s context; treating as a clean no-op."),
				GetEffectOperationName(EffectSpec.Operation),
				GetEffectTriggerName(Context.Trigger));
			OutResult.bResolvedAnyEffect = false;
			return true;
		}

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
	if (!bAppliedAny && CanTreatNoTargetsAsNoOp(Context))
	{
		return true;
	}
	return bAppliedAny;
}

TArray<ABattleUnit*> FJargonEffectResolver::GatherTargetUnits(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context,
	FJargonEffectTrace* OutTrace,
	int32 EffectIndex)
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
	{
		const bool bCanFallbackToSource =
			(EffectSpec.Operation == EJargonEffectOperation::Heal ||
				EffectSpec.Operation == EJargonEffectOperation::ApplyShield ||
				EffectSpec.Operation == EJargonEffectOperation::CleanseStatus) &&
			Context.SourceUnit &&
			DoesUnitPassTargetFilter(Context.SourceUnit, EffectSpec, Context);

		if (ABattleUnit* TargetUnit = GetResolvedTargetUnit(Context))
		{
			if (DoesUnitPassTargetFilter(TargetUnit, EffectSpec, Context))
			{
				TargetUnits.Add(TargetUnit);
			}
			else if (bCanFallbackToSource)
			{
				AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::FallbackUsed, EffectIndex, TEXT("ExplicitFriendlyFallbackToSource"), TEXT("Explicit target failed the target filter; friendly effect used the source unit."));
				TargetUnits.Add(Context.SourceUnit);
			}
		}
		else if (bCanFallbackToSource)
		{
			AddTraceEvent(OutTrace, EffectSpec, Context, EJargonEffectTraceEventType::FallbackUsed, EffectIndex, TEXT("ExplicitFriendlyFallbackToSource"), TEXT("No explicit target was resolved; friendly effect used the source unit."));
			TargetUnits.Add(Context.SourceUnit);
		}
		break;
	}

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
	{
		const int32 ActualHealthDamage = TargetUnit->ApplyDamageFromEffectContextAndGetHealthDamage(Amount, Context);
		if (EffectSpec.bLifesteal && ActualHealthDamage > 0 && IsLiveSourceUnit(Context))
		{
			Context.SourceUnit->ApplyHeal(ActualHealthDamage);
			EmitEffectResolverCue(
				Context,
				EffectSpec,
				EJargonCombatCueType::Lifesteal,
				Context.SourceUnit.Get(),
				Context.SourceUnit->GetCurrentTile(),
				ActualHealthDamage);
		}
		return true;
	}

	case EJargonEffectOperation::Heal:
		TargetUnit->ApplyHeal(Amount);
		return true;

	case EJargonEffectOperation::ApplyShield:
		TargetUnit->AddTemporaryShield(Amount);
		return true;

	case EJargonEffectOperation::ApplyStun:
		TargetUnit->ApplyStun(Amount);
		return true;

	case EJargonEffectOperation::ApplyFreeze:
		TargetUnit->ApplyFreeze(Amount);
		return true;

	case EJargonEffectOperation::ApplyBurn:
		TargetUnit->ApplyBurn(Amount);
		return true;

	case EJargonEffectOperation::ApplyRoot:
		TargetUnit->ApplyRoot(Amount);
		return true;

	case EJargonEffectOperation::ApplyVulnerable:
		TargetUnit->ApplyVulnerable(Amount);
		return true;

	case EJargonEffectOperation::ApplyStatus:
		if (!EffectSpec.StatusEffectDefinition)
		{
			return false;
		}

		switch (EffectSpec.StatusEffectDefinition->StatusKind)
		{
		case EJargonStatusEffectKind::Stun:
			TargetUnit->ApplyStun(Amount);
			return true;
		case EJargonStatusEffectKind::Freeze:
			TargetUnit->ApplyFreeze(Amount);
			return true;
		case EJargonStatusEffectKind::Burn:
			TargetUnit->ApplyBurn(Amount);
			return true;
		case EJargonStatusEffectKind::Root:
			TargetUnit->ApplyRoot(Amount);
			return true;
		case EJargonStatusEffectKind::Vulnerable:
			TargetUnit->ApplyVulnerable(Amount);
			return true;
		case EJargonStatusEffectKind::Regen:
			TargetUnit->ApplyRegen(Amount);
			return true;
		case EJargonStatusEffectKind::Weak:
			TargetUnit->ApplyWeak(Amount);
			return true;
		case EJargonStatusEffectKind::None:
		default:
			return false;
		}

	case EJargonEffectOperation::CleanseStatus:
		if (EffectSpec.StatusEffectDefinition)
		{
			TargetUnit->CleanseStatus(EffectSpec.StatusEffectDefinition->StatusKind);
		}
		else
		{
			TargetUnit->CleanseAllNegativeStatuses();
		}
		return true;

	default:
		return false;
	}
}

bool FJargonEffectResolver::CanTreatNoTargetsAsNoOp(const FJargonEffectContext& Context)
{
	switch (Context.Trigger)
	{
	case EJargonEffectTrigger::OnSummoned:
	case EJargonEffectTrigger::OnEnterTile:
	case EJargonEffectTrigger::OnTurnStart:
	case EJargonEffectTrigger::OnDeath:
	case EJargonEffectTrigger::OnCombatStart:
	case EJargonEffectTrigger::OnEnemyDeath:
		return true;

	case EJargonEffectTrigger::OnPlayed:
	case EJargonEffectTrigger::Activated:
	default:
		return false;
	}
}

AGridTile* FJargonEffectResolver::ResolvePlacementTile(
	const FJargonEffectSpec& EffectSpec,
	const FJargonEffectContext& Context)
{
	AGridTile* AnchorTile = ResolvePlacementAnchorTile(EffectSpec.PlacementAnchor, Context);
	if (!AnchorTile)
	{
		return nullptr;
	}

	switch (EffectSpec.PlacementAnchor)
	{
	case EJargonAbilityPlacementAnchor::NearestEmptyToSourceTile:
	case EJargonAbilityPlacementAnchor::NearestEmptyToPrimaryTile:
	case EJargonAbilityPlacementAnchor::NearestEmptyToTriggeringUnit:
	case EJargonAbilityPlacementAnchor::NearestEmptyToOwningTile:
		return FindNearestEmptyWalkableTile(Context, AnchorTile);

	case EJargonAbilityPlacementAnchor::AbilityTargetTile:
	case EJargonAbilityPlacementAnchor::SourceTile:
	case EJargonAbilityPlacementAnchor::PrimaryTile:
	case EJargonAbilityPlacementAnchor::TriggeringUnitTile:
	case EJargonAbilityPlacementAnchor::OwningTileEffectTile:
	default:
		return AnchorTile;
	}
}

AGridTile* FJargonEffectResolver::ResolvePlacementAnchorTile(
	EJargonAbilityPlacementAnchor PlacementAnchor,
	const FJargonEffectContext& Context)
{
	switch (PlacementAnchor)
	{
	case EJargonAbilityPlacementAnchor::AbilityTargetTile:
		return GetResolvedTargetTile(Context);

	case EJargonAbilityPlacementAnchor::SourceTile:
	case EJargonAbilityPlacementAnchor::NearestEmptyToSourceTile:
		return GetResolvedSourceTile(Context);

	case EJargonAbilityPlacementAnchor::PrimaryTile:
	case EJargonAbilityPlacementAnchor::NearestEmptyToPrimaryTile:
		return GetResolvedTargetTile(Context);

	case EJargonAbilityPlacementAnchor::TriggeringUnitTile:
	case EJargonAbilityPlacementAnchor::NearestEmptyToTriggeringUnit:
		return Context.TriggeringUnit ? Context.TriggeringUnit->GetCurrentTile() : nullptr;

	case EJargonAbilityPlacementAnchor::OwningTileEffectTile:
	case EJargonAbilityPlacementAnchor::NearestEmptyToOwningTile:
		return Context.OwningTileEffect ? Context.OwningTileEffect->GetCurrentTile() : nullptr;

	default:
		return nullptr;
	}
}

AGridTile* FJargonEffectResolver::FindNearestEmptyWalkableTile(
	const FJargonEffectContext& Context,
	AGridTile* AnchorTile)
{
	if (!AnchorTile)
	{
		return nullptr;
	}

	if (AnchorTile->IsWalkable())
	{
		return AnchorTile;
	}

	AGridBoard* GridBoard = Context.GameMode ? Context.GameMode->GetGridBoard() : AnchorTile->GetOwningGridBoard();
	if (!GridBoard)
	{
		return nullptr;
	}

	AGridTile* BestTile = nullptr;
	int32 BestDistance = MAX_int32;
	FHexCoord BestCoord;
	bool bHasBestCoord = false;

	const TArray<AGridTile*> CandidateTiles = GridBoard->GetTilesWithinRadius(AnchorTile, 999);
	for (AGridTile* CandidateTile : CandidateTiles)
	{
		if (!CandidateTile || !CandidateTile->IsWalkable())
		{
			continue;
		}

		const FHexCoord CandidateCoord = CandidateTile->GetCoord();
		const int32 CandidateDistance = GridBoard->GetTileDistance(AnchorTile, CandidateTile);
		const bool bBetterDistance = CandidateDistance < BestDistance;
		const bool bSameDistanceEarlierCoord =
			CandidateDistance == BestDistance &&
			(!bHasBestCoord ||
				CandidateCoord.Q < BestCoord.Q ||
				(CandidateCoord.Q == BestCoord.Q && CandidateCoord.R < BestCoord.R));

		if (!BestTile || bBetterDistance || bSameDistanceEarlierCoord)
		{
			BestTile = CandidateTile;
			BestDistance = CandidateDistance;
			BestCoord = CandidateCoord;
			bHasBestCoord = true;
		}
	}

	return BestTile;
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
