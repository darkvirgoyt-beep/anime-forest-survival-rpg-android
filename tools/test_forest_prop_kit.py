#!/usr/bin/env python3
"""Validate the authored AETHELGRAD forest prop source kit."""
from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACK = ROOT / "assetpack_forest/src/main/assets/launch_slice"
APK = ROOT / "app/src/main/assets/launch_slice/assetpack_forest"
EXPECTED_MESHES = {
    "SM_Rock_Large",
    "SM_Rock_Mossy",
    "SM_Rock_Small",
    "SM_Log_Fallen",
    "SM_Ruin_Pillar_Left",
    "SM_Ruin_Pillar_Right",
    "SM_Ruin_Arch_Lintel",
    "SM_Ruin_Wall_Segment",
    "SM_Camp_FireRing",
    "SM_Camp_EmberCore",
    "SM_Camp_WoodPile",
    "SM_Shrine_Pedestal",
    "SM_Shrine_StandingStone",
}
EXPECTED_FILES = {"forest_prop_kit.obj", "forest_prop_kit.mtl", "forest_prop_kit.json"}


def fail(message: str) -> None:
    raise SystemExit(f"FAIL forest_prop_kit: {message}")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    if set(path.name for path in PACK.iterdir() if path.name.startswith("forest_prop_kit.")) != EXPECTED_FILES:
        fail("source kit file set is incomplete or contains unapproved files")
    for name in EXPECTED_FILES:
        source, mirror = PACK / name, APK / name
        if not source.is_file() or not mirror.is_file():
            fail(f"missing source/APK mirror: {name}")
        if sha256(source) != sha256(mirror):
            fail(f"APK mirror differs from source: {name}")

    manifest = json.loads((PACK / "forest_prop_kit.json").read_text(encoding="utf-8"))
    if manifest.get("contentId") != "aethelgard-forest-launch-v1":
        fail("prop manifest content ID is not the Stage 1 content ID")
    if manifest.get("kitId") != "forest-prop-kit-v1":
        fail("unexpected prop kit ID")
    if "third-party" in manifest.get("source", "").lower() or "third-party" in manifest.get("license", "").lower() and "no third-party" not in manifest["license"].lower():
        fail("prop manifest does not prove project-owned source")

    profile = manifest.get("mobileProfile", {})
    if len(profile.get("lods", [])) != 3:
        fail("mobile prop contract must define source plus two lower LODs")
    if profile.get("collision") != "simple convex per prop; no per-poly mobile collision":
        fail("mobile collision contract is not fail-closed")
    if profile.get("maxTrianglesPerCombinedSource", 0) <= 0 or profile["maxTrianglesPerCombinedSource"] > 650:
        fail("combined source exceeds the launch mobile triangle budget")

    props = manifest.get("props", [])
    prop_ids = {prop.get("id") for prop in props}
    required_ids = {"rock_large", "rock_mossy", "rock_small", "log_fallen", "ruin_arch", "ruin_wall", "camp_fire_ring", "camp_wood_pile", "shrine_pedestal", "shrine_standing_stone"}
    if prop_ids != required_ids:
        fail(f"prop manifest IDs differ: {sorted(prop_ids ^ required_ids)}")
    if any(not prop.get("collision") for prop in props):
        fail("every prop needs an explicit collision approximation")

    obj = (PACK / "forest_prop_kit.obj").read_text(encoding="utf-8")
    vertices = len(re.findall(r"^v -?\d", obj, flags=re.MULTILINE))
    faces = len(re.findall(r"^f \d", obj, flags=re.MULTILINE))
    groups = set(re.findall(r"^g (SM_[A-Za-z0-9_]+)$", obj, flags=re.MULTILINE))
    materials = set(re.findall(r"^usemtl (M_[A-Za-z0-9_]+)$", obj, flags=re.MULTILINE))
    if vertices != 246 or faces != 272:
        fail(f"geometry is not deterministic: vertices={vertices}, faces={faces}")
    if groups != EXPECTED_MESHES:
        fail(f"named mesh groups differ: missing={sorted(EXPECTED_MESHES - groups)} extra={sorted(groups - EXPECTED_MESHES)}")
    if len(materials) > 7 or not materials:
        fail("material count is outside the mobile prop contract")
    if "mtllib forest_prop_kit.mtl" not in obj:
        fail("OBJ does not reference its owned material library")

    placement = manifest.get("placementContract", {})
    if placement != {"chunk": "forest-sector-00", "seed": 74291, "minimumSpacingMeters": 2.5, "avoidWaterMeters": 1.5, "ruinLandmarkCount": 1, "campCount": 1}:
        fail("placement contract changed without a reviewed deterministic update")
    region = json.loads((PACK / "forest_region.json").read_text(encoding="utf-8"))
    if region.get("propKit", {}).get("manifest") != "forest_prop_kit.json" or region["propKit"].get("placementSeed") != 74291:
        fail("forest region does not reference the deterministic prop placement contract")
    if (APK / "forest_region.json").read_bytes() != (PACK / "forest_region.json").read_bytes():
        fail("forest region APK mirror differs from asset-pack source")

    receipts = json.loads((ROOT / "assets/production_content/pack_receipts.json").read_text(encoding="utf-8"))
    forest = next((entry for entry in receipts["packs"] if entry["pack"] == "assetpack_forest"), None)
    if forest is None or not EXPECTED_FILES.issubset({Path(item).name for item in forest.get("files", [])}):
        fail("central forest receipt does not cover the complete prop kit")
    for pack in ROOT.glob("assetpack_*/src/main/assets"):
        if (pack / "content_receipt.json").exists():
            fail(f"duplicate root receipt would break bundletool: {pack.parent.parent.parent.name}")

    print("FOREST_PROP_KIT_PASS=1")
    print(f"FOREST_PROP_MESHES={len(groups)}")
    print(f"FOREST_PROP_VERTICES={vertices}")
    print(f"FOREST_PROP_FACES={faces}")


if __name__ == "__main__":
    main()
