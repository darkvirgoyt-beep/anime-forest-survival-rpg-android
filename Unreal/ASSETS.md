# Unreal asset governance

This production tree accepts **only original or appropriately licensed** assets. It does not contain Unreal Engine source code, assets extracted from another game, character likenesses, maps, music, dialogue, or copied UI.

| Asset group | Current production status | Required evidence before player-facing use |
|---|---|---|
| Hero character | No approved production mesh in this tree | Creator/source, commercial license, rig, material setup, LOD policy, animation list, Android memory budget |
| Emberling companion | Gameplay contract only; no final mesh | Original concept, creator/source, license, rig, animation list, LOD and VFX policy |
| Forest region | Procedural placement contract only | Original or licensed tree/rock/ground kit, biome manifest, collision/nav policy, LOD and texture streaming budgets |
| Combat VFX and audio | Code/UI hook points only | Original/licensed source, platform compression settings, loudness target, replacement plan |
| UMG/HUD art | Blueprint-ready C++ boundary only | Original icons, type license, safe-area test, localization and accessibility review |

## Import gate

Before importing an asset into `Content/`, record its filename, source, creator, license, intended use, target platform memory budget, texture compression, LOD policy, and replacement plan in the project asset manifest. Placeholder geometry is permitted only for local engineering tests and cannot be represented as final player-facing art.
