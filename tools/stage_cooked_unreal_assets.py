#!/usr/bin/env python3
"""Stage real cooked Unreal runtime files for the standalone Aethelgard OBB.

The Unreal cook remains an offline build step. This tool only maps already-cooked
runtime files into the directory consumed by build_expansion_obb.py. It never
creates filler data and rejects editor-only Unreal source files.
"""
from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path

EDITOR_ONLY_SUFFIXES = {".uasset", ".umap", ".ubulk", ".uexp"}
RUNTIME_SUFFIXES = {
    ".pak",
    ".ucas",
    ".utoc",
    ".sig",
    ".so",
    ".dll",
    ".bin",
    ".json",
    ".ini",
    ".locres",
    ".ushaderbytecode",
    ".metallib",
}


def fail(message: str) -> "NoReturn":
    raise SystemExit(f"ERROR: {message}")


def load_mapping(path: Path) -> dict[str, list[str]]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"cannot read mapping file {path}: {error}")
    if not isinstance(data, dict) or not data:
        fail("mapping file must be a non-empty object of pack name to glob list")
    mapping: dict[str, list[str]] = {}
    for pack_name, patterns in data.items():
        if not isinstance(pack_name, str) or not pack_name.startswith("assetpack_"):
            fail(f"invalid asset-pack name: {pack_name!r}")
        if not isinstance(patterns, list) or not patterns or not all(isinstance(item, str) and item for item in patterns):
            fail(f"mapping for {pack_name} must be a non-empty list of globs")
        mapping[pack_name] = patterns
    return mapping


def is_allowed_runtime_file(path: Path) -> bool:
    if path.suffix.lower() in EDITOR_ONLY_SUFFIXES:
        return False
    # Cooked output commonly uses extensionless pak sidecars. Explicit runtime
    # suffixes are checked, while extensionless files are allowed only when the
    # caller's mapping selected them.
    return not path.suffix or path.suffix.lower() in RUNTIME_SUFFIXES


def matching_files(root: Path, patterns: list[str]) -> list[Path]:
    matches: set[Path] = set()
    for pattern in patterns:
        for path in root.glob(pattern):
            if path.is_symlink():
                fail(f"symlinks are not allowed in cooked input: {path}")
            if path.is_file():
                matches.add(path)
    return sorted(matches)


def stage(args: argparse.Namespace) -> None:
    cook_root = Path(args.cook_root).resolve()
    output_dir = Path(args.output_dir).resolve()
    mapping_path = Path(args.mapping_file).resolve()
    if not cook_root.is_dir():
        fail(f"cooked Unreal directory does not exist: {cook_root}")
    mapping = load_mapping(mapping_path)

    selected: dict[Path, str] = {}
    for pack_name, patterns in mapping.items():
        files = matching_files(cook_root, patterns)
        if not files:
            fail(f"mapping for {pack_name} matched no cooked runtime files")
        for path in files:
            if not is_allowed_runtime_file(path):
                fail(f"editor-only or unsupported cooked input selected: {path}")
            if path in selected and selected[path] != pack_name:
                fail(f"cooked file is mapped to multiple packs: {path}")
            selected[path] = pack_name

    if not selected:
        fail("no cooked runtime files were selected")

    pack_root = output_dir / "asset_packs"
    if output_dir.exists():
        for existing in output_dir.iterdir():
            if existing.name not in {"asset_manifest.json", "asset_packs"}:
                fail(f"output directory must be empty before staging: {existing}")
    pack_root.mkdir(parents=True, exist_ok=True)
    for pack_name in mapping:
        (pack_root / pack_name).mkdir(parents=True, exist_ok=True)

    total_bytes = 0
    for source, pack_name in sorted(selected.items(), key=lambda item: item[0].as_posix()):
        relative = source.relative_to(cook_root)
        destination = pack_root / pack_name / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        total_bytes += source.stat().st_size

    print(f"COOK_ROOT={cook_root}")
    print(f"PACKS={len(mapping)}")
    print(f"COOKED_FILES={len(selected)}")
    print(f"PAYLOAD_BYTES={total_bytes}")
    print(f"STAGING_DIR={output_dir}")


def parser() -> argparse.ArgumentParser:
    command = argparse.ArgumentParser(description=__doc__)
    command.add_argument("--cook-root", required=True, help="directory containing cooked Unreal runtime outputs")
    command.add_argument("--output-dir", required=True, help="empty directory that becomes the OBB input directory")
    command.add_argument("--mapping-file", default="tools/unreal_pack_mapping.json", help="pack-name to glob mapping JSON")
    return command


if __name__ == "__main__":
    try:
        stage(parser().parse_args())
    except (OSError, ValueError, shutil.Error) as error:
        fail(str(error))
