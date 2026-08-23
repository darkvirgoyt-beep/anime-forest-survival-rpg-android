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

After character setup, the prototype now displays a post-install preparation overlay. The flow communicates the intended production sequence: inspect a signed asset manifest, download the selected 3D world bundle, verify SHA-256 checksums, unpack compressed meshes/textures/materials/animations, build a device shader cache, and mount only the selected quality tier.

The current overlay is a **prototype flow and visual contract**, not a live content-delivery service. It does not yet download real remote bundles or compile Unreal assets on the device. In production, signed asset packs should be delivered through a CDN or platform-compatible asset-delivery system, verified before use, decompressed into app-controlled storage, and mounted through an engine asset registry. Android should not be expected to compile a complete high-end game’s authoring assets from scratch; platform-specific cooked bundles and prebuilt shader variants are the safer path.

## 3D boundary

The current Android renderer remains a compact procedural GLES3 prototype with quads, triangles, circles, and a normalized simulation view. The graphics settings and patcher are foundations for the desired device-tier architecture, but they do not by themselves convert the existing renderer into an authored 3D world. The future 3D path must add streamed terrain cells, skeletal meshes, physically based materials, foliage LODs, navigation data, animation/VFX bundles, and true dynamic lighting in the production engine.
