# AETHELGRAD Post-Install 3D Content Architecture

## Presentation script

**Suggested length:** 10–12 minutes
**Audience:** Game developers, technical artists, Android engineers, producers, QA, and release managers
**Project:** AETHELGRAD — Wild Horizons
**Message:** Install a small bootstrap, download compiled graphics and shaders, then start the game only when the required world is ready.

---

## Opening — “The APK is the doorway, not the whole world”

**Speaker:**

“Today I will show the production launch architecture for AETHELGRAD. The player does not install a six-gigabyte APK. The player installs a small Android bootstrap, opens the game, and is taken to a resource center. That resource center downloads the compiled Unreal content required for the real game: graphics, shaders, pipeline caches, world data, characters, animations, audio, effects, and cinematics.

The game does not start with missing production assets. The account screen and gameplay remain locked until the required content is downloaded, mounted, and verified. This gives us the familiar mobile-game flow we want while keeping the base installation practical.”

**On-screen cue:** Show the flow as a single line:

`Install APK/AAB → Resource Center → Download → Verify and mount → Login → Gameplay`

---

## Section 1 — “Two layers, one startup contract”

**Speaker:**

“The architecture has a small Android bootstrap layer and a large Unreal content layer. The bootstrap owns the loading screen, account shell, settings, resource-center interface, download state, and the native bridge. The Unreal cook produces the runtime `.pak` files, compiled shader libraries, pipeline-state resources, and platform-qualified assets. Android never tries to compile Unreal content on the device.

The contract between the layers is simple: Android requests named Play Asset Delivery packs, waits for their completion and mounted locations, and only then allows the game world to reference those assets. This keeps the startup rule deterministic and makes an incomplete download visible instead of turning it into a crash or a broken map.”

**On-screen cue:** Point to `ContentDownloadPlan`, `AssetPackCatalog`, the resource-center overlay, and the Unreal staged `.pak` output.

---

## Section 2 — “The content envelope: 6.6 GB after installation”

**Speaker:**

“The production post-install envelope is 6,750 MiB, displayed to the player as approximately 6.6 GB. There is also a small install-time core pack for the bootstrap and loading experience. The post-install content is represented by 18 separately downloadable packs. We keep the pack targets below the documented dynamic-pack ceiling so the content can be delivered as multiple independently managed units rather than one oversized archive.”

| Family | Packs | Purpose |
|---|---|---|
| Bootstrap | `assetpack_core` | Install-time loading scene, fallback materials, and core runtime data |
| Compiled base and world | `assetpack_graphics_base`, `assetpack_world_streaming` | Shared materials, compiled render resources, streamed world descriptors, and navigation data |
| Launch and biomes | `assetpack_forest`, `assetpack_sand`, `assetpack_snow`, `assetpack_dungeons` | Terrain, villages, caves, weather, collision, navigation, and biome content |
| Characters and animation | `assetpack_characters`, `assetpack_animation_sets` | Skeletal meshes, rigs, locomotion, combat, traversal, emotes, and montage sections |
| Shaders and pipeline | `assetpack_shaders_vulkan`, `assetpack_shaders_gles`, `assetpack_pipeline_cache` | Device-appropriate compiled shaders and pipeline warm-up resources |
| Textures and LODs | `assetpack_hd_textures`, `assetpack_foliage_lods`, `assetpack_terrain_lod` | PBR materials, virtual-texture pages, foliage impostors, terrain LODs, and mobile shadow data |
| Presentation | `assetpack_audio_hd`, `assetpack_cinematics`, `assetpack_vfx`, `assetpack_voice` | High-quality audio, voice, Niagara effects, story scenes, and localization content |

**Speaker:**

“The important distinction is that these numbers are a production budget, not fake padding. The repository keeps tiny development bundles so tests remain fast. The final 6.6 GB must come from real authored and licensed content.”

---

## Section 3 — “Cooking the real Unreal content”

**Speaker:**

“The source of truth is the real Unreal content project. We enable the GooglePAD plugin, enable Generate Chunks, configure Android texture formats and shader targets, and assign Primary Asset Labels or Asset Manager rules to the planned chunks.

The bootstrap content stays in chunk zero. Regions, characters, texture tiers, shader variants, pipeline resources, animation sets, audio, VFX, and cinematics are assigned to later chunks. We then run `BuildCookRun` for Android Shipping with build, cook, stage, pak, and archive enabled. The output is inspected for the expected `.pak` files, byte sizes, and hashes.

At this point the Unreal cook has produced the compiled runtime artifacts. The Android packaging step only stages those artifacts into the corresponding asset-pack modules. It does not compile or unpack Unreal authoring files on the device.”

**On-screen cue:** Show this command shape:

```bash
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$PWD/Unreal/ForestSlice.uproject" \
  -noP4 -platform=Android -clientconfig=Shipping \
  -build -cook -stage -pak -prereqs -archive \
  -archivedirectory="$PWD/Build/Android/Archive" \
  -utf8output
```

---

## Section 4 — “From cooked chunks to Play Asset Delivery packs”

**Speaker:**

“After cooking, each selected Unreal `.pak` is copied into the `src/main/assets` directory of the matching Gradle asset-pack module. The module name must match the runtime name exactly. The repository’s 18 downloadable pack names are registered in `settings.gradle.kts`, in the Android `assetPacks` list, in the signed manifest, and in `AssetPackCatalog`.

The core pack is install-time. The forest launch pack is fast-follow. The remaining production packs are on-demand and are requested together by the resource center. The final signed Android App Bundle contains the base module plus the asset-pack table. A direct APK is useful for UI smoke tests, but it cannot reproduce Play Asset Delivery.”

**On-screen cue:** Show one module layout:

```text
assetpack_shaders_vulkan/
├── build.gradle.kts
└── src/main/assets/
    ├── pakchunk12_shaders_vulkan.pak
    ├── shader_library_vulkan.bin
    └── pack_manifest.json
```

**Speaker:**

“Every module is checked for its delivery mode and target budget. Empty `.gitkeep` markers are allowed only for the development skeleton; they are not the production graphics.”

---

## Section 5 — “Why shaders and pipeline caches are separate”

**Speaker:**

“Graphics are not only meshes and textures. Unreal also needs platform-qualified compiled shader resources and pipeline-state data. Vulkan and OpenGL ES do not use the same shader binaries, so the delivery plan has separate shader packs. A pipeline-cache pack supplies warm-up data that can reduce first-use stutter when the real content is mounted.

The bootstrap includes only the minimum loading and fallback material resources. The production resource center downloads the shader variants selected for the release configuration. The device still needs a graphics-tier policy, because a lower-memory phone should not be forced to mount every high-resolution texture tier.”

---

## Section 6 — “The resource center is a blocking state machine”

**Speaker:**

“The resource center is not just a progress bar. It is a launch state machine with four important states. It starts in preparing, moves through downloading, enters an error state when Play reports a failure or a required confirmation, and reaches ready only when all required packs are mounted.

The progress bar aggregates bytes across packs. Before every pack has reported its final total, the UI uses the stable 6.6 GB envelope, which prevents progress from jumping backward when another pack is discovered. When the last pack reaches the completed state and `getPackLocation` returns a location, the overlay closes and account onboarding becomes visible.”

| State | Player message | Game behavior |
|---|---|---|
| Downloading | “Downloading compiled graphics and shaders…” | Gameplay remains locked |
| Waiting for Wi-Fi | “Waiting for Wi-Fi” | Download is paused; retry remains available |
| User confirmation | “Download confirmation required” | Player confirms the large Play download, then retries |
| Canceled or failed | “Download canceled” or a failure message | The game remains locked until retry succeeds |
| Ready | “Compiled graphics and shaders ready” | Login and gameplay are unlocked |

---

## Section 7 — “The launch sequence in production”

**Speaker:**

“On a clean Play installation, Android creates the surface and immediately shows the resource center. It requests the required production pack names through Play Asset Delivery. The player can leave and return; Play handles resumable delivery, while the app recomputes state from mounted pack locations.

If the device has no Play Asset Delivery context—for example, a direct APK—the online client does not pretend that the full world is available. It explains that the Play Store or internal-test AAB is required before online world entry.”

**On-screen cue:** Animate a failed direct-APK request stopping at the resource center, then show the AAB path continuing to ready.

---

## Section 8 — “Quality gates before release”

**Speaker:**

“The pipeline validates the architecture before it builds a release artifact. The resource-center test injects synthetic pack events and checks the initial state, aggregate progress, monotonicity, completion, failed-pack retry, waiting-for-Wi-Fi behavior, user confirmation, cancellation, invalid byte counts, source contracts, and the Unreal project descriptor.

The asset-budget validator checks that the planned total reconciles, no dynamic pack exceeds its ceiling, and no second fast-follow pack is accidentally introduced. CI then runs the native tests, online-service checks, resource-center test, budget validation, Android release build, APK upload, AAB upload, and signing-certificate report.”

**On-screen cue:** Show the test command:

```bash
./tools/test_resource_center.py \
  --repo . \
  --unreal-project Unreal/ForestSlice.uproject
```

---

## Section 9 — “Publishing and internal testing”

**Speaker:**

“The artifact for real pack delivery is the signed AAB. We use the Play Publishing API workflow to insert the bundle into an edit, upload the AAB, assign the new version code to the internal `qa` track, and commit the edit. The repository publisher is dry-run by default and requires explicit confirmation before it can change Play Console state.

Internal testers then install from the Play test link, not from a sideloaded APK. That distinction matters because only the Play-delivered AAB can exercise fast-follow and on-demand asset-pack behavior. Testers should verify clean install, resumed download, Wi-Fi pause, cancellation, retry, insufficient storage, cached-pack update, and a lower graphics tier.”

---

## Section 10 — “What is implemented and what remains”

**Speaker:**

“The Android side now has the bootstrap gate, the 18-pack naming contract, compiled shader and pipeline pack categories, aggregated progress, explicit error states, budget validation, and CI packaging. The Unreal side has the cooking and chunking procedure and the camera/content integration contract.

The remaining production task is content production itself. The repository does not contain the final authored 6.6 GB of Unreal `.uasset`, `.umap`, `.pak`, texture, shader, audio, and animation data. The `.gitkeep` files mark the module boundaries only. Replacing them with real cooked assets is the step that turns the architecture into the full 3D game.”

---

## Closing — “Small install, complete world”

**Speaker:**

“The final player experience is simple: install the game, download the full compiled world, and then start playing. The complexity stays behind the launch contract. Android handles identity, delivery, progress, retry, and readiness. Unreal handles the authored world and the compiled runtime assets. Play Asset Delivery keeps those assets outside the bootstrap installation and makes the content updateable as the world grows.

That is the AETHELGRAD strategy: a small doorway, a verified content download, and a complete 3D world only when it is ready.”

---

## Technical references

[1]: https://dev.epicgames.com/documentation/unreal-engine/using-google-play-asset-delivery-in-unreal-engine?lang=en-US "Epic Games — Using Google Play Asset Delivery in Unreal Engine"

[2]: https://developer.android.com/guide/playcore/asset-delivery "Android Developers — Play Asset Delivery"

[3]: https://developer.android.com/guide/app-bundle "Android Developers — About Android App Bundles"

[4]: https://developers.google.com/android-publisher/getting_started "Google Developers — Google Play Developer API Getting Started"

[5]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/docs/UNREAL_COOK_AND_PLAY_ASSET_DELIVERY.md "AETHELGRAD — Unreal cooking and Play Asset Delivery guide"

[6]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/docs/POST_INSTALL_3D_CONTENT_DOWNLOAD.md "AETHELGRAD — Post-install compiled content flow"
