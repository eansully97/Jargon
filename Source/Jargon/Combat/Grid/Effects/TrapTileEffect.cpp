// SpikeTrapTileEffect.cpp

#include "TrapTileEffect.h"

#include "Combat/Effects/JargonEffectResolver.h"
#include "Combat/Grid/GridTile.h"
#include "Combat/Units/BattleUnit.h"

void ATrapTileEffect::HandleUnitEnteredTile(AJargonCombatGameMode* CombatGameMode, ABattleUnit* EnteringUnit)
{
	if (GetCardCategory() != ECardCategory::Trap)
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

	if (TriggeredEffects.Num() > 0)
	{
		EmitTileEffectCue(EJargonCombatCueType::TileEffectTriggered, EnteringUnit);

		FJargonEffectResult EffectResult;
		const FJargonEffectContext EffectContext = BuildEffectContext(CombatGameMode, EnteringUnit);
		const bool bResolved = FJargonEffectResolver::ResolveEffects(TriggeredEffects, EffectContext, EffectResult);
		if (!bResolved)
		{
			UE_LOG(LogTemp, Warning, TEXT("Trap '%s' failed to resolve its generic triggered effects for entering unit '%s'."),
				*GetNameSafe(this),
				*GetNameSafe(EnteringUnit));
			return;
		}

		if (bTriggerOnce && IsValid(this))
		{
			Destroy();
		}

		return;
	}

	if (EnteringUnit->GetTeam() == GetSourceTeam())
	{
		return;
	}

	EmitTileEffectCue(EJargonCombatCueType::TileEffectTriggered, EnteringUnit);

	const int32 AuthoredTrapDamage = GetEffectValue();
	const int32 TrapDamage = AuthoredTrapDamage > 0
		? AuthoredTrapDamage
		: FMath::Max(0, DefaultTrapDamage);

	if (TrapDamage <= 0)
	{
		return;
	}

	EnteringUnit->ApplyDamage(TrapDamage);

	UE_LOG(LogTemp, Log, TEXT("Trap '%s' deals %d damage to '%s' and is consumed."),
		*GetNameSafe(this),
		TrapDamage,
		*GetNameSafe(EnteringUnit));

	Destroy();
}
