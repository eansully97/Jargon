# Combat Hover Info Widget Hooks

## Summary

Phase 1 exposes a minimal, description-only hover info channel so Blueprint UI can show secondary information without adding clutter to the main HUD.

## Blueprint Surface

- `GetCurrentCombatHoverInfo()` returns the latest `FJargonCombatHoverInfo`.
- `OnCombatHoverInfoChanged` broadcasts whenever the info changes or clears.
- `bEnableCombatHoverInfo` can disable the trace and clears the current info.
- `CombatHoverInfoWidgetClass` on `AJargonCombatPlayerController` is the optional widget class slot for a Blueprint child of `UCombatHoverInfoWidget`.
- `GetCombatHoverInfoWidget()` returns the spawned widget instance when a class is assigned.

`UCombatHoverInfoWidget` provides:

- `SetHoverInfo()`
- `GetHoverInfo()`
- `UpdateHoverPosition()`
- `BP_OnHoverInfoChanged()`
- optional `DescriptionText` TextBlock binding, updated automatically when present.
- cursor-follow layout properties:
  - `bFollowMouseCursor`
  - `CursorOffset`
  - `ViewportPadding`
  - `bFlipToStayOnScreen`
  - `bClampToViewport`

By default, the shared hover widget follows the owning player's mouse cursor every tick with a fixed offset. It flips/clamps near viewport edges so the description remains visible. The widget remains `HitTestInvisible` while active and `Collapsed` when empty, so it should not contain clickable controls in this phase.

`FJargonCombatHoverInfo` intentionally contains only:

- `bHasInfo`
- `InfoType`
- `DescriptionText`
- `SourceActor`
- `SourceObject`

`SourceActor` is used by combat world hovers. `SourceObject` lets non-actor UI hovers, such as deck library entry widgets, clear only their own hover info when the mouse leaves.

## Hover Priority

- Hovering a tile-effect actor shows that tile effect definition's `Description`.
- If the tile effect description is empty, its display name is used as a temporary fallback.
- Hovering a summoned unit shows the applied summoned unit definition's `Description`.
- Hovering a tile checks the occupying unit first, then the first tile effect on that tile.
- Empty tiles and normal non-summon units do not display generic hover text in this pass.
- Hover info continues updating while selecting card targets.

## Town Deck Edit Hooks

The same `UCombatHoverInfoWidget` base can be reused in town:

- `TownHoverInfoWidgetClass` on `AJargonTownPlayerController` is the optional widget class slot.
- `GetCurrentTownHoverInfo()` returns the current town/deck hover payload.
- `OnTownHoverInfoChanged` broadcasts when the town/deck hover changes or clears.
- `bEnableTownHoverInfo` can disable town hover output.
- `GetTownHoverInfoWidget()` returns the spawned widget instance.

`UDeckLibraryCardEntryWidget` now emits native hover enter/leave handling. On hover, it asks the active `UDeckEditWidget` to build hover info from the card's CardScript:

- Trap/aura cards use the linked `UJargonTileEffectDefinition.Description`.
- If a tile-effect definition description is empty, its display name is used as the same temporary fallback as combat tile-effect hovers.
- Summon cards use the linked `UJargonSummonedUnitDefinition.Description`.
- Normal cards with no linked summon/tile-effect definition description produce no hover info in this first pass.

## Notes

- This hook is read-only presentation state and does not affect targeting, movement, card play, or combat resolution.
- Blueprint owns styling and any later richer layout. The C++ base can optionally update a bound `DescriptionText` TextBlock, auto-hide/show itself, and follow the cursor.
- Future phases can add optional fields for names, stats, statuses, duration, radius, or rules summaries after the description-only widget feels good in play.
