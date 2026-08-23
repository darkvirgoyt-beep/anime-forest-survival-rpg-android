# Aethelgard 100×100 km World Map

The world map uses a logical coordinate system from `X=0..100 km` and `Y=0..100 km`, covering **10,000 km²**. The current GLES prototype renders a normalized view for mobile safety, but biome state and future authored content should use kilometers as the stable design coordinate system.

## Biome regions

| Biome | World X range | World Y range | Population rule | Primary role |
|---|---:|---:|---:|---|
| Forest | 0–33 km | 0–100 km | Residents, farmers, passive wildlife, and early hostile creatures | Launch region, camp, farming, first crafting loop |
| Sand | 34–67 km | 0–100 km | Residents, traders, kiln workers, and settlement wildlife | Processing, water management, heat survival, mobility upgrades |
| Snow | 68–100 km | 0–100 km | Predators only; no human settlement | High-risk materials, 100 HP predators, Frostclaw route |

The native prototype maps its normalized horizontal simulation range `-0.90..0.90` onto the logical `0..100 km` world range. The biome selector uses the same 34/68 km boundaries as the map, so the HUD and future content packs share one coordinate contract. The current screen is a compressed vertical slice; the full 100×100 km space is a production-world planning envelope that will require streamed cells.

## Landmark coordinates

| Landmark | Biome | X (km) | Y (km) | Gameplay use |
|---|---|---:|---:|---|
| Forest Camp | Forest | 10 | 16 | Safe spawn, bedroll, workbench, first quest |
| Farming Village | Forest | 14 | 40 | Residents, farm plots, cooking and early trade |
| Moss Cave | Forest | 26 | 78 | Cave entrance, mineral gathering, early encounter |
| Sand Gate | Sand | 40 | 48 | Biome transition, settlement arrival, NPC guidance |
| Sun Kiln | Sand | 47 | 28 | Clay, sand, brick, glass, and ore processing |
| Oasis | Sand | 55 | 69 | Water refill, heat recovery, settlement route |
| Frost Gate | Snow | 74 | 57 | Snow route entry, cold tutorial, equipment check |
| Predator Basin | Snow | 84 | 59 | Predator encounter zone, pelt and claw drops |
| Frostclaw Arena | Snow | 88 | 83 | Boss encounter, reward chest, arena exit unlock |

Fast-travel stones are placed at approximately `(31 km,50 km)`, `(63 km,51 km)`, and `(71 km,55 km)` to connect the three regional routes after the player unlocks the waystone system.

## Traversal routes

The primary road runs from Forest Camp through Farming Village, crosses the forest river at approximately `(31 km,50 km)`, passes Sand Gate and Sun Kiln, reaches the Oasis, crosses the Frost Gate, and ends at Frostclaw Arena. The map image uses a dashed line for this route and a blue line for the river/water route.

The map is a precise 100×100 km design reference, not a claim that the current lightweight prototype already contains authored 3D terrain, navigation meshes, streaming cells, or final collision. Those belong to the production world pass.

## Map image

The deterministic map image is stored at `docs/world_maps/world_map_100x100.png`, with its source at `docs/world_maps/world_map_100x100.py`. Its axes are labeled in kilometers. It is safe to use as a design and implementation reference because all coordinates, labels, biome boundaries, and landmarks are project-authored.
