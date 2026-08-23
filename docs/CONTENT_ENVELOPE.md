# Production Content Envelope

The supplied content envelope is the planning target for a full production version of Aethelgard: Wild Horizons. It is not a claim about the size of the current Android vertical slice. The prototype should keep its runtime footprint small while using the same boundaries for future asset packs, streaming, and optional downloads.

| Content group | Target size | Planned contents | Prototype mapping |
|---|---:|---|---|
| Engine | 0.8–1.5 GB | Engine code, shaders, plugins, core UI, bootstrap maps, telemetry, and account shell | Existing C++17/GLES renderer, Kotlin shell, online-only onboarding, and native/JNI state bridge. |
| Forest launch region | 1.0–2.0 GB | Terrain, foliage, rocks, water, ruins, camp, cave, boss arena, materials, collision, and navigation data | Current forest and farming-village slice; procedural geometry is a temporary mobile-safe stand-in. |
| Additional biomes | 3.0–5.0 GB | Mountain, coast, desert, snow, marsh, caves, and settlement environment packs | Current forest, sand, and snow regions establish the biome contract; future regions should be separate downloadable packs. |
| Characters and animals | 1.5–3.0 GB | Original heroes, NPCs, enemies, bosses, animals, skins, skeletal meshes, materials, and morph targets | Procedural hero, villagers, wildlife, and snow predator establish silhouettes and gameplay roles; final models remain future work. |
| Animation and combat VFX | 0.8–1.5 GB | Locomotion, combat, traversal, creature behavior, boss phases, and Niagara-style effects | Fixed-step movement, attack/dodge pulses, hit flash, defeat state, and time-of-day scene tint. |
| Audio and cinematics | 1.0–2.0 GB | Music, ambience, voices, combat sounds, quest cinematics, language packs, and subtitles | Existing audio subsystem and login cinematic background; larger audio/localization packs remain optional. |
| World and gameplay data | 0.2–0.6 GB | Recipes, quests, dialogue, AI data, loot tables, building pieces, localization, and save schemas | Current progression, quest, inventory, biome, day-cycle, and HUD snapshot systems. |
| Platform variants and cache | 1.0–2.5 GB | Texture tiers, Vulkan/GLES shaders, PSO caches, device profiles, and patch headroom | Android API 26+ GLES 3 baseline; production should split device tiers and shader caches. |
| **Total content envelope** | **9.3–18.1 GB** | Base game plus streamed and optional asset packs | Use as a production planning boundary, not as a target APK size for the prototype. |

## Streaming and packaging rules

The production version should use a small bootstrap package containing the account shell, renderer, settings, a minimal loading scene, and the first required gameplay data. Each biome should be addressable as an independently versioned content pack with its own manifest, dependency list, compression profile, and device-quality variants. Characters, animals, animation/VFX, audio, cinematics, and localization should be split into optional packs where possible so a player does not download content for regions they have not reached.

The production Android package must not ship reference images or procedural stand-ins as the final world. It contains only the bootstrap shell, loading scene, renderer, and account/session code; original optimized meshes, atlases, animation clips, compressed runtime textures, cooked shaders, and world data arrive through signed Play Asset Delivery packs. A release remains locked until the selected resource tier is mounted and validated.

## Scope and originality

The envelope may be inspired by common open-world survival-RPG production categories, but the project must use original or properly licensed characters, creatures, environments, UI, audio, animation, and code. It must not copy another game’s models, maps, interface layout, named content, or proprietary assets.
