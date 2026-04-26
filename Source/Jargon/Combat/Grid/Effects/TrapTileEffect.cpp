// SpikeTrapTileEffect.cpp

#include "TrapTileEffect.h"

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

	if (EnteringUnit->GetTeam() == GetSourceTeam())
	{
		return;
	}

	AGridTile* TrapTile = GetCurrentTile();
	if (!TrapTile || EnteringUnit->GetCurrentTile() != TrapTile)
	{
		return;
	}

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
