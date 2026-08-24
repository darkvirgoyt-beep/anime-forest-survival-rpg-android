# Aethelgrad 3D Animation Implementation

## Current playable slice

The Android renderer now presents Aurora as a fully procedural low-poly 3D blockout rather than a billboard fallback whenever the third-person view is active. The pose is assembled from 3D boxes, cylinders, spheres, and glow volumes so it remains lightweight for the current OpenGL ES prototype while matching the concept-art palette: cream travel cloth, deep teal panels, dark boots and hair, warm brass, and an ember-lit sword.

The hero is driven by the existing gameplay state instead of a separate demo animation timer:

| Gameplay state | 3D presentation |
|---|---|
| Idle | Breathing offset, subtle head and body motion, grounded contact shadow |
| Walk | Alternating leg stride, counter-swinging arms, foot lift, vertical step bob |
| Sprint | Faster stride cadence and stronger body rhythm |
| Jump/fall | Existing vertical controller position lifts the complete body |
| Dodge | Crouched base, forward lean, and weapon offset using the live dodge pulse |
| Slide | Lower body scale and low grounded presentation |
| Light attack | Sword arc, arm motion, ember glow pulse, and combat pulse synchronization |
| Heavy attack | Larger sword arc and stronger attack presentation |

The Emberling companion, mobs, trees, rocks, camp, water, weather, and Heartfire remain procedural 3D world elements. The current renderer therefore provides a playable 3D silhouette and animation proof without adding oversized binary assets to the bootstrap APK.

## Production handoff boundary

This implementation is a **playable 3D blockout**, not the final authored character asset. The next Unreal production step is to replace the procedural hero with an original modeled and rigged Aurora skeletal mesh, then connect the existing `UForestSlicePresentationComponent` montage hooks to an Animation Blueprint or Control Rig. The required final animation set is idle, walk, sprint, jump, fall, land, dodge, slide, three light attacks, heavy attack, hit, stagger, interact, gather, craft, downed, and death.

The concept-art boards in `concept_art/` are the visual target for that handoff. The production art pipeline remains: concept lock → turnaround → model → retopology → UV/materials → rig → facial controls → locomotion and combat clips → animation integration → LOD/device profiling → final import. Final assets must be original or compatible licensed content, with source, license, memory, LOD, and replacement metadata recorded in `ASSETS.md`.

## Validation target

A successful implementation pass means the Android build compiles, the native tests remain green, the game renders the procedural 3D Aurora in third person, movement changes the walk/sprint pose, attack input changes the sword pose, dodge and slide visibly lower the body, and the same gameplay state remains authoritative for damage and stamina. An Unreal skeletal replacement is a later content milestone and requires the real Unreal editor or an equivalent asset-authoring pipeline to import and validate `.uasset` and `.umap` content.

## Mobile fidelity tiers

The existing procedural animation slice is intentionally kept small in the bootstrap APK. Low Resources uses compact locomotion and combat clips, GLES-safe materials, reduced secondary motion, and the required world/animation-set pack. High Resources uses the complete locomotion, combat, traversal, emote, montage, cloth/hair secondary-motion, and cinematic camera sets from `assetpack_animation_sets` and `assetpack_cinematics`.

The selected Play Asset Delivery tier must finish mounting before production world entry. The APK contains the control surface, loading scene, native renderer, and minimal state machine; it does not claim to contain the final high-resolution skeletal meshes or animation libraries.

## Physics and animation synchronization

Movement remains deterministic at the simulation layer. The native test suite covers acceleration, sprint behavior, water transitions, combat/controller integration, and collision response. Unreal movement exposes configurable ground acceleration, braking, air control, walkable slope, water acceleration, and camera lag. Animation notifies should trigger presentation cues only; authoritative combat, inventory, stamina, and health mutations remain on the gameplay/server path.

## Unreal integration checklist

Assign the production skeletal mesh and animation blueprint to the player character, map locomotion speed and grounded state to the blend space, bind sprint/jump/dodge/slide/swim montages, assign weapon and trail sockets, and verify root-motion policy for mobile networking. Cook the resulting runtime assets into the appropriate downloadable pack. Do not stage editor-only `.uasset` or `.umap` files as if they were runtime-ready payloads.

## CI contract

The production-foundation job checks this documentation file, the Unreal character and presentation source seams, native controller tests, Android resource-center tests, the graphics-tier contract, and the final APK/AAB build. A passing contract confirms integration points and packaging structure; it does not replace a device test with the final cooked Unreal content.

## Reference-informed animation foundation

The supplied `Android_Unity3D_OpenWorld` repository was inspected only for high-level separation patterns. It contains no source or animation assets in the checkout and has no detected license, so AETHELGRAD imports nothing from it. The useful architectural cues were independently re-authored as a deterministic animation-intent layer rather than copied implementation.

`app/src/main/cpp/rpg/animation_state.h` now defines the original `AnimationMotionState`, `AnimationUpperBodyState`, `AnimationStateInput`, and `AnimationIntent` contracts. `deriveAnimationIntent` converts existing controller/combat state into a presentation-only result containing locomotion blend, play rate, body lean, vertical offset, root-motion policy, and secondary-motion policy. The native renderer consumes this result for idle/walk/sprint cadence, jump/fall offset, dodge/slide silhouette, hitstun posture, swimming cadence, and light/heavy attack layering. It does not mutate health, stamina, inventory, companion ownership, or camp state.

The Unreal presentation component now exposes the matching Blueprint-facing `FForestSliceAnimationIntent` contract with explicit motion and upper-body enums. `UpdateAnimationIntent` receives movement context from the character tick, including swimming and falling, while combat and traversal events update montage-facing upper-body/root-motion intent. Animation notifies remain presentation cues only; authoritative gameplay outcomes continue to come from the existing server/gameplay components.

The intended authored Animation Blueprint graph is:

```text
Locomotion Blend Space (Motion + LocomotionBlend + PlayRate)
  + Upper-Body Combat/Interaction Layer (UpperBody)
  + Additive Aim/Look Layer
  + Additive Hair/Cloth Layer (bAdditiveSecondaryMotion)
  + Full-Body Reaction Layer (Hitstun, Dodge, Slide, Dead)
```

Dodge and slide may use root motion only after the movement component and network policy explicitly approve the montage. The current Android path remains a procedural low-poly proof and the current Unreal path remains a Blueprint/asset integration seam; neither claims that final skeletal meshes or animation clips have been authored or cooked yet.

## Animation validation

`tests/animation_state_test.cpp` covers idle, walk, sprint, jump, fall, swim, dodge, slide, hitstun, dead, light-combo selection, heavy attacks, play-rate changes, bounded blend values, root-motion flags, and secondary-motion policy. The native CI job compiles and runs this test beside the physics, controller, cloud-state, companion, and RPG system tests.
