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

The current scene uses procedural OpenGL ES geometry and shader colors for the forest backdrop, trees, rocks, resources, animals, moon, combat pulse, and the hero. These elements are code-generated and do not embed third-party artwork. The hero is an original cel-shaded anime-fantasy design rendered as layered flat-color polygons and circles: a deep-violet hair silhouette, warm skin planes, a plum tunic with hard shadow blocks, teal sash accents, ink-like outlines, and a short gold-edged sword.

The concept references are also bundled into `app/src/main/res/drawable-nodpi/` as `reference_*` resources. The onboarding screen exposes them through the **VIEW ART REFERENCES** library so the board, player expressions, environments and lighting, creatures, assets and weapons, and UI target can be reviewed on-device. These are reference images for the prototype and are not used as final runtime 3D models or textures.

## Planned original assets

| Asset | Intended use | Status |
|---|---|---|
| Original cel-shaded hero silhouette | Third-person exploration and combat in the Android vertical slice | Included as procedural, original OpenGL ES geometry in `app/src/main/cpp/forest_game.cpp`; hard-edged shadow planes and ink outlines are intentional mobile-safe art direction. |
| Two animal models | Passive and hostile encounters | Planned; must be original or properly licensed. |
| Forest texture atlas | Terrain, foliage, and props | Planned; optimize for mobile GPU and texture memory. |
| UI icon set | Inventory, crafting, resources, and abilities | Planned; original vector or generated artwork. |
| Ambient audio | Wind, water, wildlife, and combat cues | Planned; original or compatible license. |

The design references broad anime-fantasy and survival-RPG aesthetics only. Do not import or recreate named characters, logos, maps, or assets from commercial games.
