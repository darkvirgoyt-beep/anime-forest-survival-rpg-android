# Cooking and packaging real Unreal assets for Play Asset Delivery

Use this procedure when the project has a real Unreal content project with `.uasset` and `.umap` files. The checked-in Aethelgard prototype has source and empty pack folders but no authored Unreal content, so do not expect its current CI build to create a multi-gigabyte world.

## 1. Prepare the content project

Open the actual Unreal Engine project in the project’s pinned engine version. Enable the **GooglePAD** plugin. In **Project Settings → Packaging**, enable **Generate Chunks**. In **Platforms → Android**, select **Android App Bundle** as the packaging format and configure the Android SDK/NDK, signing, package name, minimum SDK, texture formats, and shipping configuration.

Keep the bootstrap small. Put the account shell, settings, loading map, core shaders, save schema, and the minimum playable data in chunk 0. Put the forest launch region, characters, textures, VFX, audio, cinematics, and optional biomes into later chunks using Primary Asset Labels or Asset Manager rules. Use names that are stable across versions, for example `pakchunk1_forest`, `pakchunk2_characters`, and `pakchunk3_textures_mobile_high`.

> Do not treat a target size as a reason to add filler files. The 6.6 GB target must be real cooked content: original meshes, textures, animation clips, audio, world data, and shader/LOD variants.

## 2. Split content within Play limits

Epic’s Unreal GooglePAD reference documents one install-time pack up to 1 GB, one fast-follow pack up to 512 MB, on-demand packs up to 512 MB each, and up to 50 packs per app. Google’s current Play documentation also says fast-follow and on-demand packs are downloaded outside the base install and must be accessed through the Play Asset Delivery API. Verify the current Play Console limits before release because platform limits can change.

The repository’s 6,750 MiB post-install envelope is split into 18 physical packs. Every dynamic pack is below 512 MiB, with one install-time core pack and one fast-follow launch-region pack. The exact names must stay synchronized across `ContentDownloadPlan`, Gradle, the manifest, and the runtime catalog:

| Pack family | Exact pack names | Delivery | Target |
|---|---|---|---:|
| Bootstrap | `assetpack_core` | Install-time | 358 MiB |
| Compiled base and world | `assetpack_graphics_base`, `assetpack_world_streaming` | On-demand | 850 MiB |
| Launch and biomes | `assetpack_forest`, `assetpack_sand`, `assetpack_snow`, `assetpack_dungeons` | Fast-follow + on-demand | 1,550 MiB |
| Characters and animation | `assetpack_characters`, `assetpack_animation_sets` | On-demand | 925 MiB |
| Shaders and pipeline | `assetpack_shaders_vulkan`, `assetpack_shaders_gles`, `assetpack_pipeline_cache` | On-demand | 650 MiB |
| Textures and LODs | `assetpack_hd_textures`, `assetpack_foliage_lods`, `assetpack_terrain_lod` | On-demand | 1,325 MiB |
| Presentation | `assetpack_audio_hd`, `assetpack_cinematics`, `assetpack_vfx`, `assetpack_voice` | On-demand | 1,450 MiB |

Choose final byte sizes from the cooked output. Keep safety headroom below the platform limit and let `tools/validate_asset_budget.py` reject oversized dynamic packs or a second fast-follow pack.

## 3. Cook and stage Unreal chunks

From the Unreal installation, run `RunUAT` with the real project file. The exact executable path differs by operating system. A shipping Android cook typically follows this shape:

```bash
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$PWD/Unreal/ForestSlice.uproject" \
  -noP4 -platform=Android -clientconfig=Shipping \
  -build -cook -stage -pak -prereqs -archive \
  -archivedirectory="$PWD/Build/Android/Archive" \
  -utf8output
```

On Windows, use `RunUAT.bat` and a Windows path. Confirm the cook log contains the intended chunks and that the staged output contains `.pak` files under `Saved/StagedBuilds/Android/ForestSlice/Content/Paks` or the equivalent archive directory. Record each file’s byte size and SHA-256.

The important rule is that the Unreal cook produces the runtime `.pak` files. Android should not try to compile Unreal content on the device.

## 4. Put external chunks into asset-pack modules

For fast-follow and on-demand packs, copy the selected `.pak` files into the Gradle asset-pack module’s `src/main/assets` directory. Keep the pack names unique and use the same names that the runtime requests. A module has this shape:

```text
assetpack_forest/
├── build.gradle.kts
└── src/main/assets/
    ├── pakchunk1_forest.pak
    └── pack_manifest.json
```

The module declaration is:

```kotlin
plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_forest")
    dynamicDelivery {
        deliveryType.set("fast-follow")
    }
}
```

Use `on-demand` for optional modules. Keep install-time content minimal because Android needs additional free space during installation and install-time content is present before the game can start.

For projects using Unreal’s generated Gradle layout, follow Epic’s required directories instead:

```text
Build/Android/gradle/assetpacks/fast-follow/<pack-name>/src/main/assets/
Build/Android/gradle/assetpacks/on-demand/<pack-name>/src/main/assets/
```

Do not mix the generated Unreal layout and the custom Android modules without checking the final AAB’s asset-pack table. Have one source of truth for each pack.

## 5. Configure runtime access

At startup, check whether the required pack is already available. If not, request it through GooglePAD/Play Asset Delivery and monitor status, bytes downloaded, total bytes, waiting-for-Wi-Fi, insufficient-storage, cancellation, and failure states. Query the pack location after completion; do not hardcode an internal path because Play may move or invalidate packs during updates.

The resource center should show the actual total bytes reported by Play, not only the planned 6.6 GB label. It should allow retry, explain that the Play Store AAB is required, and avoid entering a map that references missing assets. On updates, handle the short period when an old pack has been invalidated but the replacement is not ready.

## 6. Build the AAB

Use the project’s Gradle build or Unreal’s Android packaging command to create a **signed AAB**, not only a universal APK. For the Aethelgard Android bridge, the project build command is conceptually:

```bash
gradle bundleRelease
```

For the Unreal path, package the Android App Bundle from the Unreal Editor or the corresponding UAT command after the chunks and asset-pack staging are complete. Inspect the output with `bundletool` or the Play Console internal app-sharing flow. Verify that the base module, install-time pack, fast-follow pack, and on-demand packs are present and that no external `.pak` accidentally landed in the base APK.

A direct APK is useful for local UI smoke tests but cannot reproduce the full Google Play Asset Delivery experience. Use an internal-test track or Play Console app sharing to test real pack delivery on a physical device.

## 7. Release checklist

Before upload, verify the application ID, signing certificate, version code, target SDK, package names, pack names, delivery modes, cooked platform, texture compression variants, and the byte size of every pack. Test a clean install, an update with cached packs, a retry after network loss, insufficient storage, Wi-Fi-only behavior, and a device with a lower graphics tier.

> Sources: [Epic Games — Google Play Asset Delivery Reference](https://dev.epicgames.com/documentation/unreal-engine/using-google-play-asset-delivery-in-unreal-engine?lang=en-US); [Android Developers — Play Asset Delivery](https://developer.android.com/guide/playcore/asset-delivery); [Android Developers — About Android App Bundles](https://developer.android.com/guide/app-bundle).
