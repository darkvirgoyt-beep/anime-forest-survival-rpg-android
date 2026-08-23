# Graphics Upgrade Plan

## Current state
The Android prototype uses a GLES3 renderer in `app/src/main/cpp/forest_game.cpp` with flat-color procedural 3D boxes, cylinders, spheres, 2D biome panels, and one uploaded heroine billboard texture. The Android HUD is composed in `MainActivity.kt` with native `TextView` and button overlays.

## Visual target
Original anime-fantasy survival-RPG presentation inspired by the broad qualities of polished stylized open-world games: stronger depth, richer material separation, readable silhouettes, atmospheric distance, warm practical lights, and a compact premium HUD. Do not reproduce proprietary characters, logos, screenshots, or maps.

## High-impact implementation slice
1. Add low-cost per-face lighting to the GLES3 3D shader path with a sun direction, ambient fill, and fog; keep compatibility with GLES 3.0 and current procedural geometry.
2. Upgrade world composition with layered biome terrain, shoreline/stream treatment, larger silhouettes, foliage variation, rocks, and a more readable camp/tower focal area.
3. Add lightweight bloom-like emissive halos and better translucent weather particles without changing gameplay or JNI contracts.
4. Refresh the HUD styling with translucent panels, compact status chips, improved hierarchy, and keep all existing buttons/controls functional.
5. Build the Android project and run existing native tests where available.

## Verification criteria
- The project compiles with the current C++17/Android GLES3 configuration.
- Existing JNI methods remain unchanged.
- Graphics quality tiers still affect weather density and remain usable.
- The scene has visible depth and material variation beyond flat unlit colors.
- HUD text and controls remain readable and touch-safe in landscape orientation.
