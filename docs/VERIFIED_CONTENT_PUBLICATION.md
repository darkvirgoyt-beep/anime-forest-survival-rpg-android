# Verified high-graphics content publication

## Current repository status

The Android screenshot’s `HTTP 503 {"error":"private_content_not_configured"}` is accurate: the private-content route has no configured manifest or archive. The repository inventory at this revision contains only a 1-byte `.gitkeep` under `assets/runtime_content/payload`, **zero** cooked Unreal `.pak`, `.utoc`, `.ucas`, or `.obb` files, **zero** Unreal `.uasset`, `.umap`, `.fbx`, `.glb`, or `.gltf` source assets, an empty runtime receipt ledger, and 3,166 bytes in declared Android asset-pack scaffolds.

> Therefore this repository contains Unreal C++ production source and packaging plans, not a downloadable Unreal map, model set, or high-graphics archive. The Android client must not represent the planned 6,750 MiB budget as an installed size, available download, or progress denominator.

## Android behavior now

The app now keeps high-graphics publication disabled by default. It does not send a request to an unconfigured archive endpoint, does not display a synthetic 6.6 GB total, and does not advance the bar from 0% until either Play Asset Delivery or a signed private manifest reports an exact byte total. Progress interpolates smoothly **toward** real transfer events only; the app never invents bytes or advances independently of the provider.

| Delivery state | Size shown | Progress shown | World entry behavior |
|---|---|---|---|
| No cooked release published | No size; explicit not-published explanation | 0% | Core online world remains available; high-graphics mode stays unavailable |
| Signed private manifest available | Exact `archiveBytes` from manifest | Actual downloaded bytes divided by the same signed total | Mount only after byte count, SHA-256, package name, and version code match |
| Play Asset Delivery reports every total | Sum of Play-reported pack bytes | Actual downloaded bytes divided by the same provider totals | Mark ready only after all packs are mounted and measured payload bytes are nonzero |
| Integrity or network failure | Only a provider-reported total, if one exists | Stops at last verified value, then failure state | Do not enable high graphics; offer retry or core world when appropriate |

## Required publication sequence

1. Author or license original maps, 3D models, rigs, textures, animation, audio, navigation, and shader assets. Record each item in `Unreal/ASSETS.md` with source, creator, license, import settings, LOD, Android budget, and replacement plan.
2. Open `Unreal/ForestSlice.uproject` in Unreal Engine 5.6+ and produce a shipping Android cook. Editor assets such as `.uasset` and `.umap` are not Android delivery payloads; stage only cooked runtime files such as `.pak`, `.utoc`, and `.ucas`.
3. Run `tools/stage_cooked_unreal_assets.py` against that real cook. The tool rejects editor-only inputs and reports measured staged bytes.
4. Build an OBB from the staged output and run `tools/build_private_content_manifest.py <real-obb> <manifest.json> --archive-url <https-url>`. This records the actual archive bytes and SHA-256.
5. Publish that exact archive and manifest to the chosen HTTPS host, configure the backend only with absolute paths/URLs for the published files, then set the Android `published_high_end_content` resource to `true` and add the matching HTTPS endpoints.
6. Build a new AAB/APK, validate the exact package/version match, download on a physical device, verify 0–100% provider bytes, SHA-256, mounted size, renderer compatibility, thermals, and frame pacing. Do not enable high-graphics claims before this succeeds.

The present environment does not have Unreal Engine, cooked source assets, or a physical Android graphics test runner. This document is a truthful delivery boundary, not evidence of a complete high-end asset release.
