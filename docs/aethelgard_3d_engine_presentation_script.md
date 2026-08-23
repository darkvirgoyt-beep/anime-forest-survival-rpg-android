# AETHELGRAD 3D Engine and Camera Architecture

## Presentation script

**Suggested length:** 8–10 minutes  
**Audience:** Game developers, technical artists, producers, and testers  
**Project:** AETHELGRAD — Wild Horizons

---

## Opening — “A living world in a small Android build”

**Speaker:**

“Welcome to AETHELGRAD. This build turns a lightweight Android prototype into a readable, atmospheric 3D survival world without copying external reference artwork or depending on a heavyweight asset pipeline for every interaction.

The engine is split into two layers. The native layer owns movement, camera math, world rendering, combat, weather, day-night progression, and deterministic gameplay state. The Android layer owns the touch surface, HUD, account flow, profile presentation, graphics settings, and authenticated co-op room controls. That separation keeps the game responsive on a phone while keeping gameplay rules close to the renderer.”

**Transition:**

“First, let us follow one frame through the game.”

---

## Section 1 — “The frame pipeline”

**Speaker:**

“Every rendered frame begins in `GameRenderer`. Android calculates a bounded frame delta and forwards it through the native bridge. The native loop accumulates time, advances fixed physics steps at 60 updates per second, refreshes the calendar, updates combat and locomotion, and then renders the current world.

The fixed-step loop is important because movement, weather motion, tower cooldowns, and combat timing should not change simply because one phone renders at a different frame rate. Rendering can run at the configured display rate, while gameplay simulation advances in stable increments.

The renderer has a 2D program for lightweight overlay geometry and a GLES3 program for 3D positions and model-view-projection matrices. Procedural boxes are the first geometry vocabulary: terrain slabs, trees, structures, the teleportation tower, peer avatars, and the first-person weapon.”

**On-screen cue:**

“Show the path from `GameRenderer` to `NativeGameBridge.render`, then to the fixed-step update and `draw3DWorld`.”

---

## Section 2 — “Procedural 3D world composition”

**Speaker:**

“AETHELGRAD’s world is composed from original procedural forms rather than copied screenshots or reference assets. The terrain is divided into readable environmental bands: a forest zone, a warm sand zone, and a cool snow zone. Large color fields establish navigation, while trees, paths, structures, and the tower provide landmarks.

The first target is spatial clarity rather than photorealism. The player should immediately understand where they are, where the tower is, and which direction leads toward another biome. Strong silhouettes and controlled colors make that possible. Authored meshes and materials can later replace the procedural forms without changing the gameplay contracts.

The teleportation tower is an anchor landmark. It has a dark vertical body, amber structural supports, a glowing crown, and a circular arrival platform. Its job is both functional and navigational: it is a fast-travel destination in gameplay and a shared meeting point in co-op.”

**On-screen cue:**

“Point to the tower, the three terrain regions, the player, and the map markers.”

---

## Section 3 — “The camera model”

**Speaker:**

“The camera is driven by yaw and pitch. A swipe changes the orbit angles, and pitch is clamped to a usable range so the player can look over the complete scene without flipping the view. Horizontal yaw wraps continuously, giving a full 360-degree orbit.

The view matrix uses a look-at transform. The projection is perspective-based, so distant terrain and the tower read as a 3D space rather than a flat board. The same camera state informs movement direction, which means the joystick moves relative to the direction the player is looking.

There are two explicit camera modes.”

### Third-person mode

“In third-person mode, the camera follows behind and above the player. The player avatar is visible as a compact low-poly character with a body, head, cloak colors, and sword. This mode is best for traversal, spatial awareness, and watching the character approach the tower or other landmarks.”

### First-person mode

“In first-person mode, the camera moves to the player’s eye position. The avatar body is hidden to avoid placing the full character inside the camera. Instead, a foreground weapon element gives the player a clear sense of embodiment. This mode is best for close exploration and for testing how weather and lighting feel from the player’s viewpoint.”

**On-screen cue:**

“Press `VIEW: THIRD PERSON`, orbit around the character, then press `VIEW: FIRST PERSON` and repeat the same swipe.”

---

## Section 4 — “360-degree touch and gyroscope control”

**Speaker:**

“The right side of the game surface is the look region. A finger drag produces yaw and pitch input, so the player can slide around the scene in a full 360-degree view. The vertical range is broad enough to look toward the tower crown, the terrain, and the sky effects.

A gyroscope is also supported as an optional input. Touch remains the primary control because it is predictable across devices. Gyro input is additive and can be enabled from the HUD when the device exposes a gyroscope.

This design separates movement input from look input. The left joystick controls locomotion. The right side controls camera orbit. The action buttons remain Android UI controls above the render surface, which keeps the prototype easy to test on a phone.”

**On-screen cue:**

“Show the `SWIPE RIGHT TO ORBIT 360°` hint, orbit through the scene, and toggle the optional gyro control.”

---

## Section 5 — “Dynamic day-night and weather”

**Speaker:**

“AETHELGRAD’s world clock is deterministic. A complete day lasts fifteen minutes in the current tuning. The day is divided into day, afternoon, evening, and night. The calendar increments when fixed simulation time crosses the cycle boundary.

Weather runs on its own repeating cycle. Clear weather lasts for the first portion, rain follows, and a shorter thunderstorm phase completes the loop. Because weather and time are independent, a storm can cross from afternoon into evening instead of being forced to reset at a phase boundary.

In the 3D renderer, the clock changes ambient brightness and sky color. Day is the brightest state. Evening lowers the ambient contribution and shifts the scene toward warmer colors. Night reduces the light substantially and emphasizes the dark sky. Thunderstorms further reduce ambient light and add rain particles and lightning flashes.

The weather particles are procedural. The renderer creates lightweight streaks at deterministic positions derived from the world clock, so the effect has motion without requiring a large texture library. This suits the current Android target and leaves room for later GPU instancing or authored particle systems.”

**On-screen cue:**

“Show the HUD changing from `DAY` to `EVENING` or `NIGHT`, then show rain and thunderstorm particles.”

---

## Section 6 — “Synchronized world time in co-op”

**Speaker:**

“Co-op rooms use one shared room clock. A player creates a six-character tower room code, and friends join with that code. The service advances the room clock and returns it to every active member. Each Android client applies the returned time to the native renderer.

This means two friends entering the same room see the same weather phase and the same day-night phase. Weather does not need to be transmitted as a large state object because it is derived deterministically from synchronized world time.

The client also sends a small heartbeat containing its position, whether it is at the tower, and its latest tower-arrival revision. The server returns the active participant list and the current room revision. Remote participants are drawn as original low-poly peer avatars in the native scene.”

---

## Section 7 — “Friends teleporting to the same tower”

**Speaker:**

“When a player presses `TOWER / TELEPORT`, the local native world moves the player to the tower arrival point, increments the tower revision, and produces a short amber glow. On the next co-op heartbeat, that revision is published to the room.

Other clients compare the received revision with their last observed revision. If a remote player is marked as having arrived at the tower with a newer revision, the client performs the synchronized arrival action and moves its own player to the same tower point. The result is a simple, readable rendezvous mechanic: one friend activates the tower, and the group converges on that landmark.

This is intentionally a rendezvous layer rather than a full authoritative combat server. It synchronizes the shared clock, player presence, and tower event. It does not yet claim to validate every hit, inventory change, or movement frame on a dedicated server.”

### Co-op architecture choices

| Approach | Tradeoffs | Cost | Setup complexity |
| --- | --- | --- | --- |
| HTTPS room and presence rendezvous, implemented in this build | Fast to integrate with the existing authenticated service; suitable for synchronized weather, day-night, peer markers, and tower arrival; not authoritative for real-time combat | Uses the existing backend and database; normal hosting and database costs | Low to medium |
| Dedicated authoritative game server | Best for real-time combat, movement validation, inventory authority, lag compensation, reconnects, and larger rooms; requires allocator, health checks, replication, and operations tooling | Higher hosting, monitoring, and engineering cost | High |

**Speaker:**

“For the current Android milestone, the lightweight room contract gives us a usable shared tower experience while preserving a clean seam for a future dedicated simulation server.”

---

## Section 8 — “Map and HUD as readable instruments”

**Speaker:**

“The world map is rendered as an original overlay rather than a copied reference board. It contains simple region bands, a player marker, a tower marker, and destination markers. The map is not trying to replace the world view. It is a quick orientation instrument.

The HUD exposes the current biome, phase, day, weather, camera mode, map state, tower state, health, stamina, hunger, water state, locomotion state, level, experience, and resources. In co-op mode, the status line adds the room code, participant count, tower revision, and a weather-synchronized indicator.

This is useful during testing because system state is visible without requiring a debug console. It also gives players clear feedback when the tower room is active or when a shared weather clock is being applied.”

---

## Section 9 — “Android identity and installation experience”

**Speaker:**

“AETHELGRAD presents one consistent identity across the install surface and the game. The Android launcher label is `AETHELGRAD`, and an original gold profile emblem is used for the installed icon, the character setup screen, and the in-game HUD badge.

The emblem is not a copied game logo. It uses an amber crest over a dark teal field to connect the profile identity to the tower’s gold light and the world’s dark-fantasy palette. The identity treatment is compact so it remains legible at launcher and phone-HUD sizes.”

---

## Section 10 — “Verification and next steps”

**Speaker:**

“The implementation is validated through the Android build workflow. Native tests, online-service tests, foundation checks, APK packaging, AAB packaging, prototype APK packaging, and signing-certificate upload are part of the build path.

The immediate next step for a production co-op release is a dedicated authoritative simulation layer. That layer should own movement validation, combat resolution, inventory authority, reconnect handling, and conflict-free room shutdown. The current room service already provides a useful identity and shared-clock contract for that future server.

On the rendering side, the next visual upgrades can replace procedural boxes with authored meshes, add normal-mapped materials, add a proper sky and shadow system, and introduce level-of-detail selection by graphics tier. The camera contract does not need to change for those upgrades: first-person and third-person views can continue to consume the same yaw, pitch, target, and projection state.”

---

## Closing

**Speaker:**

“AETHELGRAD’s current 3D foundation focuses on the player’s most important questions: Where am I? Where is the tower? What time is it? What is the weather doing? Which camera view feels right? Where are my friends?

The answer is a deliberately small but extensible architecture: fixed-step native gameplay, procedural GLES3 world composition, explicit first-person and third-person camera modes, deterministic weather and day-night rules, synchronized co-op room time, and a shared tower rendezvous. That foundation is original, testable, and ready to grow into a deeper survival world.”

---

## Technical reference links

[1]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/app/src/main/cpp/forest_game.cpp "AETHELGRAD native renderer and gameplay bridge"

[2]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt "AETHELGRAD Android HUD, touch input, and co-op controls"

[3]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/src/server.mjs "AETHELGRAD authenticated co-op service routes"

[4]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/sql/003_coop_rendezvous.sql "AETHELGRAD co-op room and presence schema"

[5]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/README.md "AETHELGRAD online-service architecture boundary"
