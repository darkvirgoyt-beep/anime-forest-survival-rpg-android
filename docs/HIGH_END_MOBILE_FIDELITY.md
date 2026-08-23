# High-End Mobile Fidelity Pass

This pass raises the visual and simulation baseline while preserving the small bootstrap install. The native GLES slice now has a scalable premium path for quality levels 3 and 4, and the Unreal side exposes the same intent through Blueprint-editable tuning properties.

## Verified prototype improvements

| Area | Upgrade |
|---|---|
| Native lighting | Quality-aware micro-light variation, rim response, animated time input, and stronger atmospheric fog separation. |
| World detail | Additional canopy clusters, firefly glow orbs, animated water caustic bands, and a campfire halo at high quality. |
| Streaming world | Optional `DetailInstances` HISM layer with `DetailsPerChunk = 96`, deterministic chunk seeding, and the same fade-in/unload lifecycle as trees and rocks. |
| Character physics | Configurable air control, higher ground acceleration/braking, steeper walkable slope target, water-specific acceleration, and three-pass contact resolution. |
| Camera | A full 360-degree horizontal orbit target, configurable camera lag and rotation lag, and existing obstruction testing. |
| Weather and effects | Quality-scaled rain, lightning, foliage motion, water highlights, and ambient firefly activity. |

The bootstrap APK continues to contain only the renderer, loading scene, controls, and minimal gameplay shell. The high-resource AAB/PAD path is where real authored meshes, materials, textures, Niagara effects, audio, world sectors, and compiled shaders belong.

## Real Unreal content still required

The repository does not contain the final Unreal `.uasset`/`.umap` world or cooked 3D payload. The new `DetailMesh` property is intentionally optional: assign a grass, fern, pebble, or ground-clutter mesh in the Unreal editor and the HISM layer will populate each streamed chunk. Assigning a production mesh and tuning its material, Nanite/LOD policy, collision, and shadow settings remains an Unreal-content task.

For the production High Resources tier, cook and stage real content into the existing packs. Use `assetpack_graphics_base` for shared mobile materials and render resources, `assetpack_hd_textures` for PBR/virtual-texture pages, `assetpack_foliage_lods` for high-density vegetation, `assetpack_terrain_lod` for landscape heightfields and shadow data, `assetpack_shaders_vulkan` and `assetpack_shaders_gles` for device-specific compiled shaders, and `assetpack_vfx` for Niagara/weather/impact systems.

Do not add zero-filled files to meet the 6.6 GB planning target. The budget is a ceiling and packaging plan; the authored total must come from actual cooked runtime assets.

## Device scaling

The renderer checks `gGraphicsQuality` before enabling the premium detail pass. Low and medium devices keep the compact path, while Ultra/Max can use the additional detail objects and particles. The Android settings screen should continue to expose Graphics Quality and FPS controls so the player can reduce load or battery use without changing the downloaded content tier.
