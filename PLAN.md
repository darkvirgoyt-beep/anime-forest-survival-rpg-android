# Forest Slice Plan

## Vertical slice goal

Ship one Android landscape loop that proves movement, combat feedback, gathering context, crafting action, and an anime-inspired forest mood on a mid-range phone.

## Risk slices

1. Native build and JNI loading: verify the shared library loads and all Kotlin/C++ signatures match.
2. GLES 3 rendering: verify shaders compile, the forest scene draws, and the renderer recovers after pause/resume.
3. Mobile input: verify the joystick is responsive without racing the GL thread.
4. Gameplay state: keep movement and the first combat/crafting counters deterministic and easy to unit-test.
5. Content scale: stay with procedural shapes until the asset pipeline and licensing rules are agreed.

## Acceptance criteria

- The app launches in landscape on Android API 26+ devices supporting OpenGL ES 3.
- The screen visibly contains a forest scene, hero silhouette, animals, resources, and day/night ambience.
- The virtual joystick changes the hero position.
- Attack and craft actions reach native code without crashing.
- GitHub Actions can assemble a debug APK with Android SDK, NDK, CMake, JDK, and Gradle.
- No copyrighted or ripped game assets are included.
