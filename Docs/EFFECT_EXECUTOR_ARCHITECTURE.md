# Jargon Effect Executor Architecture

## Summary

Jargon's effect runtime now has two deliberate layers:

- `FJargonEffectExecutor`: orchestration glue for authored effect arrays.
- `FJargonEffectResolver`: primitive operation resolver for each `FJargonEffectSpec`.

The executor does not replace the resolver. It centralizes the repeated runtime pattern used by cards, traps, auras, summons, hero passives, aspects, and artifacts:

```text
Authored source -> FJargonEffectSpec[] -> FJargonEffectExecutionRequest -> FJargonEffectExecutor -> FJargonEffectResolver
```

Reusable non-card ability definitions add one authoring step above this:

```text
Hook Context -> Ability Definition -> Targeting Profile / Placement Profile -> Ability Effect Lines -> FJargonEffectSpec[] -> FJargonEffectExecutor -> FJargonEffectResolver
```

## Executor Responsibilities

- Receive an already-built `FJargonEffectContext`.
- Carry the hook context type for logging, validation language, and audit readability.
- Forward authored effect arrays and optional trace output to `FJargonEffectResolver`.
- Build effect specs from `UJargonAbilityDefinition` through `ExecuteAbility` when a hook is definition-authored.
- Return a compact `FJargonEffectExecutionReport`.
- Provide consistent log labels, skipped/no-op reasons, resolver failure reporting, and async warnings.

The executor does not own presentation cues yet. Cue timing differs between cards, tile effects, artifacts, aspects, and unit hooks, so those callsites still emit cues where their gameplay context is clearest.

Ability cue metadata lives on `UJargonAbilityDefinition`, but it is only authoring/presentation metadata. The executor forwards gameplay effects and reports resolver results; cue widgets or presentation managers can read ability cue metadata in a later pass without changing resolver behavior.

## Resolver Responsibilities

- Validate operation/context compatibility during resolution.
- Gather targets through delivery and filters.
- Apply primitive operations such as damage, heal, status, summon, tile placement, draw, energy, and element charges.
- Populate `FJargonEffectResult` and optional trace events.

New gameplay primitives belong in the resolver. New execution entry points should go through the executor.

Spawn operations use placement data from the authored effect spec. Delivery still describes target gathering; placement describes where a summoned unit or tile effect appears. This keeps passive summons, death spawns, trap triggers, and aura hooks readable without creating delivery names that secretly mean "spawn near this thing."

## Authoring Guardrail

Do not call `FJargonEffectResolver::ResolveEffects` directly from gameplay systems outside the effect runtime. Build a `FJargonEffectExecutionRequest`, set a readable `SourceLabel`, `HookName`, and `HookContextType`, and call `FJargonEffectExecutor::Execute`.

When executing a `UJargonAbilityDefinition`, call `FJargonEffectExecutor::ExecuteAbility` with the hook context that matches the runtime event. Missing ability references mean the hook is empty; do not add raw effect array fallbacks.

Direct resolver calls should generally exist only inside:

- `FJargonEffectExecutor`
- `FJargonEffectResolver`
- temporary low-level tests or diagnostics that intentionally bypass orchestration

## Deferred Work

- Migrating future enemy/unit active abilities into ability definitions. Hero class/aspects, summons, tile effects, and hero artifacts are already ability-only.
- A dedicated cue adapter that reads ability cue metadata and resolved effect results.
- Typed operation payload structs.
- General condition or modifier frameworks.
