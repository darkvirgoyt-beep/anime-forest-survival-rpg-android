# Unreal production branch

This directory is the long-term 3D production path for Aethelgard, the real AAA production game. It is intentionally paired with the `app/` Android/OpenGL ES game layer: the Android layer keeps mobile controls, delivery, and gameplay contracts testable while Unreal Engine 5.6+ provides the high-end 3D presentation and dedicated-server path.

## Required local toolchain

Install Unreal Engine 5.6 or later, Android Studio with SDK 35, NDK and JDK versions required by that Unreal release, and a physical Android device for profiling. Generate project files from `ForestSlice.uproject`, open the project in Unreal Editor, create the input assets under `/Game/Input`, and assign the C++ character class to the playable game mode.

## Current C++ foundation

`Source/ForestSlice/Public/ForestSliceCharacter.h` and its implementation define the production-shaped third-person character boundary. It includes a capsule movement motor, spring-arm camera, camera-relative movement, sprint stamina, slide, dodge, jump, camera look, and a gyro input API. Damage, invulnerability, combo abilities, target hurtboxes, and server authority remain separate systems and must not be implemented as client-only UI behavior.

## Current production modules

The Unreal source now includes data-driven combat and weapon boundaries in `ForestSliceCombatComponent` and `ForestSliceWeaponComponent`, with light/heavy attack phases, combo buffering, hit-window events, replicated equipment slots, and server request methods. `ForestSliceProceduralForest` provides deterministic seed-and-chunk generation with hierarchical instanced tree/rock placement and bounded active records. `ForestSliceSurvivalComponent` owns replicated health, hunger, thirst, stamina, temperature, shelter, injury, and sleep restoration. `ForestSliceWorldClock` provides authoritative day/night progression, and `ForestSliceBed` validates nighttime sleep at a safe bed. `ForestSliceMobileHUD` routes UMG joystick, look, sprint/slide, attack, weapon switch, and gyro commands to the same character path. `ForestSliceAccountSubsystem` defines guest mode and the Google Play credential-to-backend session boundary.

These are active AAA production foundations. Final authored art, complete hit traces, engine-generated UMG assets, Play Games credentials, and a deployed live backend remain specific production deliverables rather than assumptions.

## Asset rules

The `Content` folders are reserved for original or licensed assets. A production character requires a rigged skeletal mesh, locomotion and combat animation clips, materials, VFX, and platform-specific LODs. Do not place ripped assets from existing games in this repository.

## Dedicated server targets

`Source/ForestSliceServer.Target.cs` defines the headless authoritative server target and `Source/ForestSliceClient.Target.cs` defines a separate client target. Build and cook both from a source Unreal Engine 5.6+ installation. The server must own combat outcomes, inventory, crafting, creatures, survival, world mutations, and persistence writes; clients send validated intent and receive replicated state.

The login boundary is deliberately staged: Android Play Games authentication produces a single-use server auth code, the online services backend verifies it and issues a game session, and the Unreal client presents that session to the dedicated-server admission service. `ForestSliceAccountSubsystem` does not mark a player authenticated from a raw client string or provider player ID.

## Packaging

Use the Android platform settings in `Config/DefaultEngine.ini` as a starting point. The real shipping output should be an Android App Bundle with asset packs, not a single oversized APK. Test-distribution builds may use a local debug key; production builds require a protected signing key and store configuration.

## AAA upgrade contracts

The production path now includes data-oriented contracts for the requested upgrade in `docs/AAA_WORLD_AND_GAMEPLAY_UPGRADE.md` and the Unreal module. `AForestSliceProceduralForest` exposes a 100 km world envelope, seven deterministic biome profiles, river segments, and nearest-river queries. `UForestSliceMobPresentationComponent` exposes targetable mob health, elite/boss styling, and enemy-base marker data. `UForestSliceMobileHUD` exposes focused-mob bindings for a world-space or target-frame health bar.

`UForestSliceBuildingComponent` provides server-authoritative recipes and placement state for campfires, foundations, walls, roofs, storage, beds, workbenches, farms, kilns, forges, fences, gates, lamps, and waystones. `UForestSliceToolLoadoutComponent` provides replicated starter tools, tiers, harvesting power, equipment switching, durability, and repair hooks. These are Blueprint-ready contracts; authored Blender/Unreal meshes, materials, Niagara effects, nav data, and final UI widgets still need to be connected in content assets for the full AAA presentation.

## Honest status

This directory is a compile-oriented Unreal production path and configuration seed. It cannot be packaged in this sandbox because the Unreal Engine editor/toolchain and production assets are not installed here. The existing GitHub Actions job builds the Kotlin/native-C++ Android game layer under `app/`; an Unreal-capable runner is still required for cooked AAA client and dedicated-server artifacts. The backend service under `server/` is a real credential-verifying foundation, but it is not a public live service until it is deployed with production secrets, TLS, PostgreSQL, and a dedicated-server allocator.
