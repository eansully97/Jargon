#include "AuraTileEffect.h"

#include "Combat/Effects/JargonEffectExecutor.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/JargonTileEffectDefinition.h"

void AAuraTileEffect::HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode)
{
	UJargonTileEffectDefinition* Definition = GetTileEffectDefinition();
	if (!Definition || Definition->Trigger != EJargonTileEffectTrigger::OnPlayerTurnStart)
	{
		return;
	}

	if (!Definition->TriggerAbility)
	{
		return;
	}

	const FJargonEffectContext EffectContext = BuildEffectContext(CombatGameMode);
	const FJargonEffectExecutionReport ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
		Definition->TriggerAbility,
		EffectContext,
		FString::Printf(TEXT("%s Aura Tile Effect '%s' OnPlayerTurnStart"), *GetNameSafe(this), *GetNameSafe(Definition)),
		nullptr,
		EJargonAbilityHookContextType::AuraPlayerTurnStart);

	if (!ExecutionReport.bResolverSucceeded || !ExecutionReport.Result.bResolvedAnyEffect)
	{
		return;
	}

	if (!IsValid(this))
	{
		return;
	}

	EmitTileEffectCue(EJargonCombatCueType::TileEffectTriggered);
}
