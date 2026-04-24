// AuraShieldTileEffect.cpp

#include "Grid/AuraShieldTileEffect.h"

#include "Combat/JargonCombatGameMode.h"
#include "Data/CardDefinition.h"
#include "Grid/GridTile.h"
#include "Units/BattleUnit.h"

void AAuraShieldTileEffect::HandlePlayerTurnStart(AJargonCombatGameMode* CombatGameMode)
{
	if (GetCardCategory() != ECardCategory::Aura)
	{
		return;
	}

	const UCardDefinition* EffectSourceCard = GetSourceCard();
	const int32 ShieldAmount = EffectSourceCard
		? FMath::Max(0, EffectSourceCard->Value)
		: FMath::Max(0, DefaultShieldAmount);

	if (ShieldAmount <= 0)
	{
		return;
	}

	int32 AffectedUnitCount = 0;
	const TArray<AGridTile*> TilesInAura = GetTilesInEffectRadius(CombatGameMode);
	for (AGridTile* AffectedTile : TilesInAura)
	{
		if (!AffectedTile)
		{
			continue;
		}

		ABattleUnit* OccupyingUnit = AffectedTile->GetOccupyingUnit();
		if (!OccupyingUnit || OccupyingUnit->IsDead())
		{
			continue;
		}

		if (OccupyingUnit->GetTeam() != GetSourceTeam())
		{
			continue;
		}

		OccupyingUnit->AddTemporaryShield(ShieldAmount);
		AffectedUnitCount++;
	}

	if (AffectedUnitCount <= 0)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Aura '%s' grants %d temporary shield to %d friendly unit(s) within radius %d."),
		*GetNameSafe(this),
		ShieldAmount,
		AffectedUnitCount,
		GetEffectRadius());
}
