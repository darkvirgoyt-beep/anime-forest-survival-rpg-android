# AETHELGRAD Character Progression and XP Specification

## Contract

AETHELGRAD characters begin at **level 0** and can progress through **level 100**. The player has **100 maximum HP** in the Unreal production branch. XP is earned as integer points from repeatable grinding activities and one-time milestones. The HUD exposes the live current-level XP and the requirement for the next level.

## Exact XP curve

The requirement to advance from level `L` to level `L + 1` is:

```text
XP requirement = 10 × (L + 1)²
```

This is calculated with integer arithmetic. It produces ascending requirements and reaches exactly `100,000 XP` for the final transition from level 99 to level 100:

```text
10 × 100² = 100,000
```

There is no floating-point interpolation or rounding in the curve. At level 100, the character is capped; new XP does not raise the level beyond 100.

| Transition | XP requirement |
|---|---:|
| 0 → 1 | 10 |
| 1 → 2 | 40 |
| 2 → 3 | 90 |
| 3 → 4 | 160 |
| 4 → 5 | 250 |
| 9 → 10 | 1,000 |
| 49 → 50 | 25,000 |
| 89 → 90 | 81,000 |
| 98 → 99 | 98,010 |
| 99 → 100 | **100,000** |

`experience` means live XP inside the current level. `experienceToNext` means the exact integer requirement for the next level. `totalExperience` is retained for save migration, analytics, and future progression screens.

## Grinding rewards

Repeatable gathering awards XP, combat hits and defeats award XP, and repeatable crafting awards XP. Quest milestones continue to grant their existing bonus XP. All rewards are positive integers, and level-up processing can cross multiple boundaries in one award.

## Save compatibility

New saves use schema version 3. Older schema version 1 and 2 snapshots remain readable. When an older snapshot lacks a saved level or threshold, the progression component deterministically derives the level from the stored legacy XP and initializes the new fields without invalid negative values.
