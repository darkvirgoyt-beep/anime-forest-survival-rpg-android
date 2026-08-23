# Post-Install Compiled Content Download

Aethelgard now follows a **small bootstrap install plus required compiled-content download** model for production builds. The player installs the APK/AAB, signs in, chooses **Low Resources** or **High Resources**, downloads the selected complete world package, and only then reaches character setup and gameplay. The selected tier is saved on the device so later launches resume the same pack set instead of asking again.

| Stage | What is present | What happens next |
|---|---|---|
| Bootstrap install | Account shell, settings, loading scene, minimal runtime data, and the small install-time core pack | After sign-in, the resource chooser opens automatically |
| Resource choice | **Low Resources** (about 4.2 GB) or **High Resources** (about 6.6 GB) | The selected tier is stored and its Play Asset Delivery packs are requested |
| Required compiled download | Both tiers include all world sectors, forest/sand/snow regions, gameplay characters, character-photo references, world streaming, terrain LODs, animation sets, and compiled GLES graphics; High additionally includes HD textures, dense foliage, Vulkan shaders, pipeline cache, VFX, audio, voice, and cinematics | Play Asset Delivery reports per-pack byte progress; the UI aggregates it |
| Ready gate | Every required pack reports completed and its location is available | The resource center closes and account onboarding becomes visible |
| Failure state | Missing Play services, wrong install channel, network/storage error, or unavailable pack | The game stays locked, shows the AAB/Play requirement, and exposes retry; it does not start with missing production assets |

The physical pack plan contains 18 production modules: one 358 MiB install-time core pack, one 350 MiB fast-follow forest launch pack, and 16 on-demand packs. The **Low Resources** request uses the common world, character, dungeon, GLES, streaming, terrain, and animation modules for a planned 4,300 MiB (about 4.2 GB). The **High Resources** request uses all 18 production modules for 6,750 MiB (displayed as 6.6 GB). These are pack budgets, not padding; the checked-in prototype still contains only small development content.

The runtime catalog derives the selected pack names from `ContentDownloadPlan.packNamesFor(tier)`, so the manifest, Gradle module list, resource center, and Play Asset Delivery request can be kept synchronized. The resource center uses `AssetPackCatalog.productionContentReady(tier)` to avoid re-downloading already mounted content. Until all pack totals are known, progress uses the stable envelope for the selected tier so discovering another pack cannot make the bar move backward.

For Google Play distribution, the release **AAB** is required. A direct APK is only a development smoke-test artifact; it cannot reproduce real Play Asset Delivery and remains locked if the production pack request fails. The repository now includes the four Drive character-photo references in `assetpack_characters/src/main/assets/character_photos/` and the snow-creature reference in `assetpack_snow/src/main/assets/reference_creatures/`. Those images are reference/photo content; replace or supplement them with the real licensed Unreal meshes, rigs, materials, animations, world-sector `.pak` files, compiled shader libraries, platform-qualified textures, and signed release metadata before publishing.

The camera foundation supports a full 360-degree horizontal yaw loop and nearly 180 degrees of vertical travel, exposed to the player as a 540-degree-class third-person orbit. This is camera motion support; it does not by itself create final authored terrain, skeletal characters, PBR materials, foliage LODs, or cinematic lighting.
