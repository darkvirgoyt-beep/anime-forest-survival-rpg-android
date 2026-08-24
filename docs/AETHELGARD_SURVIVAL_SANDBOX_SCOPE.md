# Aethelgrad: Wild Horizons – Crafting

## Original survival-sandbox direction

Aethelgrad is an original anime-stylized third-person survival sandbox RPG for Android. Its systemic ambition is comparable to a large survival game: the player explores a seamless wilderness, manages health and environmental needs, gathers and processes resources, builds a persistent shelter, discovers creatures, forms bonds with companions, prepares for hostile encounters, defeats region bosses, and advances with a small co-op party. The identity remains Aethelgrad’s own: a luminous wild-horizon setting, original creature ecology, modular equipment, readable mobile controls, and a world built around shelter, weather, ruins, and crafting rather than copied dinosaur or sci-fi themes.

## Core loop

The first durable loop is **explore → observe → gather → prepare → build → survive → encounter → recover → expand**. Each action should create a useful state change: harvested nodes mutate the world, crafted gear changes available routes, shelter changes temperature and sleep safety, creature behavior changes when the player is noisy or threatening, and defeated bosses unlock new materials, recipes, and map regions.

| System layer | Aethelgrad design target | First production slice |
|---|---|---|
| Wilderness | Forest, river, cliff, ruin, cave, meadow, and later desert/snow/ashen biomes with authored landmarks and deterministic streaming support | One original forest micro-region with a camp, resource node, bed, and hostile creature |
| Survival | Health, hunger, thirst, temperature, wetness, stamina, injury, shelter, sleep, status effects, and biome modifiers | Health, hunger, thirst, temperature, shelter, stamina, sleep recovery |
| Gathering | Tool-gated trees, stone, ore, fiber, plants, animal drops, and rare boss materials | One harvestable resource node with persistent mutation contract |
| Crafting | Workbench, cooking, kiln/furnace, repair, queue processing, recipes, station levels, and equipment tiers | Data-driven item and recipe contracts for one workbench path |
| Building | Snap/grid placement, foundations, walls, roofs, doors, storage, beds, stations, repair, permissions, and save mutations | One server-validated build frame that becomes a completed camp object |
| Creatures | Peaceful wildlife, threat responses, pack predators, rare elemental fauna, mounts, and bonded companions | One passive creature contract and one hostile creature contract |
| Taming/bonding | Observation, food/trust, safe approach, species-specific bonding, companion roles, and mount eligibility | Contract only; no final creature art until the movement/AI foundation is ready |
| Combat | Light/heavy chains, weapon families, ranged aim, dodge, stagger/poise, hit reactions, hurtboxes, status effects, and boss weak points | Authoritative sphere sweep, replicated health/poise, one-hit-per-target damage |
| World progression | Quests, map discovery, fog-of-war, waypoints, ruins, dungeons, region completion, and unlocks | Persistent world mutation plus one discoverable objective |
| Multiplayer | Four-player co-op authority, party permissions, shared world mutations, revive/sleep policy, replication budgets, and reconnect handling | Server-ready component boundaries; no fake peer-to-peer claim |

## Mobile-first experience

The game should remain playable with two thumbs. The left side owns movement; the right side owns camera orbit and a compact action cluster. A primary attack button is always reachable, while dodge, jump, interact, quick slots, mount, and contextual skills appear according to state. The upper-left carries the mini-map and objective context. The upper-right carries inventory, map, settings, and party access. The bottom center carries health, stamina, and compact survival status. Pinch zoom, gyro aim, haptics, aim assist, sensitivity, target FPS, resolution scale, shadows, anti-aliasing, post-processing, and audio buses belong to the settings roadmap.

## Original creature ecology

Creature design will use original silhouettes and behavior families instead of replicas of recognizable survival-game animals. Early examples include the **Mossback Grazer**, a shy antlered herbivore that flees through fern beds; the **Emberwing Kestrel**, a small aerial scout that reacts to campfire heat; the **Thornmaw Stalker**, a pack predator that tests shelter defenses; and the **Lumenhorn Warden**, a rare regional creature tied to a ruin’s power network. Names are placeholders for internal design only and should be finalized during art and narrative preproduction.

## Production rules

The target is a real content-rich product, not a padded APK. Large Android delivery must use an Android App Bundle with install-time, fast-follow, and on-demand asset packs. World content must be authored, optimized, streamed, tested on device tiers, and legally clear. The current Android harness remains useful for interaction and systems regression, while the Unreal 5.6+ branch is the production path for the real 3D world, animation, materials, AI, networking, audio, and packaging.

## Milestone order

1. **Foundation:** unify stamina and health authority, contextual interaction, quick-select, combat traces, and deterministic tests.
2. **Forest micro-region:** terrain, foliage, one camp, resource harvesting, one build frame, bed/sleep, basic weather, and saveable mutations.
3. **Creature slice:** passive wildlife, hostile predator, perception, hit reactions, loot, and original placeholder-free animation assets.
4. **Crafting/building slice:** inventory, recipes, workbench, storage, construction validation, repair, and station progression.
5. **Survival expansion:** temperature, wetness, cooking, status effects, shelter volumes, day/night threats, and biome modifiers.
6. **Encounter slice:** one original ruin, one miniboss, weak-point logic, telegraphs, stun windows, reward tables, and respawn policy.
7. **Co-op alpha:** dedicated server, four-player party authority, shared world mutations, revive/sleep vote, reconnect, and replication profiling.
8. **Content production:** additional biomes, creature families, mounts, farming, dungeons, quests, original music/foley/voice, and platform optimization.
9. **Release:** AAB, Play Asset Delivery, protected signing, Play Games Services, cloud saves, crash/performance telemetry, device certification, and live-ops support.

## Honest current status

The repository currently contains an installable Android milestone and Unreal C++ production foundations. The Unreal editor has not been run in the current sandbox because the engine and generated-header toolchain are unavailable. The next code milestone is therefore implemented as engine-ready C++ contracts and validated through structural checks and host tests, not falsely reported as a finished AAA build.
