#!/usr/bin/env python3
"""Validate the authored one-gibibyte runtime-content package without padding."""
from __future__ import annotations

import argparse
import json
from pathlib import Path


SUPPORTED_SOURCE_SUFFIXES = {".fbx", ".uasset", ".umap", ".tga", ".png", ".ktx2", ".wav", ".flac", ".ogg", ".mp4", ".mov", ".spv", ".bin"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--require-authored-payload", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    manifest = json.loads((root / "assets/runtime_content/aethelgard_1gib_v1.json").read_text())
    groups = manifest["packageGroups"]
    assert manifest["targetMiB"] == 1024
    assert manifest["bundledWorldPlayable"] is True
    assert manifest["sourceReceiptRequired"] is True
    assert sum(group["targetMiB"] for group in groups) == 1024
    assert all(group["sourceFormats"] and group["runtimeFormats"] for group in groups)

    payload_root = root / "assets/runtime_content/payload"
    authored = [path for path in payload_root.rglob("*") if path.is_file() and path.suffix.lower() in SUPPORTED_SOURCE_SUFFIXES] if payload_root.is_dir() else []
    authored_bytes = sum(path.stat().st_size for path in authored)
    target_bytes = manifest["targetMiB"] * 1024 * 1024
    receipts = json.loads((root / "assets/runtime_content/receipts.json").read_text())["receipts"]
    receipt_paths = {entry["path"] for entry in receipts}
    authored_paths = {str(path.relative_to(payload_root)) for path in authored}
    unreceipted = sorted(authored_paths - receipt_paths)
    if unreceipted:
        raise SystemExit("FAIL runtime_content_package: missing source/license receipts for " + ", ".join(unreceipted))
    print("RUNTIME_CONTENT_PACKAGE_PLAN_PASS=1")
    print(f"RUNTIME_CONTENT_TARGET_MIB={manifest['targetMiB']}")
    print(f"AUTHORED_PAYLOAD_FILES={len(authored)}")
    print(f"AUTHORED_PAYLOAD_BYTES={authored_bytes}")
    if args.require_authored_payload and authored_bytes < target_bytes:
        raise SystemExit("FAIL runtime_content_package: licensed authored payload is below the 1 GiB target")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
