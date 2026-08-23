#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    plan = (root / "app/src/main/java/com/darvirgoyt/aethelgrad/ContentDownloadPlan.kt").read_text()
    catalog = (root / "app/src/main/java/com/darvirgoyt/aethelgrad/AssetPackCatalog.kt").read_text()
    activity = (root / "app/src/main/java/com/darvirgoyt/aethelgrad/MainActivity.kt").read_text()
    cloud = (root / "app/src/main/cpp/rpg/cloud_state.h").read_text()
    native = (root / "app/src/main/cpp/forest_game.cpp").read_text()
    manifest = json.loads((root / "app/src/main/assets/asset_manifest.json").read_text())
    budget = json.loads((root / "assets/asset_budget.json").read_text())

    require("int schemaVersion = 5" in cloud, "cloud state schema must advance to version 5")
    require("discoveredSectors" in cloud and "parsed.discoveredSectors" in cloud, "cloud state must persist discovered sectors")
    require("sourceSchemaVersion" in cloud and "parsed.sourceSchemaVersion = sourceSchemaVersion" in cloud, "migration must retain source schema metadata")
    require("v4Fields == 22" in cloud and "int discoveredSectors = 1" in cloud and "normalizeDiscoveredSectors" in cloud, "legacy cloud snapshots must default to the launch sector")
    require("enum class WorldSector" in plan, "runtime plan must expose world sectors")
    require("startupPackNamesFor" in plan and "packNamesForSector" in plan, "runtime plan must separate startup and sector packs")
    require("requestWorldSector" in catalog and "sectorContentReady" in catalog, "asset catalog must support sector requests and readiness")
    require("requestDiscoveredSectorContent" in activity, "Android must trigger content requests from discovery state")
    require("sourceSchemaVersion >= 3" in native, "native restore must preserve legacy experience migration")
    require("val discoveredSectors = number(26)" in activity, "Android must consume the native discovery bitmask")
    require("gDiscoveredSectors |= 1 << 1" in native and "gDiscoveredSectors |= 1 << 2" in native, "native movement must unlock sand and snow")
    require("gDiscoveredSectors |= 1 << 3" in native, "native progression must unlock the dungeon sector")
    require("<< gDiscoveredSectors" in native, "HUD must expose discovery state")
    require("state.discoveredSectors = gDiscoveredSectors" in native, "cloud snapshots must include discovery state")

    packs = manifest["packs"]
    by_name = {pack["name"]: pack for pack in packs}
    require(by_name["assetpack_forest"]["delivery"] == "fast-follow", "forest must remain the single fast-follow launch pack")
    require(sum(pack["delivery"] == "fast-follow" for pack in packs) == 1, "manifest must contain one fast-follow pack")
    for name in ("assetpack_sand", "assetpack_snow", "assetpack_dungeons"):
        require(by_name[name]["delivery"] == "on-demand", f"{name} must be on-demand")
        require("expansionTrigger" in by_name[name], f"{name} must declare an expansion trigger")

    budget_packs = {pack["module"]: pack for pack in budget["packs"]}
    require(budget_packs["assetpack_forest"]["delivery"] == "fast-follow", "budget must agree that forest is fast-follow")
    require(sum(pack["delivery"] == "fast-follow" for pack in budget["packs"]) == 1, "budget must contain one fast-follow pack")
    print("PROGRESSIVE_CONTENT_CONTRACT_PASS=1")
    print("SECTOR_BITS=forest:1,sand:2,snow:4,dungeon:8")


if __name__ == "__main__":
    main()
