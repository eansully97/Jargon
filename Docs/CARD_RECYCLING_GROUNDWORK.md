# Card Recycling Groundwork

## Summary

Manual card recycling is a run-economy hook for converting extra owned reserve copies into currency. It is intentionally not automatic: pack purchases still grant cards normally, and the player/deck UI chooses when to call the bulk recycle function.

## Current Behavior

- `UJargonGameInstance::RecycleOwnedRunCard` is the backend authority.
- `UJargonGameInstance::RecycleAllExtraReserveCards` is the intended Deck Edit button action.
- Only owned copies above the useful copy limit can be recycled.
- The useful copy floor is `max(MaxCopiesPerDeckCard, DeckCopies)`, so normal decks keep 3 owned copies and legacy/invalid decks with more than 3 active copies are still preserved.
- Recycling removes one copy from `RunOwnedCards`, awards `CardRecycleValue`, and refreshes the reserve catalog.
- Bulk recycling removes every owned copy above the useful copy floor while preserving active deck ownership.
- `RunReserveCards` remains the visible catalog, not the ownership source of truth.
- `UDeckEditWidget` exposes one bulk action plus `bCanRecycleAllExtraReserveCards`, `RecycleAllExtraReserveCardCount`, `RecycleAllExtraReserveCurrencyValue`, and `RecycleAllExtraReserveBlockedReason` for Blueprint UI.

## Defaults

- `CardRecycleValue` currently defaults to `20` copper and is editable on `UJargonGameInstance`.
- This value is deliberately centralized for now so card balance/content tuning does not need to happen in this pass.

## Deferred

- No automatic duplicate conversion from packs.
- No per-card recycle values yet.
- No confirmation dialog or widget layout edits in this pass.
- No saved-deck interaction yet.
