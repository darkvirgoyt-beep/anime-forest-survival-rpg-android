# Progressive World Content and Cloud Saves

## Product behavior

Aethelgrad separates **account state** from **game content**. Google authentication identifies the player, profile and character identity remain recoverable from the service, and the compact world snapshot stores progression such as position, inventory, quest stage, companion state, and discovered-sector IDs. The snapshot never contains meshes, textures, audio, shader binaries, or other large files.

When the player reaches a new exploration boundary, the Android client records that the sector was discovered and requests the matching Play Asset Delivery pack. The pack is stored locally by Play and increases the installed footprint. The server receives only the small discovery/progression update, so world exploration can grow the device installation without making cloud saves large.

## Delivery tiers

| Content group | Delivery timing | Purpose |
|---|---|---|
| Core renderer and launch forest | Install-time plus launch preparation | Makes authenticated world entry safe and immediately playable. |
| Characters, shared animation, GLES shaders | Launch preparation | Keeps the first playable slice visually complete. |
| Sand frontier | On demand after frontier discovery | Adds the next biome, materials, foliage, and navigation data. |
| Snow frontier | On demand after snow discovery | Adds the cold-region terrain, caves, and ice weather. |
| Dungeons and premium presentation | On demand after progression gates | Adds dungeon cells, VFX, cinematics, voice, and high-resolution optional content. |

## State contract

The native cloud snapshot contains a bounded `discoveredSectors` bitmask. The Android client maps each newly discovered sector to one or more immutable Play Asset Delivery pack names. The server validates the snapshot size and revision as it already does for ordinary cloud saves. It does not serve or trust arbitrary client file paths.

A pack is considered available only when Play reports a valid `AssetPackLocation`. A failed or offline request leaves the sector discovered in the cloud snapshot but keeps the local presentation in a safe fallback state; the client retries the pack request on a later launch or when connectivity returns.

## Non-Play testing

The standalone APK plus matching OBB remains the local no-Play test path for the packaged expansion. The genuine automatic fast-follow/on-demand lifecycle still requires an AAB installed through a Play test track; a plain `adb install` cannot reproduce Play’s delivery service.

## IP and integrity boundary

Progressive expansion uses Aethelgrad’s original content packs and procedural renderer. It does not copy assets, characters, maps, logos, or proprietary files from any external reference.
