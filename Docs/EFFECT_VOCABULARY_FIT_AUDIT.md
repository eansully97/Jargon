# Effect Vocabulary Fit Audit

Date: 2026-05-03

## Summary

Jargon already has a solid shared effect base: damage, heal, shield, status, movement, push, pull, summon, tile-effect placement/destruction, draw, energy, element charge, radius delivery, and chain delivery all exist in the `FJargonEffectSpec` language.

The current content is structurally covered: each main element has spells, summons, traps, auras, generators, payoffs, and tactical roles. That does not mean the card experience is mechanically deep yet. The operation mix is still concentrated around a small number of repeated primitives, so the next content pass should add a few high-leverage effect vocabulary pieces before creating more cards.

## New Audit Output

`UCardCatalogAuditTool` now writes:

- `Saved/CardCatalog/EffectVocabularyFitAudit.csv`

The report separates:

- structural coverage: whether each element has the expected card types and roles
- mechanical variety: operation count, delivery count, status terms, dominant operation, and concentration warnings
- first-batch recommendations: the next effects that best unlock more card families

Use this alongside `CardElementCoverageAudit.csv`. If an element says `Covered` structurally but `ThinOperationVocabulary` or `StructurallyCoveredButConcentrated` mechanically, add effect vocabulary before adding more same-shaped cards.

## First Expansion Batch

Recommended first batch:

- `Cleanse / Remove Status`: direct resolver operation. Best for Radiance and Nature, and gives defensive cards meaningful counterplay.
- `Regen`: status definition. Turn-start healing that decays, mirroring Burn without being a hostile damage clone.
- `Weak`: status definition. Reduces the next outgoing damage instance or short-duration outgoing damage, giving control decks a softer alternative to stun/freeze/root.
- `Lifesteal`: damage modifier. Heals the source for unblocked damage dealt, supporting Quietus drain cards without creating a broad modifier framework.

Deferred:

- `Marked`: wait until there is a lightweight consume-status/payoff pattern.
- `Poison` and `Bleed`: only add if they behave meaningfully differently from Burn.
- `Drain` variants beyond Lifesteal: wait until repeated card designs justify reusable damage-result modifiers.

## Workflow

1. Run the card catalog audit after major card or effect changes.
2. Check structural coverage first so pack/card-type gaps are visible.
3. Check effect vocabulary fit next so "covered" elements do not hide repetitive play patterns.
4. Add new effects only when they unlock multiple future cards or improve multiple element lanes.
5. Create cards after the effect vocabulary is ready, not before.
