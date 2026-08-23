# Asset Manifest

## Included in v0.1

The current scene uses procedural OpenGL ES geometry and shader colors for the forest backdrop, trees, rocks, resources, animals, moon, combat pulse, and the hero. These elements are code-generated and do not embed third-party artwork. The hero is an original cel-shaded anime-fantasy design rendered as layered flat-color polygons and circles: a deep-violet hair silhouette, warm skin planes, a plum tunic with hard shadow blocks, teal sash accents, ink-like outlines, and a short gold-edged sword.

## Planned original assets

| Asset | Intended use | Status |
|---|---|---|
| Original cel-shaded hero silhouette | Third-person exploration and combat in the Android vertical slice | Included as procedural, original OpenGL ES geometry in `app/src/main/cpp/forest_game.cpp`; hard-edged shadow planes and ink outlines are intentional mobile-safe art direction. |
| Two animal models | Passive and hostile encounters | Planned; must be original or properly licensed. |
| Forest texture atlas | Terrain, foliage, and props | Planned; optimize for mobile GPU and texture memory. |
| UI icon set | Inventory, crafting, resources, and abilities | Planned; original vector or generated artwork. |
| Ambient audio | Wind, water, wildlife, and combat cues | Planned; original or compatible license. |

The design references broad anime-fantasy and survival-RPG aesthetics only. Do not import or recreate named characters, logos, maps, or assets from commercial games.
