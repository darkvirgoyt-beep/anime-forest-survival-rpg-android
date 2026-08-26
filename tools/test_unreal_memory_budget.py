#!/usr/bin/env python3
"""Validate Unreal Android streaming limits and current/future asset-pack budgets."""

from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MIB = 1024 * 1024
DYNAMIC_PACK_CEILING_MIB = 512
INSTALL_TIME_PACK_CEILING_MIB = 1024
STREAMING_POOL_CEILING_MIB = 256


def read_json(relative_path: str) -> dict:
    return json.loads((ROOT / relative_path).read_text(encoding="utf-8"))


def directory_bytes(path: Path) -> int:
    return sum(item.stat().st_size for item in path.rglob("*") if item.is_file() and item.name != ".gitkeep")


def ini_value(text: str, key: str) -> str:
    match = re.search(rf"^{re.escape(key)}\s*=\s*(.+?)\s*$", text, flags=re.MULTILINE)
    if not match:
        raise AssertionError(f"missing Unreal setting: {key}")
    return match.group(1)


def validate_budget_manifest(manifest: dict, label: str) -> None:
    policy = manifest.get("policy", {})
    if int(policy.get("documented_per_dynamic_pack_ceiling_mib", -1)) != DYNAMIC_PACK_CEILING_MIB:
        raise AssertionError(f"{label} must retain a {DYNAMIC_PACK_CEILING_MIB} MiB dynamic-pack ceiling")
    planned_total = sum(int(pack["target_mib"]) for pack in manifest["packs"])
    if planned_total != int(manifest["target_total_mib"]):
        raise AssertionError(f"{label} pack total does not match target_total_mib")
    for pack in manifest["packs"]:
        ceiling = INSTALL_TIME_PACK_CEILING_MIB if pack["delivery"] == "install-time" else DYNAMIC_PACK_CEILING_MIB
        if int(pack["target_mib"]) > ceiling:
            raise AssertionError(f"{label} {pack['module']} exceeds its {ceiling} MiB delivery ceiling")


def main() -> None:
    engine = (ROOT / "Unreal/Config/DefaultEngine.ini").read_text(encoding="utf-8")
    if int(ini_value(engine, "r.Streaming.PoolSize")) > STREAMING_POOL_CEILING_MIB:
        raise AssertionError("Unreal Android texture-streaming pool exceeds the 256 MiB device-memory ceiling")
    if ini_value(engine, "r.TextureStreaming") != "1" or ini_value(engine, "r.Streaming.LimitPoolSizeToVRAM") != "True":
        raise AssertionError("Unreal Android texture streaming must remain enabled and VRAM-limited")
    if ini_value(engine, "bGenerateChunks") != "True" or ini_value(engine, "bCompressed") != "True":
        raise AssertionError("Unreal Android packaging must retain compressed chunk generation")

    stage = read_json("assets/asset_budget.json")
    future = read_json("assets/full_content_budget.json")
    runtime_manifest = read_json("app/src/main/assets/asset_manifest.json")
    validate_budget_manifest(stage, "stage budget")
    validate_budget_manifest(future, "full-content budget")

    stage_packs = {pack["module"]: pack for pack in stage["packs"]}
    for pack in stage["packs"]:
        actual_bytes = directory_bytes(ROOT / pack["module"] / "src/main" / "assets")
        if actual_bytes > int(pack["target_mib"]) * MIB:
            raise AssertionError(f"{pack['module']} current payload exceeds its stage budget")
        if pack["delivery"] == "deferred" and actual_bytes != 0:
            raise AssertionError(f"{pack['module']} is deferred but has packaged bytes")

    full_core = next(pack for pack in future["packs"] if pack["module"] == "assetpack_core")
    expected_post_install = int(future["target_total_mib"]) - int(full_core["target_mib"])
    if int(future["policy"]["post_install_full3d_target_mib"]) != expected_post_install:
        raise AssertionError("full-content post-install target must equal full total minus install-time core")

    for pack in runtime_manifest["packs"]:
        ceiling = INSTALL_TIME_PACK_CEILING_MIB if pack["delivery"] == "install-time" else DYNAMIC_PACK_CEILING_MIB
        if int(pack["targetMiB"]) > ceiling:
            raise AssertionError(f"runtime manifest {pack['name']} exceeds its {ceiling} MiB delivery ceiling")
    if set(stage_packs) != {pack["name"] for pack in runtime_manifest["packs"]}:
        raise AssertionError("stage budget and runtime manifest must describe the same pack set")

    print("UNREAL_MEMORY_BUDGET_PASS=1")
    print(f"UNREAL_STREAMING_POOL_MIB={ini_value(engine, 'r.Streaming.PoolSize')}")
    print(f"FUTURE_DYNAMIC_PACK_CEILING_MIB={DYNAMIC_PACK_CEILING_MIB}")
    print(f"FULL_CONTENT_TARGET_MIB={future['target_total_mib']}")
    print(f"FULL_CONTENT_POST_INSTALL_MIB={expected_post_install}")


if __name__ == "__main__":
    main()
