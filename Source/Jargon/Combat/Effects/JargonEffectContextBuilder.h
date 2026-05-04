#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"

class ABattleTileEffect;
class ABattleUnit;
class AGridTile;
class AJargonCombatGameMode;
class UCardDefinition;

class JARGON_API FJargonEffectContextBuilder
{
public:
	static FJargonEffectContext BuildForCard(
		AJargonCombatGameMode* GameMode,
		UCardDefinition* SourceCard,
		ABattleUnit* SourceUnit,
		ABattleUnit* PrimaryUnitTarget,
		AGridTile* PrimaryTileTarget);

	static FJargonEffectContext BuildForUnit(
		AJargonCombatGameMode* GameMode,
		ABattleUnit* SourceUnit,
		EJargonEffectTrigger Trigger,
		ABattleUnit* PrimaryUnitTarget = nullptr,
		AGridTile* PrimaryTileTarget = nullptr,
		UObject* SourceObject = nullptr);

	static FJargonEffectContext BuildForTileEffect(
		AJargonCombatGameMode* GameMode,
		ABattleTileEffect* SourceTileEffect,
		EJargonEffectTrigger Trigger,
		ABattleUnit* TriggeringUnit = nullptr);

	static FJargonEffectContext BuildForArtifact(
		AJargonCombatGameMode* GameMode,
		UObject* SourceArtifact,
		EJargonEffectTrigger Trigger,
		ABattleUnit* SourceUnit,
		ABattleUnit* PrimaryUnitTarget,
		AGridTile* PrimaryTileTarget,
		ABattleUnit* TriggeringUnit = nullptr);
};
