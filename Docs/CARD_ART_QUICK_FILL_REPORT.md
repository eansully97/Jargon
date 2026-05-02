# Card Art Quick Fill Report

## Summary

Assigned existing textures from `/Game/CardArt` to every card that was missing `CardArt`.

This was a content-only pass:

- changed only `CardArt` on card Data Assets
- used existing texture assets
- did not change card descriptions, costs, values, target types, keyword payloads, packs, maps, widgets, configs, or gameplay code

## Results

- Card art assignments made: 21
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

- `missing CardArt`: 0
- `legacy raw`: 0
- `missing a required Data Asset payload`: 0
- Remaining catalog warnings: 13

The remaining warnings are not missing-art warnings. They are mostly debug channel cards being intentionally outside scanned packs and intentionally keeping debug/dev description wording.

## Assignments

| Card | Texture |
| --- | --- |
| Debug Channel Fire | `/Game/CardArt/ElementIcons/T_FireIcon` |
| Debug Channel Frost | `/Game/CardArt/ElementIcons/T_FrostIcon` |
| Debug Channel Nature | `/Game/CardArt/ElementIcons/T_NatureIcon` |
| Debug Channel Quietus | `/Game/CardArt/ElementIcons/T_QuietusIcon` |
| Debug Channel Radiance | `/Game/CardArt/ElementIcons/T_RadianceIcon` |
| Debug Channel Storm | `/Game/CardArt/ElementIcons/T_StormIcon` |
| Backdraft | `/Game/CardArt/T_WildFire` |
| Bramblebind | `/Game/CardArt/T_MendRoot` |
| Cinderbrand | `/Game/CardArt/T_FlameBurst` |
| Glacier Pin | `/Game/CardArt/T_DeepFreeze` |
| Grave Mark | `/Game/CardArt/T_ReapingMark` |
| Grave Whisper | `/Game/CardArt/T_GraveWhisper` |
| Magnetic Hook | `/Game/CardArt/T_QuickCurrent` |
| Overload | `/Game/CardArt/T_Shock` |
| Root Snare | `/Game/CardArt/T_SnareTrap` |
| Shatter | `/Game/CardArt/T_DeepFreeze` |
| Soul Leech | `/Game/CardArt/T_Siphon` |
| Static Jolt | `/Game/CardArt/T_Shock` |
| Sunward | `/Game/CardArt/T_DawnPrayer` |
| Thorn Growth | `/Game/CardArt/T_MendRoot` |
| Cull the Weak | `/Game/CardArt/T_ReapingMark` |

## Recommended Next Pass

Review the 13 remaining card catalog warnings and decide whether debug channel cards should stay intentionally noisy or be filtered out of production audit status.

