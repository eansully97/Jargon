// SpikeTrapTileEffect.cpp

#include "TrapTileEffect.h"

#include "Combat/Effects/JargonEffectResolver.h"
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

	if (Definition->Effects.Num() <= 0)
	{
		return;
	}

	FJargonEffectResult EffectResult;
	const FJargonEffectContext EffectContext = BuildEffectContext(CombatGameMode, EnteringUnit);
	const bool bResolved = FJargonEffectResolver::ResolveEffects(Definition->Effects, EffectContext, EffectResult);
	if (!bResolved)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trap '%s' failed to resolve tile effect definition '%s' for entering unit '%s'."),
			*GetNameSafe(this),
			*GetNameSafe(Definition),
			*GetNameSafe(EnteringUnit));
		return;
	}

	if (!EffectResult.bResolvedAnyEffect)
	{
		return;
	}

	EmitTileEffectCue(EJargonCombatCueType::TileEffectTriggered, EnteringUnit);

	if (Definition->bDestroyAfterUnitEnter && IsValid(this))
	{
		Destroy();
	}
}
