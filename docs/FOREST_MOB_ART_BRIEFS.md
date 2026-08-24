# Aethelgard Forest Mob Art Briefs

These are original Aethelgard creatures designed for the first Unreal forest combat slice. The generated images are visual direction references, not final rigged Unreal assets.

## Shared production target

| Requirement | Standard mob | Boss mob |
|---|---:|---:|
| Final render | Unreal 5.6+ mobile-scalable material | Unreal 5.6+ mobile-scalable material |
| LOD count | 4 | 5 |
| Target LOD0 triangles | 28k–36k | 70k maximum |
| Skeleton | Shared quadruped base where possible | Dedicated humanoid/arboreal boss rig |
| Texture target | 2K hero / 1K lower LODs | 4K hero / 2K lower LODs |
| Material slots | 3–5 | 5–7 |
| Required sockets | jaw, spine, hit VFX | horns, ember core, weapon, hit VFX |
| Mobile requirement | No translucent fur cards in the gameplay baseline | Emissive core must have a low-cost fallback |

Every final mesh must be retopologized, UV’d, rigged, weighted, LOD’d, collision-authored, and performance-tested. AI-generated concept art must not be imported directly as a game mesh or texture.

## Thornfang

**Role:** aggressive stalker and first hostile forest creature.

**Silhouette:** lean, low quadruped; long forelimbs; angular bark-like shoulder plates; thorned mane; a readable tail profile. The silhouette must remain recognizable at gameplay distance without relying on eye glow.

**Palette and materials:** charcoal fur, desaturated iron-brown bark armor, restrained crimson accents, and ember-orange eyes. Keep emissive areas small enough to remain legible without dominating night scenes.

**Animation set:** idle breathing, alert sniff, roam walk, run, turn-in-place, leap wind-up, leap attack, bite attack, hit reaction, stagger, flee, death, and grounded recovery.

**Gameplay read:** the player should read the wind-up as a short crouch and shoulder drop. The leap should be punishable during recovery. Its loot identity is hide plus a low-chance forest fang.

**Reference:** `assets/mobs/thornfang_concept.png`.

## Mossback

**Role:** passive resource beast that becomes dangerous after provocation.

**Silhouette:** broad and heavy; low center of mass; short tusks; stone plates layered over moss and fur; small ancient antler nubs. Its mass should be visible in footfall and shoulder motion.

**Palette and materials:** weathered brown fur, grey-green moss, cracked stone plates, and controlled teal bioluminescent seams. The teal seams are a reward/readability accent, not a full-body glow.

**Animation set:** idle chew, ear flick, graze, slow walk, heavy run, turn, warning stomp, tusk charge wind-up, tusk charge, hit reaction, stagger, flee, death, and resource-gathering aftermath.

**Gameplay read:** it should not aggro from proximity. The warning stomp gives the player a readable decision window before the charge. Its loot identity is mossback plate plus moonleaf fiber.

**Reference:** `assets/mobs/mossback_concept.png`.

## Ember Warden

**Role:** first forest boss and capstone for The First Ember.

**Silhouette:** towering guardian with branching crown horns, asymmetrical root limbs, a bright chest ember core, and a large stone-blade arm. The chest core is the phase-read anchor.

**Palette and materials:** blackened wood, charcoal bark, dark mineral plates, ember-orange fissures, and warm amber core. The emissive core must be authored as a separate material function so low-end profiles can reduce bloom without losing phase readability.

**Animation set:** shrine idle, awakening, locomotion, heavy swing, ground slam, radial ember burst, core shield, phase transition, stagger, enraged locomotion, death collapse, and loot-reveal pose.

**Gameplay read:** the boss needs explicit attack telegraphs, a safe recovery window, a phase threshold at 66% and 33%, and a readable damage relationship between the ember core and the rest of the body. These behaviors are not included in the current base actor and belong in the future boss ability component.

**Reference:** `assets/mobs/ember_warden_concept.png`.

## Unreal asset handoff

Create the following assets under `/Game/Characters/Mobs/`:

```text
Thornfang/
  SK_Thornfang
  ABP_Thornfang
  M_Thornfang
  DA_Thornfang
Mossback/
  SK_Mossback
  ABP_Mossback
  M_Mossback
  DA_Mossback
EmberWarden/
  SK_EmberWarden
  ABP_EmberWarden
  M_EmberWarden
  DA_EmberWarden
```

The C++ `AForestSliceMob` actor exposes `MobSkeletalMesh` and `AnimationClass` for these assets. The `ForestMobArchetypes.json` file supplies the matching IDs, stats, sockets, LOD counts, and target triangle budgets.
