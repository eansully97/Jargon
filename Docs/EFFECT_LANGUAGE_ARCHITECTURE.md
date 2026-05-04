# Jargon Effect Language Architecture

Date: 2026-05-02

## Summary

Jargon should keep a small shared effect language that is easy to author, validate, trace, and reuse across cards, summons, traps, auras, hero passives, artifacts, and tile effects.

The intended model is:

```text
Effect Line = Operation + Delivery + Target Filter + Payload + optional lightweight Condition + optional Modifier
```

This is not a request for GAS, colored card costs, a broad condition framework, or a rewrite of the shared `FJargonEffectSpec` resolver language.

## Terms

- **Operation**: the backend primitive resolved by `FJargonEffectResolver`, such as `DealDamage`, `Heal`, `ApplyShield`, `ApplyStatus`, `DrawCards`, `GainEnergy`, `GainElementCharge`, `MoveSource`, `PushTarget`, `PullTarget`, `SummonUnit`, `PlaceTileEffect`, or `DestroyTileEffect`.
- **Delivery**: the target shape/source, such as self, explicit unit, explicit tile, units in radius, tiles in radius, or chain units.
- **Target Filter**: the team/ownership filter applied after delivery gathers candidates, such as enemy to source, friendly to source, any, or source only.
- **Payload**: operation-specific parameters and references, such as value, radius, chain count, status definition, summon definition, tile-effect definition, or runtime actor class.
- **Keyword**: a reusable card-game rules term, status, trait, or modifier, such as Stunned, Shield, Burn, Vulnerable, Lifesteal, Flying, Tough, Draw, or future Pierce/Retain/Exhaust.
- **Condition**: a lightweight gate, currently mostly elemental bonus requirements. Do not build a generalized condition tree yet.
- **Modifier**: a reusable alteration to a resolved operation, such as future Lifesteal on damage. Add modifiers only when they remove real duplication.

## Guardrails

- Do not add status-like `ApplyX` operations for new statuses. Prefer `ApplyStatus` plus `UJargonStatusEffectDefinition`.
- Do not encode conditions in operation names, such as `DealDamageIfPoisoned`, `DealDamagePerStack`, or `DealDamageIgnoringArmor`.
- Do not make delivery methods contain target logic like lowest health, poisoned only, or random debuffed enemy. Add a separate selector/filter concept only when the project really needs it.
- Do not put hidden branching rules in payload fields. Payload is parameters and references, not `bOnlyIfTargetPoisoned`.
- Keep presentation cues downstream of resolved effects and trace events. Gameplay payloads should not decide visual presentation.
- Do not reintroduce a card-only raw effect struct layer. CardScript should build `FJargonEffectSpec` directly.

## Current Near-Term Shape

- CardScript inline entries are editor-facing effect lines, even though source class names still say `UJargonCardAction` / `EJargonCardKeyword`.
- `FJargonEffectSpec` remains the shared runtime language.
- `FJargonEffectExecutor` is the shared orchestration layer for executing authored effect arrays. Gameplay systems should use it instead of calling `FJargonEffectResolver::ResolveEffects` directly.
- `FJargonEffectResolver` remains the primitive operation resolver for individual effect specs, target delivery, filters, payload application, and trace events.
- `JargonEffectContracts` in `JargonEffectTypes.h` owns lightweight operation contract helpers for required payloads, suspicious delivery/filter combinations, ignored payload fields, and trace payload summaries.
- Data validation and card audit output should use the same contract vocabulary where possible.

## Stress-Test Examples

| Card or Hook | Operation | Delivery / Filter | Payload | Condition or Keyword |
|---|---|---|---|---|
| Strike | `DealDamage` | explicit unit, enemy | value `2` | none |
| Guard | `ApplyShield` | self or friendly | value `3` | keyword term: Shield |
| Stun Bolt | `ApplyStatus` | explicit unit, enemy | `DA_Status_Stun`, value `1` | keyword term: Stunned |
| Chain Stun | `ApplyStatus` | chain units, enemy | `DA_Status_Stun`, value `1`, chain count `3`, radius `2` | none |
| Fire Imp | `SummonUnit` | explicit tile | summon definition + runtime summon BP class | none |
| Spike Trap | `PlaceTileEffect` | explicit tile | tile-effect definition + trap BP class | tile-effect trigger |
| Healing Grove | `PlaceTileEffect` | explicit tile | tile-effect definition + aura BP class | tile-effect trigger |
| Kindle | `GainElementCharge` | self/source | Fire, value `1` | none |
| Spark Payoff | `GainElementCharge`, bonus `DealDamage` | self, then explicit enemy | Storm +1, bonus damage | elemental charge condition |
| Mage Turn Start | `GainElementCharge` | self/source | Storm, value `1` | passive trigger |

## Deferred Work

- Typed payload structs or `FInstancedStruct` variants.
- A generic target selector/filter framework.
- A general condition tree.
- A formal modifier system.
- Reflected C++ renames away from `Action` / `Keyword`.
- Broad asset migration away from CardScript or the shared `FJargonEffectSpec` resolver language.
