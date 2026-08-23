# AETHELGRAD Realistic Graphics and Large-Content Delivery

## Scope

AETHELGRAD currently contains a mobile-safe OpenGL ES prototype and an Unreal Engine 5.6 production foundation. The realistic graphics path is therefore centered on Unreal for final 3D presentation while preserving the Android renderer as a lightweight gameplay harness. The project now has a real Play Asset Delivery structure, but the repository does not pretend that empty directories equal 10 GiB of finished content.

> The 10 GiB target must be reached with authored or properly licensed models, materials, animations, audio, cinematics, and world data. Padding files are explicitly forbidden.

## Realistic rendering target

The Unreal target uses physically based materials, HDR lighting, controlled dynamic lights, mobile-compatible shadow receivers, sky atmosphere, virtual-texture support, texture streaming, and conservative post-processing. It intentionally does not enable desktop-only Lumen or hardware ray tracing for the Android profile. The renderer should be authored in quality tiers so low-memory devices can use smaller textures, fewer dynamic lights, simpler particles, and shorter view distances without changing gameplay.

| Content area | Quality target | Production requirements |
|---|---|---|
| Hero and creatures | High-detail original models with authored LODs | Rig, skin weights, facial controls, cloth/hair hooks, animation set, collision, and device captures. |
| Environment | PBR terrain, foliage, architecture, water and snow | Albedo, normal, roughness, ambient-occlusion/mask maps, collision proxies, streaming cells, and biome variation. |
| Lighting | Baked/static support plus a limited number of movable lights | Day/night profiles, shadow bias validation, lantern pools, and mobile GPU frame-time budgets. |
| Water | Surface material plus gameplay physics volumes | Reflections or planar approximations per tier, shoreline foam, underwater fog, audio zones, swimming animation, and current volumes. |
| Hair and cloth | Simulation or authored secondary motion selected per device tier | Groom or cards for hair, cloth simulation where affordable, fallback bones/animation curves, and wetness material response. |
| Cinematics | High-resolution streamed scenes | Separate pack, resumable delivery, subtitle support, and no requirement that the entire story be installed at launch. |

## Android delivery architecture

The app is linked to eleven asset-pack modules. The core pack is install-time; the forest pack is fast-follow; regional, character, audio, cinematic, texture, dungeon, VFX, and voice content are on-demand. The runtime wrapper in `AssetPackCatalog.kt` checks pack availability on every launch, requests the forest pack safely, monitors progress, and resolves paths through Play Asset Delivery rather than assuming a permanent filesystem location.

Play Asset Delivery supports install-time, fast-follow, and on-demand modes. Fast-follow and on-demand packs are downloaded outside the base app and can be requested while the game is running. The official Android documentation also requires the app to disclose download size and handle unavailable, paused, cancelled, failed, and insufficient-storage states.[1] Epic’s Unreal guidance maps this same model to cooked `.pak` chunks and Android asset-pack directories for fast-follow and on-demand delivery.[2]

The budget manifest intentionally uses conservative per-pack targets and sums to **10,116 MiB**, approximately **9.88 GiB** of source content before platform compression. The final Play Console limits must be checked at release time because limits and program eligibility can change. The project should keep install-time content small; large regions belong in fast-follow or on-demand packs so users do not need to reserve the full game before reaching the title screen.

| Pack group | Delivery | Target |
|---|---|---:|
| Core boot content | Install-time | 358 MiB |
| Forest region | Fast-follow | 1,075 MiB |
| Sand and snow regions | On-demand | 1,946 MiB |
| Characters and animation | On-demand | 1,126 MiB |
| HD materials and textures | On-demand | 1,126 MiB |
| Audio, voice and cinematics | On-demand | 2,744 MiB |
| Dungeons and VFX | On-demand | 1,741 MiB |
| **Planned total** |  | **10,116 MiB** |

## Asset governance

Every final binary must be entered in `ASSETS.md` with its source, creator, license, import settings, compression format, LOD policy, target memory, and replacement plan. The `assets/asset_budget.json` file records pack ownership and target budgets. `tools/validate_asset_budget.py` reports actual authored bytes, rejects over-budget packs, and never creates padding. A release build may only claim the 10 GiB target after the validator reports real content in the pack directories.

For texture delivery, ASTC should be the primary mobile target where supported, with additional compression variants only when the device matrix justifies their storage cost. Large cinematics and localized voice should remain on-demand. Environment packs should be split by region and by gameplay dependency so a player can enter the forest without downloading late-game dungeons.

## Build steps

Use an Android App Bundle rather than a standalone APK when validating Play Asset Delivery. Populate the `src/main/assets` directory of each pack with real cooked assets, run the budget validator, build the bundle, and test download and resume behavior through an internal Play track. For Unreal, enable chunk generation, organize content with primary asset labels, cook the chunks, and place fast-follow or on-demand `.pak` files in the configured Android asset-pack directories before packaging.

The repository’s current CI can validate Gradle configuration and the existing Android build, but it cannot manufacture the missing high-resolution 3D content. Final visual quality requires original/licensed assets, Unreal cooking, real Android GPU profiling, thermal testing, storage testing, and Play Console delivery verification.

## References

[1]: https://developer.android.com/guide/playcore/asset-delivery "Google Play Asset Delivery — Android Developers"

[2]: https://dev.epicgames.com/documentation/unreal-engine/using-google-play-asset-delivery-in-unreal-engine?lang=en-US "Google Play Asset Delivery Reference — Epic Games Documentation"
