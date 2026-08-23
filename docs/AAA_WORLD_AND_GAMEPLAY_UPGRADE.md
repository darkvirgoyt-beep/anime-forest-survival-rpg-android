# Aethelgard AAA World and Gameplay Upgrade

## Direction

Aethelgard should target a **stylized AAA fantasy survival RPG**: physically grounded materials, high-contrast cinematic lighting, authored silhouettes, dense but performance-aware foliage, readable combat feedback, and a restrained premium HUD. The Android prototype remains a contract harness; the Unreal `ForestSlice` project is the production gameplay path.

> The goal is not to claim that procedural primitives are already AAA art. The goal is to establish the data, streaming, presentation, and gameplay contracts that let authored Blender/Unreal assets replace placeholders without rewriting the game.

## World envelope

The world is a 100 km × 100 km square, represented in Unreal centimeters as 1,000,000 × 1,000,000. World Partition or an equivalent cell streamer should keep only the player neighborhood, navigation tiles, foliage clusters, audio zones, and nearby simulation actors active. A deterministic world seed must produce identical biome, river, resource, landmark, and settlement placement on server and client.

| Biome | World role | Signature materials | Water / traversal identity |
|---|---|---|---|
| Verdant Crown | Starter forest and first village | oak, pine, moss, clay, fiber | Forest River and shallow crossings |
| Mistfen Wetlands | Gathering, poison creatures, reeds | peat, reeds, bog iron, herbs | marsh channels and fog banks |
| Emberfall Highlands | Ore and forge progression | basalt, copper, coal, ashwood | hot springs and steep passes |
| Sunscorch Expanse | Sand survival and caravan routes | sandstone, salt, cactus fiber, glass sand | oasis chains and dry riverbeds |
| Moonstone Coast | Fishing, trade, and storm events | shell, driftwood, moonstone, coral | tidal river mouth and beaches |
| Frostveil Tundra | Late-game cold survival | ice, silver, frostwood, wool | frozen rivers and avalanche routes |
| Aethel Peaks | Endgame traversal and boss arenas | starstone, obsidian, cloud iron | waterfalls, cliff lakes, and snowmelt |

The river network is a first-class world feature rather than a decorative spline. A primary river crosses the starter region and branches toward the wetlands, desert oasis chain, coast, and mountain snowmelt. River metadata must expose width, depth, flow speed, crossing difficulty, biome, and landmark connections to gameplay and navigation systems.

## Mob presentation

Every hostile or elite creature should have an optional world-space presentation contract. The contract exposes display name, level, faction, current and maximum health, elite/boss status, base affiliation, distance-based visibility, and the color treatment used by the HUD. Health bars should be hidden at long range, shown on target or recent damage, and replaced by a larger boss bar for boss encounters. Base-affiliated creatures additionally expose a world marker so players can understand that an enemy camp, patrol route, or defended settlement is nearby.

## Materials, tools, and building

Gathering is organized into wood, stone, ore, fiber, hide, food, alchemical, and rare crystal families. Tools have a stable item ID, tier, durability, harvesting power, and supported resource tags. Building is recipe-driven: a building piece declares its tier, footprint, required materials, and whether it is a foundation, wall, roof, storage, station, defense, farm, or traversal piece. The first playable village slice should support a campfire, foundation, wall, roof, chest, bed, workbench, farm plot, kiln, forge, gate, fence, lamp, and waystone.

Construction must validate server authority, distance, placement bounds, collision, support, and material availability before consuming inventory. A later persistence layer can serialize placed pieces by stable world coordinate and revision.

## Visual quality gates

The art pipeline should replace debug meshes in this order: hero and creature silhouettes, terrain and biome materials, authored foliage clusters, water and shoreline shaders, village kit, tools and weapons, VFX, and cinematic lighting. Blender-authored meshes should use clean topology, consistent scale, baked normals, LODs, collision, and texture sets for base color, normal, roughness, and ambient occlusion. Unreal materials should expose biome tint, wetness, snow coverage, wind response, and distance-based detail reduction.

## Implementation order

The practical vertical slice is: seven-biome and river metadata; mob health/base presentation data; material/tool/building recipe contracts; authoritative placement validation; readable HUD binding points; then authored asset import and visual verification in Unreal. A genuine 100 km AAA world, dense Blender asset library, multiplayer persistence, navmesh, animation, and mobile optimization remain production phases rather than one-file changes.
