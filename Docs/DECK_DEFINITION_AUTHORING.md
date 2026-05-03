# Deck Definition Authoring

## Summary

`UJargonDeckDefinition` is the source Data Asset for starter and prebuilt decks. Loose starter deck arrays have been removed; deck definitions are now the required authoring path.

## Authoring Rules

- Author duplicate card copies as duplicate entries in `Cards`.
- Keep starter/prebuilt decks at or below 30 cards.
- Keep card copies at or below 3 copies per card.
- Keep deck definitions at or below 3 unique non-neutral card elements. Neutral cards do not count.
- Use valid `UCardDefinition` assets only.

## Runtime Behavior

- Town startup requires `StarterDeckDefinition` on `AJargonTownGameMode`.
- Direct combat / no-run seeding uses `EmergencyStartingDeckDefinition` on `AJargonCombatGameMode`.
- Missing or empty deck definitions log configuration errors instead of silently seeding from fallback arrays.
- Runtime does not truncate over-size deck definitions. Validation reports size, copy, and element-limit issues, while run deck rules prevent adding more cards when the deck is full, a card is already at the copy limit, or the add would introduce a fourth non-neutral element.

## Follow-Up Cleanup

- Create starter/prebuilt deck assets under `/Game/Jargon/Data/Decks`.
- Assign the starter deck to `BP_TownGameMode`.
- Assign an emergency combat deck if direct combat map loading still needs one.
- Remove any stale Blueprint/editor references to deleted loose starter arrays if they appear during asset resaves.
