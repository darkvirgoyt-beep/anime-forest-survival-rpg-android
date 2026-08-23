# Post-Install Compiled Content Download

Aethelgard now follows a **small bootstrap install plus required compiled-content download** model for production builds. The player installs the APK/AAB, launches the game, sees the resource center, downloads the compiled graphics and shader packs, and only then reaches account onboarding and gameplay.

| Stage | What is present | What happens next |
|---|---|---|
| Bootstrap install | Account shell, settings, loading scene, minimal runtime data, and the small install-time core pack | The resource center opens automatically on first launch |
| Required compiled download | Compiled base materials, Vulkan/GLES shaders, pipeline-cache seeds, world-streaming data, terrain/foliage LODs, animation sets, characters, regions, audio, VFX, and cinematics | Play Asset Delivery reports per-pack byte progress; the UI aggregates it |
| Ready gate | Every required pack reports completed and its location is available | The resource center closes and account onboarding becomes visible |
| Failure state | Missing Play services, wrong install channel, network/storage error, or unavailable pack | The game stays locked, shows the AAB/Play requirement, and exposes retry; it does not start with missing production assets |

The physical pack plan contains 18 packs: one 358 MiB install-time core pack, one 350 MiB fast-follow forest launch pack, and 16 on-demand packs. The post-install full-content target is **6,750 MiB, displayed as 6.6 GB**. The compiled graphics portion explicitly includes Vulkan and OpenGL ES shader libraries, pipeline-state/cache resources, PBR materials, virtual-texture pages, terrain/foliage LODs, world-partition descriptors, and animation sets.

The runtime catalog derives its required pack names from `ContentDownloadPlan.requiredPackNames`, so the manifest, Gradle module list, resource center, and Play Asset Delivery request can be kept synchronized. The resource center uses `AssetPackCatalog.productionContentReady()` to avoid re-downloading already mounted content. Until all pack totals are known, progress uses the stable 6.6 GB envelope so discovering another pack cannot make the bar move backward.

For Google Play distribution, the release **AAB** is required. A direct APK is only a development smoke-test artifact; it cannot reproduce real Play Asset Delivery and remains locked if the production pack request fails. The repository still contains tiny deterministic development bundles, not the final authored 6.6 GB. Replace those placeholders with real cooked Unreal `.pak` files, compiled shader libraries, platform-qualified textures, and signed release metadata before publishing.

The camera foundation supports a full 360-degree horizontal yaw loop and nearly 180 degrees of vertical travel, exposed to the player as a 540-degree-class third-person orbit. This is camera motion support; it does not by itself create final authored terrain, skeletal characters, PBR materials, foliage LODs, or cinematic lighting.
