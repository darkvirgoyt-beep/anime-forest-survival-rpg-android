# Aethelgard reference-derived gameplay requirements

## Reference boundary

The supplied playlist is a 19-video Dawnlands gameplay series by Errorsky Gaming, and the uploaded recording shows mobile third-person survival-RPG play. These references are being used only to understand broad interaction patterns, pacing, information hierarchy, and production risks. Aethelgard must use original names, characters, creatures, maps, UI artwork, dialogue, animation, music, and assets. No protected game assets are to be copied or ripped.

## Repeated patterns across the 19 episodes

The strongest repeated pattern is a **mobile third-person survival loop** built around readable traversal, contextual interaction, preparation, and an encounter payoff. The player repeatedly moves through a recognizable biome, follows a quest or navigation marker, gathers or processes materials, equips the right tool or protection, enters a dungeon or boss arena, resolves a combat mechanic, collects a reward, and returns to a base or NPC. The uploaded recording confirms that this loop benefits from an uncluttered landscape HUD with a mini-map, quest tracker, contextual prompts, large thumb-accessible action controls, and central health/stamina information.

A second repeated pattern is **world progression through layered systems**. Episodes show modular equipment, categorized inventory and storage, recipe or station progression, material processing chains, world or biome completion, proficiency by weapon or traversal mode, and map-based discovery or teleportation. For Aethelgard, these systems should remain data-driven and server-validatable instead of becoming one-off UI scripts.

A third pattern is **encounter preparation and readable boss design**. The references repeatedly use specialized weapons, elemental or status effects, environmental hazards, summoned arenas, telegraphed attacks, destructible weak points, minions, stun or weakened windows, loot beams, and a post-boss progression update. The uploaded recording does not show enough combat depth to copy any specific encounter; it supports implementing the generic contracts only.

The final repeated pattern is **mobile performance pressure**. The analyzed episodes mention or visibly show stutter, frame drops, crashes, pop-in, and VFX-heavy boss or weather moments on low-memory phones. Aethelgard therefore needs bounded streaming, LOD budgets, pooled actors, scalable weather/VFX, frame-time capture, and device-tier acceptance tests from the beginning.

## Original Aethelgard implementation backlog

| Priority | System | Original Aethelgard requirement | Current foundation | Next action |
|---|---|---|---|---|
| P0 | Contextual interaction | Detect nearby build frames, resource nodes, beds, mounts, stations, chests, quest objects, and NPCs; present one prioritized center-screen action | Mobile HUD command router exists; bed and survival contracts exist | Add `ForestSliceInteractionComponent` and deterministic interaction priority tests |
| P0 | Inventory and quick select | Categorized grid inventory, stack counts, weight/load, quick-store, 9-slot weapon/tool palette, and equipment preview boundary | Weapon component has four slots | Add data-driven item/equipment schema and quick-slot component before UI art |
| P0 | Combat authority | Real hitboxes/hurtboxes, one-hit-per-target swing confirmation, weapon-specific traces, stamina costs, hit events, and enemy health/weakness events | Combo phases and replicated combat state exist; active hit is a placeholder | Replace placeholder hit resolution with server-authoritative trace contract and host tests |
| P0 | Stamina unification | One authoritative stamina owner shared by sprint, slide, dodge, jump attacks, heavy attacks, mounted travel, and swimming | Survival owns replicated stamina; character duplicates stamina | Remove character-side stamina authority and route all costs through survival |
| P1 | Mounts | Mount state, summon/dismount rules, mount stamina/health, camera offset, gait, terrain restrictions, and future taming boundary | No production mount component yet | Add mount state schema and interaction contract without final animal assets |
| P1 | Swimming and water | Water-volume state, enter/exit transitions, stamina and temperature effects, swim movement, animation/audio/VFX events | No production swim state yet | Add movement-state contract and tests for depth, stamina, and safe exit |
| P1 | Building | Blueprint/frame/completed/damaged states, grid or snap placement, material contribution, permissions, save mutations, repair, and station dependencies | No production building component yet | Add server-authoritative structure data and placement validation |
| P1 | Crafting stations | Workbench, kiln/furnace, cooking/processing, queue duration, recipe unlocks, station level, and offline-safe completion | No production crafting component yet | Add recipe/station data contracts and deterministic queue tests |
| P1 | Survival conditions | Hunger, thirst, temperature, wetness, shelter, injury, status effects, biome modifiers, and sleep recovery | Survival fields exist but need fixed-step and broader statuses | Refactor to fixed-step status modifiers and event-rate limiting |
| P1 | Navigation | Quest tracker, world waypoints, fog-of-war map, discovered teleport anchors, biome completion, and save persistence | No production map/quest components yet | Add data contracts for waypoint and exploration mutation |
| P1 | Companions | One passive companion and one hostile creature for the first playable slice; later follower archetypes with roles | No production AI/follower component yet | Define server-owned companion role/state contract |
| P2 | Boss encounters | Arena entry, recommended power, phases, weak points, minions, telegraphs, status effects, stun window, reward event | Combat event placeholder exists | Add encounter schema after hit resolution and enemy health contracts |
| P2 | Farming | Tile plots, soil preparation, seeds, growth timer, harvest, water, and storage | Not implemented | Keep behind building/inventory contracts |
| P2 | Weather | Rain, fog, cold, storm, biome-specific visibility and audio, scalable VFX | Config foundation only | Add weather state data and mobile scalability budgets |
| P2 | Proficiency | Usage-based weapon, swimming, mounting, gathering, and traversal proficiency with unlock thresholds | Not implemented | Add generic progression schema after item/action event contracts |

## First playable Aethelgard slice after reference analysis

The next production slice should not attempt the entire 19-episode feature set. It should make the existing Unreal foundation closer to a real game loop by implementing one original forest micro-region with one interaction detector, one build frame, one resource node, one quick-select palette, one authoritative hit trace, one passive companion placeholder contract, one hostile target, and one saveable mutation. Mounting, swimming, farming, and multi-biome weather should be represented in data contracts and tests but not falsely presented as complete until their movement and asset dependencies are ready.

## Mobile presentation rules extracted from the references

The HUD should reserve the top-left for a circular mini-map and quest/navigation context, the top-right for utility menus, the bottom-left for the movement joystick, the bottom-right for the primary action and secondary commands, and the bottom-center for health, stamina, and compact status icons. Contextual prompts should appear only when an interaction is valid and should prioritize one action rather than flooding the screen. Equipment and inventory screens should show category tabs, stack counts, weight/load, source or recipe requirements, and a clear distinction between the player backpack, storage, and companion inventory.

The target visual direction is original anime-stylized 3D with readable silhouettes, modular equipment that visibly changes the hero, biome-specific lighting, warm localized lights at camps and in interiors, fog used both for mood and mobile draw-distance management, and gameplay VFX that communicate danger, status, weak points, and rewards. These are art-direction requirements, not permission to copy any reference title.

## Performance and production gates

Aethelgard should capture frame time, memory, draw calls, shader compilation, streaming stalls, thermal behavior, and network traffic on at least one low-memory Android tier and one higher-performance tier. Boss AOE, weather particles, dense foliage, building clusters, and dungeon transitions are explicit stress scenes. If a mechanic cannot be validated on a real device or in the Unreal toolchain, the milestone report must say so.

## Sources

- User-provided playlist: https://youtube.com/playlist?list=PLyQs2YCL1R8X_JyaOZUAbjcQIDOP-7aF1&si=mdWhUjEMtuS7d1yF
- Uploaded recording: `/home/ubuntu/upload/screen-20260823-090023-1787455725981.mp4`
- Full per-episode analyses: `/home/ubuntu/dawnlands-playlist-analysis/01.txt` through `19.txt`
