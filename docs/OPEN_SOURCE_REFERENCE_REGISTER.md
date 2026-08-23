# AETHELGRAD Open-Source and Official Reference Register

## Purpose

AETHELGRAD uses external projects and official engine documentation for **architecture, terminology, testing ideas, and system decomposition only**. Commercial game assets, characters, UI layouts, names, maps, textures, audio, proprietary code, and copied gameplay implementations are excluded. All production code added to this repository remains original AETHELGRAD code.

## Approved references

| Reference | License / status | What AETHELGRAD may learn | What is excluded |
|---|---|---|---|
| [Epic Survival Game Series](https://github.com/vinjn/EpicSurvivalGameSeries) | MIT; older Unreal C++ survival sample | Third-person interaction, hunger, inventory, time-of-day, AI behavior-tree decomposition, networking seams, damage/respawn flow, and C++ component boundaries | No source files, assets, tutorial text, class names, or copied gameplay content are imported. The sample targets older UE4 versions. |
| [Unreal Gameplay Ability System documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine) | Official Epic documentation and engine feature | Attribute/effect/ability separation for RPG actions, status effects, animation/VFX/sound coordination, and multiplayer-aware ability ownership | No Epic sample assets or proprietary project code are copied. |
| [Lyra Sample Game documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine) | Official Epic sample/reference project | Modular gameplay feature plugins, experience definitions, scalable device profiles, online seams, and data-driven feature loading | No Lyra code, content, UI, characters, or branding are copied into AETHELGRAD. |
| [PCG with World Partition](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-pcg-with-world-partition-in-unreal-engine) | Official Epic documentation | Partition-aware procedural generation, Data Layers, HLOD layers, and streaming-friendly world authoring | No Epic world content or PCG graphs are copied. |
| [Nakama](https://github.com/heroiclabs/nakama) | Apache-2.0 | Server-owned match logic, storage boundaries, matchmaking concepts, and online service operational seams | No Nakama server code is vendored; current AETHELGRAD Express/PostgreSQL contracts remain the source of truth. |
| [Colyseus](https://github.com/colyseus/colyseus) | MIT | Authoritative room lifecycle, state synchronization, reconnect concepts, and real-time migration considerations | No Colyseus framework code is added unless a future migration is separately approved and documented. |

## Implementation policy

The reference register is not an invitation to paste code. Before a future dependency or source import is considered, the contributor must record the exact repository revision, license text, files or APIs used, compatibility with AETHELGRAD’s Android/Unreal/server stack, and whether attribution or notice distribution is required. Prefer reproducing a small behavior from first principles over importing a large subsystem.

## Current original AETHELGRAD modules

The production roadmap favors small, testable boundaries: creature-companion state and commands, deterministic encounter budgets, survival attributes, quality profiles, server-validated shared actions, and explicit save/revision contracts. Shared outcomes remain server-owned; camera, local input, prediction-friendly presentation, and low-level rendering remain client-owned.

## Verification links

The GitHub repositories and official documentation links above were reviewed during the August 2026 maintenance session. License identifiers were checked through repository metadata where available. This register should be updated whenever a new external reference influences production design.
