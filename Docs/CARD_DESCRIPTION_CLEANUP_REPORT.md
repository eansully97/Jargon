# Card Description Cleanup Report

## Summary

Updated production card descriptions to match the generated rules-first text from `Saved/CardCatalog/CardDescriptionSuggestions.csv`.

This was a content-only pass:

- changed card Data Asset descriptions only
- skipped debug/dev cards
- did not change costs, values, target types, keyword payloads, packs, art, maps, widgets, configs, or gameplay code

## Results

- Production card descriptions updated: 41
- Debug descriptions intentionally skipped: 6
- Script errors: 0

Targeted Data Validation after the edit:

- `/Game/Jargon/Data/Cards`: 73 assets
- `/Game/Jargon/Data/StatusEffects`: 5 assets
- `/Game/Jargon/Data/TileEffects`: 0 assets
- `/Game/Jargon/Data/SummonedUnits`: 7 assets
- Total checked: 85
- Invalid: 0
- Warnings: 0

Card catalog audit after the edit:

- Description `Matches`: 60
- Description `Review`: 6
- Remaining description reviews are only the six debug channel cards.

## Remaining Card Catalog Warnings

| Warning area | Count | Note |
| --- | ---: | --- |
| Missing card art | 21 | Includes the six debug channel cards plus production cards that need art/prompt follow-up. |
| Not included in scanned pack | 6 | These are the debug channel cards and should stay isolated unless intentionally promoted. |
| Description differs from generated rules text | 6 | These are the debug channel cards; their debug/dev wording was preserved intentionally. |
| Legacy raw effect data | 0 | Good. |
| Missing required Data Asset payloads | 0 | Good. |

## Cards Updated

- Area of Aegis
- Backdraft
- Bloom Surge
- Bramblebind
- Chain Mend
- Chain Spark
- Cinderbrand
- Consecrate
- Cull the Weak
- Deep Freeze
- Flame Burst
- Frostbite
- Glacier Pin
- Grave Mark
- Heal
- Healing Grove
- Incinerate
- Judgement Ray
- Magnetic Hook
- Mendroot
- Overload
- Rallying Banner
- Reaping Mark
- Root Snare
- Shatter
- Shield Bash
- Shove
- Siphon
- Snare Trap
- Soul Leech
- Spark Ember
- Spike Trap
- Static Arc
- Static Field
- Static Jolt
- Static Snare
- Thorn Growth
- Thunder Clap
- Unfriendly Yeti
- Wildfire
- Wither

## Debug Cards Skipped

- Debug Channel Fire
- Debug Channel Frost
- Debug Channel Nature
- Debug Channel Quietus
- Debug Channel Radiance
- Debug Channel Storm

## Recommended Next Pass

Run a card art prompt/art assignment pass for the 21 cards still missing `CardArt`. Keep debug channel cards optional and dev-only.

