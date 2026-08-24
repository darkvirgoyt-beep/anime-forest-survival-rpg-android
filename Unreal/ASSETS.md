# Unreal asset governance

This production tree accepts **only original or appropriately licensed** assets. It does not contain Unreal Engine source code, assets extracted from another game, character likenesses, maps, music, dialogue, or copied UI.

| Asset group | Current production status | Required evidence before player-facing use |
|---|---|---|
| Hero character | No approved production mesh in this tree | Creator/source, commercial license, rig, material setup, LOD policy, animation list, Android memory budget |
| Emberling companion | Gameplay contract only; no final mesh | Original concept, creator/source, license, rig, animation list, LOD and VFX policy |
| Forest region | Procedural placement plus authored source prop kit | Original or licensed tree/rock/ground kit, biome manifest, collision/nav policy, LOD and texture streaming budgets |
| Combat VFX and audio | Code/UI hook points only | Original/licensed source, platform compression settings, loudness target, replacement plan |
| UMG/HUD art | Blueprint-ready C++ boundary only | Original icons, type license, safe-area test, localization and accessibility review |
| Terrain and elevation | No heightmap approved yet; user-selected Google Earth polygon is planning-only | Independent DEM source, license/public-domain notice, boundary ID, crop/resample recipe, elevation range, landscape scale, texture/LOD budget |

Private KML references supplied by the user remain outside this repository. `ORIGINAL_WORLD_REFERENCE_POLICY.md` records the approved high-level creative signals and excludes their geographic coordinates, external overlays, imagery, terrain, and dataset links from game production.

## Forest launch prop import mapping

The Stage 1 forest source kit is at `../assetpack_forest/src/main/assets/launch_slice/forest_prop_kit.obj` with its owned material library `forest_prop_kit.mtl` and contract `forest_prop_kit.json`. Import the named groups `SM_Rock_Large`, `SM_Rock_Mossy`, `SM_Rock_Small`, `SM_Log_Fallen`, `SM_Ruin_*`, `SM_Camp_*`, and `SM_Shrine_*` as separate static meshes under `/Game/Environment/Forest/Props/`. Preserve the three mobile LOD targets from the contract, assign simple convex/box/capsule collision rather than per-poly collision, and use the deterministic placement seed `74291` when wiring the forest-sector landmark and camp/shrine cluster. This source kit is original project-owned geometry; it is not a cooked `.uasset`, and no UE 5.6 import or device profiling is claimed until that licensed environment is available.

## Import gate

Before importing an asset into `Content/`, record its filename, source, creator, license, intended use, target platform memory budget, texture compression, LOD policy, and replacement plan in the project asset manifest. Placeholder geometry is permitted only for local engineering tests and cannot be represented as final player-facing art.
