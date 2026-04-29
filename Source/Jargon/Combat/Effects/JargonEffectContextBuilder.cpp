#include "Combat/Effects/JargonEffectContextBuilder.h"

#include "Combat/Grid/Effects/BattleTileEffect.h"
#include "Combat/Units/BattleUnit.h"
#include "Data/CardDefinition.h"

FJargonEffectContext FJargonEffectContextBuilder::BuildForCard(
	AJargonCombatGameMode* GameMode,
	UCardDefinition* SourceCard,
	ABattleUnit* SourceUnit,
	ABattleUnit* PrimaryUnitTarget,
	AGridTile* PrimaryTileTarget)
{
	FJargonEffectContext Context;
	Context.GameMode = GameMode;
	Context.SourceObject = SourceCard;
	Context.SourceUnit = SourceUnit;
	Context.SourceTeam = SourceUnit ? SourceUnit->GetTeam() : ETeam::Player;
	Context.SourceTile = SourceUnit ? SourceUnit->GetCurrentTile() : nullptr;
	Context.PrimaryUnitTarget = PrimaryUnitTarget;
	Context.PrimaryTileTarget = PrimaryTileTarget;
	Context.TriggeringUnit = nullptr;
	Context.SourceCard = SourceCard;
	Context.Trigger = EJargonEffectTrigger::OnPlayed;
	return Context;
}

FJargonEffectContext FJargonEffectContextBuilder::BuildForUnit(
	AJargonCombatGameMode* GameMode,
	ABattleUnit* SourceUnit,
	EJargonEffectTrigger Trigger,
	ABattleUnit* PrimaryUnitTarget,
	AGridTile* PrimaryTileTarget,
	UObject* SourceObject)
{
	FJargonEffectContext Context;
	Context.GameMode = GameMode;
	Context.SourceObject = SourceObject ? SourceObject : SourceUnit;
	Context.SourceUnit = SourceUnit;
	Context.SourceTeam = SourceUnit ? SourceUnit->GetTeam() : ETeam::Player;
	Context.SourceTile = PrimaryTileTarget ? PrimaryTileTarget : (SourceUnit ? SourceUnit->GetCurrentTile() : nullptr);
	Context.PrimaryUnitTarget = PrimaryUnitTarget;
	Context.PrimaryTileTarget = PrimaryTileTarget ? PrimaryTileTarget : Context.SourceTile.Get();
	Context.TriggeringUnit = SourceUnit;
	Context.Trigger = Trigger;
	return Context;
}

FJargonEffectContext FJargonEffectContextBuilder::BuildForTileEffect(
	AJargonCombatGameMode* GameMode,
	ABattleTileEffect* SourceTileEffect,
	EJargonEffectTrigger Trigger,
	ABattleUnit* TriggeringUnit)
{
	FJargonEffectContext Context;
	Context.GameMode = GameMode;
	Context.SourceObject = SourceTileEffect;
	Context.SourceUnit = nullptr;
	Context.SourceTeam = SourceTileEffect ? SourceTileEffect->GetSourceTeam() : ETeam::Player;
	Context.SourceTile = SourceTileEffect ? SourceTileEffect->GetCurrentTile() : nullptr;
	Context.PrimaryUnitTarget = TriggeringUnit;
	Context.PrimaryTileTarget = Context.SourceTile;
	Context.TriggeringUnit = TriggeringUnit;
	Context.OwningTileEffect = SourceTileEffect;
	Context.SourceCard = SourceTileEffect ? SourceTileEffect->GetSourceCard() : nullptr;
	Context.Trigger = Trigger;
	return Context;
}

FJargonEffectContext FJargonEffectContextBuilder::BuildForRelic(
	AJargonCombatGameMode* GameMode,
	UObject* SourceRelic,
	EJargonEffectTrigger Trigger,
	ABattleUnit* SourceUnit,
	ABattleUnit* PrimaryUnitTarget,
	AGridTile* PrimaryTileTarget,
	ABattleUnit* TriggeringUnit)
{
	FJargonEffectContext Context;
	Context.GameMode = GameMode;
	Context.SourceObject = SourceRelic;
	Context.SourceUnit = SourceUnit;
	Context.SourceTeam = SourceUnit ? SourceUnit->GetTeam() : ETeam::Player;
	Context.SourceTile = SourceUnit ? SourceUnit->GetCurrentTile() : nullptr;
	Context.PrimaryUnitTarget = PrimaryUnitTarget;
	Context.PrimaryTileTarget = PrimaryTileTarget;
	Context.TriggeringUnit = TriggeringUnit;
	Context.Trigger = Trigger;
	return Context;
}
