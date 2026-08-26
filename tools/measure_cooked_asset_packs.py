#!/usr/bin/env python3
"""Measure staged cooked Unreal asset packs and enforce Android delivery budgets.

The tool accepts the output of ``stage_cooked_unreal_assets.py``. It never
creates content, padding, or estimates: reported bytes are the real staged files.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


MIB = 1024 * 1024
INSTALL_TIME_CEILING_MIB = 1024
EDITOR_ONLY_SUFFIXES = {".uasset", ".umap", ".ubulk", ".uexp"}
RUNTIME_SUFFIXES = {
    ".pak", ".ucas", ".utoc", ".sig", ".so", ".dll", ".bin", ".json",
    ".ini", ".locres", ".ushaderbytecode", ".metallib",
}


def fail(message: str) -> "NoReturn":
    raise SystemExit(f"ERROR: {message}")


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"cannot read JSON file {path}: {error}")
    if not isinstance(value, dict):
        fail(f"JSON object required: {path}")
    return value


def payload_files(directory: Path) -> list[Path]:
    files: list[Path] = []
    for item in directory.rglob("*"):
        if item.is_symlink():
            fail(f"symlink is not allowed in staged cooked content: {item}")
        if item.is_file() and item.name != ".gitkeep":
            files.append(item)
    return sorted(files)


def mib(value: int) -> float:
    return value / MIB


def is_runtime_file(path: Path) -> bool:
    suffix = path.suffix.lower()
    return suffix not in EDITOR_ONLY_SUFFIXES and (not suffix or suffix in RUNTIME_SUFFIXES)


def audit_raw_cook(cook_root: Path, mapping: dict[str, Any], staged_dirs: dict[str, Path]) -> dict[str, int]:
    if not cook_root.is_dir():
        fail(f"cooked Unreal directory does not exist: {cook_root}")
    missing_staged = sorted(set(mapping) - set(staged_dirs))
    if missing_staged:
        fail(f"staging is missing mapped asset-pack directories: {', '.join(missing_staged)}")
    selected: dict[Path, str] = {}
    for pack_name, patterns in mapping.items():
        if not isinstance(patterns, list) or not patterns:
            fail(f"mapping for {pack_name} must be a non-empty list of globs")
        for pattern in patterns:
            for path in cook_root.glob(pattern):
                if path.is_symlink():
                    fail(f"symlink is not allowed in cooked input: {path}")
                if not path.is_file():
                    continue
                if not is_runtime_file(path):
                    fail(f"editor-only or unsupported cooked input selected: {path}")
                previous = selected.get(path)
                if previous and previous != pack_name:
                    fail(f"cooked file is mapped to multiple packs: {path}")
                selected[path] = pack_name

    runtime_files = [path for path in cook_root.rglob("*") if path.is_file() and is_runtime_file(path)]
    unassigned = sorted(path for path in runtime_files if path not in selected)
    if unassigned:
        formatted = ", ".join(str(path.relative_to(cook_root)) for path in unassigned[:5])
        suffix = "" if len(unassigned) <= 5 else f" (+{len(unassigned) - 5} more)"
        fail(f"unassigned cooked runtime files: {formatted}{suffix}")

    source_bytes_by_pack = {pack_name: 0 for pack_name in mapping}
    for source, pack_name in selected.items():
        staged = staged_dirs[pack_name] / source.relative_to(cook_root)
        if not staged.is_file() or staged.stat().st_size != source.stat().st_size:
            fail(f"staged cooked file is missing or size-mismatched: {staged}")
        source_bytes_by_pack[pack_name] += source.stat().st_size
    return source_bytes_by_pack


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    repo_root = Path(__file__).resolve().parents[1]
    parser.add_argument("--staging-root", type=Path, required=True, help="staging directory containing asset_packs/<pack>/...")
    parser.add_argument("--budget-manifest", type=Path, default=repo_root / "assets/full_content_budget.json")
    parser.add_argument("--mapping-file", type=Path, default=repo_root / "tools/unreal_pack_mapping.json")
    parser.add_argument("--cook-root", type=Path, help="optional raw cooked directory to audit for unassigned runtime files")
    parser.add_argument("--require-nonempty", action="store_true", help="require every declared pack to contain at least one cooked file")
    parser.add_argument("--report-json", type=Path, help="optional machine-readable output path")
    args = parser.parse_args()

    staging_root = args.staging_root.resolve()
    pack_root = staging_root / "asset_packs" if (staging_root / "asset_packs").is_dir() else staging_root
    if not pack_root.is_dir():
        fail(f"staged asset-pack directory does not exist: {pack_root}")

    budget = read_json(args.budget_manifest.resolve())
    packs = budget.get("packs")
    if not isinstance(packs, list) or not packs:
        fail("budget manifest must contain a non-empty packs array")
    policy = budget.get("policy", {})
    dynamic_ceiling = int(policy.get("documented_per_dynamic_pack_ceiling_mib", 512))

    budget_by_name: dict[str, dict[str, Any]] = {}
    for pack in packs:
        if not isinstance(pack, dict):
            fail("budget packs must be objects")
        name = pack.get("module")
        if not isinstance(name, str) or not name.startswith("assetpack_"):
            fail(f"invalid budget pack name: {name!r}")
        if name in budget_by_name:
            fail(f"duplicate budget pack: {name}")
        target_mib = int(pack.get("target_mib", -1))
        delivery = pack.get("delivery")
        ceiling = INSTALL_TIME_CEILING_MIB if delivery == "install-time" else dynamic_ceiling
        if target_mib < 0 or target_mib > ceiling:
            fail(f"{name} target {target_mib} MiB exceeds its {delivery} ceiling of {ceiling} MiB")
        budget_by_name[name] = pack

    planned_total_mib = sum(int(pack["target_mib"]) for pack in packs)
    if planned_total_mib != int(budget.get("target_total_mib", -1)):
        fail(f"budget total mismatch: packs={planned_total_mib} target_total_mib={budget.get('target_total_mib')}")

    mapping = read_json(args.mapping_file.resolve())
    mapping_names = set(mapping)
    budget_names = set(budget_by_name)
    if mapping_names != budget_names:
        missing = sorted(budget_names - mapping_names)
        extra = sorted(mapping_names - budget_names)
        fail(f"mapping/budget pack mismatch: missing={missing or '-'} extra={extra or '-'}")

    staged_dirs = {item.name: item for item in pack_root.iterdir() if item.is_dir()}
    unknown_dirs = sorted(set(staged_dirs) - budget_names)
    if unknown_dirs:
        fail(f"staging contains unassigned asset-pack directories: {', '.join(unknown_dirs)}")

    source_bytes_by_pack = audit_raw_cook(args.cook_root.resolve(), mapping, staged_dirs) if args.cook_root else None

    report_packs: list[dict[str, Any]] = []
    actual_total_bytes = 0
    print(f"COOKED_ASSET_PACK_BUDGET planned_total_mib={planned_total_mib}")
    print("pack,delivery,target_mib,file_count,actual_bytes,actual_mib,status")
    for name in sorted(budget_names):
        pack = budget_by_name[name]
        directory = staged_dirs.get(name)
        files = payload_files(directory) if directory else []
        actual_bytes = sum(file.stat().st_size for file in files)
        actual_total_bytes += actual_bytes
        target_mib = int(pack["target_mib"])
        over_budget = actual_bytes > target_mib * MIB
        empty = not files
        status = "OVER_BUDGET" if over_budget else "EMPTY" if empty else "OK"
        print(f"{name},{pack['delivery']},{target_mib},{len(files)},{actual_bytes},{mib(actual_bytes):.3f},{status}")
        if over_budget:
            fail(f"{name} cooked payload is {actual_bytes} bytes, above its {target_mib} MiB budget")
        if args.require_nonempty and empty:
            fail(f"{name} has no cooked runtime files; a full-content build requires non-empty mapped packs")
        if source_bytes_by_pack is not None and actual_bytes != source_bytes_by_pack[name]:
            fail(f"{name} staged bytes {actual_bytes} differ from raw cooked assignment {source_bytes_by_pack[name]}")
        report_packs.append(
            {
                "name": name,
                "delivery": pack["delivery"],
                "targetMiB": target_mib,
                "fileCount": len(files),
                "actualBytes": actual_bytes,
                "status": status,
            }
        )

    planned_total_bytes = planned_total_mib * MIB
    if actual_total_bytes > planned_total_bytes:
        fail(f"cooked payload total {actual_total_bytes} exceeds planned total {planned_total_bytes}")

    result = {
        "stagingRoot": str(staging_root),
        "budgetManifest": str(args.budget_manifest.resolve()),
        "plannedTotalMiB": planned_total_mib,
        "actualTotalBytes": actual_total_bytes,
        "actualTotalMiB": mib(actual_total_bytes),
        "rawCookRoot": str(args.cook_root.resolve()) if args.cook_root else None,
        "packs": report_packs,
    }
    if args.report_json:
        output = args.report_json.resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"COOKED_ASSET_PACK_REPORT={output}")

    print(f"COOKED_ASSET_PACK_ACTUAL_TOTAL_BYTES={actual_total_bytes}")
    print("COOKED_ASSET_PACK_BUDGET_PASS=1")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        fail(str(error))
