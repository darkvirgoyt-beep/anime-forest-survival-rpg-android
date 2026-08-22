# Aethelargd RPG Vertical Slice Plan

## Product intent

Build a playable dark-fantasy anime RPG slice named **Aethelargd: Wild Horizons** on top of the existing Android forest prototype. The slice should communicate a complete player-facing loop rather than only a renderer demo: explore, gather, craft, fight, gain experience, and advance a quest.

## Implemented slice target

The player controls a third-person hero in the forest clearing. A dynamic HUD exposes health, hunger, stamina, level, experience, wood, fiber, stone, and the current quest objective. The opening quest, **The First Ember**, asks the player to gather wood and fiber, craft a campfire kit, and defeat the forest warden. Gathering and combat award XP; XP triggers level-up feedback; crafting advances the quest; defeating the warden completes it and grants a final reward.

## Risk slices

1. **Native/JNI state bridge:** expose a compact read-only HUD snapshot without mutating native state from the UI thread.
2. **Deterministic progression:** keep XP, level thresholds, quest stages, inventory, enemy health, and rewards in the fixed-step native simulation so they remain testable.
3. **Combat readability:** preserve the existing combo timing while adding a visible enemy health bar, hit flash, defeat state, and respawn-safe state reset.
4. **Mobile usability:** keep the current touch controls, add a quest/pause-friendly information layer, and update HUD values on the Android main thread at a bounded cadence.
5. **Build safety:** avoid new third-party dependencies and preserve the GLES 3 / C++17 / Kotlin architecture.

## Acceptance criteria

- The app launches in landscape on Android API 26+ GLES 3 devices.
- The HUD updates from native state and visibly shows progression rather than placeholder values.
- Gathering changes inventory and XP; crafting consumes materials and advances the opening quest.
- Combat displays readable hit feedback, damages the warden, grants XP on defeat, and completes the quest.
- Level-up state is deterministic and surfaced in the HUD.
- Native regression tests continue to pass; Android build is attempted when tooling is available.
- No copyrighted or ripped game assets are included.
