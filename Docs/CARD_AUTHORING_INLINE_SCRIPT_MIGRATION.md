# Card Authoring Inline Script Migration

## What Changed

Cards now author gameplay through `CardScript`, an inline object on `UCardDefinition`.

Instead of filling one large raw `Effects` struct with many conditional fields, designers add focused action objects:

- `Deal Damage`
- `Heal`
- `Apply Shield`
- `Apply Status`
- `Move Self`
- `Push Target`
- `Pull Target`
- `Summon Unit`
- `Place Tile Effect`
- `Destroy Tile Effect`
- `Draw Cards`
- `Gain Energy`
- `Gain Element Charge`
- `Chain`

Elemental bonuses now live on the same `CardScript` as readable bonus entries with their own focused action lists.

## Runtime Direction

`FCardResolver` now resolves cards from `CardScript` actions. Those actions build `FJargonEffectSpec` arrays and still use the shared `FJargonEffectResolver` backend.

The legacy raw card effect structs and `UCardDefinition` raw effect arrays have been removed. Card assets should author gameplay through `CardScript` effect lines only.

## Migration

The old migration commandlet has been removed because migrated CardScript assets are now the baseline. Future card cleanup should use validation and audit reports, not raw-effect conversion tooling.

## Validation

`UCardDefinition::IsDataValid()` treats missing `CardScript` as an error.

The CardScript validation path reports focused action labels, for example `CardScript action 0 (SummonUnit Definition=None AttackExhausted=true) requires SummonedUnitDefinition.` Audit reports and pack health checks now read `FJargonEffectSpec` arrays built from CardScript instead of projecting back into a legacy card-specific struct.

Collapsed action and elemental bonus rows use generated editor titles from the current authored values, so migrated cards should read more like card rules in the Details panel.

## Current Manual Cleanup After Migration

Targeted validation of `/Game/Jargon/Data/Cards` found the following migrated cards still need authoring data:

- `DA_Card_Summon_FireImp`: assign `SummonedUnitDefinition`.
- `DA_Card_Summon_OgreMercenary`: assign `SummonedUnitDefinition`.
- `DA_Card_Summon_PigeonProtector`: assign `SummonedUnitDefinition`.
- `DA_Card_Summon_Reapsassin`: assign `SummonedUnitDefinition`.
- `DA_Card_Summon_ShieldGuardian`: assign `SummonedUnitDefinition`.
- `DA_Card_UnfriendlyYeti`: assign `SummonedUnitDefinition`.
- `DA_Card_AreaOfAegis`: assign `TileEffectDefinition` on its place-tile-effect action.
- `DA_Card_Aura_HealingGrove`: assign `TileEffectDefinition` on its place-tile-effect action.
- `DA_Card_Aura_StaticField`: assign `TileEffectDefinition` on its place-tile-effect action.
- `DA_Card_RallyingBanner`: assign `TileEffectDefinition`.
- `DA_Card_Trap_SnareTrap`: assign `TileEffectDefinition`.
- `DA_Card_Trap_SpikeTrap`: assign `TileEffectDefinition`.
- `DA_WildfireDefinition`: this tile-effect definition asset is currently under the card trap folder and is missing `DisplayName` and `Effects`; static mesh presentation now belongs on the runtime trap/aura Blueprint child.

## Notes

- Summon actions require `UJargonSummonedUnitDefinition`.
- Trap and aura actions require `UJargonTileEffectDefinition`.
- Debug cards were migrated too so validation stays quiet.
- The shared effect resolver remains the backend; this pass changes authoring shape, not the combat rule engine.
