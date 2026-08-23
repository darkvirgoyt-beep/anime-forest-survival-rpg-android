# Aethelgard: Wild Horizons — Current Game Analysis

## Executive assessment

Aethelgard is currently a **well-scoped technical vertical slice**, not a finished AAA game. The repository proves an original Android gameplay contract: landscape presentation, touch movement, camera orbit input, jump, sprint/slide, dodge, timed attacks, gathering, crafting, survival pressure, audio settings, quest progression, and a Compose-based inventory/equipment surface.[1] The core loop is coherent enough to demonstrate the intended genre identity, but the present Android renderer is a 2D OpenGL ES 3 presentation with procedural shapes, a single stationary enemy, and local in-memory progression. It is useful as a systems prototype and input harness, not as the production rendering or content foundation.

The Unreal directory is more strategically important for the stated AAA ambition. It contains a meaningful C++ character, combat, weapon, survival, world-clock, bed, procedural-forest, mobile-HUD, audio, and account foundation.[2] However, it remains a production scaffold: the repository does not contain final authored 3D content, animation assets, Blueprint UMG layouts, generated Unreal build output, world streaming configuration, dedicated-server implementation, or a device-tested vertical slice. The most important product decision is therefore not whether to keep polishing the current 2D renderer indefinitely; it is when to freeze the Android prototype contract and move the player-facing production loop into Unreal.

> **Bottom line:** the project has a credible prototype backbone and an explicit production direction. Its strongest next milestone is a device-tested 3D forest combat slice, not a larger quantity of menu surface or placeholder content.

## Current product identity

The game presents itself as an anime-inspired forest-survival RPG with exploration, gathering, crafting, animals, light combat, hunger, stamina, and quest progression. The Android README explicitly frames the current build as a vertical slice rather than a claim to commercial production scale.[1] The production master plan expands that identity into a landscape Android open-world RPG with multiple biomes, co-op, bosses, building, farming, fishing, accounts, cloud saves, live content, and asset packs.[3]

That positioning is internally consistent if the repository is treated as two layers:

| Layer | Current role | Assessment |
|---|---|---|
| Android Kotlin/C++ | Fast, deterministic prototype for input, gameplay contracts, HUD, and native tests | Valuable and concrete, but visually and systemically limited |
| Unreal C++ | Intended production path for 3D content, replication, survival, combat, and world systems | Directionally strong, but not yet a shippable game |

The naming is also worth standardizing before content production. The repository uses **Aethelgard**, while the user request used “Aethelargd.” The current branding, icon, README, and audio resources consistently use Aethelgard, so that spelling should remain the canonical product identifier unless deliberately changed in a dedicated branding pass.[1]

## What is actually playable today

### Android loop

The current Android loop is short but legible:

```text
Launch in landscape
   ↓
Move with the left joystick and orbit the camera with right-side touch
   ↓
Gather nearby wood/fiber caches
   ↓
Complete “The First Ember” gathering requirement
   ↓
Craft the ember kit
   ↓
Approach the forest warden and use the timed attack combo
   ↓
Read enemy health, hit feedback, XP, level, and quest completion in the HUD
```

`MainActivity` creates a full-screen `GLSurfaceView`, adds the imperative HUD and joystick overlay, queues input onto the GL thread, and polls a compact native HUD snapshot every 200 milliseconds.[4] The native loop advances a fixed simulation at 60 Hz, clamps the accumulator, updates physics and combat, and renders the world as colored quads, triangles, and circles.[5]

### Gameplay state flow

The state ownership is mostly clear for the prototype. Kotlin owns Android lifecycle, touch widgets, audio settings, gyro sensor registration, and the Compose inventory overlay. C++ owns movement, collision, attack timing, enemy health, hunger, resource counters, quest progression, XP, and the rendered scene. That separation is appropriate for deterministic native tests, but the inventory/equipment integration has not yet reached the same authority boundary: the Compose overlay currently builds a starter inventory from the live HUD’s material quantities and holds equipment changes in Compose session state rather than in the native progression/save model.[4]

| System | State owner today | Player-facing result | Maturity |
|---|---|---|---|
| Movement | Native C++ controller and physics | Joystick movement, jump, sprint, dodge, slide | Prototype-complete |
| Camera | Native yaw/pitch state plus right-side drag | Orbit-like camera input in the 2D presentation | Contract only |
| Combat | Native C++ timed combo and AABB hit test | Attack pulse, enemy damage, XP, warden health bar | Prototype-complete |
| Enemy | Native static position and health float | Stationary forest warden with hit/defeat feedback | Target dummy level |
| Survival | Native hunger drain and low-hunger health drain | Hunger HUD and starvation pressure | Minimal |
| Gathering | Native distance check to two hard-coded resource points | Wood/fiber or stone counter change | Minimal but testable |
| Crafting | Native material subtraction and quest progression | Craft pulse and next quest objective | Minimal but testable |
| Quest | Native progression module | Three gather actions → craft → defeat warden | Good slice structure |
| Inventory | Compose starter data plus live material counts | 24-slot grid, filters, detail panel | UI prototype; not authoritative |
| Equipment | Compose session state | Equip/unequip interaction in overlay | UI prototype; not persistent |
| Audio | Kotlin SoundPool and SharedPreferences | Music, SFX, volume categories, mute | Polished prototype layer |
| Save/profile | Not present in Android gameplay loop | No durable inventory or quest save | Missing |
| Multiplayer | Not present in Android path | Offline only | Intentionally deferred |

## Gameplay analysis

### Movement and traversal

The movement contract is one of the strongest parts of the codebase. The controller has explicit locomotion states, acceleration, friction, gravity, grounded checks, jump impulse, stamina-gated sprint, dodge invulnerability, slide timing, hitstun, and dead state.[6] The physics module is small and deterministic: it approaches target velocity, applies gravity, resolves axis-separated AABB overlaps, clamps the player to world bounds, and supports a fixed-step update.[7]

This is good prototype engineering because it creates stable input behavior and an easy test surface. The limitation is that the “third-person” label currently describes an intended design rather than the actual simulation. Physics operates on `Vec2`, the renderer draws directly in clip-space-like coordinates, there is no terrain height, navmesh, slope handling, 3D collision, camera obstruction, traversal links, swimming, climbing, or authored traversal space. The next version should preserve the input semantics while replacing the simulation and camera implementation in Unreal.

### Combat

The combat system has credible timing primitives. It defines three attacks with startup, active, recovery, combo-window, hitbox, damage, and knockback values. It supports queued attacks during recovery and emits attack-started, hit-confirmed, and attack-finished events.[8] That is enough to prove responsiveness and make the attack button more than a visual toggle.

The combat loop is not yet an RPG combat system in the production sense. The warden is fixed in place, does not attack the player, does not telegraph, does not stagger, does not move through an AI state machine, does not drop loot, and does not create a meaningful positioning problem. The native collision test checks one player attack box against one enemy AABB; there are no hurtbox collections, invulnerability rules on the target, poise, status effects, elemental tags, ranged attacks, heavy attacks, or defensive reactions. The Unreal plan correctly identifies server-authoritative traces, replicated compact combat events, boss phases, and readable telegraphs as later requirements.[3]

### Survival and progression

The survival layer is intentionally light. Hunger decreases over time and causes health loss under a threshold. Stamina recovers or is consumed by sprint, jump, dodge, and slide. This gives the loop a small preparation pressure without overwhelming the prototype.

The **The First Ember** quest is the clearest example of good vertical-slice design. It creates a sequence with a visible objective, material interaction, crafting gate, combat encounter, XP, level-up feedback, and completion state. The progression module is deterministic and unit-tested, which is a strong foundation for future data-driven quests.

The current progression is also highly linear and easy to exhaust. There is no branching choice, reward selection, build identity, item acquisition, enemy scaling, failure state, respawn loop, or persistent save. The inventory UI adds breadth to the presentation, but until item instances, equipment stats, and quest state are stored in the authoritative gameplay model, it should be treated as a UX prototype rather than a complete RPG inventory system.

## UX and presentation analysis

The Android shell has more polish than the renderer. It supports immersive landscape presentation, a visible gyro unsupported state, a layered HUD, joystick movement, multiple action buttons, audio settings, and a live progression readout.[4] The Compose inventory surface adds a coherent information architecture: filters, a 24-slot backpack, item detail, equipment paper-doll, rarity accents, quantity labels, locked/favorite markers, accessibility content descriptions, and responsive 6-column/4-column grid behavior.[9]

The primary UX problem is **action density**. The field HUD contains sprint/slide, attack, jump, dodge, gather, craft, audio settings, and inventory actions in a vertical stack. That is acceptable for a prototype debugging harness but will compete with the game world on a phone. A production HUD should reduce the always-visible actions to the combat essentials, use contextual interaction prompts, move crafting and inventory into a pause/menu layer, and introduce configurable quick slots.

The second UX problem is the difference between the UI promise and the backend truth. The inventory screen looks like a real RPG inventory, but the current session state is created in `MainActivity`; equipping changes Compose state and does not modify native combat or derived stats. That mismatch is more dangerous than having no inventory UI because it can teach players that equipment matters when it currently does not. The next implementation should expose a versioned native snapshot and transaction API before adding more item variety.

| UX dimension | Current quality | Reason |
|---|---:|---|
| Control discoverability | 7/10 | Explicit labels, visible action buttons, gyro support state |
| Readability | 6/10 | High-contrast palette and live stats; field HUD is dense |
| Feedback | 6/10 | Attack/gather/craft pulses, enemy bar, toast feedback |
| Menu information architecture | 7/10 | Clear grid/detail/paper-doll structure and filters |
| Accessibility foundation | 5/10 | Compose semantics and 48 dp targets are planned; device/TalkBack verification is absent |
| Persistence trust | 2/10 | Inventory and equipment are not durable or authoritative |
| Production input readiness | 4/10 | Good contract; no rebinding, gamepad, safe-area QA, or device matrix |

## Technical architecture analysis

### Strengths

The repository is unusually explicit about what is prototype versus production. The Android README does not claim that a small OpenGL slice is a finished AAA game, and the master plan defines milestone gates, content budgets, device testing, asset rules, and a migration path.[1][3] That honesty reduces product risk.

The native modules are also sensibly separated into physics, controller, combat, and progression. The fixed-step simulation and small pure-C++ progression module are easy to test. The Android activity keeps platform behavior in Kotlin while the native layer owns deterministic gameplay state. The Unreal side has begun to mirror the future boundaries with character, combat, weapon, survival, world clock, bed, audio, mobile HUD, and account subsystems.[2]

Audio is a meaningful polish layer. `GameAudio` persists master, music, effects, ambience, voice, and mute settings through `SharedPreferences`, uses `SoundPool`, and loads forest music plus combat, gathering, crafting, movement, animal, boss, and UI cues.[10] This gives the prototype a stronger sense of place than its primitive geometry alone would provide.

### Gaps and risks

| Risk | Severity | Evidence | Consequence |
|---|---|---|---|
| 2D prototype mistaken for 3D production | Critical | Native physics uses `Vec2`; renderer uses primitive shapes and clip-space-style coordinates.[5][7] | Large rework if content is built before engine migration |
| Inventory UI disconnected from authority | High | Compose overlay creates starter items and holds equip state locally.[4][9] | Equipment can appear functional without affecting gameplay or saves |
| No player-vs-enemy loop | High | Warden has health and hit feedback but no AI attack or damage path | Combat lacks risk, mastery, and replayability |
| No durable Android profile | High | Native state resets during initialization; no save module in Android path | Progression and equipment cannot become a real RPG loop |
| Hard-coded content | High | Resource positions, counters, enemy location, and item definitions are embedded in code | Balancing and content expansion become expensive |
| Unreal build boundary | High | Unreal code requires editor-generated headers/toolchain and has no authored content assets in repo.[2] | Production progress cannot be verified in the current sandbox |
| Mobile HUD overload | Medium | Many always-visible actions in `MainActivity` | Reduced world visibility and slower combat comprehension |
| Test coverage concentration | Medium | Native C++ tests exist; Android UI, Compose, device, save, and Unreal tests are not present | Regressions can escape until device testing |
| Thread/lifecycle complexity | Medium | Native calls are queued to the GL thread while HUD and Compose run on Android UI thread.[4] | Race, stale snapshot, and pause/resume bugs are possible |
| Content and asset readiness | Critical for AAA | Current player-facing renderer uses procedural shapes; final 3D/animation/audio content is not present.[3] | Cannot meet the stated content-lock or visual-fidelity bar |

## AAA readiness scorecard

The following scores are an engineering/product assessment of the current repository, not a claim of benchmarked commercial performance.

| Capability | Score | Assessment |
|---|---:|---|
| Playable prototype loop | 7/10 | Explore, gather, craft, fight, XP, quest completion are connected |
| Input contract | 7/10 | Touch, joystick, camera drag, gyro gating, and action routing are clear |
| Core gameplay depth | 4/10 | Deterministic primitives exist; enemy and build depth are minimal |
| RPG progression | 4/10 | XP and one quest work; no persistent build or item economy |
| Combat readability | 5/10 | Timing and hit feedback are visible; no AI/telegraphs/defense layer |
| UI foundation | 6/10 | Strong inventory concept and Compose components; integration is session-only |
| Android production readiness | 3/10 | Toolchain and device QA are not verified locally; no release-grade persistence |
| Unreal production readiness | 3/10 | Real subsystem foundations exist; content, assets, and build verification are missing |
| Multiplayer readiness | 1/10 | Intentionally offline on Android; Unreal authority is only a foundation |
| Content readiness | 2/10 | Procedural/prototype visuals, no final 3D art or animation library |
| AAA readiness overall | 2/10 | The project is at prototype/M0-to-M1 transition, not content beta or release candidate |

## Highest-priority next steps

### Priority 0 — Freeze the contract and remove prototype ambiguity

Document the canonical product name, target Android device tiers, reference resolution, frame-rate target, and which systems are authoritative in Android versus Unreal. Freeze the input action names and the core quest/progression concepts. Do not add more player-facing UI until the inventory/equipment authority boundary is decided.

### Priority 1 — Build the first real 3D production slice in Unreal

Move the current controller contract into one authored Unreal forest micro-region with a perspective camera, one original hero placeholder, one target dummy, one hostile creature, one passive animal, and one safe camp. The milestone should be playable for five to ten minutes on a real Android device, with frame-time and memory capture. The existing Unreal character and survival components are the correct starting point, but they need generated project files, input assets, UMG assets, and real device verification.[2][3]

### Priority 2 — Make inventory and equipment authoritative

Create native item definitions, item instances, equipment slots, derived stats, transaction IDs, revision numbers, and versioned profile saves. Replace the Compose starter inventory with a snapshot from that model. An equip action must modify the same equipped weapon and stats used by combat. Add tests for equip, unequip, replace, split, drop, full backpack, unknown item, and save round-trip before expanding the catalog.

### Priority 3 — Complete the combat loop

Add enemy perception, movement, attack telegraphs, damage to the player, stagger, death, respawn, loot, and a small arena boundary. Preserve the current startup/active/recovery timing because it is a good foundation. Then add one heavy attack and one defensive choice so players have a reason to manage stamina rather than only tap attack.

### Priority 4 — Add durable survival and quest state

Persist hunger-related progression where appropriate, quest stage, XP, level, inventory, equipment, and world harvest state. Add save migration tests. The Android prototype can remain offline, but it should not reset the player’s core RPG state on initialization once the menu is presented as real gameplay.

### Priority 5 — Reduce field HUD density

Keep movement, attack, dodge, jump, and one contextual interaction visible. Move gather, craft, audio settings, and inventory into contextual or pause layers. Add quick slots for tonic and tool use. Maintain the current accessible labels and unsupported gyro behavior while reducing visual competition with the play space.

### Priority 6 — Establish the production content pipeline

Create an asset manifest for hero, weapon, armor, animals, forest materials, VFX, animation, UI icons, music, voice, and localization. Require source, license, memory target, LOD policy, import settings, and replacement plan for each asset. Placeholder geometry should remain limited to system tests, in line with the master plan.[3]

## Recommended milestone map

| Next milestone | Must demonstrate | Exit evidence |
|---|---|---|
| M1 — 3D controller | Perspective camera, real collision, movement, jump, sprint, dodge, gyro, target dummy | Android device video, 60 FPS target-tier capture, controller tests |
| M2 — Forest combat slice | Hostile AI, passive animal, three-hit combo, heavy attack, hit reactions, loot, respawn | Five-to-ten-minute loop with no debug-only dependency |
| M3 — Survival and inventory | Durable profile, equipment stats, crafting, quick slots, save/reload | Save round-trip and device usability tests |
| M4 — Boss and building | Telegraphs, phases, arena, storage, camp, basic building | Repeatable encounter and memory/frame-time capture |
| M5 — Co-op alpha | Four clients, authority, reconnect, replicated loot/building/quests | Dedicated-server test and invalid-command rejection |

## Final verdict

Aethelgard has a **credible prototype core with a clear production escape hatch**. The strongest engineering decision so far is maintaining explicit boundaries between the Android prototype and the intended Unreal production path. The weakest product risk is allowing the UI surface—especially inventory/equipment—to look more complete than the gameplay authority, persistence, and content systems underneath it.

The project should now optimize for **truthful depth**: one real 3D region, one enemy that can threaten the player, one equipment item that measurably changes combat, one save/reload cycle, and one device-tested session. If those pieces are fun and stable, expanding biomes, quests, bosses, co-op, and asset packs becomes a controlled production problem. If they are not, adding more buttons, items, or map decoration will only make the gap harder to see and more expensive to remove.

## References

[1]: ../README.md "Aethelgard repository README"
[2]: ../Unreal/README_GAMEPLAY_SYSTEMS.md "Unreal gameplay systems implementation"
[3]: ../AAA_PRODUCTION_MASTER_PLAN.md "AAA production master plan"
[4]: ../app/src/main/java/com/darkvirgoyt/forestslice/MainActivity.kt "Android activity, HUD, Compose overlay integration, and JNI bridge"
[5]: ../app/src/main/cpp/forest_game.cpp "Android native renderer and gameplay loop"
[6]: ../app/src/main/cpp/controller/third_person_controller.cpp "Native movement controller"
[7]: ../app/src/main/cpp/physics/physics.cpp "Native prototype physics"
[8]: ../app/src/main/cpp/combat/combat_system.h "Native combat timing contract"
[9]: ../app/src/main/java/com/darkvirgoyt/forestslice/ui/inventory/AethelgardInventoryUi.kt "Compose inventory and equipment UI"
[10]: ../app/src/main/java/com/darkvirgoyt/forestslice/GameAudio.kt "Android audio helper and settings persistence"
