# 3D terrain import workflow

## What the selected Google Earth polygon means

The selected polygon is a **planning boundary**. It can communicate the approximate region, broad coastline, elevation range, and biome inspiration for Aethelgrad. It must not become the shipped terrain by copying Google Earth screenshots, satellite imagery, 3D meshes, map tiles, or Google elevation output.

Google’s Earth terms permit creating KML files and map layers, but prohibit copying Google Earth content and creating a new product or service based on Google Earth. Google’s geo guidelines specifically prohibit using output from Google Earth, Google Earth Pro, or Earth Studio to reconstruct 3D models or create similar content. [1] [2]

## Required input from the designer

Export the **polygon only** as `.kml` or `.kmz` from Google Earth. Do not export a screenshot or a 3D model. Store it as a private design reference outside `Content/`, give it an identifier such as `aethelgard_reference_boundary_v1`, and record its date and intended use.

The polygon shown is far too large to load as one Android world. The first playable Unreal landscape is limited to **8 km × 8 km by default** and **16 km × 16 km maximum**. Larger selected territory becomes a world atlas made of future streamed regions, each with a separate terrain definition, save boundary, budget, and device profile.

## Heightmap source decision

| Region type | Valid elevation starting point | Important limitation |
|---|---|---|
| United States territory | USGS 3DEP / The National Map | USGS says its map data are public domain; retain the requested USGS acknowledgment. [3] |
| Global first-pass terrain | NASA SRTM DEM | Near-global land elevation data; it is a terrain starting point, not final art. [4] |
| Global 30 m / 90 m alternative | Copernicus DEM | Review and retain the applicable Copernicus license, version, and source record. [5] |
| Custom art-directed landscape | Original sculpt or separately licensed DEM | Keep the creator, license, import settings, and replacement plan in `ASSETS.md`. |

Do not use Google Maps Platform Elevation API values or Google Earth output to build the terrain model. Google Maps Platform terms expressly prohibit building terrain models from Elevation API values and creating content based on Maps content. [6]

## Local conversion into Unreal landscape input

1. Use QGIS or another GIS tool to open the exported KML/KMZ boundary and the chosen independent DEM.
2. Crop the DEM to a new **smaller first-playable rectangle** inside the boundary; do not attempt to process the full atlas at once.
3. Fill voids, apply only documented resampling, and retain the original DEM plus processing settings outside Git.
4. Export a **16-bit grayscale heightmap** such as `.r16` or a supported 16-bit PNG. Do not export a Google satellite image as a terrain texture.
5. In Unreal Editor, import the heightmap into a Landscape or World Partition level. Configure XY scale from the chosen real-world meter coverage and tune Z scale from the DEM’s recorded elevation range.
6. Create new, original biome masks and materials: forest soil, grass, rock, riverbed, wetland, beach, snow line, and volcanic/ruin accents. Materials, foliage, rivers, landmarks, settlements, quests, and creature habitats are authored for Aethelgrad; they are not copied from the real map.
7. Create a `UForestSliceTerrainDefinition` data asset. Record the KML boundary ID, DEM citation, license record, source kind, heightmap, scale, and original biome masks. Its validation forbids a Google-derived terrain input and blocks an oversized first-playable landscape.

## Unreal setup map

| Unreal asset | Owner | First pass |
|---|---|---|
| `UForestSliceTerrainDefinition` | C++ + Data Asset | Validates data source, heightmap, boundary record, and 16 km first-playable maximum. |
| Landscape / World Partition level | Level design | Imports independently derived 16-bit heightmap; partitions regions for streaming. |
| Landscape material | Technical art | Original albedo, normal, roughness, height, and biome masks only. |
| `AForestSliceProceduralForest` | Gameplay C++ | Uses the landscape/biome context to place original foliage and resources deterministically. |
| Save and server system | Backend + dedicated server | Persists player mutations separately from generated terrain baseline. |

## Verification gate

- [ ] The user exports and provides a polygon-only KML/KMZ boundary.
- [ ] A source record identifies an independent DEM, license/public-domain notice, date, and processing recipe.
- [ ] A 16-bit heightmap is imported in Unreal Engine on a capable local PC.
- [ ] Original landscape materials, rivers, landmarks, and biomes replace real-world imagery.
- [ ] Android profiling verifies frame time, texture memory, streaming, shader compilation, and thermal behavior.
- [ ] Source and processed DEM files remain outside public Git unless their license and size policy explicitly permit inclusion.

## References

[1]: https://www.google.com/help/terms_maps-earth/ "Google Earth End User Additional Terms of Service"
[2]: https://about.google/brand-resource-center/products-and-services/geo-guidelines "Google Geo Guidelines"
[3]: https://www.usgs.gov/faqs/what-are-terms-uselicensing-map-services-and-data-national-map "USGS National Map licensing"
[4]: https://www.earthdata.nasa.gov/data/instruments/srtm "NASA Shuttle Radar Topography Mission"
[5]: https://registry.opendata.aws/copernicus-dem/ "Copernicus Digital Elevation Model"
[6]: https://cloud.google.com/maps-platform/terms "Google Maps Platform Terms of Service"
