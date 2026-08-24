# AETHELGRAD world-map runtime contract

## Purpose

The two user-supplied KML files are private design references. They communicate broad interests in forest, snow, water, vegetation, and relief. They are not copied into the repository, bundled into Android, fetched at runtime, or used as a source of Google Earth imagery, map tiles, elevation values, real-world coordinates, roads, localities, or terrain geometry.

The shipped map is the fictional **Aethelgard 100 × 100 km atlas** already defined by `docs/WORLD_MAP_100X100.md`. The Android GLES harness and the future Unreal 5.6 world share this logical coordinate contract. The 3D presentation is art-directed: biome bands, raised relief silhouettes, topographic contour lines, river corridors, authored routes, landmarks, and a live player marker are generated from project-owned data.

## Runtime layers

| Layer | Runtime owner | Contract |
|---|---|---|
| 3D terrain | Native GLES harness now; Unreal Landscape/World Partition in production | Deterministic seeded height field with visible collision in the production map; no external terrain tiles |
| Biomes | Shared world-coordinate mapping | Forest `0–34 km`, Sand `34–68 km`, Snow `68–100 km` |
| Landmarks | Shared authored atlas | Camp, Village, Cave, Gate, Kiln, Oasis, Frost Gate, Basin, and Arena |
| Route and water | Native map/world renderer now; authored spline/water actors in Unreal | Gold traversal route and cyan river corridor; water movement remains an authored gameplay/presentation system |
| Minimap | `CircularMiniMapView` | Displays biome, river, player position, and heading; opens the world map |
| Full map | `AethelgardWorldMapView` | Displays relief, contours, discovered landmarks, live player marker, pan, and pinch zoom |
| Persistence/authority | Existing local guest and hosted Google paths | Map position is local gameplay state in guest mode and follows the existing authoritative co-op state boundary online |

## Live state format

`NativeGameBridge.getWorldMapState()` returns a compact pipe-delimited snapshot:

```text
world_x_km|world_y_km|camera_yaw_degrees|discovered_sector_mask
```

The Kotlin views consume this snapshot only for presentation. They do not own movement, collision, teleport authority, quest completion, or multiplayer state.

## Acceptance checks

The map increment is complete only when the following remain true:

1. The player marker moves as the character traverses the world.
2. The marker heading follows the current camera yaw.
3. The minimap and full map use the same world-coordinate conversion.
4. Full-map pan and pinch zoom do not alter gameplay input.
5. The current HUD, login modes, package identity, API routes, JNI prefix, and map visibility bridge remain intact.
6. No KML, Google Earth URL, Earth tile identifier, satellite image, or external terrain payload is added to the repository or APK.
7. Android build and native/static contract tests pass before a phone artifact is reported.

This is a production foundation increment, not a claim that a final Unreal Landscape, cooked `.uasset`/`.umap` world, navigation mesh, or signed AAA content archive exists in the sandbox.
