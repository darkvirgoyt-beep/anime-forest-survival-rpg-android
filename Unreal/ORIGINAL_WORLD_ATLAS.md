# Aethelgard original world atlas

## Design boundary

This is a fictional, authored world atlas. It uses no geographic coordinates, maps, coastline geometry, elevation data, roads, buildings, images, or landmarks from the uploaded KML references. The cold-region and forest-region interests expressed by the user become new Aethelgard biomes rather than representations of any place on Earth.

## First playable region: Verdant Veil

The initial Unreal level is an **8 km × 8 km** original landmass centered on the Verdant Veil. A rain-fed basin sits beneath the fractured Spine of Ael, with a luminous river cutting through giant-root woods into a hidden saltwater inlet. The first camp, Emberling encounter, crafting grove, and ruin beacon are invented placements designed for exploration, performance, and tutorial flow.

| Zone | Fictional role | First-playable gameplay purpose |
|---|---|---|
| Frostwake Crown | High alpine ice shelf and aurora caverns | Future cold-survival expansion; no entry in the first slice. |
| Ironroot Highlands | Wind-carved stone mesas and mineral hollows | Mining, climbing, and glider-route expansion. |
| Verdant Veil | Giant-root temperate basin with mist rivers | Starting forest, gathering, camp building, Emberling bonding, and first ruin. |
| Shardwater Coast | Black-sand bays and crystalline tide pools | Fishing, sailing, and shore-cave expansion. |
| Sunken Canopy | Flooded jungle terraces and canopy bridges | Later traversal, rare herbs, and vertical exploration. |
| Emberfall Hollow | Volcanic redstone basin with ancient forges | Midgame crafting and boss encounter expansion. |

## Fictional atlas orientation

```text
                         FROSTWAKE CROWN
                    ice shelf / aurora caverns

       IRONROOT HIGHLANDS              SHARDWATER COAST
       mesas / mineral hollows          black-sand tide pools

                         VERDANT VEIL
                 first camp / river / giant roots

       SUNKEN CANOPY                 EMBERFALL HOLLOW
       flooded terraces               forge basin / redstone
```

The illustrated orientation is a gameplay composition, not a trace of a real map. `UForestSliceOriginalWorldGenerator` supplies a deterministic fictional elevation field from a game seed and normalized local game-space coordinates. It must not receive KML latitude/longitude values or source external map data at runtime.

## Original 3D build sequence

1. Create a new World Partition level named `L_VerdantVeil` and define an 8 km × 8 km playable rectangle.
2. Use a seed such as `190721` with `SampleOriginalElevationMeters` to block out ridge, basin, and river corridors from scratch.
3. Replace the procedural blockout with hand-sculpted ridges, original cave volumes, a fictional river route, and authored landmarks.
4. Paint original biome masks for root-forest floor, granite ridge, river mud, luminous moss, ruined stone, and shoreline sand.
5. Place only original/licensed foliage, rocks, creatures, props, VFX, sounds, and materials with LOD, collision, navigation, and Android memory budgets documented in `ASSETS.md`.
6. Profile on Android before expanding the landscape; future zones stream as separate authored regions rather than increasing the first level indefinitely.

## Authority and save rule

The seed establishes the baseline terrain only. Player structures, harvested nodes, companion state, camp ownership, and co-op mutations remain server-authoritative and are stored separately from the baseline terrain field.
