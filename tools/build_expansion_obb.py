#!/usr/bin/env python3
"""Build a standalone Android expansion OBB for production content delivery.

Google Play releases should use the repository's AAB + Play Asset Delivery
modules. This script supports private APK channels, device labs, and
legacy expansion-file workflows. It never pads an archive to a target size.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import stat
import sys
import zipfile
from pathlib import Path, PurePosixPath

OBB_FORMAT = "aethelgard-expansion-obb-v1"
FIXED_DATE = (1980, 1, 1, 0, 0, 0)
PACKAGE_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_]*(?:\.[A-Za-z][A-Za-z0-9_]*)+$")


def fail(message: str) -> "NoReturn":
    raise SystemExit(f"ERROR: {message}")


def safe_relative(path: Path, root: Path) -> str:
    relative = path.relative_to(root).as_posix()
    pure = PurePosixPath(relative)
    if not relative or relative.startswith("/") or ".." in pure.parts:
        fail(f"unsafe input path: {relative}")
    return relative


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def content_files(root: Path) -> list[tuple[str, Path]]:
    files: list[tuple[str, Path]] = []
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            fail(f"symlinks are not allowed: {path}")
        if path.is_file():
            files.append((safe_relative(path, root), path))
    if not files:
        fail("input directory contains no content files; refusing to create an empty OBB")
    return files


def build(args: argparse.Namespace) -> Path:
    input_dir = Path(args.input_dir).resolve()
    output_dir = Path(args.output_dir).resolve()
    if not input_dir.is_dir():
        fail(f"input directory does not exist: {input_dir}")
    if not PACKAGE_RE.fullmatch(args.package_name):
        fail(f"invalid Android package name: {args.package_name}")
    if args.version_code <= 0:
        fail("version code must be positive")

    files = content_files(input_dir)
    entries = []
    total_bytes = 0
    for relative, path in files:
        size = path.stat().st_size
        total_bytes += size
        entries.append({"path": f"content/{relative}", "byteSize": size, "sha256": sha256_file(path)})

    manifest = {
        "format": OBB_FORMAT,
        "packageName": args.package_name,
        "versionCode": args.version_code,
        "contentVersion": args.content_version,
        "delivery": "standalone-expansion-obb",
        "playRecommendation": "Use the AAB with Play Asset Delivery for Google Play releases.",
        "contentRoot": "content",
        "fileCount": len(entries),
        "payloadBytes": total_bytes,
        "entries": entries,
        "note": "This archive contains staged runtime content only. It is not padded and does not manufacture missing Unreal assets."
    }

    output_dir.mkdir(parents=True, exist_ok=True)
    name = f"main.{args.version_code}.{args.package_name}.obb"
    output = output_dir / name
    temp = output.with_suffix(output.suffix + ".part")
    if temp.exists():
        temp.unlink()

    with zipfile.ZipFile(temp, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9, strict_timestamps=False) as archive:
        manifest_info = zipfile.ZipInfo("obb_manifest.json", FIXED_DATE)
        manifest_info.compress_type = zipfile.ZIP_DEFLATED
        manifest_info.external_attr = (stat.S_IFREG | 0o644) << 16
        archive.writestr(manifest_info, json.dumps(manifest, indent=2, sort_keys=True) + "\n")
        for relative, path in files:
            archive_name = f"content/{relative}"
            info = zipfile.ZipInfo(archive_name, FIXED_DATE)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = (stat.S_IFREG | 0o644) << 16
            with path.open("rb") as stream:
                archive.writestr(info, stream.read())
    temp.replace(output)
    print(f"OBB={output}")
    print(f"PACKAGE={args.package_name}")
    print(f"VERSION_CODE={args.version_code}")
    print(f"CONTENT_FILES={len(entries)}")
    print(f"PAYLOAD_BYTES={total_bytes}")
    print(f"ARCHIVE_BYTES={output.stat().st_size}")
    print(f"ARCHIVE_SHA256={sha256_file(output)}")
    return output


def parser() -> argparse.ArgumentParser:
    command = argparse.ArgumentParser(description=__doc__)
    command.add_argument("--input-dir", required=True, help="staged runtime content directory")
    command.add_argument("--output-dir", required=True, help="directory for main.<version>.<package>.obb")
    command.add_argument("--package-name", default="com.darvirgoyt.aethelgrad")
    command.add_argument("--version-code", type=int, default=int(os.environ.get("ANDROID_VERSION_CODE", "3")))
    command.add_argument("--content-version", default="production-v1")
    return command


if __name__ == "__main__":
    try:
        build(parser().parse_args())
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        fail(str(error))
