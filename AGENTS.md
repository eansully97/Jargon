# Jargon Codex Instructions

Jargon is a single-player Unreal Engine tactical card / board game.

## Project Rules

- C++ owns gameplay rules and backend systems.
- Data Assets remain the source of truth for content.
- Blueprint children own visuals, UI polish, layout, meshes, VFX, SFX, and editor setup.
- Do not add Gameplay Ability System unless explicitly requested after discussion.
- Do not replace the lightweight shared Jargon effect system.
- Do not add new fallback-first architecture or compatibility paths unless explicitly requested.
- Remove legacy fallback paths once a Data Asset path exists and the migration target is clear; breakage during migration is acceptable when the follow-up fix list is explicit.
- Preserve the `Town -> Exploration -> Combat -> Rewards -> Town` loop.
- Current element names are `Fire`, `Frost`, `Storm`, `Nature`, `Radiance`, and `Quietus`; do not reintroduce `Light` or `Shadow` as element names.
- Energy remains the normal card play resource. Element charges are combat-local optional combo resources.
- `UCardDefinition` owns manual card art prompt generation. Do not recreate `CardCreationTemplateTool`, image generation, PNG import, or placeholder art assignment unless explicitly requested.
- Cards author gameplay through inline `CardScript` actions that build `FJargonEffectSpec` arrays. Do not reintroduce raw card effect structs or compatibility authoring arrays.
- Card authoring should distinguish `Operation` from `Keyword`: operations are backend primitives such as damage/heal/draw/summon, while keywords are reusable rules terms such as status definitions, traits, and future modifiers.
- Card effect lines should be described as `Operation + Delivery + Filter + Payload`, with lightweight conditions only where already supported, such as elemental bonuses.
- Elemental bonuses are manually chosen at card play time through an assigned elemental bonus choice widget class. Do not restore automatic elemental bonus resolution or boolean-gated prompt flow.
- Cards must own a `CardElement` value. `EJargonElementType::None` is the internal neutral value for cards; card-facing audit/UI language should call it Neutral.
- Deck editing is limited to three unique non-neutral card elements plus any number of Neutral cards. `CardElement` remains for filtering, packs, audits, and content identity.
- Hero aspects are combat-long transformations. The first authored element to reach 10 combat-local charges locks its aspect for the rest of combat; other element charges remain useful for bonuses and spending but do not change the transformation. Element charges are not Energy.
- Starter and prebuilt decks must be authored with `UJargonDeckDefinition`; do not reintroduce loose starter card arrays on game modes.
- Legacy, template, placeholder, duplicate, and transitional content should be actively verified and removed instead of kept indefinitely.
- Editor clarity and presentation are top priorities: avoid duplicate/conflicting fields, hide obsolete authoring surfaces, and prefer readable categories, ToolTips, validation, and clean designer-facing names.
- Prefer small, focused, reviewable changes.
- Do not edit unrelated systems.
- Do not edit binary assets unless explicitly necessary.
- Do not migrate or delete Data Assets unless explicitly requested.
- The user handles content/balance tuning. Prefer backend architecture, editor hooks, validation, and readable authoring support over per-card/per-enemy number tuning.

## Unreal Data Asset Authoring Rules

- Data Assets are static authored definitions only: tuning, UI text, default config, and content references.
- Do not store runtime state in Data Assets, such as current HP, owners, cooldowns, temporary buffs, or per-match counters.
- Data Assets are the gameplay source of truth for cards, summons, enemies, encounters, traps, auras, boons, heroes, and other definition-style systems wherever practical.
- Prefer `UPrimaryDataAsset` for catalog-style gameplay definitions when it can be done without breaking existing assets or references.
- Use soft references for heavy art, mesh, VFX, SFX, widget, and Blueprint class content when a safe migration path exists.
- Keep editor tooling guarded and out of runtime dependencies; editor-only mutation, generation, and package-save code belongs behind editor guards.
- Add Unreal data validation for authored definitions and reuse local `IsValidDefinition()` / audit helpers where appropriate.
- Preserve Blueprint compatibility when it does not keep stale fallback systems alive. Prefer explicit migrations over indefinite compatibility fields.
- Avoid giant enum / mega-struct sprawl; document pressure points before replacing them with status definitions, instanced structs, or polymorphic effect data.
- New status-like effects should prefer `UJargonStatusEffectDefinition` / future `ApplyStatus` style authoring over adding more `ApplyX` effect enum operations.
- Use metadata that makes authoring readable, such as useful `ToolTip`, `ClampMin`, `EditConditionHides`, and `TitleProperty` entries.
- End each implementation pass with a green build when C++ changes are made, plus a clear list of assets that need manual authoring or cleanup.

## Workflow

- Before editing, inspect the relevant files and summarize the exact files you plan to touch.
- Keep implementation minimal and boring.
- Prefer readable C++ with Blueprint-friendly hooks.
- After C++ changes, build:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' JargonEditor Win64 Development -Project='C:\Users\eansu\Documents\Unreal Projects\Jargon\Jargon.uproject' -WaitMutex -NoHotReloadFromIDE
```

If the build cannot link because Unreal Editor or Rider is locking `UnrealEditor-Jargon.dll` / `.pdb`, say that clearly and rerun after the editor/debugger is closed.
