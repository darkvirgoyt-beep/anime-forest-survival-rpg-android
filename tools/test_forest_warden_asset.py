#!/usr/bin/env python3
"""Validate the supplied Forest Warden boss integration and player preservation contract."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACK = ROOT / "assetpack_characters/src/main/assets/launch_slice"
MIRROR = ROOT / "app/src/main/assets/launch_slice/assetpack_characters"
CONTRACT_PATH = PACK / "forest_warden_boss_asset.json"
CHARACTER_CONTRACT_PATH = PACK / "character_runtime_contract.json"
RECEIPT_PATH = ROOT / "assets/production_content/pack_receipts.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL forest_warden_asset: {message}")


def obj_counts(path: Path) -> dict[str, int]:
    counts = {"v": 0, "vt": 0, "f": 0, "o": 0, "mtllib": 0, "usemtl": 0}
    with path.open("r", encoding="utf-8") as handle:
        for raw in handle:
            key = raw.split(maxsplit=1)[0] if raw.strip() else ""
            if key in counts:
                counts[key] += 1
    return counts


def main() -> None:
    contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    character_contract = json.loads(CHARACTER_CONTRACT_PATH.read_text(encoding="utf-8"))
    receipts = json.loads(RECEIPT_PATH.read_text(encoding="utf-8"))
    receipt = next(item for item in receipts["packs"] if item["pack"] == "assetpack_characters")

    require(contract["role"] == "boss", "Forest Warden must remain a boss asset")
    require(contract["runtime"]["existingBossType"] == "ForestWarden", "boss must map to existing ForestWarden gameplay")
    require(contract["runtime"]["proceduralFallback"] == "draw3DForestWarden", "procedural Android fallback must remain available")
    require(contract["runtime"]["cookedUassetPresent"] is False, "must not claim a cooked Unreal asset")
    require(contract["playerPreservation"]["mainPlayerCharacter"] == "Aurora", "Aurora must remain the main player")
    require(contract["playerPreservation"]["playerReplacement"] is False, "boss must not replace the player")
    require(contract["source"]["sourceMesh"] == "model.obj", "source provenance must identify the uploaded model")
    require(contract["source"]["rawSourcePolicy"].find("not copied") >= 0, "raw model.obj shipping policy is missing")
    require("commercial distribution rights" in contract["source"]["provenance"], "rights gate must be explicit")

    for lod, expected_faces in (("boss_forest_warden_lod0.obj", 16000), ("boss_forest_warden_lod1.obj", 8000)):
        path = PACK / lod
        mirror = MIRROR / lod
        require(path.is_file(), f"missing pack LOD: {lod}")
        require(mirror.is_file(), f"missing APK mirror LOD: {lod}")
        require(path.read_bytes() == mirror.read_bytes(), f"pack/APK LOD mismatch: {lod}")
        counts = obj_counts(path)
        require(counts["f"] == expected_faces, f"{lod} face budget drifted")
        require(counts["v"] == counts["vt"], f"{lod} vertex/UV count mismatch")
        require(counts["o"] == 1, f"{lod} must contain one stable named object")
        require(counts["mtllib"] == 1 and counts["usemtl"] == 1, f"{lod} material linkage is incomplete")

    for name in ("forest_warden_boss_asset.json", "boss_forest_warden.mtl", "boss_forest_warden_diffuse.jpg", "character_runtime_contract.json"):
        require((PACK / name).is_file(), f"missing pack file: {name}")
        require((PACK / name).read_bytes() == (MIRROR / name).read_bytes(), f"pack/APK mirror mismatch: {name}")
        require(f"launch_slice/{name}" in receipt["files"], f"receipt missing: {name}")

    require(character_contract["character"] == "Aurora", "character contract must still identify Aurora")
    require(character_contract["playerReplacement"] is False, "character contract cannot replace Aurora")
    require(any(item["asset"] == "forest_warden_boss_asset.json" for item in character_contract["bossAssets"]), "boss contract must be linked from character contract")
    require(not any(path.name == "model.obj" for path in ROOT.rglob("model.obj")), "raw model.obj must not exist in repository")

    print("FOREST_WARDEN_ASSET_CONTRACT_PASS=1")
    print("AURORA_PLAYER_PRESERVED=true")
    print("RAW_MODEL_OBJ_SHIPPED=false")
    print("MOBILE_LODS=16000,8000")
    print("PROCEDURAL_ANDROID_FALLBACK=draw3DForestWarden")


if __name__ == "__main__":
    main()
