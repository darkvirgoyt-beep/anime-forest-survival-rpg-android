#!/usr/bin/env python3
"""Validate that downloadable resource tiers map to real runtime quality behavior."""
from __future__ import annotations

import json
from pathlib import Path


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL graphics_tier_contract: {message}")


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    manifest = json.loads((root / "app/src/main/assets/asset_manifest.json").read_text())
    profiles = json.loads((root / "assets/graphics_profiles.json").read_text())["profiles"]
    content_plan = (root / "app/src/main/java/com/darvirgoyt/aethelgrad/ContentDownloadPlan.kt").read_text()
    activity = (root / "app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt").read_text()
    native = (root / "app/src/main/cpp/forest_game.cpp").read_text()

    tiers = {tier["id"]: tier for tier in manifest["resourceTiers"]}
    require(set(tiers) == {"low", "high"}, "manifest must define exactly low and high resource tiers")
    require(tiers["high"]["targetMiB"] > tiers["low"]["targetMiB"], "high tier must have a larger content envelope")
    require(len(tiers["high"]["packs"]) > len(tiers["low"]["packs"]), "high tier must include additional content packs")
    require("assetpack_hd_textures" in tiers["high"]["packs"], "high tier must include HD textures")
    require("assetpack_vfx" in tiers["high"]["packs"], "high tier must include VFX")
    require("assetpack_pipeline_cache" in tiers["high"]["packs"], "high tier must include pipeline cache")
    require({"cinematic", "high", "balanced", "performance"}.issubset(profiles), "graphics profiles must include the declared device bands")
    require(profiles["cinematic"]["texture_max_size"] >= 4096, "cinematic profile must retain 4K source textures")
    require(profiles["cinematic"]["foliage_density"] >= 1.0, "cinematic profile must enable full foliage density")
    approved_reference = root / "assets/aethelgard_high_end_visual_target.jpg"
    require(approved_reference.is_file(), "approved art-direction image must be present")
    require(manifest["contentDelivery"]["highFidelityFeatures"]["approvedArtDirection"] == "assets/aethelgard_high_end_visual_target.jpg", "manifest must register the approved art direction")
    require("QualityEnvelope" in content_plan, "runtime plan must expose quality envelopes")
    require("qualityEnvelopeFor" in content_plan, "runtime plan must map tiers to quality envelopes")
    require("setContentTierReady" in activity, "Android must notify native rendering when downloaded content is ready")
    require("effectiveGraphicsQuality" in native, "native rendering must gate quality by content readiness")
    require("gContentTierReady" in native, "native rendering must retain downloaded-tier readiness")
    print("GRAPHICS_TIER_CONTRACT_PASS=1")
    print(f"LOW_PACKS={len(tiers['low']['packs'])}")
    print(f"HIGH_PACKS={len(tiers['high']['packs'])}")
    print(f"HIGH_TARGET_MIB={tiers['high']['targetMiB']}")


if __name__ == "__main__":
    main()
