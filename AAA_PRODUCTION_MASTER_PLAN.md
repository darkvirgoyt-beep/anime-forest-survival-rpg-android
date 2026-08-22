# Forest Slice — 10–20 GB AAA Production Master Plan

## Purpose

This document replaces the original quick-prototype framing with a serious production plan for an Android open-world anime survival RPG. The supplied screenshot is treated as a layout reference only: landscape orientation, left joystick, right action controls, top survival HUD, and a broad forest composition. The final game must use original characters, environments, animations, effects, audio, writing, and branding.

A 10–20 GB install footprint is a content budget, not a quality switch. The project reaches that class through authored 3D content, animation, audio, cinematic sequences, localization, shader caches, optional asset packs, and multiple biomes. The correct Android delivery format is a base application plus install-time, fast-follow, or on-demand asset packs rather than one oversized APK.

## Product target

The target is a landscape Android RPG for capable mid-to-high-end devices, with a scalable mode for lower-end phones. The launch slice is a four-player cooperative forest region. The long-term world contains forests, mountains, coasts, deserts, snow regions, caves, ruins, settlements, dynamic weather, day/night, hunting, taming, farming, fishing, cooking, crafting, modular building, quests, bosses, clans, trading, cloud saves, and live-content updates.

The production renderer should be Unreal Engine 5.6+ with C++ gameplay modules, skeletal animation, mobile-appropriate materials, Vulkan-first rendering, OpenGL ES fallback, World Partition or equivalent region streaming, and dedicated-server authority. The existing Kotlin/C++ OpenGL ES project remains useful as an input and gameplay contract prototype, but it is not the final AAA renderer.

## Asset budget target

| Content class | Target compressed size | Production content |
|---|---:|---|
| Base executable and engine | 0.8–1.5 GB | Engine code, shaders, plugins, core UI, bootstrap maps, telemetry, account shell. |
| Forest launch region | 1.0–2.0 GB | Terrain, foliage, rocks, water, ruins, camp, cave, boss arena, materials, collision, nav data. |
| Additional biomes | 3.0–5.0 GB | Mountain, coast, desert, snow, marsh, caves, and settlement environments delivered as asset packs. |
| Characters and animals | 1.5–3.0 GB | Original heroes, NPCs, enemies, bosses, animals, skins, skeletal meshes, materials, and morph targets. |
| Animation and combat VFX | 0.8–1.5 GB | Locomotion, combat, traversal, creature behavior, boss phases, Niagara effects, hit reactions. |
| Audio and cinematics | 1.0–2.0 GB | Music, ambience, voices, combat sounds, quest cinematics, language packs, subtitles. |
| World and gameplay data | 0.2–0.6 GB | Recipes, quests, dialogue, AI data, loot tables, building pieces, localization, save schemas. |
| Platform variants and cache | 1.0–2.5 GB | Texture tiers, Vulkan/GLES shaders, PSO caches, device profiles, patch headroom. |
| **Total content envelope** | **9.3–18.1 GB** | Delivered in base plus streamed and optional asset packs. |

The budget is reviewed at every milestone. The project should not inflate files with meaningless padding to reach 20 GB; size must come from useful, licensed, compressed content.

## Milestone sequence

| Milestone | Deliverable | Definition of done |
|---|---|---|
| M0 — Production reset | Engine decision, repository layout, asset manifest, coding standards, build matrix, copyright rules. | A clean production branch and reproducible toolchain are documented. |
| M1 — 3D controller | One original hero placeholder or licensed test mesh, perspective camera, spring-arm collision, locomotion, jump, sprint, slide, dodge, gyro, target dummy. | 60 FPS target on the reference device tier; controller passes automated and device tests. |
| M2 — Forest combat slice | One authored forest micro-region, one hostile animal, one passive animal, one camp, three-hit combo, heavy attack, hit reactions, loot, save. | A complete five-to-ten-minute loop is playable without debug-only dependencies. |
| M3 — Survival and crafting | Hunger, thirst, stamina, temperature, shelter, resources, recipes, tool progression, cooking, inventory, quick slots. | New game, gather, craft, build camp, survive night, and save/reload work offline. |
| M4 — Building and boss | Snap-grid building, structural validation, storage, farm plot, boss arena, telegraphs, phases, loot table. | Solo boss encounter is readable, repeatable, and performant. |
| M5 — Co-op alpha | Four-player dedicated server, party flow, replication, reconnect, server-authoritative combat and building. | Four clients can explore, fight, gather, craft, build, revive, and reconnect in one region. |
| M6 — World expansion | Additional biomes, streaming, weather, caves, settlements, NPCs, quest chains, ecosystem simulation. | Region transitions do not exceed memory, loading, or frame-time budgets. |
| M7 — Content beta | Original hero roster, animals, bosses, quests, recipes, cosmetics, audio, cinematics, localization. | Content lock candidate with complete progression and no placeholder-required path. |
| M8 — Release candidate | Device profiling, asset delivery, crash reporting, account/cloud saves, security, compliance, signing. | Store-ready AAB, rollback plan, privacy flows, and tested release keystore. |

## Controller and camera implementation

The character uses a capsule-based movement component with camera-relative intent. The left joystick produces a normalized vector. The camera spring arm owns yaw, pitch, shoulder offset, and collision shortening, while the movement component owns direction, acceleration, braking, slope limits, step height, floor detection, jump buffering, coyote time, sprint stamina, slide, dodge, swimming, and climb links.

The production animation state machine contains idle, walk, sprint, slide, jump, fall, land, dodge, attack startup, attack active, attack recovery, hit, stagger, interact, climb, swim, downed, and death. Gameplay events drive animation selection, but damage and inventory never depend on an animation callback. Root motion is allowed only for authored actions with server validation.

## Combat implementation

Combat is data-driven and server-authoritative. Each ability defines startup, active, recovery, cancel rules, input buffer, stamina or resource cost, range, damage, poise, hit reaction, knockback, elemental tags, and VFX/audio cues. The server resolves traces or hitboxes against target hurtboxes, ensures one result per target per swing, and replicates a compact combat event. The client predicts input feel and effects but cannot grant damage, loot, recipes, or progression.

The first production combat package is one light three-hit combo, heavy attack, dodge with invulnerability frames, jump attack, target dummy, hostile creature, stagger, death, respawn, and a boss with readable telegraphs and three phases. Later systems add elemental reactions, armor, poise, team roles, status effects, interrupts, parries, ranged attacks, and co-op revive rules.

## World, ecosystem, and survival

The first world is a hand-authored forest region rather than an empty procedural landscape. It contains a safe camp, river, resource clearings, a cave entrance, ruins, a settlement road, animal habitats, hostile patrols, and a boss arena. World streaming is validated early. Foliage uses instancing and hierarchical LOD. Off-screen animals use a lower-cost simulation and are promoted to full AI near players.

Survival is meaningful but not punitive. Hunger, thirst, temperature, stamina, injuries, shelter, farming, fishing, cooking, and rest create preparation choices. All inventory and world mutations use versioned data and transaction-like server commands once co-op is enabled.

## Building and crafting

Recipes and building pieces are data assets, not hard-coded UI rules. Crafting uses stations and progression tiers. Building uses snap sockets, overlap tests, support checks, permissions, co-op ownership, storage rules, and server validation. A player can preview, rotate, place, undo, and repair a structure. The first slice uses a campfire, storage box, bedroll, walls, floor, roof, farm plot, and defensive gate before expanding to villages and machines.

## Multiplayer and services

The initial online target is four-player invite-only co-op. Dedicated servers own movement validation, combat results, creature AI, loot claims, building changes, quest progress, and persistent world mutations. Client-only effects include camera shake, local particles, UI animation, and non-gameplay audio variations. The service layer provides identity, sessions, matchmaking, parties, cloud profile storage, telemetry consent, moderation, support tools, and anti-cheat signals. No provider secret belongs in an APK.

## Android UI and device capabilities

The reference UI is rebuilt as a safe-area-aware landscape HUD. The left joystick is continuous. Attack, jump, dodge, interact, gather, craft, and menu are edge actions. Sprint is hold-to-run; release can request slide when grounded. Camera orbit is right-drag. Gyro aiming is a runtime capability: devices without `TYPE_GYROSCOPE` show a disabled gray toggle labeled unsupported. Settings are versioned and include sensitivity, invert-Y, gyro sensitivity, graphics preset, frame-rate cap, resolution scale, shadows, foliage, motion blur, haptics, audio, and control layout.

The game exposes Balanced, Performance, and Quality presets. Each preset is constrained by device profiles. Unsupported or thermally unsafe combinations are disabled or automatically reduced. Performance targets are measured on real reference phones, not inferred from the editor.

## Art and animation pipeline

All assets require an entry in `ASSETS.md` with source, creator, license, import settings, target memory, LOD policy, and replacement plan. The hero pipeline is concept sheet, turnaround, modeling, retopology, UVs, materials, rig, facial controls, animation set, combat integration, device-tier validation, and final import. Animals follow the same path with locomotion, idle, threat, flee, attack, hurt, death, and interaction clips.

Placeholder geometry may be used only for system tests. It must not be mistaken for final art or remain on a player-facing critical path at content lock.

## QA and release gates

Every milestone has automated tests, host tests, device tests, network tests where applicable, visual review, memory capture, frame-time capture, crash-free session tracking, and save migration checks. Release builds use an AAB with protected signing credentials and content packs. A 20 GB target is delivered through asset delivery and optional packs; a single APK is not the correct distribution artifact.

## Current repository status

The public repository currently proves Android packaging, landscape UI, touch joystick, sprint/slide, gyro unsupported-state handling, C++ physics, controller state, combat timing, and native tests. It does not yet contain the Unreal toolchain, original final 3D assets, animation library, world streaming, dedicated servers, or the 10–20 GB content envelope. The next real production milestone is M1: a 3D engine project with one original hero, one forest micro-region, one target dummy, and one device-tested combat loop.

## Acceptance standard

The standard for this project is not “the source code exists.” A milestone is complete only when the feature is playable on an Android device, has a stable frame-time and memory profile, survives pause/resume and reconnect where applicable, is covered by tests, uses original or licensed content, and is packaged through the intended release pipeline.
