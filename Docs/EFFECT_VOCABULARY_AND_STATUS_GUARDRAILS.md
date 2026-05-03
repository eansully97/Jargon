# Effect Vocabulary And Status Guardrails

Date: 2026-05-02

## Summary

Jargon should keep the shared effect resolver as the backend gameplay language, but future effect growth should not automatically mean adding another `ApplyX` enum value. Status-like keywords should use `UJargonStatusEffectDefinition` instead of growing the operation enum.

Designer-facing card authoring should use **Effect Line = Operation + Delivery + Filter + Payload** language. `Keyword` is reserved for reusable rules terms such as statuses, traits, and future modifiers.

## Vocabulary Rules

Prefer status definitions for per-unit condition keywords:

- poison
- bleed
- weak
- marked
- regen
- curse
- thorns-style retaliate effects
- any future condition that lives on a unit and changes behavior over time or on a later event

Prefer direct resolver operations for immediate effects:

- movement, push, pull, and targeting
- draw cards
- gain energy
- gain element charge
- summon a unit definition
- place or destroy a tile-effect definition
- simple immediate damage, heal, or shield

Prefer tile-effect definitions for board-space effects:

- hazards
- traps
- auras
- zones
- delayed area effects
- effects whose behavior belongs to a tile actor shell and a tile-effect definition

Prefer summon/unit definitions for unit gameplay:

- unit stats
- lifecycle effects
- animation overrides where currently supported
- AI-facing or capability data once enemy/unit definitions exist

## Status Definition Direction

`UJargonStatusEffectDefinition` owns the authoring identity for the built-in status kinds: Stun, Freeze, Burn, Root, Vulnerable, Regen, and Weak. Shared effects can use `ApplyStatus` with a `StatusEffectDefinition`; the resolver dispatches that definition to `ABattleUnit` runtime status functions.

The built-in assets are created by the status migration commandlet:

- `/Game/Jargon/Data/StatusEffects/DA_Status_Stun`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Freeze`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Burn`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Root`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Vulnerable`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Regen`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Weak`

CardScript `Apply Status` actions should reference one of those definitions. The old direct shared operations (`ApplyStun`, `ApplyFreeze`, `ApplyBurn`, `ApplyRoot`, `ApplyVulnerable`) remain as temporary migration scaffolding for old authored shared effects and unmigrated content.

The next migration should be:

1. Verify all production CardScript status actions reference status definitions.
2. Remove direct `ApplyBurn` / `ApplyRoot` style authoring once no production content needs them.

## Guardrail

Do not add a new `ApplyX` enum operation for a status-like effect unless explicitly approved. If the new keyword is a per-unit condition, start with a status definition design instead.

## Future Keyword Candidates

Implemented first expansion batch:

- Cleanse / Remove Status: direct resolver operation for removing all negative statuses or one authored status definition from delivered units.
- Regen: status definition for turn-start healing that decays, giving Nature and Radiance sustain cards more room.
- Weak: status definition for reducing outgoing damage, giving Frost, Quietus, and Radiance softer control than full stun/freeze/root lockouts.
- Lifesteal: narrow damage modifier on `DealDamage` effects; heals the source for unblocked HP damage dealt without introducing a broad modifier framework.

Good later status-definition candidates:

- Poison: turn-start damage with different stack behavior than Burn.
- Bleed: damage when moving or taking actions.
- Marked: bonus damage or targeting payoff from later effects.
- Curse: flexible negative status for Quietus-style payoffs.

Good direct resolver candidates:

- RepeatEffect: repeat an existing effect spec a fixed number of times, only if repeated-card patterns become common.
- ConditionalEffect: resolve nested effects only if a simple condition passes, deferred until elemental bonuses are not enough.
- ConsumeStatus: remove or spend a status from a target for payoff, likely needed before Marked becomes interesting.
- AdjacentBonus: apply a payoff based on adjacency, deferred until board-position payoffs are repeated enough to justify a primitive.

Good tile-effect-definition candidates:

- CreateHazard: place a hazard definition with duration and trigger.
- DelayedBurst: a tile definition that resolves after a turn count.
- AuraPulse: recurring area effect with definition-owned trigger and radius.

## Validation Notes

Validation has shared helper classification for current status-like card and shared resolver operations. `ApplyStatus` requires a valid `UJargonStatusEffectDefinition` and a positive value; `CleanseStatus` can optionally reference a valid status definition. Lifesteal is only valid on `DealDamage`.

## Audit Notes

`UCardCatalogAuditTool` writes `EffectVocabularyFitAudit.csv` to separate structural card coverage from mechanical variety. Treat element rows with `Covered` structural status but concentrated operation vocabulary as a sign to add or use effect vocabulary before creating more cards.
