# Video Graphics Reference and Aethelgard Adaptation

## Reference analyzed

The linked video documents a 30-day Unreal Engine 5 development process focused on a high-fidelity combat system and a detailed environment. Its visible production techniques include advanced locomotion, root-motion animation, IK retargeting, motion warping for attack alignment, trace-based hit detection, hit-stop and localized time dilation, animation-notify-synchronized VFX/SFX, procedural content generation for landscapes, dense foliage and high-resolution materials, and spline-guided paths.

## Original Aethelgard adaptation

Aethelgard should adapt the production principles rather than copy the reference game’s assets or identity. The mobile-friendly target is stylized anime-fantasy presentation with controlled silhouettes, efficient shaders, readable combat effects, and deterministic performance.

| Reference principle | Aethelgard implementation target |
|---|---|
| Root motion | Use authored attack and dodge timing to reduce foot sliding in the hero and animal companions. |
| Motion warping | Add a short, bounded attack lunge toward the selected target so touch combat feels responsive without teleporting. |
| IK retargeting | Standardize future Aurora, Emberling, Warden, and tameable-animal rigs around a shared mobile skeleton. |
| Hit-stop and time dilation | Use a brief stylized impact pause, camera impulse, and ember slash burst instead of photorealistic violence. |
| Animation notifies | Synchronize sword trails, tame/bond pulses, creature sounds, and hit sparks to attack frames. |
| PCG and splines | Improve forest path readability with optimized foliage clusters and authored hero routes around camps and the Warden arena. |
| Dense materials | Use layered stylized PBR accents, rim lighting, fog bands, water highlights, and low-cost mobile LODs. |
| Reusable enemy logic | Keep shared encounter state and swap species-specific silhouettes, movement speeds, attacks, and taming rules. |

## IP boundary

The reference video is used only as a technique study. Aethelgard’s character designs, creatures, environment identity, materials, VFX language, names, and gameplay presentation remain original.

## Sources

[1]: https://youtu.be/eKPKpREZfLY?si=W2hTdRMS-k40ACsj "User-provided YouTube graphics-development reference"
[2]: https://developer.android.com/guide/playcore/asset-delivery/integrate-java "Android Developers: Integrate asset delivery"
