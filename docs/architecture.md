# Architecture

## Runtime boundary

The Android app uses Kotlin as the lifecycle and platform shell around a native C++17 gameplay/rendering core.

```text
Android Activity (Kotlin)
        |
        | JNI bridge: init / resize / render / setMove / attack / craft
        v
Native game core (C++17)
        |
        +-- OpenGL ES 3 renderer
        +-- movement and camera state
        +-- combat feedback state
        +-- resource and crafting state
        +-- future deterministic AI/inventory modules
```

Kotlin owns the `Activity`, orientation, immersive presentation, `GLSurfaceView`, HUD buttons, and virtual joystick. C++ owns frame rendering and the authoritative prototype state. The bridge is intentionally small so the game core can later be tested independently and expanded into modules.

## Planned modules

| Module | Initial location | Purpose |
|---|---|---|
| Renderer | `app/src/main/cpp/forest_game.cpp` | OpenGL ES scene, shader setup, procedural forest silhouettes, hero, animals, and resource landmarks. |
| Locomotion | future `native/src/locomotion/` | Movement, camera spring arm, collision, and touch input smoothing. |
| Combat | future `native/src/combat/` | Hitboxes, combo buffer, damage, health, and enemy state machines. |
| Inventory | future `native/src/inventory/` | Item definitions, stacking, recipes, and save-safe serialization. |
| Android bridge | `app/src/main/java/.../MainActivity.kt` | Lifecycle, controls, UI, haptics, local persistence, and device capability detection. |

## Design principles

The first slice favors a stable vertical loop over asset volume. A single original hero, a compact forest, two animal archetypes, a few resources, and three recipes are enough to validate touch movement and the gathering/crafting rhythm. Content data should move into JSON or native resources once the first loop is playable.

All art and narrative must be original or properly licensed. The requested commercial-game references are treated as genre and mood references only; this repository must not include copied characters, names, logos, dialogue, models, textures, or ripped files.
