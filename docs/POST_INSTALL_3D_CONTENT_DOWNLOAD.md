# Post-Install Compiled Content Download

Aethelgard follows a **small bootstrap install plus required high-end content download** model for the private four-player build. The player installs the APK, downloads the complete high-end world archive over HTTPS, signs in, selects a server region, chooses or creates a network world, selects an original character, enters a username, and only then reaches gameplay. The archive is resumed and verified on later launches.

| Stage | What is present | What happens next |
|---|---|---|
| Bootstrap install | Account shell, settings, loading scene, and minimal runtime code | The private HTTPS archive download begins before account/world setup |
| Required high-end download | One complete archive containing all 18 production payload groups: world sectors, original characters, HD textures, foliage, shaders, pipeline cache, VFX, audio, cinematics, and animation | The app resumes with HTTP Range and shows real bytes, percentage, and status |
| Ready gate | Exact archive size and SHA-256 match the private manifest for the installed APK version | The resource center closes and Google sign-in becomes available |
| Failure state | Missing private service, wrong APK/content version, network/storage error, or checksum mismatch | The game remains locked and exposes retry; it never substitutes procedural/reference content |

The high-end plan contains 18 production payload groups totaling 6,750 MiB in the current envelope, displayed as about 6.6 GB. The private OBB builder packages the real cooked Unreal files into the APK-identity archive, and the backend serves that archive to the small trusted player group. The size is a content budget, not padding; the real authored cook must be supplied before release.

The runtime uses the complete high-end set from `ContentDownloadPlan.packNamesFor(tier)`. `AssetPackCatalog` first checks the APK-identity OBB and then uses `PrivateContentDownloader` when the archive is absent. The downloader resumes interrupted transfers, enforces HTTPS, verifies exact bytes and SHA-256, and only then marks content ready.

A direct APK can be used for the private four-player test because content is served by the HTTPS backend rather than Play Asset Delivery. The archive must contain original cooked Unreal meshes, rigs, materials, animations, world-sector `.pak`/`.ucas`/`.utoc` files, compiled shader libraries, platform-qualified textures, and signed release metadata. Character reference photos are not runtime content.

The camera foundation supports a full 360-degree horizontal yaw loop and nearly 180 degrees of vertical travel, exposed to the player as a 540-degree-class third-person orbit. This is camera motion support; it does not by itself create final authored terrain, skeletal characters, PBR materials, foliage LODs, or cinematic lighting.
