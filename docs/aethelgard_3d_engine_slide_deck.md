# AETHELGRAD 3D Engine Architecture

## Cover
AETHELGRAD
3D Engine Architecture, Camera Modes, and Authoritative Co-op
Manus AI | Android RPG Technical Overview

## Slide 1
### A small engine with a clear separation of responsibilities
- **Native gameplay core:** Fixed-step movement, combat, weather, day-night, camera math, and GLES3 rendering.
- **Android shell:** Touch controls, HUD, account session, profile identity, and co-op room UX.
- **Online service:** Authenticated rooms, synchronized world clock, tower rendezvous, and validated room actions.
- **Design principle:** Keep the player-facing loop responsive while preserving clean seams for production services.

## Slide 2
### The frame pipeline keeps simulation stable
- Android forwards a bounded frame delta through `NativeGameBridge`.
- The native loop accumulates time and advances physics at a fixed 60 Hz step.
- Calendar, weather, combat, locomotion, and tower cooldowns update inside the simulation loop.
- GLES3 renders the current state; Android overlays the readable HUD and action controls.

## Slide 3
### Procedural 3D composition prioritizes spatial clarity
- Environmental bands create immediate navigation: forest, sand, and snow regions.
- Procedural terrain, trees, paths, structures, and avatars provide original readable silhouettes.
- The amber teleportation tower is both a travel mechanic and a world-scale landmark.
- Authored meshes and materials can replace primitives without changing gameplay contracts.

## Slide 4
### One camera contract supports two play styles
- **Third person:** Follow camera behind and above the visible player avatar for traversal and spatial awareness.
- **First person:** Eye-level camera with a foreground weapon for close exploration and atmospheric immersion.
- **Shared state:** Yaw, pitch, target, projection, and movement-facing direction.
- **Mobile input:** Left-side locomotion; right-side swipe orbit with full horizontal 360° rotation.

## Slide 5
### Weather and time turn the map into a living system
- A deterministic 15-minute world clock drives day, afternoon, evening, and night.
- An independent weather cycle moves through clear, rain, and thunderstorm states.
- Ambient light and sky color shift with the day phase; storms add procedural rain and lightning.
- Deterministic effects make the same room state reproducible across Android clients.

## Slide 6
### The co-op tower room synchronizes the experience
- A player creates a six-character room code; friends join through authenticated requests.
- Two-second heartbeats publish bounded position, presence, and tower-arrival revision.
- The room service advances one shared clock and returns active peer state.
- Clients render remote friends as original low-poly companions and apply shared world time.

## Slide 7
### Tower teleport is a readable shared event
- Local tower activation moves the player to the tower arrival point and increments a room revision.
- The server records the highest accepted revision and exposes it to current members.
- Friends observe a newer remote revision and perform a synchronized arrival teleport.
- The amber glow and HUD revision make the rendezvous visible during testing.

## Slide 8
### Server authority protects combat and inventory
- Combat is accepted only for known targets, joined members, valid range, and cooldown-safe actions.
- Server-side damage updates shared boss health and increments a combat revision inside a transaction.
- Gathering validates resource identity and player proximity; crafting validates server-held material costs.
- Request receipts make retries idempotent, preventing duplicate damage or rewards.

## Slide 9
### The Android HUD makes system state testable
- Status exposes biome, phase, weather, camera, map, tower, health, stamina, hunger, and resources.
- Co-op status adds room code, participant count, tower revision, boss health, and weather synchronization.
- Gold AETHELGRAD profile identity appears at install, during setup, and in the game HUD.
- The map overlay remains an orientation instrument, not a replacement for world exploration.

## Slide 10
### Today’s prototype is a seam for production scale
- **Verified now:** Android build, native tests, online-service tests, co-op simulation, APK/AAB packaging, and artifact uploads.
- **Current authority:** Shared room state plus transactional combat and inventory actions.
- **Next production layer:** Dedicated server for movement validation, hit detection, inventory authority, reconnects, and low-latency replication.
- **Rendering path:** Authored meshes, shadows, level of detail, GPU-instanced effects, and richer materials can arrive without changing camera or room contracts.

## Slide 11
### AETHELGRAD answers the player’s key questions
Where am I? Where is the tower? What time is it? What is the weather doing? Which camera feels right? Where are my friends?

A fixed-step native core, explicit camera modes, deterministic atmosphere, authenticated room state, and server-validated actions form the foundation for the next AETHELGRAD milestone.

## References
[1]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/app/src/main/cpp/forest_game.cpp "Native renderer, gameplay, camera, weather, and co-op bridge"
[2]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/app/src/main/java/com/darkvirgoyt/aethelgrand/MainActivity.kt "Android HUD, touch input, and client synchronization"
[3]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/src/server.mjs "Authenticated co-op and authoritative action routes"
[4]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/sql/004_authoritative_gameplay.sql "Authoritative gameplay persistence migration"
[5]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/test/coop_room_simulation.test.mjs "Local co-op room and tower-flow simulation"
