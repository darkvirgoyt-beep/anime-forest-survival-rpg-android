# Asset Manifest

## Shared visual reference

| Asset | Intended use | Status |
|---|---|---|
| [Game visual reference board](docs/art_reference/game_visual_reference.png) | Shared direction for anime-fantasy characters, farming forest, populated sand settlement, snow predators, lighting phases, UI, and color palette | User-provided reference copied into the repository; use for style guidance only, not asset reproduction. |

## Generated concept references

| Asset | Intended use | Status |
|---|---|---|
| [Player character and emotions](docs/art_reference/generated/player_character_emotions.png) | Player identity, skin tone, costume, front/back/side views, and neutral/happy/sad/surprised/angry expressions | Generated concept reference; not yet wired as a runtime texture. |
| [Environment and lighting times](docs/art_reference/generated/environment_lighting_times.png) | Forest village, farming, monument, and day/afternoon/evening/night lighting targets | Generated concept reference; current renderer approximates the palette procedurally. |
| [Enemy, creatures, and boss](docs/art_reference/generated/enemy_creatures_boss.png) | Boar, deer, snow wolf, leafy spirit, and Frostclaw boss silhouettes | Generated concept reference; current renderer contains a procedural snow predator approximation. |
| [Assets, weapons, and monument](docs/art_reference/generated/assets_weapons_monument.png) | House, drying rack, cooking pot, banner, nature props, monument, sword, bow, spear, and great blade | Generated concept reference; runtime assets remain planned for the 3D production path. |
| [UI gameplay reference](docs/art_reference/generated/ui_gameplay_reference.png) | Minimap, player bars, quest box, time panel, abilities, inventory, and readable HUD hierarchy | Generated concept reference; current Android HUD implements a lightweight native subset. |

## Included in v0.1

The Android bootstrap contains the production account shell, settings, loading scene, native renderer, save schema, and the authored **forest launch slice**. The launch slice is distributed into the Gradle asset-pack modules and includes original mobile textures, terrain heightfield data, foliage LOD data, water movement/material descriptors, Aurora motion/palette data, GLES material metadata, the original/procedural forest audio bank, a deterministic low-poly `forest_prop_kit` source set for rocks, logs, ruins, camp, and shrine props, and the derived Forest Warden boss LOD source. Its measured payload is exactly 4,803,743 bytes (approximately 4.58 MiB); it is real content, not budget padding.

The launch slice is not the complete high-end Unreal archive. The final cooked world meshes, materials, animation graphs, LODs, platform shaders, cinematics, voice, and future biome sectors must arrive through a trusted Unreal cook and a verified Play Asset Delivery or HTTPS OBB release before those expansions are unlocked.

## Forest launch prop kit

| Prop family | Authored source | Mobile contract | Runtime/import boundary |
|---|---|---|---|
| Rocks | `assetpack_forest/src/main/assets/launch_slice/forest_prop_kit.obj` groups `SM_Rock_*` | Low-poly silhouettes, convex or simple-box collision, three LOD targets | Mirrored into the direct APK source tree; requires Unreal import and cook for `.uasset` use |
| Fallen log | `SM_Log_Fallen` with bark/cut materials | Capsule collision, traversal obstacle, three LOD targets | Descriptor is integrated into `forest_region.json`; cooked collision remains deferred |
| Ruin arch and wall | `SM_Ruin_*` groups | Simple box collision, one landmark arch and one cover segment | Original source geometry only; no fake cooked world is claimed |
| Camp/shrine cluster | `SM_Camp_*` and `SM_Shrine_*` groups | Camp anchor plus pedestal/standing-stone interaction silhouette | Deterministic placement seed `74291`, protected from water by the placement contract |

The authoritative source, ownership, LOD, collision, and placement metadata is `assetpack_forest/src/main/assets/launch_slice/forest_prop_kit.json`; its receipt is centralized in `assets/production_content/pack_receipts.json`. The OBJ/MTL files are authored source stand-ins for the Android launch slice. A licensed Unreal Engine 5.6 import/cook and device profiling pass is still required before these become cooked `.uasset` static meshes.

## Forest Warden boss asset

The supplied `Hi3D_Untitled_allparts_20260824_215831.zip` was inspected as source data. Its raw `model.obj` is approximately 2,000,000 triangles and 212 MB, so it is deliberately not copied into the repository or Android package. A derived mobile source set is included at `assetpack_characters/src/main/assets/launch_slice/`: near LOD at 16,000 triangles, mid LOD at 8,000 triangles, one diffuse material, and a simple capsule collision contract. The original Aurora player character and its `character_runtime_contract.json` remain preserved; the Forest Warden is a separate boss asset and does not replace the player.

The authoritative boss mapping, provenance, mobile limits, and Unreal import boundary are in `forest_warden_boss_asset.json`. The files are user-supplied/derived source pending final commercial-rights confirmation; the raw source textures and raw `model.obj` are not included. The Android GLES build continues to use the existing procedural `draw3DForestWarden` fallback until a licensed Unreal 5.6 import, rig/animation pass, collision setup, cooked `.uasset`, and device profiling pass are completed.

## Planned original assets

| Asset | Intended use | Status |
|---|---|---|
| Original cel-shaded hero silhouette | Third-person exploration and combat in the Android vertical slice | Included as procedural, original OpenGL ES geometry in `app/src/main/cpp/forest_game.cpp`; hard-edged shadow planes and ink outlines are intentional mobile-safe art direction. |
| Two animal models | Passive and hostile encounters | Planned; must be original or properly licensed. |
| Forest texture atlas | Terrain, foliage, and props | Planned; optimize for mobile GPU and texture memory. |
| UI icon set | Inventory, crafting, resources, and abilities | Planned; original vector or generated artwork. |
| Ambient audio | Wind, water, wildlife, and combat cues | Planned; original or compatible license. |

The design references broad anime-fantasy and survival-RPG aesthetics only. Do not import or recreate named characters, logos, maps, or assets from commercial games.


## Realistic graphics and large-content delivery

The production graphics and delivery plan is documented in [REALISTIC_GRAPHICS_AND_ASSET_DELIVERY.md](docs/REALISTIC_GRAPHICS_AND_ASSET_DELIVERY.md). The authoritative current release budget is the Stage 1 **1 GiB** manifest at [assets/asset_budget.json](assets/asset_budget.json); the separate deferred full-cook plan is [assets/full_content_budget.json](assets/full_content_budget.json). The Stage 1 payload is generated from original project-owned content, while future Unreal-cooked payloads require a licensed cook and signed publication.

The repository does not create dummy files or claim that empty asset directories are finished content. Stage 1 reports its measured authored bytes separately from the 1 GiB plan. Populate each later pack with original or properly licensed models, textures, animations, audio, cinematics, voice, VFX, and cooked Unreal chunks before publishing it. Run `python3 tools/validate_asset_budget.py` for Stage 1, or pass `--manifest assets/full_content_budget.json --require-nonempty --require-target` for the strict full cook; both reject over-budget or undersized content without generating padding.

The Unreal graphics baseline is in `Unreal/Config/DefaultEngine.ini`. It enables a mobile-realistic PBR/HDR baseline with controlled lights, shadows, atmosphere, virtual-texture support, and streaming while leaving desktop-only ray-traced features disabled for the Android profile. Final AAA presentation still requires authored assets, Unreal material and animation graphs, LODs, quality tiers, and device profiling.

## Aurora Vale 3D presentation integration

The Unreal presentation layer is implemented in `Unreal/Source/ForestSlice/Public/ForestSlicePresentationComponent.h` and `Private/ForestSlicePresentationComponent.cpp`. Assign original or properly licensed Unreal assets to the component’s soft-reference cue set on the Aurora character Blueprint.

| Cue | Unreal asset target | Runtime use |
|---|---|---|
| Light attack montage | `/Game/Characters/Aurora/Animations/AM_Aurora_LightCombo` | `Light_01`, `Light_02`, and `Light_03` sections selected from combat event IDs |
| Heavy attack montage | `/Game/Characters/Aurora/Animations/AM_Aurora_HeavyFinisher` | `Heavy_01` section and authoritative heavy-attack event |
| Dodge / slide / jump montages | `/Game/Characters/Aurora/Animations/AM_Aurora_Dodge`, `AM_Aurora_Slide`, `AM_Aurora_Jump` | Mobile and Enhanced Input movement actions |
| Sword trail | `/Game/VFX/Aurora/NS_Aurora_SwordTrail` | Short-lived Niagara system attached to `hand_r_socket` during attack presentation |
| Hit burst | `/Game/VFX/Aurora/NS_Aurora_SwordHitBurst` | Local presentation burst on `HitConfirmed`; damage remains server-authoritative |
| Weapon switch burst | `/Game/VFX/Aurora/NS_Aurora_WeaponSwitchBurst` | Niagara burst and magic pulse on weapon change |
| Sword whoosh | `/Game/Audio/SFX/sfx_attack_sword` | Attack start cue; source file is `assets/audio/sfx_attack_sword.wav` |
| Footstep | `/Game/Audio/SFX/sfx_footsteps_forest` | Locomotion notify cue; source file is `assets/audio/sfx_footsteps_forest.wav` |
| Dodge whoosh | `/Game/Audio/SFX/sfx_slide` | Dodge and slide presentation cue; source file is `assets/audio/sfx_slide.wav` |
| Magic pulse | `/Game/Audio/SFX/sfx_aurora_magic_pulse` | Original or licensed crystalline/magical cue for weapon/VFX transitions |
| Sword hit | `/Game/Audio/SFX/sfx_aurora_sword_hit` | Original or licensed impact cue for confirmed hit presentation |

The runtime component intentionally uses `TSoftObjectPtr` references and does not embed third-party or copied game assets. Asset import, socket verification, animation-notify authoring, Niagara parameter binding, LOD setup, and device profiling remain required content steps before a production release.

## 2026 visual enhancement target

| Asset | Intended use | Status |
|---|---|---|
| [`assets/aethelgard_visual_upgrade_target.png`](assets/aethelgard_visual_upgrade_target.png) | In-game screenshot target for the Android GLES visual pass: teal moonlit forest, warm Heartfire camp, reflective stream, readable Forest Warden silhouette, premium mobile HUD hierarchy, emissive VFX, and atmospheric depth | Generated original reference; used as art-direction guidance and not loaded as a runtime texture |

## Scoped runtime visual pass

The Android slice will stay dependency-free and GLES 3 compatible. The implementation focuses on procedural upgrades that are visible on-device: per-face stylized lighting with rim response and biome-tinted fog, layered terrain and canopy silhouettes, emissive ground rings and motes for camp/teleport/combat feedback, richer rain and lightning response, and a compact HUD color hierarchy. Existing JNI names, gameplay state serialization, touch controls, and graphics-tier contracts remain unchanged.

All new visual forms are original procedural geometry and shader logic. The generated screenshot is a target reference only; it is not copied into the runtime scene.
