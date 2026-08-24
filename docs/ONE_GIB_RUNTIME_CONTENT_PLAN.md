# Aethelgrad One-GiB Runtime Content Package

The optional `aethelgard-authored-runtime-v1` archive targets **1,024 MiB** of real runtime material. It is not padding and does not block the bundled Android world. Every incoming binary must have a source file, a license or ownership receipt, and a cooked mobile runtime output.

| Group | Target | Runtime purpose |
|---|---:|---|
| Environment textures | 240 MiB | Biome materials, decals, water, terrain surfaces |
| World sectors | 200 MiB | Cooked sector maps, collision, streaming and navigation |
| Characters and animation | 180 MiB | Original heroes, creatures, rigs and combat/traversal animation |
| Audio and voice | 130 MiB | Music, ambience, combat, wildlife and dialogue |
| Cinematics | 90 MiB | Story video, camera data and playable sequence support |
| VFX and shaders | 70 MiB | Weather, impacts, river effects and device shader variants |
| Foliage and props | 80 MiB | Instanced foliage, structures, pickups and mobile LODs |
| Navigation and pipeline | 34 MiB | Nav data, terrain LOD and pipeline-cache seeds |
| **Total** | **1,024 MiB** | **Optional authored runtime archive** |

The archive is considered release-ready only when `tools/validate_runtime_content_package.py --require-authored-payload` passes against licensed source payloads. Large binary sources should be stored in the designated private content archive or Git LFS rather than fabricated in Git history.
