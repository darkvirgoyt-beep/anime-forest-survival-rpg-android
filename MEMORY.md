# Memory

## Current state

- GitHub repository: `darkvirgoyt-beep/anime-forest-survival-rpg-android`
- Stack: Android Kotlin shell + C++17 OpenGL ES 3 renderer through JNI.
- Kie.ai Claude Fable 5 was used to recommend the stack and review the JNI/GLES integration.
- The review highlighted three important rules that are applied: `setEGLContextClientVersion(3)` runs before `setRenderer`, native initialization is in `onSurfaceCreated`, and UI input is queued onto the GL thread.
- The prototype renders procedural shapes only. No external or copyrighted game assets are included.

## Verification notes

- `git diff --check` passes.
- JNI export names match the Kotlin package/class/method names.
- The sandbox does not have Android SDK, Gradle, NDK, or Godot installed, so APK compilation must run in GitHub Actions or Android Studio.
- The GitHub workflow installs SDK 35, NDK 27.0.12077973, CMake 3.22.1, JDK 17, and Gradle 8.10.2 before assembling the debug APK.

## 2026-08-24 visual enhancement pass

The active Android visual path is the native GLES 3 renderer in `app/src/main/cpp/forest_game.cpp`, with Kotlin Canvas/TextView overlays in `MainActivity.kt` and `HudOverlayViews.kt`. The patch remains dependency-free and keeps the JNI bridge and HUD snapshot contract unchanged.

Implemented: a `uEmissive` shader uniform with pseudo-face lighting, stronger top/side separation, animated rim response, and distance fog; reusable 3D VFX rings and glow accents for attack, dodge, enemy hit, teleport tower, campfire, and ambient motes; layered wisteria vines and foreground leaf framing; and a darker teal/cyan HUD button palette with a cyan vital-meter border.

Validation: all compiled native tests passed when built with C++17 and the controller implementation included. Graphics-tier, production-foundation, progressive-content, and startup-warmup contracts passed. The online-only contract still reports a pre-existing `BuildConfig.PROTOTYPE_MODE` occurrence in `MainActivity.kt`; the baseline `HEAD` contains the same occurrences and the visual patch did not introduce them. The sandbox has no Android SDK/NDK, Gradle wrapper, GLES headers, or Android device, so APK build and renderer syntax compilation could not be performed here.
