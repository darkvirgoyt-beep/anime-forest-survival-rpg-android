# GDD Implementation Map

The supplied `AAA_Open_World_Survival_GDD.docx` repeats the same production brief across six expansion modules. This file consolidates that brief into an implementable production sequence. Repetition in the source document is treated as a request for extensibility, not as six copies of the same system.

## Product target

The target is an original Android landscape open-world survival RPG with high-fidelity anime-fantasy presentation, third-person controls, hunting, animals, crafting, building, quests, bosses, co-op, accounts, cloud persistence, scalable graphics, and live content. The public repository currently proves the native Android and gameplay foundations; the Unreal production architecture document defines the path for the full 3D product.

## Requirement map

| GDD requirement | First playable implementation | Production implementation | Release gate |
|---|---|---|---|
| Open world | One forest region with camp, river, resources, cave entrance, and boss arena. | World Partition, streamed regions, biomes, caves, ruins, settlements, seasons, weather, and persistence. | Streaming, save migration, memory, and traversal tests pass on target devices. |
| Animals and hunting | Passive and hostile prototypes with proximity interaction and loot. | Data-driven species, perception, StateTree/Behavior Tree, navmesh, tracking, taming, migration, breeding budgets, and rare creatures. | AI remains deterministic enough for server authority and does not exceed CPU budgets. |
| Survival | Health, hunger, stamina, and craftable recovery loop. | Hunger, thirst, temperature, injuries, shelter, farming, fishing, cooking, preservation, and status effects. | Survival creates meaningful choices without blocking new players unfairly. |
| Crafting | Wood, fiber, stone, campfire, torch, tool, and cooked-food recipes. | Versioned recipe data assets, stations, technology tiers, equipment, potions, machines, and discoverable recipes. | Recipe validation and inventory transactions are server-authoritative. |
| Building | Planned snap-grid foundation. | Modular sockets, support validation, overlap checks, cooperative placement, storage, farms, defenses, and decoration. | Placement is deterministic, undo-safe, and replicated correctly. |
| Missions | One objective loop around gathering and camp progression. | Story, side, exploration, hunting, NPC, event, repeatable, and boss missions with reward tables. | Quest state survives reconnects and content updates. |
| Bosses | Target dummy and hitbox pipeline. | Telegraphs, phases, arena hazards, co-op roles, enrage, loot, and reset rules. | Boss behavior is readable, performant, and replayable. |
| Multiplayer | Offline first; no false online claim. | Four-player dedicated-server slice, sessions, parties, replication, reconnect, cloud saves, and anti-cheat validation. | Server rejects invalid damage, loot, crafting, and building claims. |
| Graphics | Procedural GLES 3 proof-of-life renderer. | Unreal mobile renderer, Vulkan-first with GLES fallback, original skinned characters, toon materials, VFX, LOD, instancing, and device profiles. | Target-tier frame pacing, memory, thermals, and visual QA pass. |
| Mobile controls | Landscape joystick, camera drag, attack, jump, dodge, gather, craft, gyro toggle. | Rebindable touch layout, aim assist rules, gyro calibration, haptics, gamepad, accessibility, and safe-area support. | Unsupported sensors show disabled controls and never crash or silently activate. |
| Login and services | Local/offline profile boundary. | Backend-owned authentication, profiles, cloud saves, entitlements, parties, moderation, analytics, and support tools. | Secrets stay off-device; privacy and account deletion flows are tested. |
| Live service | Versioned local data and GitHub CI. | Content bundles, staged rollout, migrations, telemetry opt-in, remote configuration, and rollback. | New content does not corrupt old saves or require a full client reinstall unnecessarily. |

## Mobile control contract

The game uses a landscape layout. Movement is a continuous normalized vector on the left. Camera orbit is a right-side drag. Attack, jump, dodge, slide/sprint, gather, interact, and craft are edge-triggered actions. Gyro aiming is optional. Android runtime detection must query the gyroscope sensor; if the device lacks one, the gyro setting is visibly disabled and labeled unsupported.

Settings should be versioned and local-first:

```text
Settings {
  graphicsPreset: Balanced | Performance | Quality
  frameRateCap: 30 | 45 | 60
  resolutionScale: float
  shadowsEnabled: bool
  foliageDensity: Low | Medium | High
  gyroEnabled: bool
  gyroSensitivity: float
  invertLookY: bool
  vibrationEnabled: bool
  audioVolumes: { master, music, effects, voice }
  controlLayoutVersion: int
}
```

## Production order

The correct order is not to build every system at once. First lock the runtime, build system, input contract, and one authoritative simulation. Next create one original hero and one forest region. Then add a real enemy, target dummy, inventory, recipes, and a boss arena. After the loop is fun and profiled on real Android devices, add dedicated-server co-op. Only then expand the world, species, recipe count, cosmetics, social systems, and live-service cadence.

This order prevents a common failure mode in large game projects: building a large map and menu surface before proving that movement, camera, combat, network authority, and performance are reliable.

## Asset and copyright rule

Anime inspiration is acceptable as a broad style reference. Characters, animals, names, logos, maps, dialogue, music, textures, models, and animation clips must be original or legally licensed. The project must maintain an asset manifest with source, license, author, import settings, target memory budget, and replacement plan.

## Current status

The public Android repository has a C++17 physics/controller/combat core, a Kotlin landscape shell, touch input, gyro capability handling, procedural GLES presentation, native tests, and a release-style Android CI pipeline. The next major migration is the Unreal C++ project for real 3D content, skeletal animation, world streaming, and dedicated-server co-op.
