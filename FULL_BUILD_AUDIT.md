# Aethelgard Full-Build Audit

## Executive finding

The 30 MB result is expected from the current checkout because it is the base Android bootstrap client. The repository contains the Android/Kotlin/C++ source, eighteen Gradle asset-pack declarations, Unreal C++ source, and planning manifests, but it does not contain the real cooked Unreal runtime payload that would make those packs large. The asset-pack `src/main/assets` directories contain only `.gitkeep` markers, while `assets/runtime_content/payload` is also empty.

A direct APK is not the complete Play Asset Delivery product. For a real large build, the signed AAB remains the authoritative artifact and the downloaded asset-pack splits are delivered separately. A direct APK can therefore stay close to the base-client size even after the AAB has real asset packs. The local-testing `.apks` set or a Play internal-test installation is required to exercise the full pack delivery path.

## Root causes found

| Finding | Evidence | Effect |
|---|---|---|
| Asset-pack modules are empty | Every `assetpack_*/src/main/assets` directory contains only `.gitkeep` | Gradle has pack declarations but no payload bytes to package |
| Cooked Unreal output is absent | No trusted `pakchunk*.pak` tree exists in the checkout | The OBB staging job intentionally skips the large archive |
| Startup content was optional by default | `ContentDownloadPlan` used an empty `requiredBeforeStart` subset; the manifest declared `requiredBeforeStart: false` and `automaticExpansion: false` | The bundled renderer was marked ready without requesting real content |
| The private downloader is transport-only | `PrivateContentDownloader` verifies and copies an existing OBB | It cannot create missing world assets |
| Size targets are planning values | `asset_budget.json` and runtime manifests describe target MiB but do not manufacture files | A target such as 1 GiB cannot be achieved by rebuilding source alone |

## Changes made

The Android build now exposes an explicit `-PfullContent=true` mode. In this mode, the runtime selects all eighteen packs, requests them as a complete set, rejects an empty readiness set, and keeps world entry locked when real content is unavailable, canceled, or waiting for confirmation. The default build remains a small bootstrap so development can continue without the external cook.

The new executable `tools/build_full_content.sh` is the complete rebuild rule. It can run a real Unreal Android cook when `AETHELGARD_RUN_UAT=1` and `UE_ROOT` are provided, or consume an existing trusted cook through `AETHELGARD_COOKED_ROOT`. It stages every mapped `pakchunk0`–`pakchunk17`, copies each pack into its Gradle module, rejects empty or undersized authored packs, runs `clean bundleRelease assembleRelease -PfullContent=true`, and creates/verifies the matching OBB when Android `aapt` is available.

The staging tool now accepts `--gradle-root`, allowing the same cooked files to populate the AAB asset-pack modules and the private OBB staging tree. CI also runs `tools/test_full_content_build_contract.py`, which protects the full-content selection, fail-closed startup, staging, and validation rules from regression.

## Correct production procedure

First create or obtain a trusted shipping Unreal Android cook. The cook must contain original or properly licensed runtime files such as `.pak`, `.ucas`, `.utoc`, mobile shader libraries, pipeline caches, platform-qualified textures, audio, VFX, animation, navigation, and world-sector data. Then run:

```bash
AETHELGARD_COOKED_ROOT=/absolute/path/to/Saved/StagedBuilds/Android/ForestSlice/Content/Paks \\
  ./tools/build_full_content.sh
```

Alternatively, on a machine with Unreal Engine installed:

```bash
AETHELGARD_RUN_UAT=1 UE_ROOT=/absolute/path/to/Unreal \\
  ./tools/build_full_content.sh
```

For Google Play, test the resulting AAB through Play internal testing or the generated bundletool local-testing `.apks` set. Do not judge completeness by the direct base APK alone. For private APK/device-lab distribution, use the matching APK together with the generated `main.<version>.<package>.obb` and the app’s verified OBB delivery path.

## Current sandbox result

The strict full-content script correctly stops because no trusted cooked Unreal output is present. This is intentional and safer than creating a padded archive or claiming that the procedural renderer is the finished authored 3D world. Syntax checks, all repository contracts, the full-content contract, and native C++ regression tests pass. Android APK/AAB assembly cannot run in this sandbox because Android SDK/NDK, Gradle wrapper, Unreal Engine, and device tooling are unavailable.

## References

[1]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android "Aethelgard Android repository"
[2]: https://developer.android.com/guide/playcore/asset-delivery "Android Developers: Play Asset Delivery"
[3]: https://dev.epicgames.com/documentation/unreal-engine/using-google-play-asset-delivery-in-unreal-engine?lang=en-US "Epic Games: Google Play Asset Delivery in Unreal Engine"
