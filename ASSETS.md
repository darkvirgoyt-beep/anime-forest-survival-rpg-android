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

The Android bootstrap contains only the production account shell, settings, loading scene, native renderer, save schema, and minimum runtime code required before the private high-end archive is mounted. It does not ship a reference-image library or character-photo gallery. The final world meshes, materials, animation graphs, cooked shaders, LODs, audio, VFX, and world-sector data must arrive through the verified HTTPS OBB.

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

The production graphics and delivery plan is documented in [REALISTIC_GRAPHICS_AND_ASSET_DELIVERY.md](docs/REALISTIC_GRAPHICS_AND_ASSET_DELIVERY.md). The authoritative envelope is [assets/asset_budget.json](assets/asset_budget.json), while the private high-end archive currently targets 6,750 MiB across 18 payload groups. The payloads are generated from the shipping Unreal cook and served by the private HTTPS backend; Google Play Console is not required for this distribution path.

The repository does not create dummy files or claim that empty asset directories are finished 10 GiB content. Populate each pack with original or properly licensed models, textures, animations, audio, cinematics, voice, VFX, and cooked Unreal chunks. A production release must fail or remain locked until those signed cooked packs are present. Run `python3 tools/validate_asset_budget.py` before release; it reports actual bytes and rejects over-budget packs without generating padding.

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
