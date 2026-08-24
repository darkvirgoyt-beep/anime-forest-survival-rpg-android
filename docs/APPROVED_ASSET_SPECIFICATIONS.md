# Aethelgrad Approved Asset Specifications

These specifications turn the approved visual target into reusable production requirements. They are intended for original Blender models, Substance-style texture authoring, Unreal materials, and Android-ready cooked asset packs.

## Heroine and creatures

The heroine should use a clean humanoid rig with a consistent Unreal centimeter scale, authored facial and hair silhouettes, white/gold/navy/violet armor materials, four LODs, physics assets, and locomotion, combat, gathering, building, climbing, swimming, and emote animation sets. Mob assets should preserve readable silhouettes at mobile distance and expose sockets for hit effects, status effects, and world-space HP bars. Elite and boss creatures require a larger silhouette, unique material accents, and a separate boss presentation profile.

## Environment kit

The launch environment kit should include modular village foundations, walls, roofs, gates, fences, lamps, bridges, workbenches, kilns, forges, chests, beds, farm plots, waystones, riverbanks, cliffs, rocks, logs, stumps, reeds, grass, flowers, moss, and snow overlays. Every asset needs a clean collision strategy, pivot convention, material instances, and at least three runtime LODs. The kit must support player-built village growth rather than only a fixed background settlement.

## River and water kit

Water assets should include riverbed decals or terrain materials, shoreline foam, wet-rock blending, shallow/deep color transitions, flow direction, waterfall sheets, bridge crossings, swimming volumes, and audio zones. River segments must connect to the procedural world metadata so water affects traversal, gathering, weather, ambient audio, and map navigation.

## Biome material kit

| Biome | Primary material language | Key asset families |
|---|---|---|
| Verdant Crown | Wet moss, dark soil, warm wood, bright river greens | Pines, oaks, farms, bridges, village walls |
| Mistfen Wetlands | Cool fog, peat, reeds, dark water, bioluminescent accents | Marsh trees, boardwalks, herbs, poison creatures |
| Emberfall Highlands | Basalt, ash, copper, warm rock, ember glow | Cliffs, mines, forge props, ashwood |
| Sunscorch Expanse | Sandstone, sun-bleached wood, salt, dry grass | Mesas, oasis props, caravans, desert fauna |
| Moonstone Coast | Driftwood, shells, wet stone, blue moonlight | Beaches, docks, fishing props, coastal creatures |
| Frostveil Tundra | Snow, ice, silver rock, cold blue bounce | Frozen rivers, caves, frostwood, tundra mobs |
| Aethel Peaks | Alpine rock, cloud mist, starstone, hard snow | Cliffs, waterfalls, boss arenas, endgame structures |

## High-end effects and lighting

The cinematic profile should use controlled volumetric mist, warm/cool directional-light contrast, water and wetness response, contact shadows, foliage wind, fire embers, river spray, weather particles, impact bursts, and night-sky accents. Effects must have reduced mobile variants so the same authored effect can select a safe quality path without changing gameplay contracts.

## Acceptance checklist

An asset is ready for a downloadable high-end pack only when it is original or licensed, matches the approved palette and silhouette, has collision and LODs, is referenced by a stable pack mapping, fits the Android memory and frame-time budget, and is visible in a playable Unreal scene rather than existing only as a concept render.
