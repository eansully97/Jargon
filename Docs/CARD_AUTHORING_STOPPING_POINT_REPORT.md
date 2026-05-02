# Card Authoring Stopping Point Report

## Summary

The CardScript keyword authoring model is stable enough to use in-editor for normal card creation. The current blocker is no longer backend shape or missing payload validation; it is content polish.

Latest targeted validation checked:

- `/Game/Jargon/Data/Cards`: 73 assets
- `/Game/Jargon/Data/StatusEffects`: 5 assets
- `/Game/Jargon/Data/TileEffects`: 0 assets
- `/Game/Jargon/Data/SummonedUnits`: 7 assets

Result: 85 checked, 0 invalid, 0 warnings.

Latest card catalog audit scanned 66 card rows. Audit warnings remain, but they are authoring-quality warnings rather than broken data.

## What Is Stable

- Production cards use CardScript keyword entries.
- Legacy raw `Effects` and `ElementalBonusGroups` are not showing up in the audit as active authored data.
- Status, summon, and tile-effect keyword payloads are present where the CardScript model requires them.
- All six current elements have baseline generator, payoff, damage, defense/utility, and tactical coverage.
- The generated audit CSVs use keyword language such as `PrimaryKeyword`, `SecondaryKeywords`, and `UniquePrimaryKeywords`.

## Remaining Audit Work

| Area | Count | Meaning |
| --- | ---: | --- |
| Description differs from generated rules text | 47 | Mostly wording/style drift, not gameplay breakage. |
| Missing card art | 21 | These need art prompt/art assignment passes. |
| Card not in scanned pack | 6 | These appear to be debug/dev cards and should stay isolated unless intentionally promoted. |
| Boring pattern design flags | 15 | Mostly simple single-keyword cards or debug generators. |
| Sameness design flags | 5 | Mostly repeated summon-card tactical fingerprints. |
| Legacy raw effect data | 0 | Good. |
| Missing required keyword payloads | 0 | Good. |

## Highest-Value Next Content Pass

Do a controlled description cleanup pass first.

Why:

- It is low risk and does not change gameplay.
- It makes the in-editor card authoring surface feel coherent immediately.
- The audit already generated exact suggestions in `Saved/CardCatalog/CardDescriptionSuggestions.csv`.
- Many differences are terminology-only, such as `Apply 1 Root` becoming `Apply 1 Root turn`, or `in an area` becoming `in radius 1`.

Suggested scope:

- Update production card descriptions to rules-first wording.
- Keep debug-card descriptions explicitly debug/dev-only even if they intentionally do not match the generated rules text.
- Do not change cost, value, pack membership, target type, card art, or keyword payloads.
- Rerun targeted Data Validation and the card catalog audit afterward.

## Second Content Pass

Handle missing art prompts/art assignment after descriptions.

Current missing-art count is 21. This should be a separate pass because card art work has different review criteria than rules text.

Suggested scope:

- Generate or update card art prompts for missing-art cards.
- Do not assign placeholder art unless explicitly requested.
- Keep debug cards without art if they remain dev-only.
- Use the CardScript keyword summaries as the visual prompt source.

## Third Design Pass

Review the remaining design flags after descriptions are clean.

Suggested focus:

- Simple starter cards can stay simple if they teach the game.
- Debug generators should be excluded from production pack quality judgments.
- Repeated summon fingerprints are expected until summon definitions carry more distinctive lifecycle effects, visuals, or role text.
- Pack sameness should be reviewed after the final production/debug split is clear.

## What Not To Do Yet

- Do not build the bigger generic Delivery/Payload object model yet.
- Do not add new effect keywords just to quiet audit warnings.
- Do not rebalance cards as part of description cleanup.
- Do not migrate or delete debug cards until the production/debug folder and pack policy is explicit.
- Do not change the shared resolver while the current CardScript authoring surface is being tested in-editor.

## Recommended Next Prompt

```text
Implement a controlled card description cleanup pass. Modify only production card Data Assets whose authored description differs from the generated rules-first suggestion. Use Saved/CardCatalog/CardDescriptionSuggestions.csv as the source of truth, but preserve explicit debug/dev-only wording on debug cards. Do not change card costs, values, target types, keyword payloads, packs, art, maps, widgets, configs, or gameplay code. Rerun targeted Data Validation for /Game/Jargon/Data/Cards and rerun the card catalog audit. Build JargonEditor Win64 Development only if source changes are required.
```

