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

	if (!Definition->TriggerAbility)
	{
		return;
	}

	const FJargonEffectContext EffectContext = BuildEffectContext(CombatGameMode, EnteringUnit);
	const FJargonEffectExecutionReport ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
		Definition->TriggerAbility,
		EffectContext,
		FString::Printf(TEXT("%s Trap Tile Effect '%s' OnUnitEnter"), *GetNameSafe(this), *GetNameSafe(Definition)),
		nullptr,
		EJargonAbilityHookContextType::TrapUnitEnter);

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
