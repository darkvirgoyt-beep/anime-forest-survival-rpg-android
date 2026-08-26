#!/usr/bin/env python3
"""Regression checks for the cooked asset-pack size measurement tool."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools/measure_cooked_asset_packs.py"


def write_json(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value), encoding="utf-8")


def run(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run([sys.executable, str(TOOL), *args], text=True, capture_output=True, check=False)


def main() -> None:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        staging = root / "stage/asset_packs"
        cook = root / "cook"
        for pack in ("assetpack_core", "assetpack_optional"):
            (staging / pack).mkdir(parents=True)
        (cook / "Content/Paks").mkdir(parents=True)
        (staging / "assetpack_core/Content/Paks").mkdir(parents=True)
        (staging / "assetpack_optional/Content/Paks").mkdir(parents=True)
        (staging / "assetpack_core/Content/Paks/core.pak").write_bytes(b"core")
        (staging / "assetpack_optional/Content/Paks/optional.pak").write_bytes(b"optional")
        (cook / "Content/Paks/core.pak").write_bytes(b"core")
        (cook / "Content/Paks/optional.pak").write_bytes(b"optional")
        budget = root / "budget.json"
        mapping = root / "mapping.json"
        report = root / "report.json"
        write_json(
            budget,
            {
                "target_total_mib": 3,
                "policy": {"documented_per_dynamic_pack_ceiling_mib": 2},
                "packs": [
                    {"module": "assetpack_core", "delivery": "install-time", "target_mib": 1},
                    {"module": "assetpack_optional", "delivery": "on-demand", "target_mib": 2},
                ],
            },
        )
        write_json(mapping, {"assetpack_core": ["**/core.pak"], "assetpack_optional": ["**/optional.pak"]})
        passed = run("--staging-root", str(root / "stage"), "--budget-manifest", str(budget), "--mapping-file", str(mapping), "--cook-root", str(cook), "--require-nonempty", "--report-json", str(report))
        assert passed.returncode == 0, passed.stderr
        assert "COOKED_ASSET_PACK_BUDGET_PASS=1" in passed.stdout
        assert json.loads(report.read_text(encoding="utf-8"))["actualTotalBytes"] == len(b"coreoptional")

        (staging / "assetpack_unknown").mkdir()
        unknown = run("--staging-root", str(root / "stage"), "--budget-manifest", str(budget), "--mapping-file", str(mapping))
        assert unknown.returncode != 0 and "unassigned" in unknown.stderr
        (staging / "assetpack_unknown").rmdir()

        (staging / "assetpack_optional/Content/Paks/oversized.pak").write_bytes(b"x" * (2 * 1024 * 1024 + 1))
        oversized = run("--staging-root", str(root / "stage"), "--budget-manifest", str(budget), "--mapping-file", str(mapping))
        assert oversized.returncode != 0 and "above its 2 MiB budget" in oversized.stderr

        (staging / "assetpack_optional/Content/Paks/oversized.pak").unlink()
        (cook / "Content/Paks/unmapped.pak").write_bytes(b"unmapped")
        unassigned = run("--staging-root", str(root / "stage"), "--budget-manifest", str(budget), "--mapping-file", str(mapping), "--cook-root", str(cook))
        assert unassigned.returncode != 0 and "unassigned cooked runtime files" in unassigned.stderr

    print("MEASURE_COOKED_ASSET_PACKS_TEST_PASS=1")


if __name__ == "__main__":
    main()
