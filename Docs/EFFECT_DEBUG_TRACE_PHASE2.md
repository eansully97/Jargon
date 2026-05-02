# Effect Debug Trace Phase 2

Phase 2A adds the first practical consumer for `FJargonEffectTrace`: a one-shot developer command that logs the next played card's base effect trace.

## Command

Use this console command while in combat:

```text
JargonLogNextCardEffectTrace
```

The command is exposed on `AJargonCombatPlayerController`. It finds the current `AJargonCombatGameMode`, arms a one-shot flag with `RequestLogNextCardEffectTrace()`, and logs a short confirmation.

The flag lives in `AJargonCombatGameMode` as `bLogNextCardEffectTrace`.

## How It Works

When the next card play reaches `AJargonCombatGameMode::TryPlayCardWithResolvedTile`, the game mode checks the one-shot flag immediately before calling `FCardResolver::ResolveCard`.

If the flag is not armed, the existing card resolve path is unchanged:

```cpp
FCardResolver::ResolveCard(Card, ResolveContext, ResolveResult);
```

If the flag is armed, the game mode:

- clears the flag for this resolver attempt
- creates a local `FJargonEffectTrace`
- calls the trace-aware `FCardResolver::ResolveCard` overload
- logs a compact header with card, source unit, target unit, target tile, and resolved true/false
- logs `Trace.ToMultilineString()` when a resolver trace was produced

Earlier `TryPlayCardWithResolvedTile` validation failures, such as wrong phase, missing target, insufficient energy, or out-of-range target, do not consume the flag because no card resolver call was reached. If `FCardResolver` itself rejects the card before the base effects reach `FJargonEffectResolver`, the flag is consumed and the log states that no shared resolver trace was produced.

## What Gets Traced

Phase 2A traces only the base card effects that flow through:

```text
UCardDefinition::Effects -> FJargonEffectSpec -> FJargonEffectResolver
```

Visible trace events depend on what the resolver observes, but may include:

- `ResolveStarted`
- `ValidationFailed`
- `TargetsGathered`
- `NoTargetsNoOp`
- `OperationApplied`
- `OperationFailed`
- `FallbackUsed`
- `AsyncStarted`
- `EffectsSkipped`
- `ResolveFinished`

Trace logs use stable enum token names, such as `OperationApplied`, `DealDamage`, `ExplicitUnit`, `EnemyToSource`, and `OnPlayed`, rather than localized display names or raw integer values.

If the base trace continues asynchronously and the card has elemental bonus groups, the log also prints:

```text
Card trace note: base effects continued asynchronously. Elemental bonus groups are skipped by CardResolver for this resolve pass.
```

## Deferred

Phase 2A intentionally does not add:

- UI, widgets, or Blueprint debug panels
- persistent trace history
- always-on trace logging
- console variables
- card trace storage
- elemental bonus charge check/spend/refund trace events
- per-bonus `FJargonEffectTrace` output
- trace output for relics, auras, traps, summons, tile effects, or turn-start effects
- trace-driven gameplay branching

Elemental bonus tracing should be a later card-level wrapper around base and bonus resolver calls.

## Behavior Preservation

This phase is intended to preserve gameplay behavior:

- existing public `FCardResolver::ResolveCard` call sites still compile unchanged
- the existing `FCardResolver::ResolveCard(Card, Context, OutResult)` signature delegates to the trace-aware overload with `nullptr`
- the trace pointer is passed only to the base-effect `FJargonEffectResolver::ResolveEffects` call
- elemental bonus resolution still uses the existing non-traced resolver call
- energy cost flow is unchanged
- card consume/discard flow is unchanged
- target validation is unchanged
- async movement behavior is unchanged
- element charge check/spend/refund behavior is unchanged
- card cue behavior is unchanged

## Example Log Shape

Before the readability pass, enum values were printed as integers:

```text
Trace=1F9D... Trigger=0 Source=DA_Card_FireLash Card=Assigned Events=5 Resolved=true AnyEffect=true Async=false Warnings=false
[0] Event=6 Operation=1 Delivery=1 Filter=2 Source=DA_Card_FireLash Card=Assigned UnitTarget=Assigned TileTarget=Assigned Units=0 Tiles=0 Success=true AnyEffect=true Async=false SpawnedUnit=None SpawnedTileEffect=None Value=1 EnergyGain=0 Element=0 ElementDelta=0 Reason=Resolved Warning=
```

After the readability pass, enum values are stable token names:

```text
JargonLogNextCardEffectTrace armed. The next played card that reaches FCardResolver will log its base effect trace.
Jargon next card effect trace: Card=Fire Lash Source=BP_PlayerBattleUnit_C UnitTarget=BP_EnemyBattleUnit_C TileTarget=BP_GridTile_C Resolved=true
Trace=1F9D... Trigger=OnPlayed Source=DA_Card_FireLash Card=Assigned Events=5 Resolved=true AnyEffect=true Async=false Warnings=false
[-1] Event=ResolveStarted Operation=None Delivery=ExplicitUnit Filter=None Source=DA_Card_FireLash Card=Assigned UnitTarget=Assigned TileTarget=Assigned Units=0 Tiles=0 Success=false AnyEffect=false Async=false SpawnedUnit=None SpawnedTileEffect=None Value=0 EnergyGain=0 Element=None ElementDelta=0 Reason=ResolveStarted Warning=
[0] Event=ResolveStarted Operation=DealDamage Delivery=ExplicitUnit Filter=EnemyToSource Source=DA_Card_FireLash Card=Assigned UnitTarget=Assigned TileTarget=Assigned Units=0 Tiles=0 Success=false AnyEffect=false Async=false SpawnedUnit=None SpawnedTileEffect=None Value=1 EnergyGain=0 Element=None ElementDelta=0 Reason=ResolveStarted Warning=
[0] Event=TargetsGathered Operation=DealDamage Delivery=ExplicitUnit Filter=EnemyToSource Source=DA_Card_FireLash Card=Assigned UnitTarget=Assigned TileTarget=Assigned Units=1 Tiles=0 Success=false AnyEffect=false Async=false SpawnedUnit=None SpawnedTileEffect=None Value=1 EnergyGain=0 Element=None ElementDelta=0 Reason=TargetsGathered Warning=
[0] Event=OperationApplied Operation=DealDamage Delivery=ExplicitUnit Filter=EnemyToSource Source=DA_Card_FireLash Card=Assigned UnitTarget=Assigned TileTarget=Assigned Units=0 Tiles=0 Success=true AnyEffect=true Async=false SpawnedUnit=None SpawnedTileEffect=None Value=1 EnergyGain=0 Element=None ElementDelta=0 Reason=Resolved Warning=
[-1] Event=ResolveFinished Operation=DealDamage Delivery=ExplicitUnit Filter=EnemyToSource Source=DA_Card_FireLash Card=Assigned UnitTarget=Assigned TileTarget=Assigned Units=0 Tiles=0 Success=false AnyEffect=false Async=false SpawnedUnit=None SpawnedTileEffect=None Value=1 EnergyGain=0 Element=None ElementDelta=0 Reason=Resolved Warning=
```

The readability pass is formatting-only. It uses `StaticEnum<T>()->GetNameStringByValue(...)` token names, not display names, so the output remains stable for logs and diffs.
