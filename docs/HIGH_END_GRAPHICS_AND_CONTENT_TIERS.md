# Aethelgard High-End Graphics and Content Tiers

The canonical art-direction brief is [Approved Art Direction](APPROVED_ART_DIRECTION.md), and the reusable production requirements are in [Approved Asset Specifications](APPROVED_ASSET_SPECIFICATIONS.md). Both documents use the approved visual target image as the source of truth.

## Visual target

The reference image `assets/aethelgard_high_end_visual_target.jpg` defines the intended direction: a premium fantasy survival RPG with physically readable materials, strong character silhouettes, cinematic atmospheric depth, a dense handcrafted village, rivers and waterfalls, layered terrain, and clear biome contrast.

![Aethelgard high-end visual target](../assets/aethelgard_high_end_visual_target.jpg)

This is the canonical visual target for the real AAA production game. The current checkout contains the delivery contracts and runtime source foundation; the authored production result requires original or licensed Blender/Unreal meshes, rigging, animation, PBR textures, water materials, foliage clusters, VFX, lighting, collision, navigation, and device profiling.

## Download-to-quality contract

| Required package | Runtime profile | Effect after download | Intended deployment |
|---|---|---|---|
| HIGH-END RESOURCES | `cinematic-high` | High-resolution PBR, full foliage budget, layered water, compiled shader and pipeline resources, all world/character/audio/VFX content, and four-player online gameplay data | Private Android APK plus HTTPS archive for the trusted four-player test group |

The Android runtime gates the native renderer on `setContentTierReady`. Before the high-end archive is mounted and verified, the world remains locked. After the private HTTPS downloader or matching OBB reports completion, the high-end graphics envelope is applied. This prevents a settings label from pretending that missing content is available.

## Production asset rules

Real production builds must supply cooked Unreal `.pak`, `.ucas`, and `.utoc` runtime content, with stable pack names and verified manifests. No padding is generated to create the impression of a large game. A high-end release is complete only when the cooked payload contains the authored assets described by the manifest and the Android device profile can load them without thermal, memory, or frame-time failure.

## Quality gates

The high-end contract test verifies the single complete package, including HD textures, VFX, pipeline cache, private HTTPS verification, and Android/native readiness hooks. CI validates asset budgets, resource-center behavior, native tests, online-service tests, Android alignment, APK/AAB output, and standalone OBB verification when a real cook is supplied.
