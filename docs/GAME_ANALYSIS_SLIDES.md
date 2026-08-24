# Aethelgard: Wild Horizons
## Game Analysis and AAA Roadmap

### Slide 1 — Executive verdict
**Claim:** Aethelgard has a credible prototype foundation, but it is not yet an AAA-ready game.

- The repository already proves a deterministic Android loop: explore, gather, craft, fight, gain XP, and complete a quest.
- The Unreal path contains genuine 3D character, combat, survival, weapon, world-clock, mobile-HUD, and account foundations.
- The immediate priority is truthful depth: one device-tested 3D forest combat slice, durable progression, and measurable performance.

**Visual direction:** Dark teal field, warm ember-gold title, two-layer “prototype → production” bridge.

### Slide 2 — Product identity
**Claim:** The product identity is a landscape anime-fantasy survival RPG.

| Current identity | Long-term target |
|---|---|
| Forest exploration, gathering, crafting, animals, light combat | Multiple biomes, settlements, caves, bosses, building, farming, fishing |
| Android offline prototype | Four-player invite-only co-op |
| Kotlin + C++ GLES proof | Unreal 5.6+ 3D production renderer |

- Keep Aethelgard as the canonical product spelling.
- Preserve original characters, environments, writing, audio, and branding.

### Slide 3 — The playable loop already connects
**Claim:** The current Android slice connects a complete micro-loop rather than isolated buttons.

```text
Move through the clearing
        ↓
Gather three nearby resource caches
        ↓
Craft the ember kit
        ↓
Approach and strike the forest warden
        ↓
Read HP, stamina, hunger, XP, level, loot counters, and quest completion
```

- Fixed-step native simulation keeps progression deterministic.
- The First Ember quest gives the prototype a clear beginning, middle, and end.
- Combat, resource, and quest feedback are visible in the HUD.

### Slide 4 — Two-layer architecture
**Claim:** Android is a contract prototype; Unreal is the intended production path.

```text
Android prototype                         Unreal production path
Kotlin activity + Compose HUD             ACharacter + UCharacterMovementComponent
C++17 fixed-step 2D-ish simulation        3D capsule movement + spring-arm camera
Procedural GLES primitives                 Authored meshes, animation, VFX, LOD
Local/offline state                       Replicated authority + dedicated server
Native host tests                         Device, cook, network, and performance gates
```

- Do not mistake the Android renderer for the final 3D game.
- Freeze input and progression contracts before content scale-up.

### Slide 5 — What is working
**Claim:** The project’s strongest assets are engineering clarity and a coherent prototype loop.

- Camera-relative mobile input, sprint/slide, jump, dodge, gyro capability gating.
- Native physics, controller, combat timing, and progression modules are separated and testable.
- The First Ember progression sequence creates a visible RPG arc.
- Audio has music, movement, combat, gathering, craft, animal, boss, UI, volume categories, and mute support.
- Compose inventory/equipment UI has filters, 24 slots, detail, paper-doll, accessibility labels, and lifecycle-safe overlay integration.

**Evidence marker:** Native physics, combat, and progression tests pass in the repository environment.

### Slide 6 — The critical gaps
**Claim:** The largest risks are depth, authority, persistence, and content—not a lack of menu screens.

| Gap | Why it matters |
|---|---|
| 2D procedural presentation | Cannot validate real camera, traversal, animation, or visual target |
| Static warden | No threat, telegraph, AI, stagger, death loop, or positioning mastery |
| UI-only inventory | Equipment does not yet drive authoritative combat stats or durable saves |
| No Android profile save | Quest, XP, inventory, and equipment reset rather than becoming an RPG |
| Hard-coded content | Balancing, data authoring, and content expansion remain expensive |
| No multiplayer authority | Cannot validate co-op, reconnect, loot, building, or anti-duplication |

**Callout:** UI completeness must not outrun gameplay truth.

### Slide 7 — Readiness scorecard
**Claim:** Prototype readiness is healthy; AAA readiness is low.

| Capability | Score |
|---|---:|
| Playable prototype loop | 7 / 10 |
| Input contract | 7 / 10 |
| UI foundation | 6 / 10 |
| Combat readability | 5 / 10 |
| RPG progression | 4 / 10 |
| Android production readiness | 3 / 10 |
| Unreal production readiness | 3 / 10 |
| Multiplayer readiness | 1 / 10 |
| Content readiness | 2 / 10 |
| AAA readiness overall | 2 / 10 |

**Footnote:** Scores are an engineering/product assessment, not benchmarked commercial performance.

### Slide 8 — Priority order
**Claim:** The next work must deepen the core loop in a strict order.

1. Freeze product name, target devices, reference resolution, input actions, and authority boundaries.
2. Build the M1 Unreal 3D controller and one forest micro-region.
3. Make inventory, equipment, XP, quests, and saves authoritative and versioned.
4. Complete the combat loop with enemy AI, telegraphs, player damage, stagger, death, respawn, and loot.
5. Reduce field HUD density and add contextual interactions and quick slots.
6. Establish the original asset, animation, audio, localization, and device-profile pipeline.
7. Add co-op only after the offline loop is stable and measured.

### Slide 9 — Milestones and exit evidence
**Claim:** Each milestone needs playable, measured proof rather than source-only completion.

| Milestone | Exit evidence |
|---|---|
| M1 — 3D controller | Device video, target-tier frame-time capture, controller tests |
| M2 — Forest combat | 5–10 minute loop, hostile AI, passive animal, combo, heavy attack, loot, respawn |
| M3 — Survival and inventory | Save round-trip, equipment stats, crafting, quick slots, migration tests |
| M4 — Boss and building | Readable phases, arena, storage, camp, structure validation, memory capture |
| M5 — Co-op alpha | Four clients, server authority, reconnect, replicated loot/building/quests |

### Slide 10 — Immediate decision
**Claim:** Start the device-tested M1 Unreal slice and stop expanding placeholder surface area.

- Preserve the Android prototype as an input and deterministic gameplay contract harness.
- Promote the controller and combat design into Unreal with real 3D collision, camera obstruction, animation hooks, and authoritative sweep traces.
- Make one equipment item measurably change combat and survive a save/reload cycle.
- Require a real Android device test before calling the first production milestone complete.

> Build one real region, one enemy that can threaten the player, one item that changes the build, and one save cycle. Then scale.

### Source note

All factual findings and recommendations are based on the repository’s `docs/GAME_ANALYSIS_REPORT.md`, `README.md`, current Android source, and Unreal source foundations. The deck intentionally avoids unlicensed external imagery and uses an original dark-teal/ember-gold visual language.
