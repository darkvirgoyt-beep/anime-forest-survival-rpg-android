#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${AETHELGARD_BUILD_ROOT:-$ROOT/Build/Android}"
ARCHIVE_ROOT="${AETHELGARD_ARCHIVE_ROOT:-$BUILD_ROOT/Archive}"
COOK_ROOT="${AETHELGARD_COOKED_ROOT:-$ARCHIVE_ROOT/Saved/StagedBuilds/Android/ForestSlice/Content/Paks}"
STAGING_ROOT="${AETHELGARD_STAGING_ROOT:-$BUILD_ROOT/full-content-staging}"
OBB_ROOT="${AETHELGARD_OBB_ROOT:-$BUILD_ROOT/expansion-obb}"
GRADLE_BIN="${GRADLE_BIN:-gradle}"

fail() {
  echo "ERROR: $*" >&2
  exit 1
}

if [[ ! -d "$COOK_ROOT" && "${AETHELGARD_RUN_UAT:-0}" == "1" ]]; then
  [[ -n "${UE_ROOT:-}" ]] || fail "AETHELGARD_RUN_UAT=1 requires UE_ROOT"
  UAT="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
  [[ -x "$UAT" ]] || fail "RunUAT.sh not found or not executable: $UAT"
  mkdir -p "$ARCHIVE_ROOT"
  "$UAT" BuildCookRun \
    -project="$ROOT/Unreal/ForestSlice.uproject" \
    -noP4 -platform=Android -clientconfig=Shipping \
    -build -cook -stage -pak -prereqs -archive \
    -archivedirectory="$ARCHIVE_ROOT" \
    -utf8output
fi

[[ -d "$COOK_ROOT" ]] || fail "trusted cooked Unreal output not found: $COOK_ROOT. Supply AETHELGARD_COOKED_ROOT or run with AETHELGARD_RUN_UAT=1 and UE_ROOT."
find "$COOK_ROOT" -type f -name 'pakchunk*.pak' -print -quit | grep -q . || fail "no cooked pakchunk*.pak files found under $COOK_ROOT"

rm -rf "$STAGING_ROOT" "$OBB_ROOT"
mkdir -p "$STAGING_ROOT"
cp "$ROOT/app/src/main/assets/asset_manifest.json" "$STAGING_ROOT/asset_manifest.json"

python3 "$ROOT/tools/stage_cooked_unreal_assets.py" \
  --cook-root "$COOK_ROOT" \
  --output-dir "$STAGING_ROOT" \
  --mapping-file "$ROOT/tools/unreal_pack_mapping.json" \
  --gradle-root "$ROOT"

python3 "$ROOT/tools/validate_asset_budget.py" \
  --root "$ROOT" \
  --require-nonempty \
  --require-target

"$GRADLE_BIN" clean bundleRelease assembleRelease --stacktrace

if [[ -n "${ANDROID_HOME:-}" && -x "$ANDROID_HOME/build-tools/35.0.0/aapt" ]]; then
  APK_PATH="$ROOT/app/build/outputs/apk/release/app-release.apk"
  AAPT_BIN="$ANDROID_HOME/build-tools/35.0.0/aapt"
  APK_PACKAGE="$($AAPT_BIN dump badging "$APK_PATH" | sed -n "s/^package: name='\([^']*\)'.*/\1/p")"
  APK_VERSION_CODE="$($AAPT_BIN dump badging "$APK_PATH" | sed -n "s/^package:.*versionCode='\([^']*\)'.*/\1/p")"
  [[ "$APK_PACKAGE" == "com.darkvirgoyt.aethelgrad" ]] || fail "unexpected APK package: $APK_PACKAGE"
  [[ "$APK_VERSION_CODE" =~ ^[1-9][0-9]*$ ]] || fail "invalid APK version code: $APK_VERSION_CODE"
  mkdir -p "$OBB_ROOT"
  python3 "$ROOT/tools/build_expansion_obb.py" \
    --input-dir "$STAGING_ROOT" \
    --output-dir "$OBB_ROOT" \
    --package-name "$APK_PACKAGE" \
    --version-code "$APK_VERSION_CODE" \
    --content-version "full-content-${GITHUB_SHA:-local}"
  python3 "$ROOT/tools/verify_expansion_obb.py" \
    "$OBB_ROOT/main.${APK_VERSION_CODE}.${APK_PACKAGE}.obb" \
    --expected-package "$APK_PACKAGE" \
    --expected-version "$APK_VERSION_CODE"
else
  echo "INFO: Android aapt unavailable; skipped APK-derived OBB generation."
fi

printf 'FULL_CONTENT_BUILD_PASS=1\n'
printf 'COOK_ROOT=%s\n' "$COOK_ROOT"
printf 'STAGING_ROOT=%s\n' "$STAGING_ROOT"
printf 'AAB=%s\n' "$ROOT/app/build/outputs/bundle/release/app-release.aab"
printf 'APK=%s\n' "$ROOT/app/build/outputs/apk/release/app-release.apk"
