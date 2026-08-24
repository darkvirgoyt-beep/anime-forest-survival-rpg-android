# Drive Asset Review

Reviewed the contents of the Google Drive folder `My game components` (folder ID `1p4tmS1SnFhtEFy5Nj1suI7fNoDESyXdE`) on 2026-08-23.

## Contents identified

| File | Type | Intended role |
|---|---|---|
| `1787473123870.png` | 1408x768 PNG | Silver-haired armored female hero with sword; character portrait/reference. |
| `1787475963776.png` | 1408x768 PNG | Hooded female archer in a neon forest; character portrait/reference and forest mood reference. |
| `1787476000911.png` | 1408x768 PNG | Brown-haired female archer in a bright garden/ruins setting; character portrait/reference and forest settlement mood reference. |
| `1787476845599.png` | 1408x768 PNG | Purple-haired male archer/elf in a neon forest; character portrait/reference. |
| `IMG-20260823-WA0004.jpg` | Portrait JPEG | Blue/white ice creature or boss; snow biome enemy reference. |
| `file_00000000a3988211ac5fdb67308d3525.png` | UI screenshot | AETHELGRAD login screen reference, not a runtime character asset. |
| `Screenshot_20260823-115635_Aethelgard_Wild_Horizons_Crafting.png` | UI screenshot | Crafting/login flow reference, not a runtime character asset. |

## Pack mapping recommendation

The four humanoid references should be treated as character-photo/UI references and, after the user supplies or confirms the corresponding Unreal source meshes/materials, mapped to `assetpack_characters` for high-resolution portraits and character materials. The blue/white creature should be mapped to `assetpack_snow` plus `assetpack_characters` if it becomes a playable or animated enemy. The login and crafting screenshots should guide UI art and remain outside the cooked 3D world packs unless explicitly converted into UI textures.

The images are **reference artwork**, not Unreal `.uasset`, `.umap`, `.pak`, mesh, rig, animation, or texture-source packages. They cannot by themselves create an animated 3D character or a full 6.6 GB world download. The implementation must therefore keep the PAD packs honest: real cooked assets go into the packs; these references can be used for portraits, loading cards, and visual targets after licensing/ownership confirmation.

## Proposed first-launch tiers

- **Low Resources:** optimized mobile textures, reduced foliage/LOD density, GLES shaders, core world sectors, character gameplay meshes and low-resolution portraits.
- **High Resources:** high-resolution texture pages, denser foliage/LOD variants, Vulkan/GLES compiled shader variants, pipeline cache, all world sectors, character high-resolution portraits/materials, snow boss references, VFX, audio, cinematics, and animation sets.

This file is a review record only; the downloaded Drive files are not yet committed as game assets because they are user-provided binary references and require explicit asset-source/licensing confirmation before packaging.
