# AAA Production Architecture Decision

## Decision

The requested target—high-end 3D anime characters, animation, open-world streaming, boss encounters, co-op multiplayer, large asset packs, mobile scalability, gyro aiming, and store distribution—should use **Unreal Engine 5.6+ with C++ gameplay modules** for the production game. The current Kotlin/C++ OpenGL ES application remains a lightweight Android prototype and test harness; it is not the correct long-term renderer for AAA 3D content.

Epic’s mobile documentation describes Unreal as a framework for optimized Android applications and provides dedicated guidance for Vulkan, OpenGL ES 3.2 fallback, frame pacing, device profiles, Android Asset Delivery, packaging, profiling, and release signing.[1] Android’s Unreal guidance also describes Vulkan as the preferred high-performance path with OpenGL ES 3.2 fallback and points to Play Asset Delivery for large asset packs.[2]

## Production repository shape

```text
ForestSlice/
├── Unreal/
│   ├── ForestSlice.uproject
│   ├── Config/                         # device profiles, input, scalability
│   ├── Content/                        # original assets, maps, materials, UI
│   └── Source/ForestSlice/              # C++ gameplay modules
├── Server/
│   ├── DedicatedServer.Target.cs
│   └── Source/                         # authoritative simulation and services
├── Tools/
│   ├── AssetValidation/
│   ├── BuildAutomation/
│   └── ContentCooking/
├── MobilePrototype/                    # current Kotlin/OpenGL ES validation app
├── Docs/
│   ├── Design/
│   ├── Technical/
│   └── Operations/
└── .github/workflows/                  # lint, tests, cook, package, release
```

## Runtime layers

| Layer | Production implementation | Authority |
|---|---|---|
| Presentation | Unreal mobile renderer, materials, Niagara effects, animation blueprints, UMG/Common UI. | Client visual state. |
| Character | `AForestCharacter`, movement component, camera spring arm, locomotion and ability components. | Server validates gameplay outcomes. |
| Combat | Gameplay Ability System-style abilities, hit traces, damage cues, stagger/poise, invulnerability frames, combo windows. | Server-authoritative damage and inventory. |
| World | World Partition, level streaming, data layers, hierarchical LOD, foliage instancing, navmesh regions, weather volumes. | Server owns persistent world changes. |
| Survival | Data assets for hunger, thirst, temperature, injury, shelter, cooking, farming, fishing, and status effects. | Server owns progression in co-op. |
| Animals | Behavior Trees or StateTree, perception, smart objects, navmesh, spawn budgets, off-screen simulation. | Server owns AI outcomes. |
| Persistence | Versioned save schema, cloud profile service, migration handlers, conflict policy. | Backend validates writes. |
| Multiplayer | Dedicated authoritative servers, replication graph, session/matchmaking service, party and presence service. | Server and backend. |
| Android shell | Input, gyro sensor, frame pacing, permissions, account handoff, asset delivery, crash reporting. | Platform layer. |

## Character controller

The production controller should be a capsule-based character motor rather than a free-floating transform. Input is sampled in Kotlin or Unreal Enhanced Input, converted to camera-relative intent, and passed to the movement component. The motor handles acceleration, braking, slope limits, step height, floor detection, coyote time, jump buffering, sprint stamina, swimming volumes, sliding surfaces, climb links, and root-motion handoff for attacks.

The camera uses a spring arm with collision tests and a shoulder offset. Right-side touch drag changes yaw and pitch. The camera never owns gameplay direction; it publishes a view basis and the movement system consumes it. This separation prevents aim and navigation from becoming coupled to a particular device layout.

## Advanced combat

Combat is data-driven. Each ability has startup, active, recovery, cancel, resource, range, damage, poise, knockback, and animation-event data. The server validates the attack request, resolves the hit trace or hitbox against authoritative hurtboxes, applies damage once per target per swing, and replicates a compact result event. The client predicts input feel and effects but cannot grant damage, loot, recipes, or progression.

The first combat package should contain one original hero, one target dummy, one hostile creature, a three-hit light combo, heavy attack, dodge with invulnerability frames, jump attack, hit reaction, stagger, death, respawn, and a boss telegraph prototype. Hit-stop, camera shake, slash trails, impact particles, sound, and floating damage numbers consume combat events instead of modifying health themselves.

## Open world and assets

The world should begin as one authored forest region with a camp, river, cave entrance, ruins, resource nodes, passive animals, hostile creatures, and one boss arena. World Partition and streaming should be validated before adding multiple biomes. Forest density comes from instanced foliage and hierarchical LOD rather than thousands of unique actors.

All anime characters, animals, animation clips, textures, audio, UI art, and VFX must be original or properly licensed. The supplied references define broad genre intent only. No copied commercial characters, logos, maps, dialogue, ripped models, or unlicensed game files are acceptable.

## Co-op architecture

The first multiplayer milestone should be a four-player invite-only session with a dedicated server. Replicate player movement, combat events, enemy state, loot claims, building changes, and quest progress. Keep cosmetic effects client-side where they do not affect simulation. Persist only stable player/world records; do not write every frame. Add rate limits, validation, reconnect handling, version handshakes, and replayable server logs before opening public matchmaking.

## Gyro and settings requirements

On Android, detect `Sensor.TYPE_GYROSCOPE` at runtime. If absent, the gyro aiming toggle must be visibly disabled and labeled unsupported, never silently enabled. Store gyro sensitivity, invert-Y, aim acceleration, graphics preset, frame-rate cap, resolution scale, shadows, foliage density, motion blur, vibration, audio, and control-layout preferences in a versioned local settings schema. The game should expose Balanced, Performance, and Quality presets; device profiles can override unsupported or unsafe combinations.

Login should begin with guest/local profile support for offline development, then add a real identity provider through a backend-owned authentication boundary. Never place provider secrets in the APK. Account tokens, cloud saves, entitlements, party membership, and anti-cheat decisions belong behind server APIs.

## Build and release

Development requires a machine with Unreal Engine 5.6+, Android SDK/NDK/JDK, and a physical device matrix. GitHub should run source checks, host unit tests, Unreal automation tests, cooking/package validation, Android debug packaging, and release packaging. A production Android App Bundle should use a protected signing key and Play Asset Delivery for large optional content. The current repository’s CI can prove the native prototype and test modules, but it cannot compile an Unreal project without the Unreal toolchain and production assets.

## Honest milestone boundary

This document is the production architecture for the requested game. The current public repository contains a functioning Android prototype with C++ physics/controller/combat foundations, not the complete AAA game. The correct next production action is to migrate the gameplay contracts into an Unreal C++ project, create the first authored forest map and original hero asset, and validate a four-player dedicated-server slice before expanding to the full world.

## References

[1]: https://dev.epicgames.com/documentation/en-us/unreal-engine/getting-started-with-mobile-development-in-unreal-engine "Epic Games — Mobile Development in Unreal Engine"
[2]: https://developer.android.com/games/engines/unreal/unreal-on-android "Android Developers — Unreal on Android"
