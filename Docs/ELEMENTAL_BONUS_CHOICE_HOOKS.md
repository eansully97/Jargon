# Elemental Bonus Choice Hooks

Date: 2026-05-03

## Summary

Elemental bonuses are optional player choices. Cards always resolve their base CardScript effect lines. Bonus groups resolve only when the combat controller receives explicit selected bonus indices from Blueprint UI.

If no prompt widget class is assigned, eligible bonuses are skipped and the card still plays normally with base effects only.

## Combat Controller Hooks

On `AJargonCombatPlayerController`:

- Assign `ElementalBonusChoiceWidgetClass` to a Blueprint child of `UElementalBonusChoiceWidget`.
- The controller creates the widget once at `BeginPlay`, adds it to the viewport, hides it, and reuses it.
- The assigned widget class is the enablement signal. There is no separate boolean.
- The controller exposes `GetPendingElementalBonusChoiceRequest` for debugging or direct reads.

On the `UElementalBonusChoiceWidget` Blueprint child:

- Implement `BP_OnChoiceRequestChanged`.
- Build one checkbox or toggle per `Options` entry.
- Confirm with `ConfirmSelectedBonusIndices`, passing selected `BonusIndex` values.
- Skip with `SkipBonuses`.
- Cancel/back with `CancelChoice`.
- Hide/clear presentation from `BP_OnChoiceRequestCleared`.

## Request Data

Each option includes:

- card reference
- bonus index
- element type and display text
- required charge count
- current charge count
- whether charges will be spent
- summary text
- rules text
- usability flag

The first pass only offers currently usable options, so unavailable bonus groups do not need disabled UI rows.

## Resolve Rules

- No bonus selected: base effects only.
- One or more bonuses selected: selected groups resolve in authored order after base effects.
- If an earlier selected group spends charges and a later selected group can no longer pay, the later group is skipped.
- Async base effects skip selected bonus groups for that resolve pass.
- `ElementalBonusTriggered` cues fire only for selected bonus groups that actually resolve.

## Blueprint Recommendation

Use a compact modal or side prompt after target selection:

- card name/title
- checkbox list using option `RulesText`
- Confirm
- Skip Bonuses
- Back

Back should cancel the pending request without playing the card. For targeted cards, the card remains selected so the player can choose another target or retry.
