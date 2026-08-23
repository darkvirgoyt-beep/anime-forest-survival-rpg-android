# Aethelgard: Wild Horizons – Crafting

**Aethelgard: Wild Horizons – Crafting** is an original anime-inspired Android game prototype built around a small third-person forest-survival loop. It is inspired by broad genre themes—exploration, gathering, crafting, animals, and light RPG combat—but does not copy any existing game's characters, names, story, models, textures, or proprietary assets.

> This repository is a vertical slice, not a claim to reproduce the production scale or visual fidelity of a large commercial title.

## Current milestone

The current slice renders a stylized forest scene through OpenGL ES 3, includes an original hero silhouette, animals, resource landmarks, a day/night ambience pulse, mobile joystick movement, attack feedback, gathering, and a basic craft action. The native core is C++17. Kotlin owns Android lifecycle, immersive full-screen landscape presentation, touch UI, haptics-ready integration points, and future online session wiring. The physics foundation includes acceleration, friction, gravity, grounded state, jump impulses, stamina-gated dodge movement, fixed-step simulation, world bounds, and axis-separated AABB collision resolution.

The intended player-facing product flow is now **online-only**: Google Play authentication, backend session exchange, server-region health/ping, cloud-save preflight, and co-op session validation must succeed before entering a production world. A developer-only guest path may exist in debug builds for automation, but it is hidden from release builds and is not a production account mode.

The Aethelargd RPG layer now adds a deterministic progression loop: the **The First Ember** quest tracks three resource caches, an ember-kit craft, and a forest-warden defeat; gathering, combat hits, crafting, and the quest reward grant XP; level thresholds are surfaced in the live HUD; and the warden has a readable health bar, hit flash, defeat feedback, and quest completion state. The progression rules live in `app/src/main/cpp/rpg/progression.*` and are covered by a native regression test.

## Brand and audio assets

The launcher is branded as **Aethelgard: Wild Horizons – Crafting** and uses `assets/ui/aethelgard_game_icon.png`. The generated audio bank is cataloged in [`assets/audio/AUDIO_MANIFEST.md`](assets/audio/AUDIO_MANIFEST.md). The Android harness loads the forest exploration track and gameplay/UI Foley through `GameAudio`, with persistent Master, Music, Effects, Ambience, and Mute controls. The Unreal path uses `UForestSliceAudioSubsystem` as the settings boundary.

The current image-service quota and external audio-service availability prevented final AI character sheets and a full AAA sound library in this run. The icon and procedural SFX are original fallback assets and are intentionally marked for later replacement by production-quality original or licensed content.

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

The left virtual joystick moves the hero. Right-side drag controls camera orbit. `JUMP` applies a grounded impulse, `DODGE` applies a stamina-gated burst, `ATTACK` shows native combat feedback, `GATHER` rewards nearby resource collection and XP, and `CRAFT` consumes wood and fiber when available. Gather three nearby caches to unlock the ember-kit objective, craft once to reveal the forest warden objective, then defeat the warden with the attack combo. The live HUD shows HP, stamina, hunger, level, XP, inventory, warden health, and quest state. The gyro toggle is enabled only when Android reports a gyroscope; otherwise it is visibly disabled as `GYRO: UNSUPPORTED`.

## Production blueprint and GDD map

The complete base-to-release implementation document is [`FULL_IMPLEMENTATION_PLAN.md`](FULL_IMPLEMENTATION_PLAN.md). The 10–20 GB production reset, content budget, milestone gates, and acceptance standard are in [`AAA_PRODUCTION_MASTER_PLAN.md`](AAA_PRODUCTION_MASTER_PLAN.md). The AAA engine and multiplayer decision is documented in [`docs/AAA_PRODUCTION_ARCHITECTURE.md`](docs/AAA_PRODUCTION_ARCHITECTURE.md), and the supplied GDD requirements are mapped in [`docs/GDD_IMPLEMENTATION_MAP.md`](docs/GDD_IMPLEMENTATION_MAP.md). These documents separate the current prototype from the future Unreal C++ production path instead of claiming that a small OpenGL prototype is already a finished AAA game.

## Roadmap

The next production slices should add a proper 3D camera, original rigged anime characters, authored or legally licensed environment assets, animation clips, collision geometry, real inventory/crafting UI, enemy state machines, authenticated Google Play session exchange, live server health/ping, versioned cloud saves, dedicated co-op sessions, dynamic music states, voice/animal libraries, device performance profiles, and a signed release pipeline. Generated or imported assets must be original or carry a compatible license.

## Repository status

This repository is now being developed in two explicit layers: a verified Android/Kotlin/C++ prototype for input and gameplay contracts, and a planned Unreal C++ production path for the real 3D open-world game. The prototype is intentionally not padded with fake data to reach 10–20 GB; content size will come from useful original assets, audio, animation, cinematics, language packs, and optional Android asset packs.
