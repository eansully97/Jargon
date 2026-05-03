# Hero Aspect Authoring

## Summary

Hero aspects are authored on `UJargonHeroDefinition` as combat-long transformations.

The player-facing rule is:

```text
Save 10 charges of an element to transform into that element's aspect for the rest of combat.
```

Energy remains the normal card play resource. Fire, Frost, Storm, Nature, Radiance, and Quietus charges are element charges, not Energy.

## Authoring

- Add one `HeroAspects` entry per element transformation the hero can enter.
- Set `RequiredElement` to the element that locks the transformation.
- Set `Aspect`, `DisplayName`, `Description`, and optional `TransformationName`.
- Add `TransformationEffects` for one-shot impact when the transformation locks.
- Add `TurnStartEffects` and `EnemyDeathEffects` for passive hooks that fire only while transformed.
- Do not author per-aspect charge thresholds. The runtime rule uses the combat element charge cap, currently 10.

## Runtime Rule

- Element charges cap at 10.
- The first authored element to move from below 10 charges to 10 charges locks its aspect immediately.
- The locked aspect stays active for the rest of combat.
- Spending or clearing element charges does not remove the transformation.
- Later elements reaching 10 do not switch the transformation.
- If an element reaches 10 but the active hero definition has no aspect for it, no transformation locks and later eligible elements may still transform.
- Transformation state resets when a new combat initializes.

## Presentation

Blueprint UI can use `FJargonHeroAspectInfo`:

- `ProgressText`, such as `Fire Transformation: 7/10`
- `StatusText`, such as `Pyromancer Transformed`
- `bIsTransformed`
- `bCanTransformNow`
- `bTransformationLocked`
- `CurrentElementCharges`
- `RequiredElementCharges`, which is the runtime transformation threshold

Combat cues should use transformation language such as `Pyromancer Transformed`, not vague awakened or temporary-active language.
