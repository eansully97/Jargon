# Data Asset Audit

Date: 2026-05-01

Scope: source/docs-only guardrail pass for current `UDataAsset` and `UPrimaryDataAsset` usage in Jargon. No assets, maps, widgets, Blueprints, or configs were modified.

## Executive Summary

Jargon is moving in the right direction: gameplay content is already mostly authored through Data Assets and interpreted by C++ runtime systems. This pass made the static gameplay catalog definitions `UPrimaryDataAsset` subclasses, kept audit/settings objects as `UDataAsset`, added Unreal asset-validation hooks, and improved collapsed-array readability with `TitleProperty` metadata.

The project direction is now no-fallback Data Asset ownership. Data Assets should become the canonical authoring surface for cards, summons, enemies, encounters, traps, auras, boons, heroes, and other definition-style gameplay systems. Legacy compatibility paths should be removed once a replacement Data Asset path exists and the required asset authoring work is identified.

The biggest remaining architecture risk is not the base class of the assets. It is the growing "wide union" shared effect-data shape: `FJargonEffectSpec` continues to accumulate operation-specific fields and enum entries. That is acceptable for the current prototype, but status/effect growth should eventually move toward generic status definitions and/or instanced effect payloads once the gameplay language stabilizes.

## Inventory

| Class | Previous Base | Current Base | Decision | Priority | Notes |
| --- | --- | --- | --- | --- | --- |
| `UCardDefinition` | `UDataAsset` | `UPrimaryDataAsset` | Converted | High | Core card catalog definition. Added validation for missing/invalid effects, summon/tile requirements, element bonus sanity, and hard-reference warnings. |
| `UCardPackDefinition` | `UDataAsset` | `UPrimaryDataAsset` | Converted | High | Static shop/pack catalog data. Added validation for pack price, grant count, pool entries, weights, and referenced cards. |
| `UJargonHeroDefinition` | `UDataAsset` | `UPrimaryDataAsset` | Converted | High | Static hero class/aspect definition. Added validation for core stats, passive/aspect effect specs, duplicate/invalid aspect authoring, and hard presentation references. |
| `UJargonRelicDefinition` | `UDataAsset` | `UPrimaryDataAsset` | Converted | Medium | Static Hero Boon/relic definition. Added validation for required text/effects, eligibility filters, icon hard-reference warning, and effect specs. |
| `UJargonSummonedUnitDefinition` | `UDataAsset` | `UPrimaryDataAsset` | Converted | High | Static data-driven summon definition. Added validation for unit stats, required skeletal mesh and animation overrides, optional special runtime shell override, and lifecycle effect specs. |
| `UJargonTileEffectDefinition` | New | `UPrimaryDataAsset` | Added | High | Static data-driven trap/aura definition. Runtime shell class, required static mesh override, trigger, duration, radius, and shared effects are authored here. |
| `UEncounterDefinition` | `UDataAsset` | `UPrimaryDataAsset` | Converted | Medium | Static encounter definition. Added validation for combat map, enemy spawns, reward values, and hard enemy class references. |
| `UCardCatalogAuditTool` | `UDataAsset` | `UDataAsset` | Kept | Low | Editor/audit tool object, not gameplay catalog data. Existing editor work remains implementation-guarded. |
| `UProjectSetupAuditTool` | `UDataAsset` | `UDataAsset` | Kept | Low | Editor/audit tool object, not gameplay catalog data. Existing editor work remains implementation-guarded. |
| `UJargonCombatPresentationSettings` | `UDataAsset` | `UDataAsset` | Kept | Medium | Runtime presentation settings, not a gameplay catalog asset. Hard VFX/SFX/widget references are intentional for now. |

## Supporting Structs Reviewed

| Struct | Use | Notes |
| --- | --- | --- |
| `CardScript` action classes | Card effect-line authoring | Card actions now build `FJargonEffectSpec` directly. The old raw card effect structs were removed after migration. |
| `FJargonCardElementalBonusScript` | Card elemental bonus authoring | Readable collapsed entries for bonus groups and bonus actions. Validation checks required element, charge count, and empty actions. |
| `FJargonEffectSpec` | Shared effect payload | Used by hero passives, boons, summons, and shared resolver. Validation helper now checks common invalid combinations. |
| `FWeightedCardPackEntry` | Card pack entry | Added editor ToolTips and pack array `TitleProperty`. Validation checks null card and invalid weight. |
| `FJargonHeroClassPassiveDefinition` | Hero passive authoring | Added `TitleProperty` to effect arrays. Validation checks authored effect specs. |
| `FJargonHeroAspectDefinition` | Hero aspect authoring | Added `TitleProperty` to effect arrays. Validation warns about duplicate or incomplete aspect entries. |
| `FEncounterEnemySpawn` | Encounter authoring | Forward-declared `ABattleUnit`, added ToolTips, and made encounter arrays readable by unit class. |
| `FJargonCurrencyAmount` | Currency values | Used in packs and encounter rewards. Validation now catches negative denominations where definitions own rewards/prices. |

## Runtime State Risks

No current catalog Data Asset was found storing obvious mutable runtime state such as current HP, current owner, remaining cooldown, temporary buffs, or per-combat counters.

Deferred watch items:

- `UJargonCombatPresentationSettings` is runtime-facing configuration, not gameplay state. It should remain static settings and avoid accumulating live combat state.
- Future saved deck data should stay in run/save structs, not in card or pack definition assets.
- Hero Boon/relic ownership should remain in run state (`RunRelics` / boon aliases), not in `UJargonRelicDefinition`.

## Hard Reference Risks

This pass intentionally did not convert hard references to soft references. That would require asset migration review and careful runtime loading updates.

Current hard-reference risks:

- `UCardDefinition::CardArt` is a hard `UTexture2D` reference. This is fine for the prototype, but a large card catalog should consider `TSoftObjectPtr<UTexture2D>`.
- CardScript summon keywords now require `SummonedUnitDefinition` plus `RuntimeSummonedUnitClass`; the runtime class is authored on the card keyword, not on the summon definition.
- CardScript trap/aura keywords now require `TileEffectDefinition` plus `RuntimeTileEffectClass`; the runtime class is authored on the card keyword, not on the tile-effect definition.
- `UJargonHeroDefinition::HeroSkeletalMesh` and `Portrait` are hard presentation references.
- `UJargonRelicDefinition::Icon` is a hard UI reference.
- `UJargonSummonedUnitDefinition::Icon` and animation overrides are hard references. The skeletal mesh now belongs on the runtime summon Blueprint child.
- `UJargonTileEffectDefinition::Icon` is a hard reference. Static mesh/VFX presentation now belongs on the runtime trap/aura Blueprint child.
- `UEncounterDefinition::EnemySpawns` uses hard `ABattleUnit` classes.
- `UJargonCombatPresentationSettings` uses hard VFX/SFX/widget class references and is intentionally left as a presentation settings asset.

Recommended migration path:

1. Add the Data Asset-owned field or definition type that represents the intended gameplay authoring surface.
2. Migrate current production assets to the new field/type.
3. Update runtime code to require the Data Asset path.
4. Remove stale fallback branches and duplicate authoring fields instead of keeping them indefinitely.
5. Keep a short fix list for any assets broken by the hard switch.

## No-Fallback Data Asset Roadmap

Summons:

- Make summon card/effect authoring require `UJargonSummonedUnitDefinition`.
- UnitClass-only card/effect fallback branches have been removed from the normal resolver path; old card-level UnitClass data is deprecated serialized cleanup work.
- Summon definitions now own the skeletal mesh and base idle/basic-attack/death animations applied to the generic runtime summon shell during initialization.
- Keep Blueprint unit classes for visuals or special actors only when referenced by the summon definition, not as duplicate card-level gameplay authoring.

Enemies and encounters:

- Introduce an enemy/unit definition asset for combat enemy stats, spawn/presentation class, effects, and AI-facing data.
- Update encounter spawns to reference enemy definitions instead of raw `ABattleUnit` classes.
- Remove raw encounter `UnitClass` authoring once encounter assets are migrated.

Traps, auras, and tile effects:

- `UJargonTileEffectDefinition` now owns trigger type, duration, radius, shared effect specs, and the runtime shell class.
- `UJargonTileEffectDefinition` also owns the static mesh applied to the generic trap/aura runtime shell during initialization.
- Cards now require tile-effect definitions rather than direct tile-effect Blueprint classes in validation/runtime.
- Keep tile-effect actor classes as generic runtime presenters/executors, not as the primary gameplay definition.

Cards and effects:

- Keep the lightweight shared effect system.
- Remove duplicate legacy payload fields once their Data Asset replacement exists.
- Keep validation focused on actionable authoring errors, stale fallback usage, and mixed/ambiguous effect context.

Cleanup:

- Verify legacy/template/placeholder deletion candidates with source search and Asset Registry dependency checks.
- Delete verified clutter instead of moving it into permanent "keep for now" status.
- Prefer small cleanup batches with clear rollback paths.

## Why Tile Effect Definitions Help

Previous trap and aura cards pointed at `TileEffectClass`, so behavior was split between card fields, Blueprint actor classes, legacy tile-effect enums, and shared effect specs. A tile-effect Data Asset gives designers one readable place to author what the tile does while a generic actor handles placement/runtime lifecycle.

This reduces Blueprint child proliferation, makes traps and auras auditable like cards and summons, and lets validation catch missing duration, trigger, target, and effect data before combat.

## Data Validation Noise Reduction

Unreal Data Validation should point designers toward content that needs action now. Hard-reference migration warnings are still real architecture notes, but they are not immediate asset-health problems, so this pass keeps them documented here and in audit-style reports rather than emitting them from `IsDataValid`.

Validation should still report:

- Missing required data, invalid numeric ranges, and invalid enum combinations.
- Missing summon or tile-effect definitions, because those now block data-driven runtime resolution.
- Risky mixed target-context elemental bonuses.
- Placeholder or legacy authoring fields that require a deliberate content decision.
- Asset-load or serialized-data health problems reported by Unreal itself.

## Validation Added

### Cards

`UCardDefinition::IsDataValid` now reports:

- Empty `DisplayName`.
- Negative `Cost` or `Range`.
- Missing `Effects`.
- `Operation=None`.
- Effect values that must be positive.
- `GainElementCharge` without an element.
- invalid move/push/pull distances.
- Chain effects with invalid `ChainCount`.
- Chain effects with implicit/default radius as a warning.
- `SummonUnit` missing `SummonedUnitDefinition`.
- `PlaceTileEffect` missing `TileEffectDefinition`.
- Elemental bonus groups with invalid element, invalid charge count, empty effects, or risky mixed friendly/hostile spend context.

### Card Packs

`UCardPackDefinition::IsDataValid` now reports:

- Empty `DisplayName`.
- Negative price denominations.
- Invalid grant count.
- Empty `CardPool`.
- Null card entries.
- Invalid weights.
- Referenced cards that fail `IsValidDefinition()`.
- Duplicate-disallowed packs whose grant count exceeds unique card count.

### Heroes

`UJargonHeroDefinition::IsDataValid` now reports:

- Empty `DisplayName`.
- Suspicious `HeroClass=None`.
- Invalid HP, move range, attack range, or attack damage.
- Invalid class passive effect specs.
- Duplicate aspect entries.
- Duplicate required element entries.
- Aspect entries with `Aspect=None`, `RequiredElement=None`, or invalid charge counts.
- Aspect entries with no passive effects.
- Invalid aspect passive effect specs.

### Hero Boons / Relics

`UJargonRelicDefinition::IsDataValid` now reports:

- Empty `DisplayName`.
- No authored combat/player-turn/enemy-death effects.
- Eligibility arrays containing `None`.
- Invalid shared effect specs.

### Summoned Units

`UJargonSummonedUnitDefinition::IsDataValid` now reports:

- Empty `DisplayName`.
- Missing `IdleAnimationOverride`, `BasicAttackAnimationOverride`, or `DeathAnimationOverride`.
- Invalid HP, movement, attack range, or attack damage.
- Invalid lifecycle effect specs.

### Tile Effects

`UJargonTileEffectDefinition::IsDataValid` now reports:

- Empty `DisplayName`.
- `TileEffectCategory` values other than Trap or Aura.
- Negative duration or radius.
- Empty `Effects`.
- Invalid shared effect specs.

### Encounters

`UEncounterDefinition::IsDataValid` now reports:

- Missing `CombatMapName`.
- Empty `EnemySpawns`.
- Enemy spawn entries with missing unit class.
- Negative reward denominations.

## Designer UX Improvements Applied

- Added `TitleProperty` to card effects, elemental bonus groups, bonus effects, pack card pools, hero passive/aspect effects, boon/relic effect arrays, summon lifecycle effect arrays, and encounter enemy spawns.
- Added obvious ToolTips and clamp metadata to pack, encounter, and shared run-state authoring fields.
- Kept existing editor buttons and reflected debug fields stable for Blueprint compatibility.
- Did not wrap reflected editor fields in `WITH_EDITORONLY_DATA` in this pass because that can alter serialized property availability and editor workflows.

## Header Cleanup Notes

- Converted gameplay definition headers inherit from `UPrimaryDataAsset` and include Unreal's `Engine/DataAsset.h`, which is where UE 5.7 declares `UPrimaryDataAsset`.
- Tool/settings Data Assets continue to include `Engine/DataAsset.h`.
- `CardDefinition.h` and `EncounterTypes.h` were checked for forward-declaration cleanup, but their `TSubclassOf` fields are used by inline/validation code paths that instantiate `StaticClass()`. Keeping the full `ABattleTileEffect` and `ABattleUnit` includes is the compile-safe choice for this pass.
- `JargonEffectTypes.h` remains forward-declared to avoid existing UHT include cycles with `ABattleUnit`, `ABattleTileEffect`, and combat cue types. Source files that inspect `TSubclassOf` values should include the concrete unit/tile-effect headers directly.

## Editor Boundary Notes

Existing editor tooling in `UCardDefinition`, `UCardCatalogAuditTool`, and `UProjectSetupAuditTool` remains runtime-module code with editor-only implementations guarded by `WITH_EDITOR`. That is acceptable for the current project layout.

Deferred stricter boundary work:

- Move asset creation, package save, prompt generation, and audit export code into editor-only modules if the project grows.
- Wrap editor-only reflected data in `WITH_EDITORONLY_DATA` only after checking asset serialization and Blueprint/editor expectations.
- Keep runtime modules free of hard dependencies on editor-only modules.

## Deferred Risky Migrations

These were intentionally documented instead of applied:

- Convert `TObjectPtr<UTexture2D>`, mesh, VFX, SFX, widget, and other presentation references to soft references.
- Convert `TSubclassOf<ABattleUnit>` summon and encounter references to fully data-driven unit definitions.
- Migrate existing trap/aura card assets to `UJargonTileEffectDefinition` assets and then remove the deprecated serialized card/effect fields.
- Replace enum growth like `ApplyBurn`, `ApplyRoot`, and `ApplyVulnerable` with a generic `ApplyStatus` plus `UStatusEffectDefinition`.
- Replace wide structs like `FJargonEffectSpec` with `FInstancedStruct`, polymorphic effect definitions, or another variant system only if the shared effect payload becomes a real maintenance blocker.
- Split gameplay definition data from presentation-only data for cards, heroes, summons, boons, and encounters.
- Remove deprecated serialized summon/tile fields after production assets are migrated to the definition-owned path.

## Priority Recommendations

High:

- Keep expanding validation around card/effect target compatibility.
- Add status-definition architecture before many more status enum entries are added.
- Continue moving summon, enemy, trap, and aura authoring toward definition assets with explicit migration checklists instead of runtime fallback gates.

Medium:

- Add soft-reference migration plans for card art, hero portraits, summon icons, and presentation assets.
- Add centralized effect validation shared by cards, boons, heroes, summons, traps, auras, and encounters.
- Add audit output for hard-reference load pressure.

Low:

- Consider moving audit tools into an editor module once the project structure justifies it.
- Add richer ToolTips and categories gradually as designers author more content.
