#!/usr/bin/env python3
"""Deterministic smoke tests for the private high-end resource center.

This script does not download gigabytes. It models the complete 18-pack state
that the private HTTPS archive represents and checks the UI-facing aggregate
state, then verifies the Android source integration.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

PACKS = (
    "assetpack_graphics_base",
    "assetpack_forest",
    "assetpack_sand",
    "assetpack_snow",
    "assetpack_characters",
    "assetpack_audio_hd",
    "assetpack_cinematics",
    "assetpack_hd_textures",
    "assetpack_dungeons",
    "assetpack_vfx",
    "assetpack_voice",
    "assetpack_shaders_vulkan",
    "assetpack_shaders_gles",
    "assetpack_pipeline_cache",
    "assetpack_world_streaming",
    "assetpack_foliage_lods",
    "assetpack_terrain_lod",
    "assetpack_animation_sets",
)
DOWNLOADING = "DOWNLOADING"
COMPLETED = "COMPLETED"
FAILED = "FAILED"


@dataclass
class PackState:
    status: str = "PENDING"
    downloaded: int = 0
    total: int = 0
    error: int = 0


class ResourceCenterModel:
    """Small mirror of the aggregate state exposed by AssetPackCatalog."""

    def __init__(self, packs: tuple[str, ...] = PACKS) -> None:
        self.states = {name: PackState() for name in packs}
        self.retry_count = 0
        self.history: list[int] = []

    def apply(self, name: str, status: str, downloaded: int, total: int, error: int = 0) -> dict:
        if name not in self.states:
            raise AssertionError(f"unexpected pack: {name}")
        if downloaded < 0 or total < 0 or downloaded > total:
            raise AssertionError(f"invalid byte state for {name}: {downloaded}/{total}")
        self.states[name] = PackState(status, downloaded, total, error)
        reported_total = sum(item.total for item in self.states.values())
        aggregate_downloaded = sum(item.downloaded for item in self.states.values())
        failed = next((pack for pack, item in self.states.items() if item.status == FAILED), None)
        complete = all(item.status == COMPLETED for item in self.states.values())
        all_totals_known = all(item.total > 0 for item in self.states.values())
        aggregate_total = reported_total if all_totals_known else 0
        percent = 100 if complete else (
            min(99, max(0, aggregate_downloaded * 100 // aggregate_total))
            if aggregate_total else 0
        )
        self.history.append(percent)
        return {
            "status": FAILED if failed else COMPLETED if complete else DOWNLOADING,
            "percent": percent,
            "downloaded": aggregate_downloaded,
            "total": aggregate_total,
            "failed_pack": failed,
        }

    def retry(self, name: str) -> None:
        if name not in self.states:
            raise AssertionError(f"unexpected retry pack: {name}")
        self.states[name] = PackState()
        self.retry_count += 1


def assert_source_contract(repo: Path) -> None:
    main = (repo / "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt").read_text()
    catalog = (repo / "app/src/main/java/com/darkvirgoyt/aethelgrad/AssetPackCatalog.kt").read_text()
    plan = (repo / "app/src/main/java/com/darkvirgoyt/aethelgrad/ContentDownloadPlan.kt").read_text()
    settings = (repo / "settings.gradle.kts").read_text()
    app_build = (repo / "app/build.gradle.kts").read_text()
    manifest = json.loads((repo / "app/src/main/assets/asset_manifest.json").read_text())
    cpp = (repo / "app/src/main/cpp/controller/third_person_controller.cpp").read_text()
    required = (
        ("stage-1 preparation title", "PREPARE STAGE 1 FOREST CONTENT", main),
        ("locked failure state", "GRAPHICS DOWNLOAD FAILED  •  GAME LOCKED", main),
        ("high-end content description", "resourceTier.description", main),
        ("tier-aware production request", "requestProductionContent(resourceTier)", main),
        ("tier pack selection", "startupPackNamesFor(tier)", catalog),
        ("private downloader", "PrivateContentDownloader", catalog),
        ("production readiness gate", "productionContentReady", catalog),
        ("Vulkan shader pack", "assetpack_shaders_vulkan", plan),
        ("OpenGL ES shader pack", "assetpack_shaders_gles", plan),
        ("pipeline cache pack", "assetpack_pipeline_cache", plan),
        ("all packs in settings", "assetpack_animation_sets", settings),
        ("all packs in app bundle", ":assetpack_animation_sets", app_build),
        ("smooth verified progress bar", "animateVerifiedProgress(if (event.sizeVerified || event.complete) event.percent else 0)", main),
        ("retry control", "RETRY ASSET PREPARATION", main),
        ("verified publication gate", "published_high_end_content", (repo / "app/src/main/res/values/strings.xml").read_text()),
        ("real-byte-only progress", "sizeVerified", catalog),
        ("unpublished content catalog", "not-published", json.dumps(manifest)),
        ("wide pitch clamp", "1.52f", cpp),
    )
    missing = [label for label, needle, haystack in required if needle not in haystack]
    if missing:
        raise AssertionError("missing source contract: " + ", ".join(missing))
    content_delivery = manifest.get("contentDelivery", {})
    if content_delivery.get("published") is not False or content_delivery.get("measuredArchiveBytes") != 0:
        raise AssertionError("unchecked-in high-end content must be marked unpublished with zero measured bytes")
    tiers = {item.get("id"): item for item in manifest.get("resourceTiers", [])}
    launch_slice = content_delivery.get("launchSlice", {})
    if set(tiers) != {"stage-1", "high"} or tiers["high"].get("measuredBytes") != 0:
        raise AssertionError("manifest must retain the unpublished high-end tier with zero measured bytes")
    if not launch_slice.get("published") or tiers["stage-1"].get("measuredBytes") != launch_slice.get("measuredBytes"):
        raise AssertionError("published Stage 1 tier must report only the measured authored launch-slice bytes")
    if tiers["stage-1"].get("plannedBudgetMiB") != 1024:
        raise AssertionError("Stage 1 tier must use the 1 GiB plan")
    if set(tiers["high"].get("packs", [])) != set(PACKS):
        raise AssertionError("high-end pack set is inconsistent")
    if content_delivery.get("mode") != "not-published":
        raise AssertionError("manifest must declare unpublished content until a real archive exists")
    cinematic_pack = next(pack for pack in manifest.get("packs", []) if pack.get("name") == "assetpack_cinematics")
    if cinematic_pack.get("published") is not False or cinematic_pack.get("measuredBytes") != 0:
        raise AssertionError("cinematic pack must remain unpublished with zero measured bytes until real cooked media exists")
    cinematic_state = json.loads((repo / "assetpack_cinematics/CINEMATIC_PACK_STATE.json").read_text())
    if cinematic_state.get("published") is not False or cinematic_state.get("measuredPayloadBytes") != 0:
        raise AssertionError("cinematic asset-pack state must not claim a cooked payload before publication")
    if "High-end sector packs, including cinematics, are unavailable" not in catalog:
        raise AssertionError("high-tier sectors must be gated before any unpublished cinematic pack request")


def test_initial_state() -> None:
    model = ResourceCenterModel()
    assert set(model.states) == set(PACKS)
    result = model.apply(PACKS[0], DOWNLOADING, 0, 100)
    assert result["status"] == DOWNLOADING
    assert result["percent"] == 0


def test_aggregate_progress_is_monotonic() -> None:
    model = ResourceCenterModel()
    for index, pack in enumerate(PACKS):
        model.apply(pack, DOWNLOADING, 0, 100)
        model.apply(pack, DOWNLOADING, 50, 100)
        model.apply(pack, COMPLETED, 100, 100)
    assert model.history == sorted(model.history)
    assert model.history[-1] == 100


def test_complete_requires_every_pack() -> None:
    model = ResourceCenterModel()
    for pack in PACKS[:-1]:
        result = model.apply(pack, COMPLETED, 100, 100)
    assert result["status"] == DOWNLOADING
    result = model.apply(PACKS[-1], COMPLETED, 100, 100)
    assert result["status"] == COMPLETED
    assert result["percent"] == 100


def test_failure_and_retry_state() -> None:
    model = ResourceCenterModel()
    result = model.apply(PACKS[2], FAILED, 12, 100, error=9)
    assert result["status"] == FAILED
    assert result["failed_pack"] == PACKS[2]
    model.retry(PACKS[2])
    assert model.retry_count == 1
    result = model.apply(PACKS[2], DOWNLOADING, 0, 100)
    assert result["status"] == DOWNLOADING


def test_high_end_pack_set_is_complete() -> None:
    assert len(PACKS) == 18
    assert len(set(PACKS)) == 18
    assert "assetpack_shaders_vulkan" in PACKS
    assert "assetpack_hd_textures" in PACKS
    assert "assetpack_pipeline_cache" in PACKS


def test_invalid_progress_is_rejected() -> None:
    model = ResourceCenterModel()
    try:
        model.apply(PACKS[0], DOWNLOADING, 101, 100)
    except AssertionError:
        return
    raise AssertionError("invalid byte progress was accepted")


def test_unknown_size_stays_at_zero() -> None:
    model = ResourceCenterModel()
    result = model.apply(PACKS[0], DOWNLOADING, 0, 0)
    assert result["total"] == 0
    assert result["percent"] == 0


def assert_unreal_project(project: Path) -> None:
    descriptor = json.loads(project.read_text())
    if not descriptor.get("FileVersion"):
        raise AssertionError("Unreal project descriptor has no FileVersion")
    if not descriptor.get("Modules"):
        raise AssertionError("Unreal project descriptor has no Modules")


def run(repo: Path | None, unreal_project: Path | None) -> None:
    tests = (
        test_initial_state,
        test_aggregate_progress_is_monotonic,
        test_complete_requires_every_pack,
        test_failure_and_retry_state,
        test_high_end_pack_set_is_complete,
        test_invalid_progress_is_rejected,
        test_unknown_size_stays_at_zero,
    )
    for test in tests:
        test()
        print(f"PASS {test.__name__}")
    checks = len(tests)
    if repo is not None:
        assert_source_contract(repo)
        print(f"PASS source_contract {repo}")
        checks += 1
    if unreal_project is not None:
        assert_unreal_project(unreal_project)
        print(f"PASS unreal_project {unreal_project}")
        checks += 1
    print(f"RESOURCE_CENTER_TESTS_PASS={checks}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, help="repository root for source-contract checks")
    parser.add_argument("--unreal-project", type=Path, help="Unreal .uproject descriptor to validate")
    args = parser.parse_args()
    try:
        run(
            args.repo.resolve() if args.repo else None,
            args.unreal_project.resolve() if args.unreal_project else None,
        )
    except (AssertionError, FileNotFoundError, json.JSONDecodeError) as error:
        print(f"FAIL {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
