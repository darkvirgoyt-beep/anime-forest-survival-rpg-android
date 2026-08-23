# Aethelgard High-End Graphics and Content Tiers

## Visual target

The reference image `assets/aethelgard_high_end_visual_target.jpg` defines the intended direction: a premium fantasy survival RPG with physically readable materials, strong character silhouettes, cinematic atmospheric depth, a dense handcrafted village, rivers and waterfalls, layered terrain, and clear biome contrast.

![Aethelgard high-end visual target](../assets/aethelgard_high_end_visual_target.jpg)

This is a visual target, not a claim that the checked-in prototype already contains a complete AAA asset library. The production result requires original or licensed Blender/Unreal meshes, rigging, animation, PBR textures, water materials, foliage clusters, VFX, lighting, collision, navigation, and device profiling.

## Download-to-quality contract

| Download tier | Runtime profile | Effect after download | Intended device |
|---|---|---|---|
| LOW RESOURCES | `mobile-balanced` / performance-oriented | Balanced PBR, reduced foliage and effects, animated water, compact shader path | Lower-memory or thermally constrained Android |
| HIGH RESOURCES | `cinematic-high` / cinematic | High-resolution PBR, full foliage budget, stronger effects, layered water accents, compiled shader/pipeline resources, expanded world/character/audio content | Upper-midrange and high-end Android |

The Android runtime now gates the native renderer on `setContentTierReady`. Before the selected pack set is mounted, the fallback scene is capped at a safe quality level. After Play Asset Delivery reports completion, the selected graphics tier is applied and the UI identifies the mounted quality envelope. This prevents a settings label from pretending that missing content is available.

## Production asset rules

The checked-in pack directories are development payloads and must remain small. Real production builds should replace them with cooked Unreal `.pak` or equivalent runtime content, with the same stable pack names and validated manifests. No padding is generated to create the impression of a large game. A high-end release is complete only when the cooked payload contains the authored assets described by the manifest and the Android device profile can load them without thermal, memory, or frame-time failure.

## Quality gates

The graphics-tier contract test verifies that the high tier contains more content than the low tier, includes HD textures, VFX, and pipeline cache packs, and that the Android/native readiness hooks exist. CI still validates asset budgets, resource-center behavior, native tests, online-service tests, Android alignment, APK/AAB output, and standalone OBB verification.
