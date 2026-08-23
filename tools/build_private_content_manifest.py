#!/usr/bin/env python3
"""Create the server manifest consumed by the private Android content downloader."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

NAME_RE = re.compile(r"^main\.(?P<version>[1-9][0-9]*)\.(?P<package>[A-Za-z][A-Za-z0-9_]*(?:\.[A-Za-z][A-Za-z0-9_]*)*)\.obb$")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("obb", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--archive-url", default="")
    args = parser.parse_args()

    if not args.obb.is_file():
        raise SystemExit(f"ERROR: OBB does not exist: {args.obb}")
    match = NAME_RE.fullmatch(args.obb.name)
    if not match:
        raise SystemExit("ERROR: OBB filename must be main.<versionCode>.<packageName>.obb")
    payload = {
        "format": "aethelgard-private-content-manifest-v1",
        "contentTier": "high",
        "packageName": match.group("package"),
        "versionCode": int(match.group("version")),
        "archiveBytes": args.obb.stat().st_size,
        "archiveSha256": sha256(args.obb),
    }
    if args.archive_url:
        if not args.archive_url.startswith("https://"):
            raise SystemExit("ERROR: archive URL must use HTTPS")
        payload["archiveUrl"] = args.archive_url
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(payload, indent=2))


if __name__ == "__main__":
    main()
