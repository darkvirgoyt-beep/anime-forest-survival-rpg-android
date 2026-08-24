#!/usr/bin/env python3
"""Validate the authored AETHELGRAD forest launch slice.

This gate checks real startup payloads only. It intentionally does not require the
future 6.75 GiB Unreal cook, and it never counts receipts or .gitkeep files as
payload bytes.
"""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STARTUP_PACKS = (
    "assetpack_core",
    "assetpack_graphics_base",
    "assetpack_forest",
    "assetpack_characters",
    "assetpack_shaders_gles",
    "assetpack_world_streaming",
    "assetpack_terrain_lod",
    "assetpack_animation_sets",
    "assetpack_foliage_lods",
    "assetpack_audio_hd",
)
DEFERRED_PACKS = (
    "assetpack_sand",
    "assetpack_snow",
    "assetpack_dungeons",
    "assetpack_hd_textures",
    "assetpack_vfx",
    "assetpack_cinematics",
    "assetpack_voice",
    "assetpack_shaders_vulkan",
    "assetpack_pipeline_cache",
)
PAYLOAD_SUFFIXES = {".json", ".png", ".wav", ".mp3", ".glsl"}


def payload_files(pack: str) -> list[Path]:
    root = ROOT / pack / "src/main/assets"
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.name not in {".gitkeep", "content_receipt.json"} and path.suffix.lower() in PAYLOAD_SUFFIXES
    )


def main() -> None:
    manifest = json.loads((ROOT / "assets/production_content/pack_receipts.json").read_text(encoding="utf-8"))
    android_manifest = json.loads((ROOT / "app/src/main/assets/asset_manifest.json").read_text(encoding="utf-8"))
    launch_manifest = android_manifest["contentDelivery"]["launchSlice"]
    by_pack = {entry["pack"]: entry for entry in manifest["packs"]}
    if manifest["contentId"] != "aethelgard-forest-launch-v1":
        raise SystemExit("FAIL launch_slice_content: unexpected content ID")
    for pack in (*STARTUP_PACKS, *DEFERRED_PACKS):
        if (ROOT / pack / "src/main/assets/content_receipt.json").exists():
            raise SystemExit(f"FAIL launch_slice_content: duplicate root receipt would break bundletool: {pack}")
    for pack in STARTUP_PACKS:
        files = payload_files(pack)
        if not files:
            raise SystemExit(f"FAIL launch_slice_content: startup pack has no real payload: {pack}")
        receipt = by_pack.get(pack)
        if not receipt or receipt["status"] != "authored-launch-slice":
            raise SystemExit(f"FAIL launch_slice_content: missing authored receipt: {pack}")
        listed = set(receipt["files"])
        actual = {str(path.relative_to(ROOT / pack / "src/main/assets")) for path in files}
        if not actual.issubset(listed):
            raise SystemExit(f"FAIL launch_slice_content: unreceipted files in {pack}: {sorted(actual - listed)}")
    for pack in DEFERRED_PACKS:
        if by_pack.get(pack, {}).get("status") != "deferred-unreal-cook":
            raise SystemExit(f"FAIL launch_slice_content: deferred pack changed state: {pack}")
        if payload_files(pack):
            raise SystemExit(f"FAIL launch_slice_content: deferred pack contains unapproved payload: {pack}")
    total_bytes = sum(path.stat().st_size for pack in STARTUP_PACKS for path in payload_files(pack))
    if total_bytes <= 0:
        raise SystemExit("FAIL launch_slice_content: measured payload is empty")
    if launch_manifest["contentId"] != manifest["contentId"] or not launch_manifest["published"]:
        raise SystemExit("FAIL launch_slice_content: Android launch manifest is not published for the authored slice")
    if set(launch_manifest["startupPacks"]) != set(STARTUP_PACKS[:8]):
        raise SystemExit("FAIL launch_slice_content: startup pack list does not match the authored launch plan")
    if set(launch_manifest["authoredOnDemandPacks"]) != {"assetpack_foliage_lods", "assetpack_audio_hd"}:
        raise SystemExit("FAIL launch_slice_content: authored on-demand pack list is incorrect")
    if launch_manifest["measuredBytes"] != total_bytes:
        raise SystemExit(f"FAIL launch_slice_content: manifest measuredBytes={launch_manifest['measuredBytes']} but files={total_bytes}")
    if total_bytes >= 6_750 * 1024 * 1024:
        raise SystemExit("FAIL launch_slice_content: launch slice unexpectedly claims the full high-end archive")
    print("LAUNCH_SLICE_CONTENT_PASS=1")
    print(f"LAUNCH_SLICE_PACKS={len(STARTUP_PACKS)}")
    print(f"LAUNCH_SLICE_PAYLOAD_BYTES={total_bytes}")
    print(f"DEFERRED_UNREAL_PACKS={len(DEFERRED_PACKS)}")


if __name__ == "__main__":
    main()
