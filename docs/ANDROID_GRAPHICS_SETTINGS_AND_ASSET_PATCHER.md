# Android Graphics Settings and Asset Patcher

## FPS selection

The Android shell inspects the active display’s supported modes and derives the largest supported refresh rate. The settings dialog exposes only the supported choices among **60 FPS**, **90 FPS**, and **120 FPS**. A device that reports only 60 Hz therefore hides 90 and 120 FPS. The automatic mode selects the highest supported option, while manual mode allows the player to choose a lower supported target for thermal, battery, or stability reasons.

The renderer requests the target cadence through Android’s `Surface.setFrameRate` API on supported OS versions. The native game simulation remains independent of this presentation choice: `GameRenderer` measures real frame delta and `forest_game.cpp` advances gameplay through its fixed 1/60-second accumulator. Selecting 120 FPS must not make the survival clock or combat run twice as fast.

## Graphics quality tiers

| Tier | Prototype mapping | Production direction |
|---|---|---|
| Low | Fewer night stars, fewer rain streaks, reduced lightning intensity, lower effect density | Lowest texture tier, simplified materials, reduced foliage and VFX budgets |
| Medium | Balanced effect counts and lighting accents | Standard mobile texture tier and moderate foliage/particle density |
| High | Full prototype effect density and stronger storm readability | Higher texture resolution, more foliage instances, richer materials and shadows |
| Ultra | Maximum prototype effect density | High-resolution environment packs, enhanced particles, more dynamic lights and higher LOD distances |
| Max | Maximum supported presentation target | Highest device-qualified assets, lighting, animation, and VFX budgets; still subject to thermal and memory limits |

## Asset patch flow

After character setup, the app now displays a post-install preparation overlay backed by `AssetDeliveryManager`. The manager reads a catalog, verifies its Ed25519 signature when configured for production, resolves the selected tier, downloads with HTTP range-resume support, verifies the exact archive byte count and SHA-256 digest, extracts through canonical-path checks and file/size limits, writes a mount marker, and exposes a ready state to the UI. The development build includes tiny `asset://` bundles so this flow can be exercised deterministically without a CDN.

The delivery system is an **asset-delivery foundation**, not a finished high-end content service. A production catalog should be hosted over HTTPS with a real Ed25519 signature and signed, pre-cooked runtime bundles. Android should verify, decompress, and mount those bundles; it should not compile a complete Unreal authoring project from scratch. Prebuilt platform-specific mesh, texture, animation, and shader variants are the safer path for an ARK/Wuthering-Waves-scale experience.

## 3D boundary

The current Android renderer remains a compact procedural GLES3 prototype with quads, triangles, circles, and a normalized simulation view. The graphics settings and patcher are foundations for the desired device-tier architecture, but they do not by themselves convert the existing renderer into an authored 3D world. The future 3D path must add streamed terrain cells, skeletal meshes, physically based materials, foliage LODs, navigation data, animation/VFX bundles, and true dynamic lighting in the production engine.
