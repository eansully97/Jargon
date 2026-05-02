#include "AuraTileEffect.h"

#include "Combat/Effects/JargonEffectResolver.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/JargonTileEffectDefinition.h"

void AAuraTileEffect::HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode)
{
	UJargonTileEffectDefinition* Definition = GetTileEffectDefinition();
	if (!Definition || Definition->Trigger != EJargonTileEffectTrigger::OnPlayerTurnStart)
	{
		return;
	}

	if (Definition->Effects.Num() <= 0)
	{
		return;
	}

	FJargonEffectResult EffectResult;
	const FJargonEffectContext EffectContext = BuildEffectContext(CombatGameMode);
	const bool bResolved = FJargonEffectResolver::ResolveEffects(Definition->Effects, EffectContext, EffectResult);
	if (!bResolved)
	{
		UE_LOG(LogTemp, Warning, TEXT("Aura '%s' failed to resolve tile effect definition '%s'."),
			*GetNameSafe(this),
			*GetNameSafe(Definition));
		return;
	}

	if (!EffectResult.bResolvedAnyEffect)
	{
		return;
	}

	EmitTileEffectCue(EJargonCombatCueType::TileEffectTriggered);
}
