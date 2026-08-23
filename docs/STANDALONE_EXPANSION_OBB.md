# Standalone Aethelgard Expansion OBB

Aethelgard now has a reproducible standalone expansion-file path for private APK distribution, device labs, and legacy Android channels. The builder creates the standard opaque expansion filename:

```text
main.<versionCode>.com.darvirgoyt.aethelgrad.obb
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
adb shell mkdir -p /sdcard/Android/obb/com.darvirgoyt.aethelgrad
adb push main.3.com.darvirgoyt.aethelgrad.obb \
  /sdcard/Android/obb/com.darvirgoyt.aethelgrad/
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
