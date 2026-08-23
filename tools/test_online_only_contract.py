"""Validate that the Android release has one production online launch path."""
from __future__ import annotations

from pathlib import Path


REQUIRED = (
    ("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", "beginOnlineStartup", "automatic online startup"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", "requestGuestSignIn", "guest session request"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", "requestProductionContent", "production content request"),
    ("app/build.gradle.kts", "release {", "release build contract"),
    (".github/workflows/android-build.yml", "gradle bundleRelease assembleRelease", "release CI build"),
    ("tools/build_expansion_obb.py", "production-v1", "production content version"),
)

FORBIDDEN = (
    ("app/build.gradle.kts", 'create("prototype")', "prototype build type"),
    ("app/build.gradle.kts", "PROTOTYPE_MODE", "prototype build flag"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", "PROTOTYPE_MODE", "prototype runtime flag"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", "AssetDeliveryManager", "local asset fallback"),
    ("app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt", "showLocalPreparationFallback", "local preparation fallback"),
    (".github/workflows/android-build.yml", "assemblePrototype", "prototype CI build"),
    (".github/workflows/android-build.yml", "aethelgard-prototype-apk", "prototype CI artifact"),
    ("tools/build_expansion_obb.py", "prototype-v1", "prototype content version"),
)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    failures: list[str] = []
    for relative, needle, label in REQUIRED:
        path = root / relative
        if not path.is_file():
            failures.append(f"missing file for {label}: {relative}")
        elif needle not in path.read_text(errors="replace"):
            failures.append(f"missing symbol for {label}: {needle} in {relative}")
    for relative, needle, label in FORBIDDEN:
        path = root / relative
        if path.is_file() and needle in path.read_text(errors="replace"):
            failures.append(f"forbidden {label}: {needle} in {relative}")
    if failures:
        for failure in failures:
            print(f"FAIL online_only_contract: {failure}")
        raise SystemExit(1)
    print(f"ONLINE_ONLY_CONTRACT_PASS={len(REQUIRED) + len(FORBIDDEN)}")


if __name__ == "__main__":
    main()

