# AETHELGRAD High-End Systems Roadmap

AETHELGRAD is being expanded as an **original stylized creature-survival RPG**. The goal is a premium, readable mobile experience with authored creatures, responsive traversal, shared online outcomes, scalable world detail, and a data-driven production path. The code is not a Palworld clone and does not reuse commercial assets, characters, UI, or source code.

## Current production modules

| Module | Location | Responsibility |
|---|---|---|
| Companion rules | `app/src/main/cpp/rpg/companion_system.h` | Deterministic capture constraints, Fiber cost, follow/stay commands, bond-scaled assist damage, and online-authority assist gating. |
| Encounter director | `app/src/main/cpp/rpg/encounter_director.h` | Biome/time/weather-aware active-creature limits, elite slots, respawn timing, and threat scaling. |
| Runtime quality profile | `app/src/main/cpp/rpg/quality_profile.h` | Low/High vegetation, weather, effect, shadow, texture, and water budgets with content-readiness gating. |
| Unreal companion component | `Unreal/Source/ForestSlice/Public/ForestSliceCreatureCompanionComponent.h` | Replicated companion state, server RPC boundaries, health/range/tag validation, and inventory-backed Fiber deduction. |
| Unreal encounter component | `Unreal/Source/ForestSlice/Public/ForestSliceEncounterDirectorComponent.h` | Blueprint-facing encounter budgets for level/world directors, matching the deterministic mobile-domain rules. |

## Reference-backed design decisions

Epic’s Gameplay Ability System documentation motivates separating attributes, abilities, effects, and presentation. Lyra motivates modular gameplay features and data-driven experience boundaries. Epic’s PCG and World Partition guidance motivates partition-aware biome generation and HLOD-friendly world streaming. The MIT-licensed Epic Survival Game Series provides a useful older survival-game decomposition for interaction, hunger, inventory, AI, time of day, and networking. Nakama’s Apache-2.0 backend and Colyseus’ MIT authoritative room framework provide room/server authority references, while AETHELGRAD keeps its existing Express/PostgreSQL contracts until a deliberate real-time transport migration is justified. See `OPEN_SOURCE_REFERENCE_REGISTER.md` for the license and no-copy policy.

## Production sequence

The next production sequence is to persist companion and base state in the cloud-save schema, add idempotent server routes for capture/commands/camps, export typed Android/Unity/Godot wrappers, and reconcile approved state into the native and Unreal presentation layers. Movement, hitboxes, projectile simulation, reconnect reconciliation, and lag compensation remain future dedicated-server work; the current HTTPS room service must not be described as a full real-time authoritative game server.

Once authority and persistence are stable, authored creature rigs, animation state machines, navigation data, biome-specific encounter tables, structure meshes, VFX, audio, and quality-tier cooked packs can replace the current lightweight runtime geometry without rewriting the gameplay contracts.
