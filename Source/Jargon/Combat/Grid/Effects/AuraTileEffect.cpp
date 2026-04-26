#include "AuraTileEffect.h"

#include "Combat/Units/BattleUnit.h"

void AAuraTileEffect::HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode)
{
	if (!bApplyOnPlayerTurnStart)
	{
		return;
	}

	const int32 EffectAmount = ResolveEffectAmount(DefaultEffectValue);
	if (EffectAmount <= 0)
	{
		return;
	}

	int32 AffectedUnitCount = 0;

	const TArray<ABattleUnit*> UnitsInAura = GetLivingUnitsInEffectRadius(CombatGameMode);
	for (ABattleUnit* Unit : UnitsInAura)
	{
		if (!DoesUnitPassTargetFilter(Unit, TargetFilter))
		{
			continue;
		}

		if (ApplyConfiguredOperationToUnit(Unit, AuraOperation, EffectAmount))
		{
			AffectedUnitCount++;
		}
	}

	if (AffectedUnitCount <= 0)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Aura '%s' applied operation %d with value %d to %d unit(s) within radius %d."),
		*GetNameSafe(this),
		static_cast<int32>(AuraOperation),
		EffectAmount,
		AffectedUnitCount,
		GetEffectRadius());
}