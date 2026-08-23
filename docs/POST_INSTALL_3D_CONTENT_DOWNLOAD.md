# Post-Install 3D Content Download

Aethelgard now follows a **small bootstrap install plus separately delivered content** model for production builds. The first launch opens the resource preparation screen before account onboarding. The player sees the target full-content envelope, then the app requests the independent Play Asset Delivery packs.

| Delivery boundary | Contents | Current delivery mode |
|---|---|---|
| Bootstrap | Account shell, settings, renderer, loading scene, minimal gameplay data, install-time core pack | APK/AAB plus `assetpack_core` install-time pack |
| Forest launch region | Terrain, foliage, water, village, ruins, collision, navigation | `assetpack_forest` fast-follow |
| Full 3D world expansion | Sand, snow, dungeons, textures, characters, VFX, audio, voices, cinematics | On-demand packs requested together from the resource center |

The target production content envelope is **6,750 MiB, displayed as 6.6 GB**. The six planned groups are represented by `ContentDownloadPlan`. The current repository still contains tiny deterministic development bundles so CI, direct APK testing, and offline prototype mode remain practical. Those files are not the final 6.6 GB authored 3D content.

For Google Play distribution, the release **AAB** is required so Play Asset Delivery can keep the content outside the base install. The app requests the production pack set through `AssetPackCatalog.requestProductionContent`, aggregates byte progress, and continues to a local fallback only when a direct APK cannot access Play Asset Delivery. Real production delivery still requires cooked, original, platform-qualified Unreal/runtime bundles uploaded to Play or a signed HTTPS CDN manifest.

The camera foundation now supports a full 360-degree horizontal yaw loop and nearly 180 degrees of vertical travel, exposed to the player as a 540-degree-class third-person orbit. This is camera motion support; it does not by itself create final authored terrain, skeletal characters, PBR materials, foliage LODs, or cinematic lighting.
