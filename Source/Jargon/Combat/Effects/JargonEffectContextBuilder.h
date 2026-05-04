#pragma once

#include "CoreMinimal.h"
#include "Combat/Effects/JargonEffectTypes.h"

class ABattleTileEffect;
class ABattleUnit;
class AGridTile;
class AJargonCombatGameMode;
class UCardDefinition;

/** Factory helpers for the runtime context roles consumed by FJargonEffectExecutor and FJargonEffectResolver. */
class JARGON_API FJargonEffectContextBuilder
{
public:
	/** Builds the card-play context after the player has selected any required unit/tile target. */
	static FJargonEffectContext BuildForCard(
		AJargonCombatGameMode* GameMode,
		UCardDefinition* SourceCard,
		ABattleUnit* SourceUnit,
		ABattleUnit* PrimaryUnitTarget,
		AGridTile* PrimaryTileTarget);

	/** Builds a unit-owned hook context, such as summon enter, turn start, or death effects. */
	static FJargonEffectContext BuildForUnit(
		AJargonCombatGameMode* GameMode,
		ABattleUnit* SourceUnit,
		EJargonEffectTrigger Trigger,
		ABattleUnit* PrimaryUnitTarget = nullptr,
		AGridTile* PrimaryTileTarget = nullptr,
		UObject* SourceObject = nullptr);

	/** Builds a trap/aura context, including the owning tile effect and optional entering unit. */
	static FJargonEffectContext BuildForTileEffect(
		AJargonCombatGameMode* GameMode,
		ABattleTileEffect* SourceTileEffect,
		EJargonEffectTrigger Trigger,
		ABattleUnit* TriggeringUnit = nullptr);

	/** Builds a run Artifact context, preserving both the Artifact source object and combat unit roles. */
	static FJargonEffectContext BuildForArtifact(
		AJargonCombatGameMode* GameMode,
		UObject* SourceArtifact,
		EJargonEffectTrigger Trigger,
		ABattleUnit* SourceUnit,
		ABattleUnit* PrimaryUnitTarget,
		AGridTile* PrimaryTileTarget,
		ABattleUnit* TriggeringUnit = nullptr);
};
