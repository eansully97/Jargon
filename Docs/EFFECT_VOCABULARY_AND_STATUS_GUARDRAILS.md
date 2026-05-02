# Effect Vocabulary And Status Guardrails

Date: 2026-05-02

## Summary

Jargon should keep the shared effect resolver as the backend gameplay language, but future effect growth should not automatically mean adding another `ApplyX` enum value. Status-like keywords should use `UJargonStatusEffectDefinition` instead of growing the operation enum.

No gameplay behavior changed in this pass. Existing CardScript `Apply Status` actions still build the same `ApplyStun`, `ApplyFreeze`, `ApplyBurn`, `ApplyRoot`, and `ApplyVulnerable` effect operations.

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

`UJargonStatusEffectDefinition` now owns the authoring identity for the built-in status kinds: Stun, Freeze, Burn, Root, and Vulnerable. Shared effects can use `ApplyStatus` with a `StatusEffectDefinition`; the resolver dispatches that definition to the same existing `ABattleUnit` runtime status functions used before this migration.

The built-in assets are created by the status migration commandlet:

- `/Game/Jargon/Data/StatusEffects/DA_Status_Stun`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Freeze`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Burn`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Root`
- `/Game/Jargon/Data/StatusEffects/DA_Status_Vulnerable`

CardScript `Apply Status` actions should reference one of those definitions. The old status enum and direct shared operations (`ApplyStun`, `ApplyFreeze`, `ApplyBurn`, `ApplyRoot`, `ApplyVulnerable`) remain as temporary migration scaffolding for legacy raw effects, chain stun, and unmigrated content.

The next migration should be:

1. Verify all production CardScript status actions reference status definitions.
2. Move chain status authoring to a definition-based shape if chain statuses remain useful.
3. Remove direct `ApplyBurn` / `ApplyRoot` style authoring once raw effects and chain scaffolding no longer need them.

## Guardrail

Do not add a new `ApplyX` enum operation for a status-like effect unless explicitly approved. If the new keyword is a per-unit condition, start with a status definition design instead.

## Future Keyword Candidates

Good next status-definition candidates:

- Poison: turn-start damage with different stack behavior than Burn.
- Bleed: damage when moving or taking actions.
- Weak: reduce next outgoing damage.
- Marked: bonus damage or targeting payoff from later effects.
- Regen: heal at turn start, then decay.
- Curse: flexible negative status for Quietus-style payoffs.

Good direct resolver candidates:

- RepeatEffect: repeat an existing effect spec a fixed number of times.
- ConditionalEffect: resolve nested effects only if a simple condition passes.
- ConsumeStatus: remove or spend a status from a target for payoff.
- AdjacentBonus: apply a payoff based on adjacency.

Good tile-effect-definition candidates:

- CreateHazard: place a hazard definition with duration and trigger.
- DelayedBurst: a tile definition that resolves after a turn count.
- AuraPulse: recurring area effect with definition-owned trigger and radius.

## Validation Notes

Validation now has shared helper classification for current status-like card and shared resolver operations. `ApplyStatus` requires a valid `UJargonStatusEffectDefinition` and a positive value; CardScript status actions validate the same requirement after migration.
