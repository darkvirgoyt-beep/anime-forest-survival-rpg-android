# AETHELGRAD Character Progression and XP Specification

## Contract

AETHELGRAD characters begin at **level 0** and can progress through **level 100**. The player has **100 maximum HP** in the Unreal production branch. XP is earned as integer points from repeatable grinding activities and one-time milestones. The HUD exposes the live current-level XP and the requirement for the next level.

## Non-repeating XP curve

The requirement to advance from level `L` to level `L + 1` is generated deterministically from the level number using an integer hash-jittered threshold:

```text
seed = integerHash(L + 1)
jitter = (seed mod 701) - 350
fineOffset = ((seed >> 10) mod 89)
requirement = 1000 × (L + 1) + jitter + fineOffset
```

The final transition is explicitly fixed to exactly `100,000 XP`:

```text
level 99 → 100 = 100,000 XP
```

This is not a random value at runtime and does not change between sessions or devices. The hidden deterministic hash makes the individual thresholds difficult to guess from one level to the next, while the 1,000-XP level bands guarantee strict ascending order. Every threshold is an integer. There is no floating-point XP calculation and no rounding of intermediate requirements.

| Transition | XP requirement |
|---|---:|
| 0 → 1 | 991 |
| 1 → 2 | 1,955 |
| 2 → 3 | 2,885 |
| 3 → 4 | 4,299 |
| 4 → 5 | 5,036 |
| 9 → 10 | 10,136 |
| 39 → 40 | 39,792 |
| 49 → 50 | 49,954 |
| 89 → 90 | 89,894 |
| 98 → 99 | 98,996 |
| 99 → 100 | **100,000** |

`experience` means live XP inside the current level. `experienceToNext` means the exact integer requirement for the next level. `totalExperience` is retained for save migration, analytics, and future progression screens.

## Grinding rewards

Repeatable gathering awards 12 XP, confirmed combat hits award 10 XP, and additional successful crafting awards 8 XP. The first ember-kit craft awards 35 XP, and the first Forest Warden defeat awards a 120 XP milestone bonus. All rewards are positive integers, and level-up processing can cross multiple boundaries in one award.

## Save compatibility

New saves use schema version 3. Older schema version 1 and 2 snapshots remain readable. When an older snapshot lacks a saved level or threshold, the progression component deterministically derives the level from the stored legacy XP and initializes the new fields without invalid negative values.
