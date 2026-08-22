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
