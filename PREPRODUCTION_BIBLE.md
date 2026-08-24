# Aethelgrad: Wild Horizons — Complete Pre-Production Bible

**Project type:** Original anime-fantasy third-person survival RPG for Android, with a future Unreal Engine 5.6 production path.

**Document status:** Pre-production foundation, art-direction locked for concept development.

**Author:** Manus AI

## 1. Executive vision

Aethelgrad is a landscape mobile survival RPG about rebuilding a frontier life at the edge of a living magical wilderness. The player arrives as **Aurora**, a practical young wayfarer carrying a short sword, a half-remembered map, and the responsibility to relight the Heartfire at a ruined frontier camp. Exploration, gathering, crafting, survival preparation, and combat are not separate modes; they are one repeating rhythm of deciding what the wild requires next.

The intended experience is **warm, readable, and adventurous rather than grim**. The world can be dangerous, but the player should always understand the immediate goal, the source of danger, the value of preparation, and the reward for taking a risk. The game’s visual language is bright anime-fantasy with clean silhouettes, controlled saturation, soft atmospheric lighting, and localized warm lights that keep nighttime playable. This follows the project’s established art direction and its requirement that all final characters, maps, interface art, dialogue, music, and assets remain original.[1]

> **North-star promise:** Explore a beautiful wild frontier, make it livable, and become the person who can protect it.

## 2. Product definition

| Field | Decision |
|---|---|
| Working title | Aethelgrad: Wild Horizons — Crafting |
| Genre | Mobile third-person survival RPG with light action combat, crafting, building, exploration, and optional four-player co-op |
| Primary platform | Android phones and tablets in landscape orientation |
| Long-term engine | Unreal Engine 5.6+ with C++ gameplay modules and mobile-optimized rendering |
| Current prototype | Kotlin/C++ OpenGL ES vertical slice used to validate controls, HUD, and gameplay contracts |
| Camera | Third-person, camera-relative movement, orbit camera with spring-arm collision |
| Target audience | Players who enjoy approachable survival, exploration, anime-fantasy worlds, crafting, and short repeatable sessions |
| Session shape | 10–30 minute mobile sessions with longer base-building and progression arcs |
| Launch promise | One polished forest region, one complete survival/crafting/combat loop, a safe camp, a boss encounter, and stable four-player co-op foundations |
| Business posture | Premium-quality gameplay foundation first; monetization and live content are deferred until the core loop is proven |

The product is intentionally scoped as a **vertical-first RPG**. The first shippable experience is a complete forest micro-region rather than a huge empty map. The larger world expands only after the first region meets performance, usability, originality, and replayability gates.[2]

## 3. Design pillars

### 3.1 Make the wild legible

Every biome uses strong shape language, readable paths, obvious resource silhouettes, and purposeful landmarks. Fog is used for mood and draw-distance management, never to hide confusing navigation. A player should be able to pause for a moment and identify camp, objective direction, likely resources, and immediate threats.

### 3.2 Preparation creates meaningful choices

Hunger, thirst, temperature, stamina, shelter, equipment, recipes, and weather create preparation decisions without turning the game into punishment. A player may choose the safe route with fewer rewards or the dangerous route with a faster progression opportunity.

### 3.3 Every system returns value to the home base

Gathered materials become tools, meals, structures, repairs, or progression. Exploration reveals shortcuts and resources. Combat protects routes and unlocks rewards. Building turns a temporary camp into a recognizable home.

### 3.4 Combat is readable and authoritative

The client should feel responsive, but the server owns damage, loot, progression, world mutations, and co-op truth. Attacks have clear startup, active, recovery, range, stamina, hit reaction, and VFX/audio cues. The first boss teaches telegraphs and preparation before demanding mastery.[2]

### 3.5 Original identity over imitation

Reference material may inform pacing, interaction hierarchy, or broad genre expectations, but Aethelgrad must not reproduce another game’s recognizable characters, logos, maps, screenshots, UI artwork, or proprietary assets.[1]

## 4. World and story foundation

### 4.1 Premise

The old ward network that protected the frontier has failed. Its central flame, the Heartfire, is fading because the region’s ancient **Aether Roots** have been severed by storms, neglect, and the awakening of territorial guardians. Aurora reaches the abandoned Wisteria frontier camp at the beginning of a new cycle and discovers that restoring one light will require restoring the relationships around it: land, animals, villagers, tools, and trust.

### 4.2 Tone

The tone combines wonder, practical survival, quiet melancholy, and optimistic action. The player is not conquering a dead world; they are learning to live responsibly inside a living one. Threats should feel territorial and ecological rather than randomly evil. The player’s first victories are small and tangible: a repaired bedroll, a meal before rain, a lit lantern, a safe path, a rescued companion, or a finished roof.

### 4.3 Narrative structure

| Arc | Player question | Major outcome |
|---|---|---|
| Arrival | Can I make a safe place here? | Restore the frontier camp and relight the Heartfire |
| First Ember | What does this forest need from me? | Gather caches, craft the ember kit, and defeat the Forest Warden |
| Rootbound | Why are the biomes becoming unstable? | Discover the Aether Root network and repair regional anchors |
| Wild Horizons | Who benefits from restoring the old routes? | Reconnect settlements, trade paths, and companion factions |
| The Long Light | Can the frontier survive its own growth? | Choose how the restored world balances settlement and wilderness |

## 5. Player character and cast

### 5.1 Aurora, the wayfarer

Aurora is an original anime-fantasy adventurer with an expressive oval face, warm medium skin, dark teal-black hair with a distinct swept fringe, a practical cream-and-teal travel outfit, muted gold fasteners, leather utility pieces, a short sword, and a compact field satchel. Her design must communicate competence, mobility, and care for equipment rather than aristocratic power.

**Stable visual anchors:**

| Anchor | Direction |
|---|---|
| Silhouette | Short sword, shoulder capelet or scarf, asymmetrical satchel, fitted boots, high readable ponytail or swept hair mass |
| Palette | Deep teal, cream, moss green, warm brass, small ember-orange accents |
| Face | Expressive, alert, approachable; broad emotional readability at mobile viewing distance |
| Materials | Cloth, weathered leather, brushed metal, polished wood, glowing aether crystal |
| Animation | Grounded locomotion, readable combat anticipation, quick gathering motions, careful crafting gestures |
| Progression | Visible tool upgrades, improved travel gear, camp insignia, and earned protective details |

### 5.2 Emberling companion

The Emberling is a small, original foxlike aether creature with a charcoal-to-copper gradient, bright ember markings, large expressive ears, and a tail-tip glow. It is a passive companion and emotional guide, not a combat replacement. It should react to weather, campfire proximity, hidden resources, danger, and successful crafting.

### 5.3 Villagers

The first settlement uses grounded, helpful silhouettes: a cook, a cartographer, a carpenter, a field medic, and a young animal keeper. Each NPC has a practical relationship to the frontier loop and a visual prop that communicates their role at a glance.

### 5.4 Creatures

The forest includes passive grazers, small gatherable wildlife, territorial predators, and one large guardian encounter. Snow introduces predator-only encounters with strong silhouettes and readable health bars. Final creature art must remain original and must follow the same concept, turnaround, rig, animation, VFX, and device-validation pipeline as the hero.[1]

## 6. Biome and landmark plan

### 6.1 Wisteria Forest launch region

The launch region is a hand-authored forest micro-region containing the frontier camp, a river bend, three resource clearings, a farming meadow, a cave mouth, old ruins, a settlement road, animal habitats, hostile patrol territory, and the Forest Warden arena. The environment should feel navigable in a loop rather than spread as an empty square.

| Zone | Function | Visual signature | Gameplay role |
|---|---|---|---|
| Frontier Camp | Safe base | Heartfire, timber homes, warm lanterns, workbench | Craft, store, rest, accept quests |
| Wisteria Meadow | Early gathering | Purple flowering canopy, green grass, tilled plots | Learn gathering and farming direction |
| Riverbend | Traversal landmark | Shallow reflective water, stepping stones, reeds | Navigation, future fishing/swimming hook |
| Rootway Ruins | Discovery | Broken stone arches, luminous roots, moss | Lore, shortcuts, rare materials |
| Mossfang Cave | Preparation | Cool rock, mineral glow, narrow paths | Mining, shelter, future dungeon gate |
| Warden Grove | Boss arena | Ring of roots, open canopy, warm/cool contrast | Telegraph-driven boss encounter |

### 6.2 Future biomes

The world expands to an ochre desert settlement, a blue-white snow frontier, a coastal route, mountain passes, marshland, and ruins. Each biome requires a distinct material palette, weather behavior, traversal pressure, NPC culture, food chain, and boss language. Biomes should not be palette swaps.

## 7. Core gameplay loop

1. **Read the situation:** Check the quest tracker, time phase, weather, inventory, stamina, and nearby landmarks.
2. **Choose an objective:** Gather, scout, craft, build, escort, hunt, investigate, or prepare for an encounter.
3. **Traverse:** Use the joystick, camera orbit, sprint, slide, dodge, jump, and future mount/swim states.
4. **Interact:** Prioritize one nearby action such as gather, talk, open, sleep, craft, repair, or build.
5. **Prepare:** Select tools, craft food or equipment, manage load, and decide whether to return to camp.
6. **Resolve the risk:** Fight, evade, survive weather, enter a cave, or complete a boss mechanic.
7. **Claim the outcome:** Collect materials, rewards, quest progress, proficiency, and new routes.
8. **Invest at home:** Upgrade camp, storage, stations, farming, companion care, and future regional anchors.

The first playable quest, **First Ember**, should demonstrate the whole loop: Aurora arrives, gathers three resource caches, crafts the ember kit, encounters the Forest Warden, wins a readable fight, and returns to a more capable camp.[3]

## 8. Systems pre-production

| System | First production slice | Long-term expansion | Authority |
|---|---|---|---|
| Movement | Walk, sprint, jump, dodge, slide, camera orbit | Climb, swim, mount, traversal proficiency | Server validates state in co-op |
| Interaction | Nearby prioritized prompt | Stations, NPCs, beds, mounts, objects, quests | Server validates mutation |
| Inventory | Categorized items, stacks, quick slots | Weight, equipment preview, storage, companion inventory | Server owns items and claims |
| Crafting | Ember kit, basic station | Cooking, kiln, recipes, queues, unlock tiers | Server owns costs and outputs |
| Building | Campfire, bedroll, storage, basic walls | Snap grid, support, repair, permissions, farm plots | Server owns placement and save |
| Survival | HP, stamina, hunger direction, rest | Thirst, temperature, wetness, injury, shelter, status effects | Fixed-step, server-synchronized |
| Combat | Three-hit combo, heavy, dodge, hostile target | Weak points, elements, poise, ranged roles, revives | Server authoritative |
| Companion | Emberling follow and reactions | Roles, commands, bonding, utility actions | Server owns progression |
| Navigation | Quest marker and region landmark | Fog of war, map discovery, teleport anchors | Versioned save mutation |
| Weather | Clear, rain, thunderstorm direction | Biome modifiers, fog, seasonal events | Server world state |

## 9. Boss encounter brief: Forest Warden

The Forest Warden is a large guardian formed from bark plates, moss, antler-like branch growth, and controlled green-gold aether light. It is not a copy of an existing creature. The encounter takes place in a circular grove with three readable phases:

| Phase | Teaching goal | Signature behavior |
|---|---|---|
| I — Rootwake | Learn spacing and telegraphs | Ground roots, slow sweep, exposed core after recovery |
| II — Stormbark | Read changing arena pressure | Branch projectiles, moving safe zones, summoned rootlings |
| III — Heartwood | Commit to preparation | Shorter telegraphs, core vulnerability windows, arena hazards |

The fight must communicate danger through anticipation, sound, ground decals, body language, and camera framing before impact. Rewards include a Heartwood shard, camp upgrade materials, a quest progression event, and a visual change to the Heartfire.

## 10. Mobile HUD and UX

The interface must remain readable on a landscape phone with thumb reach and safe-area margins.

| Region | Contents |
|---|---|
| Top-left | Circular mini-map, compass/biome context, quest marker |
| Top-right | Settings, inventory, map, co-op, connection status |
| Top-center | Quest title, contextual event text, interaction priority |
| Bottom-left | Continuous movement joystick |
| Bottom-center | HP, stamina, hunger/temperature status, current day and time phase |
| Bottom-right | Attack, heavy, jump, dodge, gather, craft, interact, sprint/slide |
| Center-screen | One contextual prompt only when an action is valid |

The login and account flow should be visually separate from gameplay but share the same palette: deep translucent panels, warm gold accents, readable type, and a single clear primary action. Account sessions persist using encrypted Android Keystore-backed storage and are cleared only by explicit logout or confirmed backend invalidation.

## 11. Art direction and asset pipeline

The visual target is stylized anime-fantasy 3D with a polished concept-art foundation and mobile-aware readability. The launch forest uses bright greens, violet flowers, warm wood, weathered stone, and localized orange firelight. Afternoon adds gold; evening adds orange and violet; night uses deep blue with stars, moonlight, lanterns, and campfires. The project’s established time/weather direction defines a 900-second cycle with day, afternoon, evening, and night phases.[1]

### 11.1 Required art deliverables

| Order | Deliverable | Acceptance gate |
|---:|---|---|
| 1 | Vision key art | Communicates hero, camp, forest, and Heartfire in one glance |
| 2 | Character exploration | Three distinct silhouettes with stable Aurora anchors |
| 3 | Aurora turnaround | Front, side, back, three-quarter, expression strip, weapon |
| 4 | Emberling sheet | Silhouette, color, expression, tail-glow states |
| 5 | Forest biome sheet | Camp, meadow, river, ruins, cave, arena |
| 6 | Warden sheet | Silhouette, phases, weak point, attacks, scale |
| 7 | Props and station sheet | Heartfire, workbench, storage, bedroll, lantern, resources |
| 8 | HUD and login mockups | Safe-area hierarchy, readable controls, account state |
| 9 | Color and lighting board | Day/afternoon/evening/night plus clear/rain/storm |
| 10 | Production turnarounds | Modeling, UV, rig, animation, LOD, VFX, import notes |

### 11.2 Asset safety

Every production asset receives a source, creator, license, import settings, target memory, LOD policy, and replacement plan entry in `ASSETS.md`. Generated concept art is a design reference until it passes originality review and is converted into production-ready source assets. No concept image should be treated as a final game asset without modeling, rigging, collision, texture, animation, optimization, and device validation.

## 12. Technical pre-production

The production path is Unreal Engine 5.6+ with C++ gameplay modules, skeletal animation, Vulkan-first rendering, OpenGL ES fallback, region streaming, and dedicated-server authority. The existing Android prototype remains valuable for control contracts, HUD behavior, session UX, and regression tests, but it is not the final renderer.[2]

**Target technical constraints:**

| Area | Pre-production decision |
|---|---|
| Rendering | Mobile-appropriate materials, scalable foliage, baked or bounded lighting where possible |
| World streaming | Hand-authored region chunks, HLOD/instancing, bounded memory, measured transitions |
| Network | Dedicated server owns movement validation, combat, AI, loot, building, quest progress, saves |
| Input | Touch joystick, right-drag orbit, edge action buttons, optional gyro |
| Device tiers | Performance, Balanced, Quality presets with real-device acceptance tests |
| Asset delivery | Base application plus install-time, fast-follow, and on-demand packs |
| Save model | Versioned cloud snapshots, conflict handling, migration tests |
| Security | No provider secret in the APK; tokens and session data handled through secure boundaries |

## 13. Production milestones from first to last

| Milestone | Scope | Exit condition |
|---|---|---|
| M0 — Vision lock | Product pillars, original IP boundary, target devices, asset rules, first region layout | Team can explain the game and identify non-goals |
| M1 — Controller proof | Hero placeholder, camera, locomotion, jump, sprint, slide, dodge, gyro, target dummy | Stable control feel and measured target-device performance |
| M2 — Forest combat slice | Camp, meadow, resource nodes, passive animal, hostile target, combo, loot, save | Complete 5–10 minute loop without debug-only dependency |
| M3 — Survival and crafting | Hunger, stamina, resources, stations, cooking, inventory, quick slots, shelter | New game → gather → craft → rest → save/reload works |
| M4 — Building and Warden | Snap building, storage, farm plot, arena, three boss phases, reward | Solo encounter is readable, repeatable, performant |
| M5 — Co-op alpha | Four-player session, invite flow, reconnect, server authority, revive | Four clients complete a region loop together |
| M6 — World expansion | Desert, snow, coast, mountain, cave, settlement, weather, streaming | Region transitions fit memory and frame-time budgets |
| M7 — Content beta | Hero polish, NPCs, creatures, quests, recipes, VFX, audio, cinematics, localization | No player-facing placeholder-critical path remains |
| M8 — Release candidate | Account, cloud saves, asset delivery, security, crash reporting, compliance, signing | Store-ready AAB and rollback plan pass release review |
| M9 — Live operations | Seasonal content, balance telemetry, support tools, moderation, events | Updates are safe, measurable, reversible, and content-authored |

## 14. Team and production handoff

The smallest credible production team is a cross-functional strike team: creative director, game designer, technical director, Unreal gameplay engineer, network/backend engineer, environment artist, character artist, technical artist, animator, UI/UX designer, VFX artist, audio designer, QA/device specialist, producer, and localization/support partners as needed. A smaller team can share roles, but the responsibilities cannot disappear.

Each feature handoff contains a one-page brief, player goal, rules, state diagram, wireframe, art references, data schema, authority model, edge cases, test cases, performance budget, and acceptance video. Work is not considered complete because a source file exists; it is complete when the feature is playable, profiled, tested, save-safe, and visually coherent.

## 15. QA and acceptance gates

Aethelgrad requires automated tests, host tests, device tests, reconnect tests, save migration tests, visual review, memory capture, frame-time capture, crash-free sessions, and asset-budget checks at every milestone.[2]

| Gate | Question |
|---|---|
| Playability | Can a new player understand and complete the intended loop? |
| Readability | Are objectives, threats, resources, and rewards visible at phone scale? |
| Performance | Does the target device tier meet frame-time, memory, thermal, and load budgets? |
| Authority | Can the client be prevented from granting damage, loot, recipes, or progression? |
| Persistence | Do pause/resume, restart, reconnect, conflict, and migration behave safely? |
| Originality | Are characters, environments, UI, audio, and writing original or properly licensed? |
| Delivery | Are APK/AAB, asset packs, signing, rollback, and release notes reproducible? |

## 16. Final definition of done

The game is ready for release only when a first-time player can install the base package, authenticate, download the chosen content tier, enter the Wisteria region, restore the Heartfire, gather and craft, survive weather and night, defeat the Forest Warden, save and reload, reconnect with a co-op party, and understand what to do next without developer instructions. The release must use original or compatible licensed content, a protected signing path, measured device performance, safe account/session handling, and a rollback plan.

## References

[1]: docs/ART_DIRECTION.md "Aethelgrad Art Direction"
[2]: AAA_PRODUCTION_MASTER_PLAN.md "Aethelgrad AAA Production Master Plan"
[3]: docs/AETHELGARD_REFERENCE_GAMEPLAY_REQUIREMENTS.md "Aethelgrad Reference Gameplay Requirements"
