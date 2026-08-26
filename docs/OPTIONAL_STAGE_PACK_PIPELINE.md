# Adding a New Optional Stage 1 Pack

This guide shows the safe end-to-end path for adding a **real optional Stage 1 pack**. It uses the example name `assetpack_camp_cosmetics`. Replace that name only if the final pack purpose is different.

> **Do not create a large empty pack or add filler files.** A pack becomes published only after it contains original or licensed cooked runtime files, its byte count is measured, its source/license record exists, and all validation passes.

## 1. Decide whether the pack is truly optional

Use an optional Stage 1 pack only for content that can download after the forest world has opened. Good candidates are extra authored ambience, cosmetic camp props, optional foliage variants, or non-critical UI/audio improvements. Do not put login, authentication, collision, navigation required for the launch forest, or the player’s required startup assets into this pack.

| Decision | Required value |
|---|---|
| Delivery mode | `on-demand` |
| Startup gate | `requiredBeforeStart: false` |
| Stage | `stage-1` only; not the unpublished high tier |
| Dynamic-pack budget ceiling | At most **512 MiB** |
| Publication | `false` until real cooked files have been staged and measured |

The existing Stage 1 optional pattern is `assetpack_audio_hd` and `assetpack_foliage_lods`. After the world opens, Android can request only packs whose manifest is both `published: true` and has `measuredBytes > 0`. [1]

## 2. Create the Android asset-pack module

Create this directory structure:

```text
assetpack_camp_cosmetics/
├── build.gradle.kts
├── CAMP_COSMETICS_PACK_STATE.json
└── src/main/assets/.gitkeep
```

Use this Gradle module file:

```kotlin
plugins {
    id("com.android.asset-pack")
}

assetPack {
    packName.set("assetpack_camp_cosmetics")
    dynamicDelivery {
        deliveryType.set("on-demand")
    }
}
```

Then register `:assetpack_camp_cosmetics` in both places:

1. `settings.gradle.kts` with `include(":assetpack_camp_cosmetics")`.
2. `app/build.gradle.kts` inside the `assetPacks += listOf(...)` declaration.

Do not put the state JSON inside `src/main/assets`; packaged state metadata changes the measured payload and can cause the budget contract to fail.

## 3. Define the Unreal cook chunk

On an Unreal Engine 5.6+ desktop, create a Primary Asset Label for the owned camp-cosmetic assets. Use an explicit chunk ID that is not already in `tools/unreal_pack_mapping.json`; for this example, use **Chunk 18**.

The label should include only final runtime assets such as cooked static meshes, material instances, textures, sound cues, and collision data. It must not include editor-only source assets, raw work files, or copied third-party game content. In the project’s packaging/chunk setup, make that label cook into `pakchunk18`.

Run a real Android cook from the Unreal-capable desktop. An illustrative command is:

```bash
"<UE_ROOT>/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun \
  -project="<REPO>/Unreal/ForestSlice.uproject" \
  -platform=Android -clientconfig=Shipping \
  -build -cook -stage -pak -iostore -archive
```

The exact archive location depends on the desktop’s Unreal installation. The required result is a trusted cooked output directory containing the `pakchunk18` runtime files and their required sidecars, such as `.pak`, `.utoc`, `.ucas`, and `.sig` where applicable.

## 4. Map and stage only cooked runtime output

Add the new pack mapping to `tools/unreal_pack_mapping.json`:

```json
"assetpack_camp_cosmetics": [
  "**/pakchunk18",
  "**/pakchunk18[!0-9]*"
]
```

For a focused first staging run, create a one-pack mapping file rather than using the full mapping before every existing chunk is available:

```json
{
  "assetpack_camp_cosmetics": [
    "**/pakchunk18",
    "**/pakchunk18[!0-9]*"
  ]
}
```

Then stage and synchronize the real cooked files:

```bash
python3 tools/stage_cooked_unreal_assets.py \
  --cook-root "<COOKED_ANDROID_OUTPUT>" \
  --output-dir build/camp-cosmetics-stage \
  --mapping-file tools/camp_cosmetics_mapping.json \
  --gradle-root .
```

The staging tool rejects an empty mapping, an empty matched pack, duplicate file assignment, symlinks, editor-only extensions, and unsupported source files. It copies only already-cooked runtime files into `assetpack_camp_cosmetics/src/main/assets/`. [2]

## 5. Record source, license, and exact measured bytes

Before publishing, add source/license records to `Unreal/ASSETS.md` for every authored binary asset. Record the creator, license, import settings, LOD policy, compression, target memory, and replacement plan.

Calculate the actual byte count from the staged pack. Do not estimate it from the target MiB. Update `CAMP_COSMETICS_PACK_STATE.json` with the exact number and publish state only after staging succeeds:

```json
{
  "packName": "assetpack_camp_cosmetics",
  "delivery": "on-demand",
  "published": true,
  "measuredPayloadBytes": 12345678,
  "plannedBudgetMiB": 96,
  "content": "Original cooked optional forest camp cosmetics",
  "verification": {
    "assetBudgetReportsExactBytes": true,
    "sourceAndLicenseReceiptsRequired": true
  }
}
```

The number above is an example only. Replace `12345678` with the actual staged byte count. A 96 MiB planning target is also only an example; the final target must be at or below the 512 MiB dynamic-pack ceiling.

## 6. Add the runtime declarations

Update these synchronized records:

| File | Required change |
|---|---|
| `assets/asset_budget.json` | Add the Stage 1 budget entry with `delivery: on-demand` and the declared target MiB. |
| `assets/full_content_budget.json` | Add the corresponding full-content plan entry; keep it at or below 512 MiB. |
| `app/src/main/assets/asset_manifest.json` | Add `name`, `delivery`, `targetMiB`, exact `measuredBytes`, `published: true`, `requiredBeforeStart: false`, and an accurate `contents` description. Add the name to `contentDelivery.launchSlice.authoredOnDemandPacks` and the Stage 1 resource-tier pack list. |
| `ContentDownloadPlan.kt` | Add the pack to `stageOnePacks` with `requiredBeforeStart = false`. The automatic Stage 1 expansion request will then consider it after world entry. |
| `tools/test_resource_center.py` | Add the root state-file mapping and exact-state assertion for the new module. |
| `tools/test_progressive_content_contract.py` | Extend assertions if this pack uses a progression trigger. |

If the optional pack is tied to a world discovery event rather than a safe background enhancement, also add an `expansionTrigger` field to the runtime manifest and assign the pack to a `WorldSector` in `ContentDownloadPlan.kt`. Never attach a pack to a sector until that sector’s playable content is actually cooked and published.

## 7. Validate locally before Android packaging

Run the following checks from the repository root:

```bash
python3 tools/validate_asset_budget.py
python3 tools/validate_asset_budget.py --manifest assets/full_content_budget.json
python3 tools/test_resource_center.py --repo . --unreal-project Unreal/ForestSlice.uproject
python3 tools/test_progressive_content_contract.py
python3 tools/test_launch_slice_content.py
python3 tools/test_full_content_build_contract.py
git diff --check
```

For a real complete cook, use `tools/build_full_content.sh` with the trusted cooked output or a configured Unreal automation path. That pipeline stages cooked files, enforces non-empty targeted payloads, builds Android artifacts, and produces/verifies the matching OBB compatibility artifact. [3]

## 8. Verify Play delivery and the OBB fallback

Publish the AAB to a Play internal-test track. Install it from Play—not from a raw GitHub APK—then open the forest world on a test device. The game should request the new optional pack only after world entry, show exact bytes only once Play reports them, preserve playability if the user delays the download, and display the local size increase only when the pack has mounted.

For private APK/device-lab use, build the non-empty verified OBB only after real cooked files exist. The private downloader requires HTTPS, a manifest with exact archive size and SHA-256, package/version matching, resumable download support, and atomic mounting after verification. A raw APK cannot silently place an OBB into Android’s protected expansion directory. [4]

## 9. Release gate

The pack is ready to mark `published: true` only when all conditions below are true:

1. The source assets are original or licensed and listed in `Unreal/ASSETS.md`.
2. Unreal has produced real Android-cooked runtime output.
3. The staged asset-pack directory contains real non-empty runtime files.
4. The state record and runtime manifest report exact measured bytes.
5. Every budget, resource-center, progressive-delivery, Android build, and OBB verification check passes.
6. A real Android device has tested download, pause/resume, low storage, restart, and uninstall/reinstall behavior.

## References

[1] [Stage 1 pack selection and automatic measured expansion](../app/src/main/java/com/darkvirgoyt/aethelgrad/ContentDownloadPlan.kt)

[2] [Cooked Unreal asset staging tool](../tools/stage_cooked_unreal_assets.py)

[3] [Full-content build and packaging path](../tools/build_full_content.sh)

[4] [Automatic content expansion delivery boundary](AUTOMATIC_CONTENT_EXPANSION.md)
