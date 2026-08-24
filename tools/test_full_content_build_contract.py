"""Validate that the repository exposes a strict, non-padded full-content build path."""
from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL full_content_build_contract: {message}")


def main() -> None:
    app_gradle = (ROOT / "app/build.gradle.kts").read_text(encoding="utf-8")
    plan = (ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/ContentDownloadPlan.kt").read_text(encoding="utf-8")
    catalog = (ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/AssetPackCatalog.kt").read_text(encoding="utf-8")
    activity = (ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt").read_text(encoding="utf-8")
    staging = (ROOT / "tools/stage_cooked_unreal_assets.py").read_text(encoding="utf-8")
    build_script_path = ROOT / "tools/build_full_content.sh"
    build_script = build_script_path.read_text(encoding="utf-8")
    validator = (ROOT / "tools/validate_asset_budget.py").read_text(encoding="utf-8")
    manifest = json.loads((ROOT / "app/src/main/assets/asset_manifest.json").read_text(encoding="utf-8"))

    packs = manifest["resourceTiers"][0]["packs"]
    require(len(packs) == 18, "high tier must declare all 18 physical packs")
    require("assetPacks += listOf" in app_gradle and "assetpack_animation_sets" in app_gradle, "Gradle must include the complete physical pack set")
    require('requiredBeforeStart: Boolean = true' in plan and 'requiredPackNames' in plan, "content plan must require the complete pack set")
    require("expectedPacks.isNotEmpty()" in catalog and "expectedPacks.all(::isReady)" in catalog, "runtime readiness must not succeed on an empty pack list")
    require("GAME LOCKED" in activity and "HIGH-END CONTENT" in activity, "startup must lock entry on unavailable content")
    require("sync_gradle_asset_packs" in staging and "--gradle-root" in staging, "staging must populate Gradle asset-pack modules")
    require(build_script_path.is_file() and build_script_path.stat().st_mode & 0o111, "full-content build script must be executable")
    require("--require-nonempty" in validator and "--require-target" in build_script, "full builds must reject empty or undersized packs")
    print("FULL_CONTENT_BUILD_CONTRACT_PASS=1")
    print(f"FULL_CONTENT_PACK_COUNT={len(packs)}")


if __name__ == "__main__":
    main()
