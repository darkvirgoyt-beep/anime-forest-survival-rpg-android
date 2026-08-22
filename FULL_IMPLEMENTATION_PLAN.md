# Forest Slice — Full Implementation Plan

## 1. Product definition

**Forest Slice** is an original Android landscape action-RPG with survival, exploration, gathering, crafting, animals, and light combat. The target is a professional vertical slice that can grow into a larger commercial game. It is inspired by broad anime-fantasy and forest-survival design language, including the supplied gameplay reference, but it must not copy protected characters, names, maps, logos, dialogue, models, textures, music, or other assets.

The supplied video was used as a design reference for a behind-the-character camera, landscape touch layout, a vibrant forest environment, contextual interaction, resource gathering, crafting, combat feedback, animals, traversal variety, and progression through objectives. These observations guide the implementation; they are not a request to reproduce the original game.

## 2. Scope discipline

A game with the world scale, asset density, online infrastructure, and polish of a large commercial AAA mobile title requires a multidisciplinary production team, a real asset pipeline, device testing, backend operations, and a multi-year schedule. This repository therefore uses a staged production approach. The current milestone proves the core interaction loop and native Android foundation. It does not claim to be a finished open-world multiplayer product.

| Stage | Goal | Completion gate |
|---|---|---|
| V0.1 Foundation | Launch in landscape, render a forest scene, move, jump, dodge, gather, attack, and craft. | Debug APK builds in CI and runs on an Android API 26+ GLES 3 device. |
| V0.2 Playable loop | Add real collision terrain, camera orbit, resource nodes, enemy AI, inventory UI, and save/load. | Ten-minute loop is playable without crashes or unrecoverable state. |
| V0.3 Content slice | Add one authored forest region, original hero and animal assets, animation, sound, quests, and a camp progression loop. | External playtesters can complete a clear objective and return to camp. |
| V0.4 Technical alpha | Add profiling, asset streaming, device tiers, telemetry opt-in, account identity, and server-authoritative session experiments. | Performance and networking risks are measured on representative devices. |
| V1.0 Production | Add the chosen online model, live-ops pipeline, content cadence, store compliance, signed releases, and support tooling. | Release candidate passes QA, security, privacy, performance, and store review gates. |

## 3. Runtime architecture

The current project uses a Kotlin Android shell around a C++17 native core. Kotlin owns the Android lifecycle, immersive landscape presentation, touch UI, and platform services. C++ owns deterministic game state, physics primitives, and the OpenGL ES renderer. The JNI surface is deliberately narrow so gameplay can later be tested without depending on Android UI code.

```text
Android Activity / touch HUD (Kotlin)
        |
        | queued JNI calls on the GL thread
        v
Native game runtime (C++17)
        |
        +-- fixed-step simulation
        +-- character motor and collision solver
        +-- combat, stamina, hunger, and crafting state
        +-- animal AI and resource interactions
        +-- OpenGL ES 3 renderer
        +-- future save/network adapters
```

### Kotlin responsibilities

`MainActivity` sets landscape orientation, full-screen immersive flags, screen wake behavior, lifecycle forwarding, and the `GLSurfaceView`. `GameSurfaceView` owns the renderer and queues UI input onto the GL thread. `JoystickView` converts touch coordinates into normalized movement. The HUD exposes attack, jump, dodge, gather, and craft actions. Future Kotlin modules should provide Android save files, haptics, device capability profiles, optional sign-in, and Play Services integration without placing game rules in the Activity.

### C++ responsibilities

The native layer owns gameplay state and rendering. The `physics` module contains a small deterministic character motor, acceleration and friction, gravity, grounded checks, jump impulses, speed limits, and axis-separated AABB resolution. The renderer currently uses GLES 3 shaders and procedural geometry to keep the repository small. It can later be replaced or extended with VBOs, VAOs, texture atlases, instanced foliage, skeletal animation, and a real camera.

## 4. Physics implementation

The prototype physics model is intentionally deterministic and frame-rate safe. Input is normalized before applying acceleration. Velocity approaches a target velocity rather than changing instantly, which gives touch movement a controllable response curve. When input is released, friction approaches zero. A clamped delta time prevents a paused or backgrounded app from producing a giant simulation step.

The core update is conceptually:

```text
input = clampLength(rawTouchInput, 1)
targetVelocity = input * maxSpeed
velocity.x = approach(velocity.x, targetVelocity.x, acceleration * dt)
velocity.y = approach(velocity.y, targetVelocity.y, acceleration * dt)
velocity += gravity * dt
position += velocity * dt
resolveAabbCollisions()
clampToWorldBounds()
```

The next physics pass should use a fixed simulation tick, such as 60 Hz, with an accumulator. Rendering can interpolate between the previous and current state. This makes combat windows, stamina costs, AI decisions, and collision behavior consistent across devices with different frame rates.

The production physics backlog is:

| System | Implementation detail |
|---|---|
| Character motor | Capsule or swept-AABB controller, slope limits, step offset, grounded normals, coyote time, jump buffering, and acceleration curves. |
| Camera | Spring-arm third-person camera, obstacle raycasts, shoulder offset, orbit smoothing, look sensitivity, and indoor camera mode. |
| Traversal | Swimming volume, climbable markers, slide surfaces, fallen-log balance volumes, and stamina costs. |
| Interaction | Proximity query with priority rules: enemy, resource, chest, station, quest object, then ambient object. |
| Combat | Input buffer, hit frames, hurt frames, hit-stop, knockback, invulnerability frames, and deterministic damage events. |
| World collision | Authoring-friendly static colliders, streamed region bounds, one-way ledges, water volumes, and navmesh links. |
| Performance | Broad-phase spatial hash for entities, capped physics bodies, object pools, and no per-frame heap allocations. |

## 5. Player and survival systems

The player state should become a data-driven component rather than a collection of globals. The minimum schema is:

```text
PlayerState {
  position, rotation, velocity
  health, maxHealth
  stamina, maxStamina
  hunger, thirst, temperature
  statusEffects[]
  equippedItemIds[]
  inventory[]
  questFlags[]
  discoveredLocations[]
}
```

Health is reduced by enemy damage and severe survival penalties. Stamina is spent on sprinting, dodging, and jumping, then regenerated when the player is not performing strenuous actions. Hunger and thirst drain slowly during the day and more quickly during sprinting, hot weather, cold weather, or injury states. Temperature should be calculated from biome, time of day, weather, shelter, clothing, and nearby heat sources. Status effects should be explicit objects with a source, duration, stack count, and gameplay modifier.

A professional UX rule is that survival pressure must inform decisions rather than constantly interrupt play. The HUD should show health and stamina continuously, while hunger, thirst, temperature, and injury warnings should become prominent only when thresholds are crossed. Campfires, cooked food, shelters, clothing, and potions provide readable counterplay.

## 6. Animals and AI

Animals should use a hierarchical state machine with a small number of predictable states. A passive deer-like animal can use `Idle`, `Graze`, `Wander`, `Flee`, and `Recover`. A hostile forest creature can use `Idle`, `Patrol`, `Investigate`, `Chase`, `Attack`, `Stagger`, `Flee`, and `ReturnHome`.

Each creature needs a perception radius, hearing radius, territory center, preferred habitat, threat table, movement speed, attack range, and loot table. AI decisions should run at a lower cadence than rendering and should use a spatial query rather than scanning every entity. Navigation should use a navmesh or waypoint graph for authored regions. Flocking, migration, breeding, and legendary creatures belong after the core loop is stable; they should not be simulated for every off-screen animal.

Resource nodes should be authored data objects with a respawn policy, interaction radius, required tool tier, yield table, and depletion state. A resource interaction should produce a clear hit effect, a floating pickup, a sound cue, and an inventory event.

## 7. Crafting, inventory, and building

The first content slice should use a small recipe registry rather than thousands of hard-coded branches. Every recipe has an ID, category, input item quantities, output items, station requirement, technology requirement, craft time, and optional unlock condition.

```text
Recipe {
  id
  category
  inputs: [{ itemId, quantity }]
  outputs: [{ itemId, quantity }]
  stationId?
  technologyLevel
  unlockFlag?
  craftSeconds
}
```

V0.2 should include wood, fiber, stone, raw meat, cooked meat, a stone axe, a torch, a spear upgrade, and a basic campfire. V0.3 can add storage, cooking stations, simple modular building pieces, clothing, tools, and a camp upgrade tree. A large recipe count must come from data files and tags, not duplicated code.

The building system should snap modular pieces to a grid or socket graph, validate support and overlap, and separate preview from committed placement. Multiplayer building must be server-authoritative; local single-player building can commit immediately and serialize a change record.

## 8. World and presentation

The reference direction calls for a readable landscape frame: movement on the left, actions on the right, status and a minimap on the top edge, and contextual interaction near the player. The production UI should use large touch targets, safe-area margins, low-opacity panels over gameplay, and a consistent hierarchy for combat versus exploration actions.

The visual target is stylized anime fantasy, not photorealism. A mobile-friendly art direction should use mid-poly silhouettes, hand-authored or generated textures, a restrained outline or rim-light treatment, soft atmospheric depth, instanced foliage, baked lighting where possible, and a limited number of dynamic shadow casters. The current procedural renderer is a systems prototype. It should be replaced gradually, beginning with one original hero, two original animals, one forest kit, and a small effects library.

Asset requirements are:

| Asset group | Production rule |
|---|---|
| Characters and animals | Original or correctly licensed models, rigs, animations, and textures. |
| Environment | Modular terrain, foliage, rocks, water, ruins, camp pieces, and collision proxies. |
| VFX | GPU-friendly particles, slash trails, gathering bursts, elemental or purification effects. |
| UI | Vector icons, scalable fonts, localization-safe layouts, and accessible contrast. |
| Audio | Original or licensed ambience, footsteps, impacts, UI feedback, and music stems. |

## 9. Multiplayer path

The current prototype is offline. If multiplayer is approved, it should be introduced as a separate architecture rather than bolted directly onto the single-player globals. The recommended model is server-authoritative sessions with client prediction for movement, reconciliation, replicated gameplay events, and server validation of inventory, crafting, combat, and building.

The service boundary should eventually contain authentication, player profiles, cloud saves, session discovery, dedicated game servers, matchmaking, friends or parties, moderation, analytics, and a content delivery pipeline. Database writes should be idempotent. Sensitive inventory and progression decisions must never rely solely on client-provided values. Anti-cheat should begin with server validation and anomaly detection; commercial anti-cheat integrations can be evaluated after the gameplay model is stable.

A practical rollout is offline first, invite-only co-op second, and persistent shared-world features last. This limits infrastructure cost and avoids designing a massive backend before the game loop is fun.

## 10. Build, test, and release

The repository uses GitHub Actions to install JDK 17, Android SDK 35, NDK 27, CMake 3.22.1, and Gradle 8.10.2 before assembling a debug APK. Release builds should add reproducible versioning, a keystore stored only in GitHub encrypted secrets, Play App Signing, ProGuard/R8 review, symbol upload, crash reporting, and a staged rollout.

The test pyramid should include native unit tests for inventory math, recipe validation, physics collision resolution, stamina costs, damage calculation, and deterministic AI transitions. Integration tests should verify JNI loading, pause/resume context recovery, saved-state migration, and touch input. Device tests should cover low, mid, and high tiers, different aspect ratios, thermal throttling, background/foreground transitions, and a minimum supported Android version.

Every release candidate must pass these gates:

1. The app launches into a full-screen landscape scene without a black screen or clipped controls.
2. Movement, jump, dodge, attack, gather, craft, pause, resume, and rotation/configuration changes do not crash.
3. The physics loop remains stable after a long frame and never produces NaN positions.
4. The debug APK installs on supported devices and the release APK is signed through the protected CI path.
5. No copyrighted, ripped, or unlicensed assets are present.
6. Store metadata, privacy policy, permissions, content rating, and data-safety declarations are complete.

## 11. Current implementation status

The repository currently contains the Android Kotlin shell, immersive landscape flags, a C++17 OpenGL ES 3 renderer, touch joystick, HUD actions, deterministic movement physics, AABB obstacles, jump and dodge stamina costs, gathering, crafting counters, project documentation, and a successful GitHub Actions debug build. The current renderer uses procedural geometry and is intentionally not the final asset or animation pipeline.

The next engineering priorities are to split the native file into modules, add a real fixed-step accumulator, expose state snapshots to the HUD, add a proper camera and world collision representation, introduce an inventory/recipe registry, implement animal state machines, add local save/load, and then replace the procedural silhouettes with original mobile-optimized assets.

## 12. Definition of done for the production foundation

The foundation is ready for a larger team when a new contributor can clone the repository, build the debug APK in CI, understand the Kotlin/C++ boundary, add a recipe or creature without editing unrelated systems, run deterministic tests, profile a device build, and verify that the game returns safely from pause/resume. At that point, content production and multiplayer experiments can proceed without repeatedly rewriting the core runtime.

## References

[1]: https://youtu.be/iaUgulBr5SY?si=YFkjEMNyMq1qqFWX "User-provided gameplay reference video"
[2]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android "Forest Slice Android repository"
