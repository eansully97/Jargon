#include "Data/JargonRelicDefinition.h"

bool UJargonRelicDefinition::HasAnyEffects() const
{
	return OnCombatStartEffects.Num() > 0
		|| OnPlayerTurnStartEffects.Num() > 0
		|| OnEnemyDeathEffects.Num() > 0;
}

bool UJargonRelicDefinition::IsValidDefinition() const
{
	return !DisplayName.IsEmpty() && HasAnyEffects();
}
