# Hash-Jittered XP Code Walkthrough

The current implementation is in both progression layers:

- Android prototype: `app/src/main/cpp/rpg/progression.cpp`
- Unreal 3D branch: `Unreal/Source/ForestSlice/Private/ForestSliceProgressionComponent.cpp`

Both implementations use the same integer algorithm so the prototype and the future 3D game display consistent level requirements.

## 1. Android C++ implementation

```cpp
int Progression::experienceRequirementForLevel(int targetLevel) {
    const int safeLevel = std::clamp(targetLevel, 0, kMaxLevel);
    if (safeLevel >= kMaxLevel) return 0;
    if (safeLevel == kMaxLevel - 1) return 100000;

    // Deterministic integer hash jitter makes the thresholds difficult to guess
    // while the 1000 XP level band guarantees strict ascending order.
    std::uint32_t seed = static_cast<std::uint32_t>(safeLevel + 1)
                       * 0x9E3779B9u + 0x7F4A7C15u;
    seed ^= seed >> 16;
    seed *= 0x85EBCA6Bu;
    seed ^= seed >> 13;
    seed *= 0xC2B2AE35u;
    seed ^= seed >> 16;

    const int jitter = static_cast<int>(seed % 701u) - 350;
    const int fineOffset = static_cast<int>((seed >> 10) % 89u);
    return (safeLevel + 1) * 1000 + jitter + fineOffset;
}
```

`safeLevel` prevents invalid levels from entering the calculation. A level of 100 has no next level, so the function returns `0`. The level 99 transition is explicitly fixed to `100000`, guaranteeing that the final requirement is exact rather than produced by an approximation.

For all earlier levels, the level number becomes a deterministic 32-bit seed. The XOR shifts and multiplications mix the bits. The constants are fixed, so the result is stable across restarts and devices; this is not runtime randomness. However, the output is not an obvious linear, square, or repeating sequence.

The output has three parts:

```text
base band  = 1000 × (level + 1)
jitter     = -350 to +350
fine offset = 0 to 88
```

Because the variation is much smaller than the 1,000 XP band between levels, the implementation produces strictly ascending requirements. All operations use integers, so no XP value is rounded.

## 2. Unreal 3D implementation

The Unreal component uses the same algorithm with Unreal integer types:

```cpp
int32 UForestSliceProgressionComponent::GetXPRequirementForLevel(int32 Level) const
{
    const int32 SafeLevel = FMath::Clamp(Level, 0, MaxLevel);
    if (SafeLevel >= MaxLevel) return 0;
    if (SafeLevel == MaxLevel - 1) return 100000;

    uint32 Seed = static_cast<uint32>(SafeLevel + 1)
                * 0x9E3779B9u + 0x7F4A7C15u;
    Seed ^= Seed >> 16;
    Seed *= 0x85EBCA6Bu;
    Seed ^= Seed >> 13;
    Seed *= 0xC2B2AE35u;
    Seed ^= Seed >> 16;

    const int32 Jitter = static_cast<int32>(Seed % 701u) - 350;
    const int32 FineOffset = static_cast<int32>((Seed >> 10) % 89u);
    return (SafeLevel + 1) * 1000 + Jitter + FineOffset;
}
```

The Unreal component is replicated by default. XP awards are accepted only on the authoritative owner:

```cpp
int32 UForestSliceProgressionComponent::AwardExperience(int32 Amount)
{
    if (Amount <= 0 || !GetOwner() || !GetOwner()->HasAuthority() || IsMaxLevel()) return 0;

    const int32 PreviousLevel = State.Level;
    State.Experience += Amount;
    State.TotalExperience += Amount;

    while (State.Level < MaxLevel) {
        State.ExperienceToNext = GetXPRequirementForLevel(State.Level);
        if (State.Experience < State.ExperienceToNext) break;
        State.Experience -= State.ExperienceToNext;
        ++State.Level;
    }

    NormalizeState();
    if (State.Level != PreviousLevel) BroadcastLevelChanged();
    return Amount;
}
```

This prevents clients from independently granting themselves XP in the multiplayer-ready 3D path. The loop also supports a large XP reward crossing more than one level in a single award.

## 3. Example values

The first transitions currently resolve to:

| Transition | Exact requirement |
|---|---:|
| 0 → 1 | 991 |
| 1 → 2 | 1,955 |
| 2 → 3 | 2,885 |
| 3 → 4 | 4,299 |
| 4 → 5 | 5,036 |
| 9 → 10 | 10,136 |
| 49 → 50 | 49,954 |
| 89 → 90 | 89,894 |
| 98 → 99 | 98,996 |
| 99 → 100 | **100,000** |

## 4. How the level-up calculation works

Suppose a level-0 player has 980 XP and receives 20 XP. The current requirement is 991 XP. The new total becomes 1,000 XP, so the player levels from 0 to 1 and carries the remaining 9 XP into level 1. The next requirement is then calculated from level 1, which is 1,955 XP.

At level 100, `ExperienceToNext` becomes `0`, current-level XP is held at `0`, and the HUD displays `XP MAX`. Additional awards do not create a level 101.

## 5. Verification

`tests/progression_test.cpp` checks the exact sample values, verifies that all 100 thresholds are strictly ascending, verifies that no threshold repeats, checks the 100,000 XP final requirement, and tests the level cap. `tests/cloud_state_test.cpp` verifies schema-3 persistence and schema-1/schema-2 compatibility.
