# Jargon Codex Instructions

Jargon is a single-player Unreal Engine tactical card / board game.

## Project Rules

- C++ owns gameplay rules and backend systems.
- Data Assets remain the source of truth for content.
- Blueprint children own visuals, UI polish, layout, meshes, VFX, SFX, and editor setup.
- Do not add Gameplay Ability System unless explicitly requested after discussion.
- Do not replace the lightweight shared Jargon effect system.
- Do not remove `FCardEffectSpec`.
- Do not remove legacy fallback behavior from cards, traps, auras, or tile effects.
- Preserve the `Town -> Exploration -> Combat -> Rewards -> Town` loop.
- Current element names are `Fire`, `Frost`, `Storm`, `Nature`, `Radiance`, and `Quietus`; do not reintroduce `Light` or `Shadow` as element names.
- Energy remains the normal card play resource. Element charges are combat-local optional combo resources.
- `UCardDefinition` owns manual card art prompt generation. Do not recreate `CardCreationTemplateTool`, image generation, PNG import, or placeholder art assignment unless explicitly requested.
- Prefer small, focused, reviewable changes.
- Do not edit unrelated systems.
- Do not edit binary assets unless explicitly necessary.
- Do not migrate or delete Data Assets unless explicitly requested.
- The user handles content/balance tuning. Prefer backend architecture, editor hooks, validation, and readable authoring support over per-card/per-enemy number tuning.

## Workflow

- Before editing, inspect the relevant files and summarize the exact files you plan to touch.
- Keep implementation minimal and boring.
- Prefer readable C++ with Blueprint-friendly hooks.
- After C++ changes, build:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' JargonEditor Win64 Development -Project='C:\Users\eansu\Documents\Unreal Projects\Jargon\Jargon.uproject' -WaitMutex -NoHotReloadFromIDE
```

If the build cannot link because Unreal Editor or Rider is locking `UnrealEditor-Jargon.dll` / `.pdb`, say that clearly and rerun after the editor/debugger is closed.
