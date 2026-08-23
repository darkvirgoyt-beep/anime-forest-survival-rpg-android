# Aethelgard 3D Animation Implementation

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
