# AETHELGRAD Forest Launch Slice

This directory records the first **authored production-content slice** for AETHELGRAD Android. It is original project-owned procedural content intended for the GLES/mobile harness and the initial asset-pack wiring.

The launch slice contains a forest-sector descriptor, water movement/material data, original mobile textures, a terrain heightfield, foliage LOD data, Aurora motion and palette data, GLES material metadata, the original/procedural forest audio bank, and the authored `forest_prop_kit` source kit. The prop kit contains original low-poly rocks, a fallen log, a ruin arch and wall, a camp fire ring/ember/wood pile, and a small shrine pedestal/standing stone. Its OBJ groups, material library, collision approximations, three LOD targets, and deterministic placement seed are recorded in `forest_prop_kit.json`. The payload is distributed into the corresponding Gradle Play Asset Delivery modules under `assetpack_*/src/main/assets/launch_slice/`.

Stage 1 has a truthful **1 GiB source-budget envelope**. The currently authored payload is exactly **3,353,935 bytes (approximately 3.20 MiB)** of real content; the remaining Stage 1 budget is reserved for later authored packs and is never filled with padding. The separate full high-end Unreal plan remains deferred and is not presented as installed or downloadable.

The source OBJ/MTL prop kit is production-ready authored source geometry and is not a cooked Unreal asset. The following remain deferred until a real Unreal Engine 5.6 cook is run in a licensed Unreal build environment: imported `.uasset` static meshes and collision, `.umap` world sectors, cooked `.pak`/`.ucas`/`.utoc` files, production skeletal meshes and animation graphs, Niagara packages, platform shader libraries, cinematics, voice, and the future sand, snow, and dungeon expansions. Those packs remain deferred and must stay locked until their signed cooked payloads and source receipts exist.

Run the following checks after authoring or replacing content:

```bash
python3 tools/validate_asset_budget.py
python3 tools/validate_asset_budget.py --manifest assets/full_content_budget.json --require-nonempty --require-target
python3 tools/validate_runtime_content_package.py
python3 tools/test_full_content_build_contract.py
```

A release-quality high-end archive still requires `tools/build_full_content.sh` with a trusted cooked Unreal output, a generated expansion archive, exact SHA-256 verification, and private HTTPS publication. The launch slice is not a substitute for that final cook.
