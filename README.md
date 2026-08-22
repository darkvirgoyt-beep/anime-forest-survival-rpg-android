# Forest Slice — Android RPG Prototype

**Forest Slice** is an original anime-inspired Android game prototype built around a small third-person forest-survival loop. It is inspired by broad genre themes—exploration, gathering, crafting, animals, and light RPG combat—but does not copy any existing game's characters, names, story, models, textures, or proprietary assets.

> This repository is a vertical slice, not a claim to reproduce the production scale or visual fidelity of a large commercial title.

## Current milestone

The first slice renders a stylized forest scene through OpenGL ES 3, includes an original hero silhouette, animals, resource landmarks, a day/night ambience pulse, mobile joystick movement, attack feedback, gathering, and a basic craft action. The native core is C++17. Kotlin owns Android lifecycle, immersive full-screen landscape presentation, touch UI, haptics-ready integration points, and future local save wiring. The physics foundation includes acceleration, friction, gravity, grounded state, jump impulses, stamina-gated dodge movement, fixed-step simulation, world bounds, and axis-separated AABB collision resolution.

## Technology split

| Layer | Responsibility |
|---|---|
| Kotlin | `Activity` lifecycle, landscape/full-screen mode, HUD, touch controls, Android integration, future save and device capability APIs. |
| C++17 | OpenGL ES renderer, fixed-step physics, collision primitives, movement, jump/dodge, combat pulse, resource counters, and deterministic gameplay primitives. |
| Gradle + CMake | Reproducible Android/NDK build configuration for `arm64-v8a` and `x86_64`. |
| GitHub Actions | CI configuration for release-style APK assembly, artifact upload, and native source checks. |

## Build locally

Install Android Studio with SDK 35, NDK 27.x, CMake 3.22.1, and a JDK 17 runtime. Then run:

```bash
./gradlew assembleRelease
```

The release-style APK is produced under `app/build/outputs/apk/release/`. It is signed with the automatically generated Android debug keystore for test installation only; a protected production keystore is required for Play Store publishing. The app is configured as a full-screen landscape experience with immersive system UI flags and GLES 3 capability requirements. Install it on a landscape Android phone with:

```bash
adb install -r app/build/outputs/apk/release/app-release.apk
```

The sandbox used to create this repository does not include the Android SDK or Godot editor, so final APK compilation must run on a machine or GitHub Actions runner with Android tooling installed.

## Controls

The left virtual joystick moves the hero. Right-side drag controls camera orbit. `JUMP` applies a grounded impulse, `DODGE` applies a stamina-gated burst, `ATTACK` shows native combat feedback, `GATHER` rewards nearby resource collection, and `CRAFT` consumes wood and fiber when available. The gyro toggle is enabled only when Android reports a gyroscope; otherwise it is visibly disabled as `GYRO: UNSUPPORTED`. The prototype keeps inventory values in the native layer for the next UI-binding pass.

## Production blueprint and GDD map

The complete base-to-release implementation document is [`FULL_IMPLEMENTATION_PLAN.md`](FULL_IMPLEMENTATION_PLAN.md). The 10–20 GB production reset, content budget, milestone gates, and acceptance standard are in [`AAA_PRODUCTION_MASTER_PLAN.md`](AAA_PRODUCTION_MASTER_PLAN.md). The AAA engine and multiplayer decision is documented in [`docs/AAA_PRODUCTION_ARCHITECTURE.md`](docs/AAA_PRODUCTION_ARCHITECTURE.md), and the supplied GDD requirements are mapped in [`docs/GDD_IMPLEMENTATION_MAP.md`](docs/GDD_IMPLEMENTATION_MAP.md). These documents separate the current prototype from the future Unreal C++ production path instead of claiming that a small OpenGL prototype is already a finished AAA game.

## Roadmap

The next production slices should add a proper 3D camera, authored or legally licensed low-poly assets, animation clips, collision geometry, real inventory/crafting UI, enemy state machines, save/load, audio, device performance profiles, and a signed release pipeline. Generated or imported assets must be original or carry a compatible license.

## Repository status

This repository is now being developed in two explicit layers: a verified Android/Kotlin/C++ prototype for input and gameplay contracts, and a planned Unreal C++ production path for the real 3D open-world game. The prototype is intentionally not padded with fake data to reach 10–20 GB; content size will come from useful original assets, audio, animation, cinematics, language packs, and optional Android asset packs.
