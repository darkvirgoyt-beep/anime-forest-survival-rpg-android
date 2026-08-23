#!/usr/bin/env python3
"""Validate authored Android asset-pack content against assets/asset_budget.json.

This tool intentionally reports the current repository size separately from the
planned target. It never creates files or pads content to satisfy a budget.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def directory_bytes(path: Path) -> int:
    return sum(item.stat().st_size for item in path.rglob("*") if item.is_file() and item.name != ".gitkeep")


def mib(value: int) -> float:
    return value / (1024 * 1024)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--require-target", action="store_true", help="fail when authored bytes are below the planned target")
    args = parser.parse_args()

    root = args.root.resolve()
    manifest_path = root / "assets" / "asset_budget.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    packs = manifest["packs"]
    policy = manifest.get("policy", {})
    dynamic_ceiling = int(policy.get("documented_per_dynamic_pack_ceiling_mib", 512))
    install_ceiling = 1024
    fast_follow_count = sum(pack.get("delivery") == "fast-follow" for pack in packs)
    if fast_follow_count > 1:
        raise SystemExit(f"too many fast-follow packs: {fast_follow_count}; Play allows one")
    for pack in packs:
        delivery = pack.get("delivery")
        ceiling = install_ceiling if delivery == "install-time" else dynamic_ceiling
        if int(pack["target_mib"]) > ceiling:
            raise SystemExit(f"{pack['module']} target {pack['target_mib']} MiB exceeds {delivery} ceiling {ceiling} MiB")
    planned_total = sum(int(pack["target_mib"]) for pack in packs)
    if planned_total != int(manifest["target_total_mib"]):
        raise SystemExit(f"manifest total mismatch: packs={planned_total} target_total_mib={manifest['target_total_mib']}")

    missing_roots = [
        (root / pack["module"] / "src" / "main" / "assets")
        for pack in packs
        if not (root / pack["module"] / "src" / "main" / "assets").is_dir()
    ]
    if missing_roots:
        print("ERROR: required asset-pack content roots are missing:")
        for path in missing_roots:
            print(f"  - {path.relative_to(root)}")
        print("Create each directory and keep a tracked .gitkeep until real authored assets are added.")
        return 4

    actual_total = 0
    print(f"AETHELGRAD asset budget: planned={planned_total} MiB ({mib(planned_total * 1024 * 1024):.2f} MiB)")
    print("pack,delivery,target_mib,actual_mib,status")
    for pack in packs:
        pack_path = root / pack["module"] / "src" / "main" / "assets"
        actual_bytes = directory_bytes(pack_path)
        actual_total += actual_bytes
        actual_mib = mib(actual_bytes)
        status = "OK" if actual_mib <= float(pack["target_mib"]) else "OVER_BUDGET"
        print(f"{pack['module']},{pack['delivery']},{pack['target_mib']},{actual_mib:.3f},{status}")
        if status == "OVER_BUDGET":
            return 2

    print(f"actual_authored_total_mib={mib(actual_total):.3f}")
    if args.require_target and actual_total < planned_total * 1024 * 1024:
        print("ERROR: authored resources are below the planned target; add real licensed/generated assets, never padding files.")
        return 3
    print("PASS: no pack exceeds its planned budget and no padding was generated.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
