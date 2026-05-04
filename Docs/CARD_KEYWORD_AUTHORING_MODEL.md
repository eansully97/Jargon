# Card Effect Line Authoring Model

Date: 2026-05-02

## Summary

Cards should be authored as readable effect lines, then translated into shared resolver effects. The designer-facing model is:

- Operation: what the effect line does; this maps to a backend `FJargonEffectSpec` primitive such as Damage, Heal, Apply Status, Pull, Summon, Place Tile Effect, Draw, Gain Energy, or Gain Element.
- Delivery: how the operation finds targets, such as Self, Single Enemy, Single Ally, Target Tile, AOE, Chain Units, or placed tile effect.
- Filter: which delivered targets are valid, such as enemy, friendly, any, or source only.
- Payload: the operation-specific values and definitions, such as 2 damage, 1 Burn, Fire Imp summon definition, trap definition, radius, chain count, or element charge type.
- Keyword: a reusable rules term, status, trait, or modifier, such as Stunned, Shield, Lifesteal, Flying, Tough, Draw, Burn, or Vulnerable.
`Operation` and `Keyword` both exist. Operation is the executable primitive. Keyword is the card-game rules vocabulary that may appear in payload definitions, rules text, UI, or future modifiers.

## Current Implementation

`UJargonCardAction` remains the source class name for compatibility, but the editor-facing authoring surface treats each inline entry as a card effect line. Collapsed summaries prefer the shape:

```text
Operation=Damage Delivery=Single Enemy Payload=2 damage
Operation=Burn Delivery=AOE Enemies Radius=1 Payload=2 Burn stacks
Operation=Summon Delivery=Target Tile Payload=Fire Imp AttackExhausted=true
```

CardScript builds `FJargonEffectSpec` arrays directly. The old raw card effect structs and authoring arrays have been removed from `UCardDefinition`; cards should not add a second compatibility effect layer.

`EJargonCardKeyword` is retained as source taxonomy for CardScript action classes. It should not be treated as the complete card-game keyword system.

## Authoring Guidance

- Use CardScript effect lines for production cards.
- Prefer status Data Assets for status-like keywords instead of adding new `ApplyX` operations.
- Prefer summon, tile-effect, and future enemy/unit definitions for content-specific payloads.
- Keep rules text short and action-first, but use operation/delivery/payload summaries when debugging authoring shape.

## How To Author A Card

1. Fill the card identity fields first: display name, cost, category, target type, range, description, and card art prompt.
2. Add base CardScript effect lines in the order they should resolve.
3. For each effect line, set Delivery fields before Payload fields. Delivery answers "who or where receives this?" Payload answers "what amount or definition is applied?"
4. Use Data Asset payloads for content-specific gameplay: status definitions, summon definitions, and tile-effect definitions.
5. For runtime actor operations, author both halves explicitly on the effect line: summon lines need `SummonedUnitDefinition` plus `RuntimeSummonedUnitClass`, while trap/aura lines need `TileEffectDefinition` plus `RuntimeTileEffectClass`.
6. Add elemental bonuses only when the card has a clear optional payoff. Elemental bonuses are player-chosen at card play time; the bonus group owns the required element, charge count, spend/check behavior, and its own effect-line list.
7. Run Data Validation and the card catalog audit. Compare the authored description against `CardDescriptionSuggestions.csv` and keep descriptions rules-first.

## Examples

Damage card:

```text
Operation=Damage Delivery=Single Enemy Payload=2 damage
Rules text: Deal 2 damage.
```

Status card:

```text
Operation=ApplyStatus Delivery=Single Enemy Payload=2 Burn stacks Definition=DA_Status_Burn
Rules text: Apply 2 Burn stacks.
```

Cleanse card:

```text
Operation=CleanseStatus Delivery=Single Ally Payload=All negative statuses
Rules text: Cleanse all negative statuses.
```

Lifesteal card:

```text
Operation=Damage Delivery=Single Enemy Payload=2 damage Lifesteal=true
Rules text: Deal 2 damage. Heal for unblocked damage dealt.
```

Summon card:

```text
Operation=Summon Delivery=Target Tile Payload=Fire Imp Runtime=BP_FireImp AttackExhausted=true
Rules text: Summon Fire Imp.
```

Trap or aura card:

```text
Operation=Place Tile Effect Delivery=Target Tile Payload=Spike Trap Runtime=BP_SpikeTrap
Rules text: Place Spike Trap.
```

Chain card:

```text
Operation=Damage Delivery=Chain Units Count=3 Radius=2 Payload=1 damage
Rules text: Chain 1 damage up to 3 targets within radius 2.
```

Element bonus card:

```text
Base: Operation=Gain Element Delivery=Self Payload=1 Fire charge
Bonus: Optional: Spend 2 Fire charges: Operation=Damage Delivery=Single Enemy Payload=2 damage
Rules text: Gain 1 Fire charge. Optional: Spend 2 Fire charges: Deal 2 damage.
```

## Elemental Bonus Choice

Elemental bonuses are optional player-chosen payoffs, not automatic follow-up effects. A card resolves its base effect lines first. If the player selected one or more eligible bonus groups, those selected groups resolve afterward in authored order.

If no elemental bonus choice widget class is assigned on the combat controller, cards still play normally and resolve base effects only. This keeps unhooked UI from blocking card play while making automatic bonus spending impossible.

Blueprint prompt setup should use a Blueprint child of `UElementalBonusChoiceWidget`. Present the eligible bonus groups from `FJargonElementalBonusChoiceRequest.Options`, then call the widget helpers to confirm selected `BonusIndex` values, skip bonuses, or cancel/back.

## Audit Expectations

The card catalog audit should call out:

- missing Data Asset payloads for status, summon, trap, or aura effect lines
- missing runtime Blueprint class payloads for summon, trap, or aura effect lines
- long or noisy collapsed effect-line summaries
- authored descriptions that drift from generated rules-first effect text

Debug cards under `/Game/Jargon/Data/Cards/Debug` are intentionally allowed to be outside production packs and may use terse test descriptions. They should not create normal production audit noise unless they are accidentally included in a non-debug pack.

## Deck Element Rule

Each card owns a `CardElement`. `None` is displayed as `Neutral` for card and deck authoring. Run decks may include up to 3 unique non-neutral elements plus any number of Neutral cards.

`UDeckEditWidget` exposes source-only Blueprint hooks for element clarity: each library entry includes element text, filters can still show one or many elements, and the widget exposes a deck element summary such as `Deck elements: Fire, Frost, Radiance (3 / 3)`. Deck add blocking includes ownership, deck size, card copy limits, and introducing a fourth non-neutral element.

## Hero Aspect Rule

Energy is the normal card play resource. Element charges are separate combat-local resources. Save 10 charges of an authored element to transform into that element's hero aspect for the rest of combat. Element charges currently cap at 10, so "10+" effectively means capped.

The first eligible element to reach 10 locks the transformation. Later capped elements do not switch it, and spending below 10 does not remove it. HUD and cue text should describe this as transformation, for example `Fire Transformation: 7/10` or `Pyromancer Transformed`.

## Deferred Work

- Rename C++ symbols from Action/Keyword only after asset compatibility and reflection churn are worth it.
- Add a more generic delivery/payload object model only if the current focused keyword subclasses remain too noisy.
- Add new gameplay effects only after the current effect-line surface has been used in-editor.
- Migrate card assets only in explicit content passes; this document describes the authoring model, not an automatic asset migration.
