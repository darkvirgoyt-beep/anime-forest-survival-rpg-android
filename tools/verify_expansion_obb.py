#!/usr/bin/env python3
"""Verify a standalone Aethelgard expansion OBB without extracting it."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import zipfile
from pathlib import PurePosixPath

OBB_FORMAT = "aethelgard-expansion-obb-v1"
NAME_RE = re.compile(r"^main\.(?P<version>[1-9][0-9]*)\.(?P<package>[A-Za-z][A-Za-z0-9_]*(?:\.[A-Za-z][A-Za-z0-9_]*)*)\.obb$")


def digest(stream) -> str:
    hasher = hashlib.sha256()
    for chunk in iter(lambda: stream.read(1024 * 1024), b""):
        hasher.update(chunk)
    return hasher.hexdigest()


def verify(path: str, expected_package: str | None = None, expected_version: int | None = None) -> None:
    archive_path = __import__("pathlib").Path(path)
    match = NAME_RE.fullmatch(archive_path.name)
    if not match:
        raise SystemExit("ERROR: filename must be main.<versionCode>.<packageName>.obb")
    with zipfile.ZipFile(archive_path) as archive:
        names = archive.namelist()
        if "obb_manifest.json" not in names:
            raise SystemExit("ERROR: obb_manifest.json is missing")
        manifest = json.loads(archive.read("obb_manifest.json"))
        if manifest.get("format") != OBB_FORMAT:
            raise SystemExit("ERROR: unsupported OBB manifest format")
        if str(manifest.get("versionCode")) != match.group("version"):
            raise SystemExit("ERROR: filename versionCode does not match manifest")
        if manifest.get("packageName") != match.group("package"):
            raise SystemExit("ERROR: filename package name does not match manifest")
        if expected_package is not None and match.group("package") != expected_package:
            raise SystemExit("ERROR: OBB package does not match the APK package")
        if expected_version is not None and int(match.group("version")) != expected_version:
            raise SystemExit("ERROR: OBB versionCode does not match the APK versionCode")

        entries = manifest.get("entries")
        if not isinstance(entries, list) or not entries:
            raise SystemExit("ERROR: OBB contains no runtime content entries")
        indexed = {entry.get("path"): entry for entry in entries}
        payload_bytes = 0
        actual_content = []
        for info in archive.infolist():
            if info.is_dir() or info.filename == "obb_manifest.json":
                continue
            pure = PurePosixPath(info.filename)
            if pure.is_absolute() or ".." in pure.parts or not info.filename.startswith("content/"):
                raise SystemExit(f"ERROR: unsafe or unexpected archive path: {info.filename}")
            entry = indexed.get(info.filename)
            if entry is None:
                raise SystemExit(f"ERROR: archive file is absent from manifest: {info.filename}")
            if info.file_size != int(entry["byteSize"]):
                raise SystemExit(f"ERROR: byte size mismatch: {info.filename}")
            with archive.open(info, "r") as stream:
                actual_hash = digest(stream)
            if actual_hash.lower() != str(entry["sha256"]).lower():
                raise SystemExit(f"ERROR: SHA-256 mismatch: {info.filename}")
            payload_bytes += info.file_size
            actual_content.append(info.filename)

        if len(actual_content) != len(entries):
            raise SystemExit("ERROR: manifest and archive file counts differ")
        if payload_bytes != int(manifest.get("payloadBytes", -1)):
            raise SystemExit("ERROR: payload byte total mismatch")
        if len(actual_content) != int(manifest.get("fileCount", -1)):
            raise SystemExit("ERROR: payload file count mismatch")

    print("EXPANSION_OBB_VERIFIED=1")
    print(f"FILE={archive_path}")
    print(f"PACKAGE={match.group('package')}")
    print(f"VERSION_CODE={match.group('version')}")
    print(f"CONTENT_FILES={len(actual_content)}")
    print(f"PAYLOAD_BYTES={payload_bytes}")
    print(f"ARCHIVE_BYTES={archive_path.stat().st_size}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("obb")
    parser.add_argument("--expected-package", help="require this Android package name")
    parser.add_argument("--expected-version", type=int, help="require this Android versionCode")
    args = parser.parse_args()
    try:
        verify(args.obb, args.expected_package, args.expected_version)
    except (OSError, ValueError, KeyError, json.JSONDecodeError, zipfile.BadZipFile) as error:
        raise SystemExit(f"ERROR: {error}")
