#!/usr/bin/env python3
"""Validate the high-end four-player private content contract."""
from __future__ import annotations

import json
from pathlib import Path


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL graphics_tier_contract: {message}")


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    manifest_text = (root / "app/src/main/assets/asset_manifest.json").read_text()
    manifest = json.loads(manifest_text)
    profiles = json.loads((root / "assets/graphics_profiles.json").read_text())["profiles"]
    content_plan = (root / "app/src/main/java/com/darkvirgoyt/aethelgrad/ContentDownloadPlan.kt").read_text()
    activity = (root / "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt").read_text()
    catalog = (root / "app/src/main/java/com/darkvirgoyt/aethelgrad/AssetPackCatalog.kt").read_text()
    downloader = (root / "app/src/main/java/com/darkvirgoyt/aethelgrad/PrivateContentDownloader.kt").read_text()
    native = (root / "app/src/main/cpp/forest_game.cpp").read_text()
    workflow = (root / ".github/workflows/android-build.yml").read_text()

    tiers = {tier["id"]: tier for tier in manifest["resourceTiers"]}
    require(set(tiers) == {"stage-1", "high"}, "manifest must define Stage 1 and the preserved high-end tier")
    declared_packs = {pack["name"] for pack in manifest["packs"]}
    install_time_packs = {pack["name"] for pack in manifest["packs"] if pack.get("delivery") == "install-time"}
    high_end_packs = declared_packs - install_time_packs
    require(install_time_packs == {"assetpack_core"}, "core bootstrap must be the only install-time asset pack")
    require(high_end_packs == set(tiers["high"]["packs"]), "high-end tier must cover every non-bootstrap manifest pack exactly once")
    require(tiers["stage-1"]["plannedBudgetMiB"] == 1024, "Stage 1 must use the 1 GiB planning envelope")
    require(tiers["stage-1"]["measuredBytes"] == manifest["contentDelivery"]["launchSlice"]["measuredBytes"], "Stage 1 measured bytes must match the launch slice")
    require(set(tiers["stage-1"]["packs"]) == set(manifest["contentDelivery"]["launchSlice"]["startupPacks"] + manifest["contentDelivery"]["launchSlice"]["authoredOnDemandPacks"]), "Stage 1 must cover exactly the authored launch-slice packs")
    require(tiers["high"]["plannedBudgetMiB"] == 6750, "high-end package must retain the 6750 MiB planning envelope")
    require(tiers["high"]["measuredBytes"] == 0, "unpublished high-end content must report zero measured payload bytes")
    require(tiers["high"]["graphicsQuality"] == "unavailable-until-cooked", "unpublished high-end content must not claim runtime quality")
    for pack_name in sorted(declared_packs):
        pack_root = root / pack_name
        require((pack_root / "build.gradle.kts").is_file(), f"asset pack module is missing build.gradle.kts: {pack_name}")
        require((pack_root / "src/main/assets").is_dir(), f"asset pack is missing content root: {pack_name}/src/main/assets")
    require("assetpack_hd_textures" in tiers["high"]["packs"], "high-end tier must include HD textures")
    require("assetpack_vfx" in tiers["high"]["packs"], "high-end tier must include VFX")
    require("assetpack_pipeline_cache" in tiers["high"]["packs"], "high-end tier must include pipeline cache")
    require({"cinematic", "high", "balanced", "performance"}.issubset(profiles), "graphics profiles must include internal device bands")
    require(profiles["cinematic"]["texture_max_size"] >= 4096, "cinematic profile must retain 4K source textures")
    require(profiles["cinematic"]["foliage_density"] >= 1.0, "cinematic profile must enable full foliage density")
    require("graphicsTierIndex = 4" in content_plan, "high-end content must map to the richest native tier")
    require("foliageDensity = 100" in content_plan and "effectScalePercent = 140" in content_plan, "high-end envelope must retain richer effects")
    require("setContentTierReady" in activity, "Android must notify native rendering when content is ready")
    require('"requiredBeforeStart": false' in manifest_text, "unpublished high-end content must not block the safe core world")
    require('"published": false' in manifest_text and '"mode": "not-published"' in manifest_text, "high-end content must remain explicitly unpublished")
    require('"automaticExpansion": false' in manifest_text, "content must not be silently substituted")
    require('"mode": "not-published"' in manifest_text, "manifest must declare that high-end content is not published")
    require("PrivateContentDownloader" in catalog and "standaloneExpansionFile" in catalog, "catalog must support private OBB content")
    require("https://" in downloader and "Range" in downloader and "SHA-256" in downloader, "private downloader must enforce HTTPS, resume, and verification")
    require("archiveBytes" in downloader and "archiveSha256" in downloader, "private downloader must validate the signed manifest")
    require("requestProductionContent(resourceTier)" in activity, "startup must request the complete high-end package")
    require("productionContentReady" in catalog, "production readiness gate must remain present")
    require("currentPlayerName = \"PLAYER NAME\"" in activity and "setPlayerName" in activity, "HUD must use a dynamic username")
    require("GRAPHICS DOWNLOAD FAILED  •  GAME LOCKED" in activity, "download failure must keep gameplay locked")
    require("DOWNLOAD CANCELED  •  GAME LOCKED" in activity, "cancellation must keep gameplay locked")
    require("WARMING STAGE 1 GRAPHICS" in activity and "NECESSARY RESOURCES READY" in activity, "startup must show Stage 1 finalization and readiness states")
    require("minimumWorldLoadingDurationMs = 10_000L" in activity, "startup must reserve a ten-second preparation window")
    require("worldLoadingProgressTicker" in activity and "timelinePercent" in activity, "startup progress must update continuously")
    require("LOW GRAPHICS" not in activity and "SELECT GRAPHICS QUALITY" not in activity, "player-facing Low/High chooser must be removed")
    require("LOW RESOURCES" not in manifest_text, "manifest must not advertise a low-end package")
    require("--local-testing" in workflow and "aethelgard-local-testing.apks" in workflow, "CI must retain complete local-testing APK-set support")
    print("GRAPHICS_TIER_CONTRACT_PASS=1")
    print("RESOURCE_TIERS=stage-1,high")
    print(f"STAGE_1_PLANNED_BUDGET_MIB={tiers['stage-1']['plannedBudgetMiB']}")
    print(f"STAGE_1_MEASURED_BYTES={tiers['stage-1']['measuredBytes']}")
    print(f"HIGH_PACKS={len(tiers['high']['packs'])}")
    print(f"HIGH_PLANNED_BUDGET_MIB={tiers['high']['plannedBudgetMiB']}")
    print(f"HIGH_MEASURED_BYTES={tiers['high']['measuredBytes']}")


if __name__ == "__main__":
    main()
