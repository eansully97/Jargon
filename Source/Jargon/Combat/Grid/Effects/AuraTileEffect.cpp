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

	if (!Definition->TriggerAbility && Definition->Effects.Num() <= 0)
	{
		return;
	}

	const FJargonEffectContext EffectContext = BuildEffectContext(CombatGameMode);
	FJargonEffectExecutionReport ExecutionReport;
	if (Definition->TriggerAbility)
	{
		ExecutionReport = FJargonEffectExecutor::ExecuteAbility(
			Definition->TriggerAbility,
			EffectContext,
			FString::Printf(TEXT("%s Aura Tile Effect '%s' OnPlayerTurnStart"), *GetNameSafe(this), *GetNameSafe(Definition)));
	}
	else
	{
		FJargonEffectExecutionRequest ExecutionRequest;
		ExecutionRequest.Effects = &Definition->Effects;
		ExecutionRequest.Context = EffectContext;
		ExecutionRequest.SourceLabel = GetNameSafe(this);
		ExecutionRequest.HookName = FString::Printf(TEXT("Aura Tile Effect '%s' OnPlayerTurnStart"), *GetNameSafe(Definition));
		ExecutionRequest.bLogNoResolvedEffects = true;
		ExecutionReport = FJargonEffectExecutor::Execute(ExecutionRequest);
	}

	if (!ExecutionReport.bResolverSucceeded || !ExecutionReport.Result.bResolvedAnyEffect)
	{
		return;
	}

	EmitTileEffectCue(EJargonCombatCueType::TileEffectTriggered);
}
