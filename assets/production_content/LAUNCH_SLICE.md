# AETHELGRAD Forest Launch Slice

This directory records the first **authored production-content slice** for AETHELGRAD Android. It is original project-owned procedural content intended for the GLES/mobile harness and the initial asset-pack wiring.

The launch slice contains a forest-sector descriptor, water movement/material data, original mobile textures, a terrain heightfield, foliage LOD data, Aurora motion and palette data, GLES material metadata, and the existing original/procedural audio bank. The payload is distributed into the corresponding Gradle Play Asset Delivery modules under `assetpack_*/src/main/assets/launch_slice/`.

The measured payload is approximately **3.22 MiB**, which is real content and is intentionally far below the previously planned 7,108 MiB asset envelope. The repository does not create padding and does not claim that the complete 6,750 MiB high-end Unreal archive is finished.

The following remain deferred until a real Unreal Engine 5.6 cook is run in a licensed Unreal build environment: `.uasset` and `.umap` world sectors, cooked `.pak`/`.ucas`/`.utoc` files, production skeletal meshes and animation graphs, Niagara packages, platform shader libraries, cinematics, voice, and the future sand, snow, and dungeon expansions. Those packs remain on-demand and must stay locked until their signed cooked payloads and source receipts exist.

Run the following checks after authoring or replacing content:

```bash
python3 tools/validate_asset_budget.py --require-nonempty
python3 tools/validate_runtime_content_package.py
python3 tools/test_full_content_build_contract.py
```

A release-quality high-end archive still requires `tools/build_full_content.sh` with a trusted cooked Unreal output, a generated expansion archive, exact SHA-256 verification, and private HTTPS publication. The launch slice is not a substitute for that final cook.
