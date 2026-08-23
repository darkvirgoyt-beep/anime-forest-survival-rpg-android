# Cooking and packaging real Unreal assets for Play Asset Delivery

Use this procedure when the project has a real Unreal content project with `.uasset` and `.umap` files. The checked-in Aethelgard prototype has source and empty pack folders but no authored Unreal content, so do not expect its current CI build to create a multi-gigabyte world.

## 1. Prepare the content project

Open the actual Unreal Engine project in the project’s pinned engine version. Enable the **GooglePAD** plugin. In **Project Settings → Packaging**, enable **Generate Chunks**. In **Platforms → Android**, select **Android App Bundle** as the packaging format and configure the Android SDK/NDK, signing, package name, minimum SDK, texture formats, and shipping configuration.

Keep the bootstrap small. Put the account shell, settings, loading map, core shaders, save schema, and the minimum playable data in chunk 0. Put the forest launch region, characters, textures, VFX, audio, cinematics, and optional biomes into later chunks using Primary Asset Labels or Asset Manager rules. Use names that are stable across versions, for example `pakchunk1_forest`, `pakchunk2_characters`, and `pakchunk3_textures_mobile_high`.

> Do not treat a target size as a reason to add filler files. The 6.6 GB target must be real cooked content: original meshes, textures, animation clips, audio, world data, and shader/LOD variants.

## 2. Split content within Play limits

Epic’s Unreal GooglePAD reference documents one install-time pack up to 1 GB, one fast-follow pack up to 512 MB, on-demand packs up to 512 MB each, and up to 50 packs per app. Google’s current Play documentation also says fast-follow and on-demand packs are downloaded outside the base install and must be accessed through the Play Asset Delivery API. Verify the current Play Console limits before release because platform limits can change.

A 6,750 MiB envelope should therefore be split into at least fourteen packs if each stays below 512 MiB. A practical layout is one install-time core pack, one fast-follow launch-region pack, and fourteen or more on-demand packs. Split by region and platform quality rather than making one oversized pack:

| Pack family | Example packs | Delivery | Typical purpose |
|---|---|---|---|
| Bootstrap | `assetpack_core` | Install-time | Account shell, loading scene, core runtime data |
| First playable region | `assetpack_forest` | Fast-follow | Forest terrain, launch village, collision, navigation |
| Regions | `assetpack_sand_01`, `assetpack_sand_02`, `assetpack_snow_01`, `assetpack_dungeons_01` | On-demand | Separate biome cells and dungeon cells |
| Characters | `assetpack_characters_01`, `assetpack_characters_02` | On-demand | Skeletal meshes, materials, skins, rigs, animation sets |
| Graphics tiers | `assetpack_textures_high_01`, `assetpack_textures_ultra_01`, `assetpack_shaders_vulkan`, `assetpack_shaders_gles` | On-demand | Device-qualified textures, shaders, PSO/cache data |
| Presentation | `assetpack_vfx_01`, `assetpack_audio_01`, `assetpack_voice_01`, `assetpack_cinematics_01` | On-demand | Effects, music, voices, cinematics and localization |

Choose the final number from the cooked byte sizes. Every pack should be comfortably below the limit, not exactly at it, to leave room for metadata and future patches.

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
