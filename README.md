# Forest Slice — Android RPG Prototype

**Forest Slice** is an original anime-inspired Android game prototype built around a small third-person forest-survival loop. It is inspired by broad genre themes—exploration, gathering, crafting, animals, and light RPG combat—but does not copy any existing game's characters, names, story, models, textures, or proprietary assets.

> This repository is a vertical slice, not a claim to reproduce the production scale or visual fidelity of a large commercial title.

## Current milestone

The first slice renders a stylized forest scene through OpenGL ES 3, includes an original hero silhouette, animals, resource landmarks, a day/night ambience pulse, mobile joystick movement, attack feedback, and a basic craft action. The native core is C++17. Kotlin owns Android lifecycle, touch UI, haptics-ready integration points, and future local save wiring.

## Technology split

| Layer | Responsibility |
|---|---|
| Kotlin | `Activity` lifecycle, landscape/full-screen mode, HUD, touch controls, Android integration, future save and device capability APIs. |
| C++17 | OpenGL ES renderer, movement state, combat pulse, resource counters, and deterministic gameplay primitives. |
| Gradle + CMake | Reproducible Android/NDK build configuration for `arm64-v8a` and `x86_64`. |
| GitHub Actions | CI configuration for debug APK assembly and native source checks. |

## Build locally

Install Android Studio with SDK 35, NDK 27.x, CMake 3.22.1, and a JDK 17 runtime. Then run:

```bash
./gradlew assembleDebug
```

The debug APK is produced under `app/build/outputs/apk/debug/`. Install it on a landscape Android phone with:

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

The sandbox used to create this repository does not include the Android SDK or Godot editor, so final APK compilation must run on a machine or GitHub Actions runner with Android tooling installed.

## Controls

The left virtual joystick moves the hero. `ATTACK` shows the native combat feedback pulse. `CRAFT` consumes wood and fiber when available; the prototype keeps the inventory values in the native layer for the next UI-binding pass.

## Roadmap

The next production slices should add a proper 3D camera, authored or legally licensed low-poly assets, animation clips, collision geometry, real inventory/crafting UI, enemy state machines, save/load, audio, device performance profiles, and a signed release pipeline. Generated or imported assets must be original or carry a compatible license.

## Repository status

This is an early implementation foundation. It is intentionally small enough to review and extend, while preserving the Kotlin/C++ boundary needed for a larger Android game.
