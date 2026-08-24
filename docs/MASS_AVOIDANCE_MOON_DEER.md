# Aethelgard Moon Deer Mass Avoidance

## Scope

Aethelgard uses Unreal Engine Mass Avoidance only for lightweight, passive Moon Deer ambient herds. The existing `AForestSliceWildCreature` actor remains the fallback and remains the required path for capture candidates, companions, hostile creatures, bosses, combat, and any authoritative co-op gameplay state.

Epic documents Mass Avoidance as an experimental MassEntity system. It calculates separation and predictive avoidance forces for Mass entities using movement, transform, velocity, agent-radius, force, and navigation-edge data. Review the official overview before enabling it in a shipping configuration: https://dev.epicgames.com/documentation/unreal-engine/mass-avoidance-overview-in-unreal-engine.

## Source integration

`UForestSliceMoonDeerAvoidanceTrait` adds the Moon Deer tag, transform/movement requirements, a force fragment, an `FAgentRadiusFragment`, a circular `FMassAvoidanceColliderFragment`, and deterministic roaming state. `UForestSliceMoonDeerRoamProcessor` writes a low-frequency move target before the built-in avoidance group. It does not apply damage, perform capture logic, mutate loot, or own replicated gameplay state.

The Mass config must also include a representation/visualization trait, transform and movement traits, and any required navigation trait. The Mass Spawner used by the forest level should spawn no more than 24 ambient Moon Deer entities in the active gameplay ring. Keep the actor-based profile available for the authoritative wildlife path.

## Editor setup

1. Enable the `MassAI` plugin in `Unreal/ForestSlice.uproject`.
2. Confirm the `ForestSlice` module depends on `MassEntity`, `MassCommon`, `MassMovement`, `MassNavigation`, and `MassSpawner`.
3. Create a Mass Entity Config asset under `/Game/Aethelgard/Mass/Creatures/MoonDeer/`.
4. Add the `ForestSliceMoonDeerAvoidanceTrait` to the config.
5. Add transform, movement, navigation, and representation traits appropriate for the selected engine version.
6. Create or configure a Mass Spawner in the forest World Partition map and assign the config.
7. Limit the spawner to passive Moon Deer and keep the maximum ambient count at or below the project setting.
8. Leave `bEnableMoonDeerMassAvoidance` and `bAllowMoonDeerMassAvoidanceOnAndroid` disabled until the feature passes Unreal automation and physical Android profiling.

## Streaming and multiplayer rules

Mass entities should be spawned only in the active gameplay ring of the current forest sector. Distant wildlife should be represented by HLOD or dormant simulation data. The server/authority owns the canonical wildlife spawn seed and gameplay state; clients must not use local avoidance to decide damage, capture success, loot, quests, camp placement, or boss behavior.

When a local multiplayer session is started, all devices must use the same cooked content manifest and forest-sector version. If the Mass representation or trait is unavailable on a device tier, the session must fall back to the actor-based wildlife path instead of spawning an incompatible entity configuration.

## Validation

The sandbox can validate source contracts but cannot compile Unreal Engine modules because the Unreal Editor/toolchain is not installed. A licensed UE 5.6 environment must compile the module, create the Mass config asset, cook the World Partition map, build HLODs, and run the Mass processor with Visual Logger and Gameplay Debugger enabled. Android enablement requires physical-device frame-time, memory, thermal, navigation, and streaming measurements.
