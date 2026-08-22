# Unreal production branch

This directory is the long-term 3D production foundation for Forest Slice. It is intentionally separate from the small `app/` Android/OpenGL ES prototype so the prototype remains buildable while the real game moves to Unreal Engine 5.6+.

## Required local toolchain

Install Unreal Engine 5.6 or later, Android Studio with SDK 35, NDK and JDK versions required by that Unreal release, and a physical Android device for profiling. Generate project files from `ForestSlice.uproject`, open the project in Unreal Editor, create the input assets under `/Game/Input`, and assign the C++ character class to the playable game mode.

## Current C++ foundation

`Source/ForestSlice/Public/ForestSliceCharacter.h` and its implementation define the production-shaped third-person character boundary. It includes a capsule movement motor, spring-arm camera, camera-relative movement, sprint stamina, slide, dodge, jump, camera look, and a gyro input API. Damage, invulnerability, combo abilities, target hurtboxes, and server authority remain separate systems and must not be implemented as client-only UI behavior.

## Asset rules

The `Content` folders are reserved for original or licensed assets. A production character requires a rigged skeletal mesh, locomotion and combat animation clips, materials, VFX, and platform-specific LODs. Do not place ripped assets from existing games in this repository.

## Packaging

Use the Android platform settings in `Config/DefaultEngine.ini` as a starting point. The real shipping output should be an Android App Bundle with asset packs, not a single oversized APK. Test-distribution builds may use a local debug key; production builds require a protected signing key and store configuration.

## Honest status

This directory is a compile-oriented source foundation and configuration seed. It cannot be packaged in this sandbox because the Unreal Engine editor/toolchain and production assets are not installed here. The existing GitHub Actions job continues to build the Kotlin/C++ prototype under `app/`; a later runner with Unreal installed must be added for the production branch.
