# Card Balance Rubric And Audit

This pass adds source-only guardrails for card balance, variety, and rules text. It does not modify card Data Assets.

## Balance Target

Jargon is currently tuned toward tactical low numbers:

- 0-cost cards should be setup, movement, light utility, or small element generation.
- 1-cost cards should be modest single-target effects or simple setup.
- 2-cost cards can combine effects, hit harder, summon useful units, or affect small areas.
- 3-cost cards should be rare, high-impact, multi-effect, strong summon, aura, or trap cards.
- Costs above 3 are allowed but should be reviewed.

The audit estimates a simple base effect budget and converts that into an expected cost band. Elemental bonus groups are scored separately because they are conditional payoffs.

## Variety Target

Production card variety is checked by element and role. Each production element should trend toward at least:

- 2 generators
- 2 element payoffs
- 1 damage card
- 1 defense or utility card
- 1 tactical card

Role tags are derived from authored effects and card category: Damage, Defense, Healing, Control, Mobility, Draw, Energy, Generator, Element Payoff, Summon, Trap, Aura, and Tile Effect.

## Rules Text Standard

Descriptions should be concise, rules-first text. Examples:

- `Deal 2 damage.`
- `Gain 1 Fire.`
- `Apply 2 Shield.`
- `Move up to 3 tiles.`
- `Summon Fire Imp.`
- `Spend 2 Fire: Deal 2 damage.`

The audit generates suggested rules text from `Effects` and `ElementalBonusGroups`, then flags authored descriptions that are empty, stale, missing numeric values, missing element names, or using legacy terminology such as Light, Shadow, Mana, or Armor.

## New CSV Outputs

When `UCardCatalogAuditTool::RunCardCatalogAudit()` runs with CSV export enabled, it now writes:

- `CardBalanceAudit.csv`
- `CardVarietyMatrix.csv`
- `CardDescriptionSuggestions.csv`

These are written alongside the existing card, pack, and summon reports under `Saved/CardCatalog` by default.

## Future Content Pass

Use these reports to make a controlled card asset pass later:

- Normalize obviously off-curve costs and values.
- Rewrite descriptions to the rules-first standard.
- Add new cards only after reviewing variety gaps.
- Keep debug elemental channel cards out of production packs.
