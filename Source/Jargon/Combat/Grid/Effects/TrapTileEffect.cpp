// SpikeTrapTileEffect.cpp

#include "TrapTileEffect.h"

#include "Combat/Effects/JargonEffectExecutor.h"
#include "Combat/Grid/GridTile.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/JargonTileEffectDefinition.h"

void ATrapTileEffect::HandleUnitEnteredTile(AJargonCombatGameMode* CombatGameMode, ABattleUnit* EnteringUnit)
{
	UJargonTileEffectDefinition* Definition = GetTileEffectDefinition();
	if (!Definition || Definition->Trigger != EJargonTileEffectTrigger::OnUnitEnter)
	{
		return;
	}

	if (!EnteringUnit || EnteringUnit->IsDead())
	{
		return;
	}

	AGridTile* TrapTile = GetCurrentTile();
	if (!TrapTile || EnteringUnit->GetCurrentTile() != TrapTile)
	{
		return;
	}

	if (!Definition->TriggerAbility && Definition->Effects.Num() <= 0)
	{
		return;
	}

	const FJargonEffectContext EffectContext = BuildEffectContext(CombatGameMode, EnteringUnit);
	FJargonEffectExecutionReport ExecutionReport;
	if (Definition->TriggerAbility)
	{
		ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
			Definition->TriggerAbility,
			EffectContext,
			FString::Printf(TEXT("%s Trap Tile Effect '%s' OnUnitEnter"), *GetNameSafe(this), *GetNameSafe(Definition)));
	}
	else
	{
		FJargonEffectExecutionRequest ExecutionRequest;
		ExecutionRequest.Effects = &Definition->Effects;
		ExecutionRequest.Context = EffectContext;
		ExecutionRequest.SourceLabel = GetNameSafe(this);
		ExecutionRequest.HookName = FString::Printf(TEXT("Trap Tile Effect '%s' OnUnitEnter"), *GetNameSafe(Definition));
		ExecutionRequest.bLogNoResolvedEffects = true;
		ExecutionReport = FJargonEffectExecutor::Execute(ExecutionRequest);
	}

	if (!ExecutionReport.bResolverSucceeded || !ExecutionReport.Result.bResolvedAnyEffect)
	{
		return;
	}

	EmitTileEffectCue(EJargonCombatCueType::TileEffectTriggered, EnteringUnit);

	if (Definition->bDestroyAfterUnitEnter && IsValid(this))
	{
		Destroy();
	}
}
