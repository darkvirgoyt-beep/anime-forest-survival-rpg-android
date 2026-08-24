# Standalone Aethelgard Expansion OBB

Aethelgard now has a reproducible standalone expansion-file path for private APK distribution, device labs, and legacy Android channels. The builder creates the standard opaque expansion filename:

```text
main.<versionCode>.com.darkvirgoyt.aethelgrad.obb
```

The OBB is a ZIP container with a checked `obb_manifest.json` plus a `content/` tree containing only staged runtime files. Every content file is recorded with its byte size and SHA-256 digest. The builder rejects an empty input directory, symlinks, unsafe paths, and fake size padding.

## Build from real cooked content

Stage the output of the Unreal Android cook first. The input directory should contain the real cooked `.pak` files, compiled shader libraries, pipeline cache data, platform-qualified textures, world-sector metadata, audio, VFX, and other runtime files. Do not put `.uasset`, `.umap`, editor-only files, or a zero-filled size placeholder in this directory.

```bash
APK_PATH=app/build/outputs/apk/release/app-release.apk
AAPT_BIN="$ANDROID_HOME/build-tools/35.0.0/aapt"
APK_PACKAGE="$($AAPT_BIN dump badging "$APK_PATH" | sed -n "s/^package: name='\([^']*\)'.*/\1/p")"
APK_VERSION_CODE="$($AAPT_BIN dump badging "$APK_PATH" | sed -n "s/^package:.*versionCode='\([^']*\)'.*/\1/p")"

python3 tools/build_expansion_obb.py \
  --input-dir Build/Android/expansion-staging \
  --output-dir Build/Android/obb \
  --package-name "$APK_PACKAGE" \
  --version-code "$APK_VERSION_CODE" \
  --content-version unreal-cook-2026-08-23

OBB_PATH="Build/Android/obb/main.${APK_VERSION_CODE}.${APK_PACKAGE}.obb"
python3 tools/verify_expansion_obb.py "$OBB_PATH" \
  --expected-package "$APK_PACKAGE" \
  --expected-version "$APK_VERSION_CODE"
```

The command prints the content file count, uncompressed payload bytes, archive bytes, and archive SHA-256. The verifier now checks the OBB package and version against the APK-derived values. The OBB is not padded to a target size. Therefore, the current repository’s development OBB remains small because the checked-in project does not yet contain the real 4.2 GB/6.6 GB Unreal cooked payload.

## Private APK/device-lab installation

For a device-lab test, install the matching APK and copy the OBB to Android’s package-specific expansion directory:

```bash
adb install app-release.apk
adb shell mkdir -p /sdcard/Android/obb/com.darkvirgoyt.aethelgrad
adb push main.3.com.darkvirgoyt.aethelgrad.obb \
  /sdcard/Android/obb/com.darkvirgoyt.aethelgrad/
```

Android treats the OBB as opaque data. The game or its launcher must explicitly locate and mount/read the package; copying an OBB beside an APK does not make Play Asset Delivery packs appear automatically. The Aethelgard production resource center remains the authoritative runtime path for the AAB/PAD build.

## Google Play release rule

For Google Play, use the signed Android App Bundle and Play Asset Delivery. Play Asset Delivery is the modern replacement for legacy expansion files for large games, and it supports install-time, fast-follow, and on-demand asset packs. The repository’s 18 Play modules therefore remain the primary release path; the OBB is an additional standalone artifact for private APK channels and compatibility testing.

| Delivery channel | Artifact | Runtime behavior |
|---|---|---|
| Google Play/internal testing | Signed `app-release.aab` | Play Asset Delivery downloads the selected Low or High resource tier. |
| Private APK/device lab | `app-release.apk` plus `main.<version>.<package>.obb` | The expansion file must be copied/downloaded separately and explicitly consumed by the app or test harness. |
| Current prototype | Small development APK/OBB | Contains only checked-in prototype/reference content; it is not the final authored world. |

## References

[1]: https://developer.android.com/guide/playcore/asset-delivery "Android Developers: Play Asset Delivery"

[2]: https://developer.android.com/google/play/expansion-files "Android Developers: APK Expansion Files"

## Automated staging from Unreal cooked output

The repository now includes `tools/stage_cooked_unreal_assets.py`. Run it only after `RunUAT BuildCookRun` has produced shipping Android runtime files. The default mapping expects `pakchunk0` through `pakchunk17` and maps them to the 18 repository pack names in `tools/unreal_pack_mapping.json`. If Primary Asset Labels produce a different chunk layout, edit that mapping to match the cook log before staging; do not silently place a chunk in the wrong tier.

```bash
COOK_ROOT=Build/Android/Archive/Saved/StagedBuilds/Android/ForestSlice/Content/Paks
rm -rf Build/Android/expansion-staging Build/Android/obb
python3 tools/stage_cooked_unreal_assets.py \
  --cook-root "$COOK_ROOT" \
  --output-dir Build/Android/expansion-staging \
  --mapping-file tools/unreal_pack_mapping.json

AAPT_BIN="$ANDROID_HOME/build-tools/35.0.0/aapt"
APK_PATH=app/build/outputs/apk/release/app-release.apk
APK_PACKAGE="$($AAPT_BIN dump badging "$APK_PATH" | sed -n "s/^package: name='\\([^']*\\)'.*/\\1/p")"
APK_VERSION_CODE="$($AAPT_BIN dump badging "$APK_PATH" | sed -n "s/^package:.*versionCode='\\([^']*\\)'.*/\\1/p")"
python3 tools/build_expansion_obb.py \
  --input-dir Build/Android/expansion-staging \
  --output-dir Build/Android/obb \
  --package-name "$APK_PACKAGE" \
  --version-code "$APK_VERSION_CODE" \
  --content-version "unreal-cook-${GITHUB_SHA:-local}"

python3 tools/verify_expansion_obb.py \
  "Build/Android/obb/main.${APK_VERSION_CODE}.${APK_PACKAGE}.obb" \
  --expected-package "$APK_PACKAGE" \
  --expected-version "$APK_VERSION_CODE"
```

The staging output has the shape `asset_packs/<pack-name>/<relative-cooked-file>`. The OBB builder then records every file under `content/`, including its byte count and SHA-256. The script rejects symlinks, editor-only `.uasset`/`.umap`/`.uexp`/`.ubulk` files, duplicate pack assignments, missing chunk matches, and empty input. It does not manufacture bytes to reach 6–7 GB; the final OBB size is the compressed result of the real cooked payload.

The same cooked chunks must also be copied into the corresponding Gradle asset-pack modules for the production AAB. The OBB is only the private APK/device-lab compatibility path. For Google Play, the authoritative artifact remains the signed AAB with the 18 Play Asset Delivery modules, and the runtime must request and mount the selected Low or High tier through Play Asset Delivery.

A normal CI checkout does not contain the real Unreal cooked output, so the current workflow intentionally continues to build a small prototype OBB from checked-in reference files. To switch a release job to real content, provide a trusted cook artifact or run the Unreal cook in a separate build job, verify its SHA-256 manifest, run the staging script, and then invoke the existing APK-derived OBB build step. Never commit the 6–7 GB cooked files to the source repository.
