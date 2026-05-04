# Jargon Ability Authoring Architecture

## Summary

`UJargonAbilityDefinition` is the reusable non-card ability authoring layer above `FJargonEffectSpec`.

The intended pipeline is:

```text
Ability Definition -> Targeting Profile -> Ability Effect Lines -> Cue Metadata -> FJargonEffectExecutor -> FJargonEffectResolver
```

This does not replace CardScript, the shared effect executor, or the resolver. Cards continue to use CardScript until the ability layer proves itself on non-card systems.

## Authoring Shape

An ability definition owns:

- display name, description, and rules text
- expected trigger for validation
- a readable targeting profile that builds delivery/filter/radius/chain data
- presentation-only cue metadata
- instanced ability effect lines

Ability effect lines are source-agnostic action objects. Each line exposes only the payload it needs, then builds one or more `FJargonEffectSpec` entries. The definition's targeting profile is applied to targeted actions; self-only actions such as draw, energy, and element charge ignore it.

Targeting presets are authoring language only. They map to the shared effect resolver contract:

- `Self`
- `SelectedEnemy`
- `SelectedAlly`
- `SelectedUnit`
- `TargetTile`
- `EnemiesInRadius`
- `AlliesInRadius`
- `UnitsInRadius`
- `TilesInRadius`
- `ChainEnemies`

Cue metadata is intentionally separate from gameplay. It can provide label/icon/color/floating-text hints to future presentation code, but the resolver still owns actual effect results.

## Runtime Shape

Future runtime hooks should call `FJargonEffectExecutor::ExecuteAbility` when they are executing a `UJargonAbilityDefinition`.

`FJargonEffectExecutor` remains the orchestration layer. `FJargonEffectResolver` remains the primitive operation layer.

Current non-card definitions can now reference ability definitions while their raw arrays remain as migration surfaces:

- hero class passives
- hero aspect transformation, turn-start, and enemy-death hooks
- summon `OnSummoned`, `OnTurnStart`, and `OnDeath` hooks
- trap/aura tile-effect trigger hooks
- hero boon/relic combat-start, turn-start, and enemy-death hooks

Runtime prefers the assigned ability definition when one exists. If the ability reference is empty, runtime uses the existing raw effect array.

## Migration Direction

Migrate non-card raw effect arrays in small passes:

1. Hero aspect transformation/passive effects.
2. Summon `OnSummoned`, `OnTurnStart`, and `OnDeath` hooks.
3. Trap/aura tile-effect definitions.
4. Hero boon/relic hooks.
5. Enemy/unit active abilities.

Keep old raw arrays until migrated assets are authored, validated, and tested. Do not remove the raw arrays in the same pass that introduces the ability definition type.

Use `UJargonAbilityAuditTool` to write `Saved/AbilityAudit/AbilityAudit.csv`. The report lists ability definitions, hook references, remaining raw effect arrays, targeting summaries, cue metadata, and places where both an ability reference and a raw array are authored.

## Guardrails

- Do not add GAS.
- Do not create one C++ class per ability.
- Do not migrate cards away from CardScript yet.
- Do not add new gameplay effects as part of ability authoring cleanup.
- Do not call `FJargonEffectResolver::ResolveEffects` directly from new hooks.
