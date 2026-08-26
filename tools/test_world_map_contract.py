"""Validate the authored 3D world-map and live minimap contract."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/MainActivity.kt"
VIEWS = ROOT / "app/src/main/java/com/darkvirgoyt/aethelgrad/HudOverlayViews.kt"
NATIVE = ROOT / "app/src/main/cpp/forest_game.cpp"
DOC = ROOT / "docs/WORLD_MAP_RUNTIME_CONTRACT.md"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL world_map_contract: {message}")


def main() -> None:
    main = MAIN.read_text(encoding="utf-8")
    views = VIEWS.read_text(encoding="utf-8")
    native = NATIVE.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")

    for needle in (
        "external fun getWorldMapState(): String",
        "private val mapStateUpdater",
        "applyWorldMapState(snapshot)",
        "miniMapView = miniMap",
        "worldMapView = liveMapView",
        "latestWorldMapPlayerState = state",
        "liveMapView.setPlayerState(latestWorldMapPlayerState)",
        "val mapHeight = minOf(dp(560)",
        "NativeGameBridge.setWorldMapVisible(true)",
        "NativeGameBridge.setWorldMapVisible(false)",
        "hudHandler.removeCallbacks(mapStateUpdater)",
    ):
        require(needle in main, f"missing map integration marker: {needle}")

    for needle in (
        "data class WorldMapPlayerState",
        "class AethelgardWorldMapView",
        "class CircularMiniMapView",
        "fun setPlayerState(state: WorldMapPlayerState)",
        "ScaleGestureDetector",
        "ACTION_MOVE",
        "PINCH TO ZOOM",
        "drawRelief",
        "Topographic contour bands",
        "DRAG TO PAN  •  PINCH TO ZOOM",
        "VERDANT VEIL",
        "FROSTWAKE",
    ):
        require(needle in views, f"missing map view marker: {needle}")

    for needle in (
        "Java_com_darvirgoyt_aethelgrad_NativeGameBridge_getWorldMapState",
        "worldXFromSimulation(gPlayerX)",
        "worldYFromSimulation(gPlayerY)",
        "gController.camera.yaw",
        "gDiscoveredSectors",
        "gPlayerX = gController.body.position.x;",
        "gPlayerY = gController.body.position.y;",
        "kSimulationMinX = -1.55f",
        "kSimulationMaxX = 1.55f",
        "kSimulationMinY = -1.00f",
        "kSimulationMaxY = 1.00f",
        "draw3DPlainsAndMountainRanges",
        "minWalkablePosition = {kSimulationMinX, kSimulationMinY}",
        "maxWalkablePosition = {kSimulationMaxX, kSimulationMaxY}",
        "worldXFromSimulation(gPlayerX)",
        "worldYFromSimulation(gPlayerY)",
    ):
        require(needle in native, f"missing native live-map marker: {needle}")

    require("Untitledmapproject.kml" not in "".join(p.name for p in ROOT.rglob("*.kml")), "private KML must not be copied into repo")
    require("Google Earth" not in native and "earthdatalayer" not in native, "native runtime must not consume external map payloads")
    require("private design references" in doc, "runtime source boundary must be documented")
    require("100 × 100 km atlas" in doc, "authored world-map coordinate contract missing")
    require("No KML" in doc, "private-source exclusion acceptance check missing")

    print("WORLD_MAP_CONTRACT_PASS=1")
    print("LIVE_PLAYER_MARKER=enabled")
    print("MINIMAP_AND_FULL_MAP=enabled")
    print("RELIEF_CONTOURS=enabled")
    print("PRIVATE_KML_RUNTIME_BOUNDARY=preserved")


if __name__ == "__main__":
    main()
