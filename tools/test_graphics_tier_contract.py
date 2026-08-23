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
    manifest_text = (root / "app/src/main/assets/asset_manifest.json").read_text()
    manifest = json.loads(manifest_text)
    profiles = json.loads((root / "assets/graphics_profiles.json").read_text())["profiles"]
    content_plan = (root / "app/src/main/java/com/darvirgoyt/aethelgrad/ContentDownloadPlan.kt").read_text()
    activity = (root / "app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt").read_text()
    native = (root / "app/src/main/cpp/forest_game.cpp").read_text()
    workflow = (root / ".github/workflows/android-build.yml").read_text()
    delivery_notes = (root / "docs/EXTERNAL_ANDROID_DELIVERY_NOTES.md").read_text()

    tiers = {tier["id"]: tier for tier in manifest["resourceTiers"]}
    require(set(tiers) == {"low", "high"}, "manifest must define exactly low and high resource tiers")
    declared_packs = {pack["name"] for pack in manifest["packs"]}
    require(declared_packs == set(tiers["high"]["packs"]), "high tier must cover every manifest-declared pack exactly once")
    for pack_name in sorted(declared_packs):
        pack_root = root / pack_name
        require((pack_root / "build.gradle.kts").is_file(), f"asset pack module is missing build.gradle.kts: {pack_name}")
        require((pack_root / "src/main/assets").is_dir(), f"asset pack is missing required content root: {pack_name}/src/main/assets")
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
    require('graphicsTierIndex = 0' in content_plan, "low graphics must map to the lightweight native tier")
    require('graphicsTierIndex = 4' in content_plan, "high graphics must map to the richest native tier")
    require('foliageDensity = 55' in content_plan and 'effectScalePercent = 70' in content_plan, "low graphics must reduce foliage and effects")
    require('foliageDensity = 100' in content_plan and 'effectScalePercent = 140' in content_plan, "high graphics must retain richer foliage and effects")
    require("setContentTierReady" in activity, "Android must notify native rendering when downloaded content is ready")
    require('"selectionRequiredAfterBootstrap": true' in manifest_text, "production must require an explicit resource-tier selection")
    require('"automaticExpansion": false' in manifest_text, "production must not silently expand or bypass tier selection")
    require("showResourceTierChooser" in activity and "resourceTierChooserVisible" in activity, "first launch must show the Low/High resource chooser")
    require("startupPacksFor(tier: ResourceTier): List<Pack> = packsFor(tier)" in content_plan, "the selected tier must be complete before gameplay")
    require("effectiveGraphicsQuality" in native, "native rendering must gate quality by content readiness")
    require("gContentTierReady" in native, "native rendering must retain downloaded-tier readiness")
    require("const int quality = effectiveGraphicsQuality();" in native, "premium effects must use the effective downloaded quality")
    require("SELECT GRAPHICS QUALITY" in activity and "LOW GRAPHICS" in activity and "HIGH GRAPHICS" in activity, "selector must expose professional low/high choices")
    require("display.widthPixels * 0.92f" in activity and "display.heightPixels * 0.90f" in activity, "selector dialog must fit the landscape-safe viewport")
    require("cards, LinearLayout.LayoutParams(-1, dp(158))" in activity and "card, LinearLayout.LayoutParams(0, dp(150), 1f)" in activity, "selector cards must use compact bounded heights")
    require("continueButton = cinematicButton" in activity and "onChosen(chosenTier)" in activity, "confirm action must advance with the selected tier")
    require('currentPlayerName = "PLAYER NAME"' in activity and "setPlayerName" in activity, "HUD must use a player username placeholder and update it dynamically")
    require("GRAPHICS DOWNLOAD FAILED  •  GAME LOCKED" in activity, "download failure must keep gameplay locked")
    require("WAITING FOR WI-FI  •  GAME LOCKED" in activity, "Wi-Fi waiting state must keep gameplay locked")
    require("loadingAnimationHandler" in activity and "PREPARE ${resourceTier.name} GRAPHICS" in activity, "download screen must include an animated preparation state")
    require("OPTIONAL VISUAL CONTENT UNAVAILABLE" not in activity, "obsolete optional-content fallback must not unlock gameplay")
    require("FREE LOCAL GRAPHICS MODE READY" not in activity, "local bundled graphics fallback must not unlock production gameplay")
    require("minimumWorldLoadingDurationMs = 10_000L" in activity, "startup loading must reserve a ten-second preparation window")
    require("worldLoadingProgressTicker" in activity and "timelinePercent" in activity, "startup progress must update continuously with a timeline")
    require("WARMING HIGH-END GRAPHICS" in activity and "NECESSARY RESOURCES READY" in activity, "startup must show explicit finalization and readiness states")
    require("--local-testing" in workflow and "aethelgard-local-testing.apks" in workflow, "CI must publish a bundletool local-testing APK set")
    require("direct APK" in delivery_notes and "bundletool" in delivery_notes and "-100" in delivery_notes, "delivery notes must explain PAD installation paths and error -100")
    print("GRAPHICS_TIER_CONTRACT_PASS=1")
    print(f"LOW_PACKS={len(tiers['low']['packs'])}")
    print(f"HIGH_PACKS={len(tiers['high']['packs'])}")
    print(f"HIGH_TARGET_MIB={tiers['high']['targetMiB']}")


if __name__ == "__main__":
    main()
