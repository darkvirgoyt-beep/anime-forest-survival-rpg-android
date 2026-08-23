#!/usr/bin/env bash
set -euo pipefail

APK="${1:?usage: check_16kb_alignment.sh path/to/app.apk}"
ANDROID_HOME="${ANDROID_HOME:?ANDROID_HOME must point to the Android SDK}"
NDK_VERSION="${ANDROID_NDK_VERSION:-28.0.12433566}"
BUILD_TOOLS="${ANDROID_HOME}/build-tools/35.0.0"
OBJDUMP="${ANDROID_HOME}/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump"

[[ -f "$APK" ]] || { echo "APK not found: $APK" >&2; exit 2; }
[[ -x "${BUILD_TOOLS}/zipalign" ]] || { echo "zipalign 35.0.0 not found" >&2; exit 2; }
[[ -x "$OBJDUMP" ]] || { echo "llvm-objdump not found: $OBJDUMP" >&2; exit 2; }

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT
unzip -q "$APK" 'lib/*/*.so' -d "$tmp_dir"
mapfile -t libraries < <(find "$tmp_dir/lib" -type f -name '*.so' -print | sort)
[[ "${#libraries[@]}" -gt 0 ]] || { echo "No native libraries found in $APK" >&2; exit 3; }

for library in "${libraries[@]}"; do
    name="${library#"$tmp_dir/"}"
    echo "Checking ELF alignment: $name"
    loads="$("$OBJDUMP" -p "$library" | awk '$1 == "LOAD" {print $NF}')"
    [[ -n "$loads" ]] || { echo "No LOAD segments found in $name" >&2; exit 4; }
    while IFS= read -r alignment; do
        [[ "$alignment" == "2**14" ]] || {
            echo "UNALIGNED: $name has LOAD alignment $alignment; expected 2**14" >&2
            exit 5
        }
    done <<< "$loads"
done

"${BUILD_TOOLS}/zipalign" -c -P 16 -v 4 "$APK" >/tmp/aethelgard-zipalign.log
cat /tmp/aethelgard-zipalign.log | tail -n 4

echo "16KB_ALIGNMENT=PASS"
