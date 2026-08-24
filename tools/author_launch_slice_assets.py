#!/usr/bin/env python3
"""Author the small, original AETHELGRAD mobile launch-slice payload.

This creates usable procedural textures and data descriptors for the first forest
slice. It deliberately does not create padding, cooked Unreal assets, or pretend
the 6.75 GiB high-end archive is complete.
"""
from __future__ import annotations

import json
import math
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def write_texture(path: Path, size: int, pixel) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image = Image.new("RGBA", (size, size))
    image.putdata([pixel(x, y, size) for y in range(size) for x in range(size)])
    image.save(path, optimize=True)


def forest_albedo(x: int, y: int, size: int) -> tuple[int, int, int, int]:
    wave = (math.sin(x * 0.12) + math.sin(y * 0.09) + math.sin((x + y) * 0.035)) / 3
    shade = int(16 * wave)
    return (38 + shade, 92 + shade, 67 + shade, 255)


def forest_roughness(x: int, y: int, size: int) -> tuple[int, int, int, int]:
    value = 120 + int(55 * math.sin(x * 0.07) * math.cos(y * 0.11))
    return (value, value, value, 255)


def river_normal(x: int, y: int, size: int) -> tuple[int, int, int, int]:
    nx = 128 + int(38 * math.sin(y * 0.10) + 18 * math.sin((x + y) * 0.04))
    ny = 128 + int(28 * math.cos(x * 0.08))
    return (max(0, min(255, nx)), max(0, min(255, ny)), 255, 255)


def terrain_height(x: int, y: int, size: int) -> tuple[int, int, int, int]:
    edge = min(x, y, size - 1 - x, size - 1 - y) / (size / 2)
    ridge = 0.5 + 0.22 * math.sin(x * 0.045) + 0.15 * math.cos(y * 0.065)
    value = max(0, min(255, int(255 * (ridge + 0.10 * edge))))
    return (value, value, value, 255)


def foliage_mask(x: int, y: int, size: int) -> tuple[int, int, int, int]:
    cx, cy = size * 0.5, size * 0.44
    radius = math.sqrt(((x - cx) / (size * 0.42)) ** 2 + ((y - cy) / (size * 0.34)) ** 2)
    alpha = max(0, min(255, int((1.0 - radius) * 255)))
    return (64, 156, 100, alpha)


def main() -> None:
    packs = {
        "assetpack_core": [
            "launch_slice/content_contract.json",
        ],
        "assetpack_graphics_base": [
            "launch_slice/forest_albedo.png",
            "launch_slice/forest_roughness.png",
            "launch_slice/river_normal.png",
        ],
        "assetpack_forest": [
            "launch_slice/forest_region.json",
            "launch_slice/water_surface.json",
        ],
        "assetpack_characters": [
            "launch_slice/aurora_palette.png",
            "launch_slice/character_runtime_contract.json",
        ],
        "assetpack_shaders_gles": [
            "launch_slice/gles_material_contract.json",
            "launch_slice/forest_mobile.glsl",
        ],
        "assetpack_world_streaming": [
            "launch_slice/forest_sector_00.json",
        ],
        "assetpack_terrain_lod": [
            "launch_slice/forest_heightfield.png",
        ],
        "assetpack_animation_sets": [
            "launch_slice/aurora_motion_contract.json",
        ],
        "assetpack_foliage_lods": [
            "launch_slice/foliage_cluster.png",
            "launch_slice/foliage_lod_contract.json",
        ],
        "assetpack_audio_hd": [],
        "assetpack_sand": [],
        "assetpack_snow": [],
        "assetpack_dungeons": [],
        "assetpack_hd_textures": [],
        "assetpack_vfx": [],
        "assetpack_cinematics": [],
        "assetpack_voice": [],
        "assetpack_shaders_vulkan": [],
        "assetpack_pipeline_cache": [],
    }

    write_texture(ROOT / "assetpack_graphics_base/src/main/assets/launch_slice/forest_albedo.png", 256, forest_albedo)
    write_texture(ROOT / "assetpack_graphics_base/src/main/assets/launch_slice/forest_roughness.png", 256, forest_roughness)
    write_texture(ROOT / "assetpack_graphics_base/src/main/assets/launch_slice/river_normal.png", 256, river_normal)
    write_texture(ROOT / "assetpack_characters/src/main/assets/launch_slice/aurora_palette.png", 128, lambda x, y, s: (188 + (x % 24), 126 + (y % 18), 82 + ((x + y) % 16), 255))
    write_texture(ROOT / "assetpack_terrain_lod/src/main/assets/launch_slice/forest_heightfield.png", 256, terrain_height)
    write_texture(ROOT / "assetpack_foliage_lods/src/main/assets/launch_slice/foliage_cluster.png", 128, foliage_mask)

    json_assets = {
        "assetpack_core/launch_slice/content_contract.json": {
            "contentId": "aethelgard-forest-launch-v1",
            "status": "authored-launch-slice",
            "world": "forest",
            "runtime": "Android GLES 3",
            "notIncluded": ["cooked Unreal pak", "full 6.75 GiB high-end archive", "future biomes"],
        },
        "assetpack_forest/launch_slice/forest_region.json": {
            "sector": "forest_launch",
            "chunkCoordinates": [[0, 0], [0, 1], [1, 0], [1, 1]],
            "features": ["Heartfire camp", "stream crossing", "gathering nodes", "safe bed", "target dummy"],
            "collision": "procedural-mobile-capsule",
            "navigation": "bounded-grid-v1",
        },
        "assetpack_forest/launch_slice/water_surface.json": {
            "material": "river_normal.png",
            "movement": ["wade", "swim", "shore-exit"],
            "waveModel": "layered-sine-mobile",
            "qualityTiers": ["balanced", "high"],
        },
        "assetpack_characters/launch_slice/character_runtime_contract.json": {
            "character": "Aurora",
            "identity": "original AETHELGRAD hero",
            "states": ["idle", "walk", "sprint", "slide", "dodge", "attack", "heavy", "gather"],
            "palette": "aurora_palette.png",
        },
        "assetpack_shaders_gles/launch_slice/gles_material_contract.json": {
            "backend": "OpenGL ES 3",
            "materials": ["forest_albedo", "forest_roughness", "river_normal"],
            "features": ["rim-light", "biome-fog", "emissive-heartfire", "water-flow"],
        },
        "assetpack_world_streaming/launch_slice/forest_sector_00.json": {
            "sector": "forest_launch",
            "seed": 174031,
            "activeRadiusChunks": 2,
            "streaming": "bounded-pool",
            "saveKey": "forest-launch-v1",
        },
        "assetpack_animation_sets/launch_slice/aurora_motion_contract.json": {
            "character": "Aurora",
            "motions": ["idle", "walk", "sprint", "slide", "dodge", "jump", "light_combo_3", "heavy_finisher"],
            "source": "procedural GLES harness and Unreal presentation bindings",
        },
        "assetpack_foliage_lods/launch_slice/foliage_lod_contract.json": {
            "atlas": "foliage_cluster.png",
            "lods": ["near", "mid", "impostor"],
            "cullDistanceMeters": [18, 42, 76],
        },
    }
    for relative, value in json_assets.items():
        pack, asset_relative = relative.split("/", 1)
        write_json(ROOT / pack / "src/main/assets" / asset_relative, value)

    shader = "// Original AETHELGRAD GLES launch-slice material contract.\\n// Runtime shader integration remains in forest_game.cpp for the dependency-free harness.\\nvoid aethelgard_forest_material() { /* forest_albedo + roughness + river normal */ }\\n"
    shader_path = ROOT / "assetpack_shaders_gles/src/main/assets/launch_slice/forest_mobile.glsl"
    shader_path.parent.mkdir(parents=True, exist_ok=True)
    shader_path.write_text(shader, encoding="utf-8")

    receipt_ledger = []
    for pack, declared in packs.items():
        root = ROOT / pack / "src/main/assets"
        files = sorted(str(path.relative_to(root)) for path in root.rglob("*") if path.is_file() and path.name not in {".gitkeep", "content_receipt.json"})
        receipt_ledger.append({
            "pack": pack,
            "status": "authored-launch-slice" if files else "deferred-unreal-cook",
            "files": files,
            "origin": "original procedural and repository-owned AETHELGRAD content",
            "license": "AETHELGRAD project-owned generated source; no third-party game assets",
        })
        # Keep receipts in the single project ledger below. Repeating the same
        # root filename across asset-pack modules makes bundletool reject the AAB.

    write_json(ROOT / "assets/production_content/pack_receipts.json", {
        "contentId": "aethelgard-forest-launch-v1",
        "status": "authored-launch-slice",
        "packs": receipt_ledger,
        "deferredPacksRemainLocked": True,
        "note": "This ledger documents source ownership; it is not runtime payload and does not count toward byte budgets.",
    })

    audio_pack = ROOT / "assetpack_audio_hd/src/main/assets/launch_slice/audio"
    audio_pack.mkdir(parents=True, exist_ok=True)
    source_audio = ROOT / "assets/audio"
    for source in sorted(source_audio.glob("*")):
        if source.suffix.lower() in {".wav", ".mp3"} and source.name != "sfx_Close_20260822_174648.mp3":
            destination = audio_pack / source.name
            destination.write_bytes(source.read_bytes())
    files = sorted(str(path.relative_to(ROOT / "assetpack_audio_hd/src/main/assets")) for path in audio_pack.rglob("*") if path.is_file())
    audio_receipt = {
        "pack": "assetpack_audio_hd",
        "contentId": "aethelgard-forest-launch-v1",
        "status": "authored-launch-slice",
        "files": files,
        "origin": "original generated/procedural AETHELGRAD audio bank",
        "license": "AETHELGRAD project-owned generated source",
    }
    for entry in receipt_ledger:
        if entry["pack"] == "assetpack_audio_hd":
            entry.update(audio_receipt)
    write_json(ROOT / "assets/production_content/pack_receipts.json", {
        "contentId": "aethelgard-forest-launch-v1",
        "status": "authored-launch-slice",
        "packs": receipt_ledger,
        "deferredPacksRemainLocked": True,
        "note": "This ledger documents source ownership; it is not runtime payload and does not count toward byte budgets.",
    })


if __name__ == "__main__":
    main()
