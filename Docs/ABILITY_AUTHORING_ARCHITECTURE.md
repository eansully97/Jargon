# Jargon Ability Authoring Architecture

## Summary

`UJargonAbilityDefinition` is the reusable non-card ability authoring layer above `FJargonEffectSpec`.

The intended pipeline is:

```text
Hook Context -> Ability Definition -> Targeting Profile / Placement Profile -> Ability Effect Lines -> Cue Metadata -> FJargonEffectExecutor -> FJargonEffectResolver
```

This does not replace CardScript, the shared effect executor, or the resolver. Cards continue to use CardScript until the ability layer proves itself on non-card systems.

## Authoring Shape

An ability definition owns:

- display name, description, and rules text
- expected trigger for validation
- expected hook context for validation and audit readability
- a readable targeting profile that builds delivery/filter/radius/chain data
- placement profiles on spawn-style effect lines
- presentation-only cue metadata
- instanced ability effect lines

Ability effect lines are source-agnostic action objects. Each line exposes only the payload it needs, then builds one or more `FJargonEffectSpec` entries. The definition's targeting profile is applied to targeted actions; self-only actions such as draw, energy, and element charge ignore it.

Hook context explains what the runtime hook can actually provide. It is the editor-facing answer to "what does this ability know about the event?" Examples include source unit, source tile, primary unit, primary tile, triggering unit, owning tile effect, and source team. Validation uses this to warn when an ability asks for a role the hook cannot provide.

Placement answers where spawned actors appear. It is intentionally separate from targeting: targeting decides who receives effects, while placement decides where summons and tile effects are created. Summon and place-tile-effect actions should use a placement profile instead of hiding spawn behavior in delivery names.

Targeting presets are authoring language only. They map to the shared effect resolver contract:

- `Self`
- `PrimaryEnemy`
- `PrimaryAlly`
- `PrimaryUnit`
- `PrimaryTile`
- `EnemiesInRadiusAroundAnchor`
- `AlliesInRadiusAroundAnchor`
- `UnitsInRadiusAroundAnchor`
- `TilesInRadiusAroundAnchor`
- `ChainEnemies`

Cue metadata is intentionally separate from gameplay. It can provide label/icon/color/floating-text hints to future presentation code, but the resolver still owns actual effect results.

## Runtime Shape

Future runtime hooks should call `FJargonEffectExecutor::ExecuteAbility` when they are executing a `UJargonAbilityDefinition`.

`FJargonEffectExecutor` remains the orchestration layer. `FJargonEffectResolver` remains the primitive operation layer.

Non-card hooks migrated so far are ability-authored only:

- hero class passives
- hero aspect transformation, turn-start, and enemy-death hooks
- summon `OnSummoned`, `OnTurnStart`, and `OnDeath` hooks
- trap/aura tile-effect trigger hooks
- hero boon/relic combat-start, turn-start, and enemy-death hooks

Runtime uses assigned ability definitions only for these hooks. Missing ability references mean the hook is empty.

## Migration Direction

Raw non-card effect arrays have been removed from the migrated hero, summon, tile-effect, and hero boon authoring surfaces. Future ability migration should continue with systems that have not yet received ability-definition hooks, such as enemy/unit active abilities.

Use `UJargonAbilityAuditTool` to write `Saved/AbilityAudit/AbilityAudit.csv`. The report lists ability definitions, hook references, hook context, available context roles, targeting summaries, placement summaries, context warnings, cue metadata, and whether each migrated hook is `Ability` or `Empty`.

## Guardrails

- Do not add GAS.
- Do not create one C++ class per ability.
- Do not migrate cards away from CardScript yet.
- Do not add new gameplay effects as part of ability authoring cleanup.
- Do not call `FJargonEffectResolver::ResolveEffects` directly from new hooks.
- Do not encode placement behavior into delivery names.
- Do not author spawn-style non-card effects without a placement profile.
