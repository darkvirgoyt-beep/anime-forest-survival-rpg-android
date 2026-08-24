#!/usr/bin/env python3
"""Author an original, low-poly forest prop kit for the AETHELGRAD launch slice.

The output is intentionally small source geometry plus explicit placement/collision
metadata. It is not a fake high-resolution archive and is designed to be imported
into Unreal as source meshes and then cooked on a licensed UE 5.6 machine.
"""
from __future__ import annotations

import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACK_ROOT = ROOT / "assetpack_forest" / "src/main/assets/launch_slice"
APK_ROOT = ROOT / "app/src/main/assets/launch_slice/assetpack_forest"
OBJ_NAME = "forest_prop_kit.obj"
MTL_NAME = "forest_prop_kit.mtl"
MANIFEST_NAME = "forest_prop_kit.json"

materials = {
    "M_Prop_Rock": (0.24, 0.29, 0.28),
    "M_Prop_Moss": (0.18, 0.33, 0.22),
    "M_Prop_Bark": (0.25, 0.12, 0.055),
    "M_Prop_LogCut": (0.50, 0.28, 0.10),
    "M_Prop_RuinStone": (0.34, 0.37, 0.35),
    "M_Prop_RuinMoss": (0.20, 0.36, 0.24),
    "M_Prop_Ember": (0.78, 0.25, 0.035),
}

vertices: list[tuple[float, float, float]] = []
faces: list[tuple[str, list[int], str]] = []
current_object = "unassigned"


def vertex(x: float, y: float, z: float) -> int:
    vertices.append((x, y, z))
    return len(vertices)


def quad(a: int, b: int, c: int, d: int, material: str) -> None:
    faces.append((material, [a, b, c, d], current_object))


def triangle(a: int, b: int, c: int, material: str) -> None:
    faces.append((material, [a, b, c], current_object))


def add_box(name: str, center: tuple[float, float, float], size: tuple[float, float, float], material: str) -> None:
    cx, cy, cz = center
    sx, sy, sz = (value / 2 for value in size)
    v = [
        vertex(cx - sx, cy - sy, cz - sz), vertex(cx + sx, cy - sy, cz - sz),
        vertex(cx + sx, cy + sy, cz - sz), vertex(cx - sx, cy + sy, cz - sz),
        vertex(cx - sx, cy - sy, cz + sz), vertex(cx + sx, cy - sy, cz + sz),
        vertex(cx + sx, cy + sy, cz + sz), vertex(cx - sx, cy + sy, cz + sz),
    ]
    global current_object
    current_object = name
    object_markers.append(name)
    quad(v[0], v[1], v[2], v[3], material)
    quad(v[4], v[7], v[6], v[5], material)
    quad(v[0], v[4], v[5], v[1], material)
    quad(v[1], v[5], v[6], v[2], material)
    quad(v[2], v[6], v[7], v[3], material)
    quad(v[4], v[0], v[3], v[7], material)


def add_rock(name: str, center: tuple[float, float, float], radius: float, height: float, material: str) -> None:
    cx, cy, cz = center
    sides = 8
    rings = []
    for ring_index, z_factor in enumerate((0.0, 0.42, 0.82, 1.0)):
        ring = []
        for side in range(sides):
            angle = 2 * math.pi * side / sides
            wobble = 0.86 + 0.11 * ((side * 13 + ring_index * 7) % 5) / 4
            ring.append(vertex(cx + math.cos(angle) * radius * wobble, cy + math.sin(angle) * radius * wobble, cz + height * z_factor))
        rings.append(ring)
    bottom = vertex(cx, cy, cz)
    top = vertex(cx + 0.03 * radius, cy - 0.02 * radius, cz + height)
    global current_object
    current_object = name
    object_markers.append(name)
    for side in range(sides):
        next_side = (side + 1) % sides
        quad(rings[0][side], rings[0][next_side], rings[1][next_side], rings[1][side], material)
        quad(rings[1][side], rings[1][next_side], rings[2][next_side], rings[2][side], material)
        quad(rings[2][side], rings[2][next_side], rings[3][next_side], rings[3][side], material)
        triangle(bottom, rings[0][next_side], rings[0][side], material)
        triangle(top, rings[3][side], rings[3][next_side], material)


def add_log(name: str, center: tuple[float, float, float], length: float, radius: float) -> None:
    cx, cy, cz = center
    sides = 10
    left, right = [], []
    for side in range(sides):
        angle = 2 * math.pi * side / sides
        left.append(vertex(cx - length / 2, cy + math.cos(angle) * radius, cz + math.sin(angle) * radius))
        right.append(vertex(cx + length / 2, cy + math.cos(angle) * radius, cz + math.sin(angle) * radius))
    global current_object
    current_object = name
    object_markers.append(name)
    for side in range(sides):
        next_side = (side + 1) % sides
        quad(left[side], left[next_side], right[next_side], right[side], "M_Prop_Bark")
        triangle(left[0], left[next_side], left[side], "M_Prop_LogCut")
        triangle(right[0], right[side], right[next_side], "M_Prop_LogCut")


object_markers: list[str] = []
add_rock("SM_Rock_Large", (-2.0, 0.0, 0.0), 0.85, 1.10, "M_Prop_Rock")
add_rock("SM_Rock_Mossy", (0.0, 0.0, 0.0), 0.58, 0.72, "M_Prop_Moss")
add_rock("SM_Rock_Small", (1.7, 0.0, 0.0), 0.36, 0.42, "M_Prop_Rock")
add_log("SM_Log_Fallen", (0.0, 2.0, 0.52), 2.8, 0.23)
add_box("SM_Ruin_Pillar_Left", (-1.0, 4.0, 1.25), (0.42, 0.48, 2.5), "M_Prop_RuinStone")
add_box("SM_Ruin_Pillar_Right", (1.0, 4.0, 1.25), (0.42, 0.48, 2.5), "M_Prop_RuinMoss")
add_box("SM_Ruin_Arch_Lintel", (0.0, 4.0, 2.40), (2.42, 0.50, 0.42), "M_Prop_RuinStone")
add_box("SM_Ruin_Wall_Segment", (3.0, 4.0, 1.0), (1.8, 0.45, 2.0), "M_Prop_RuinStone")
add_box("SM_Camp_FireRing", (0.0, 6.0, 0.12), (1.2, 1.2, 0.24), "M_Prop_RuinMoss")
add_rock("SM_Camp_EmberCore", (0.0, 6.0, 0.26), 0.26, 0.24, "M_Prop_Ember")
add_box("SM_Camp_WoodPile", (1.15, 6.0, 0.30), (0.75, 0.55, 0.42), "M_Prop_Bark")
add_box("SM_Shrine_Pedestal", (-3.0, 6.0, 0.38), (0.85, 0.85, 0.76), "M_Prop_RuinStone")
add_rock("SM_Shrine_StandingStone", (-3.0, 6.0, 0.76), 0.30, 1.25, "M_Prop_RuinMoss")

mtl_lines = ["# AETHELGRAD original forest prop materials"]
for name, color in materials.items():
    mtl_lines += [f"newmtl {name}", "Ka 0.02 0.02 0.02", f"Kd {color[0]:.3f} {color[1]:.3f} {color[2]:.3f}", "Ks 0.08 0.08 0.08", "Ns 18", ""]

obj_lines = ["# AETHELGRAD original forest launch prop kit", f"mtllib {MTL_NAME}", "o ForestLaunchPropKit"]
for x, y, z in vertices:
    obj_lines.append(f"v {x:.5f} {y:.5f} {z:.5f}")
obj_lines.append("s off")
last_material = None
last_object = None
for material, points, object_name in faces:
    if object_name != last_object:
        obj_lines.append(f"g {object_name}")
        last_object = object_name
    if material != last_material:
        obj_lines.append(f"usemtl {material}")
        last_material = material
    obj_lines.append("f " + " ".join(str(point) for point in points))

manifest = {
    "contentId": "aethelgard-forest-launch-v1",
    "kitId": "forest-prop-kit-v1",
    "source": "original deterministic AETHELGRAD procedural geometry",
    "license": "AETHELGRAD project-owned generated source; no third-party game assets",
    "format": "OBJ source for Unreal import and later UE 5.6 cook",
    "mobileProfile": {"maxTrianglesPerCombinedSource": len(faces) * 2, "lods": ["LOD0 source", "LOD1 60 percent", "LOD2 25 percent"], "collision": "simple convex per prop; no per-poly mobile collision"},
    "props": [
        {"id": "rock_large", "mesh": "SM_Rock_Large", "role": "cover and resource-node anchor", "placement": "forest edge and stream bank", "collision": "convex"},
        {"id": "rock_mossy", "mesh": "SM_Rock_Mossy", "role": "moss detail cluster", "placement": "shade and ruin base", "collision": "convex"},
        {"id": "rock_small", "mesh": "SM_Rock_Small", "role": "scatter detail", "placement": "path edge", "collision": "simple box"},
        {"id": "log_fallen", "mesh": "SM_Log_Fallen", "role": "fallen traversal obstacle", "placement": "forest floor", "collision": "capsule"},
        {"id": "ruin_arch", "mesh": "SM_Ruin_Pillar_Left + SM_Ruin_Pillar_Right + SM_Ruin_Arch_Lintel", "role": "landmark gate", "placement": "forest shrine approach", "collision": "three simple boxes"},
        {"id": "ruin_wall", "mesh": "SM_Ruin_Wall_Segment", "role": "partial ruin cover", "placement": "shrine perimeter", "collision": "box"},
        {"id": "camp_fire_ring", "mesh": "SM_Camp_FireRing + SM_Camp_EmberCore", "role": "camp interactable anchor", "placement": "safe camp clearing", "collision": "flat ring"},
        {"id": "camp_wood_pile", "mesh": "SM_Camp_WoodPile", "role": "restockable camp prop", "placement": "beside fire ring", "collision": "box"},
        {"id": "shrine_pedestal", "mesh": "SM_Shrine_Pedestal", "role": "shrine interaction anchor", "placement": "behind camp clearing", "collision": "box"},
        {"id": "shrine_standing_stone", "mesh": "SM_Shrine_StandingStone", "role": "shrine silhouette", "placement": "on pedestal", "collision": "convex"}
    ],
    "placementContract": {"chunk": "forest-sector-00", "seed": 74291, "minimumSpacingMeters": 2.5, "avoidWaterMeters": 1.5, "ruinLandmarkCount": 1, "campCount": 1},
}

for destination in (PACK_ROOT, APK_ROOT):
    destination.mkdir(parents=True, exist_ok=True)
    (destination / OBJ_NAME).write_text("\n".join(obj_lines) + "\n", encoding="utf-8")
    (destination / MTL_NAME).write_text("\n".join(mtl_lines).rstrip() + "\n", encoding="utf-8")
    (destination / MANIFEST_NAME).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

receipt_path = ROOT / "assets/production_content/pack_receipts.json"
receipt_data = json.loads(receipt_path.read_text(encoding="utf-8"))
forest_receipt = next(entry for entry in receipt_data["packs"] if entry["pack"] == "assetpack_forest")
for relative_name in (f"launch_slice/{OBJ_NAME}", f"launch_slice/{MTL_NAME}", f"launch_slice/{MANIFEST_NAME}"):
    if relative_name not in forest_receipt["files"]:
        forest_receipt["files"].append(relative_name)
receipt_path.write_text(json.dumps(receipt_data, indent=2) + "\n", encoding="utf-8")

print(f"WROTE {OBJ_NAME}, {MTL_NAME}, {MANIFEST_NAME}")
print(f"VERTICES={len(vertices)} FACES={len(faces)}")
print(f"OBJ_BYTES={(PACK_ROOT / OBJ_NAME).stat().st_size}")
