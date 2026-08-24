# Original world reference policy

## Private reference intake

The user supplied two private KML files, `Untitledmapproject.kml` and `forest.kml`, as creative layout references. They are **not** copied into this repository, imported into Unreal, bundled in an APK, distributed to players, or used as an elevation/imagery source.

The review found broad user-authored biome and boundary labels plus several external Earth overlay references. Those overlays, their source links, map tiles, geographic coordinates, imagery, elevation data, and real-world feature geometry are excluded from all Aethelgard assets and source files.

## Permitted design signals

The original world brief may use only high-level, non-geographic creative intent expressed by the user:

| Reference signal | Original Aethelgard translation |
|---|---|
| Cold-region interest | The Frostwake Crown: an invented high alpine biome with glacial caves and aurora ruins. |
| Forest-region interest | The Verdant Veil: an invented temperate canopy basin with giant-root paths and Emberling habitats. |
| Broad region boundaries | A fictional six-zone world atlas whose dimensions, elevations, shorelines, rivers, landmarks, and settlement placement are authored from scratch. |

## Prohibited inputs

Do not use the KML coordinates, Google Earth imagery, Earth tile links, referenced satellite layers, named real-world datasets embedded in the KML, map screenshots, roads, building outlines, coastline geometry, localities, postcodes, real elevation values, or geographic feature placement in the game.

## Review and import gate

Every playable landscape must pass `UForestSliceTerrainDefinition::ValidateForLandscapeImport` and record an independent source license when a terrain baseline is used. Original sculpted terrain is permitted. Any terrain declared as Google-derived is rejected. See `TERRAIN_IMPORT_WORKFLOW.md` and `ASSETS.md`.
