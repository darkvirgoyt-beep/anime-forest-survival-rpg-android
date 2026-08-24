# Automatic Aethelgard Content Expansion

Aethelgard uses two independent systems: **server-backed account/world saves** and **device-backed content expansion**. Google login identifies the account, while the cloud snapshot stores compact progress such as character identity, inventory, quest state, companion state, position, and discovered-sector flags. Large files are never placed inside the cloud save.

| Distribution path | Expansion behavior | Player action |
|---|---|---|
| Google Play signed AAB | The bootstrap core is install-time, the launch forest is `fast-follow`, and later biome/presentation packs are `on-demand`. Play stores each ready pack locally and the installed footprint grows as the player discovers new sectors. | Open the game, allow the launch download if requested, then accept later sector downloads when Play asks for consent. |
| Standalone APK plus OBB | The workflow still produces `main.<versionCode>.<package>.obb`. The app detects the standard OBB location automatically when an installer or device lab has placed the file there. | Copy the matching OBB to `Android/obb/com.darkvirgoyt.aethelgrad/` or use the provided installer. |

A raw APK cannot silently install a separate OBB into Android’s protected expansion directory. For a PUBG/BGMI-style experience, the signed Google Play App Bundle with Play Asset Delivery is the supported path: Play manages pack download, resume, storage, and updates, while the app presents progress and keeps the world safe until its launch slice is ready.

## Startup behavior

On first launch, the app automatically chooses the highest resource tier that passes the storage preflight. It prepares only the compact launch slice, persists that choice, and monitors the launch pack set. If the user restarts the app, the state is checked again and an in-progress or paused request is resumed. If Play requires consent for a large mobile-data download, the app opens the Play confirmation dialog and resumes after acceptance. If the user declines, the request remains paused and can resume later.

The launch forest is delivered as `fast-follow`. Shared launch dependencies are fetched as part of the launch preparation request. Sand, snow, dungeon, high-resolution texture, foliage, audio, VFX, cinematic, voice, and pipeline packs are `on-demand` and are requested after the native game reports the corresponding exploration/progression bit. The server receives only the small discovery bitmask through the ordinary revisioned cloud save.

If the standard OBB is present at `Context.obbDir`, the app treats the expansion as available and continues to the normal content-ready path. The OBB must match the APK version code and package name; mismatched files are not accepted by the standalone workflow verifier.

## Exploration-driven growth

The native runtime starts with the forest launch sector bit set. Crossing the sand frontier sets bit `2`, crossing the snow frontier sets bit `4`, and reaching the root-dungeon gate or completing the Warden quest sets bit `8`. Android maps these bits to immutable Play Asset Delivery pack groups. A pack is considered mounted only when `getPackLocation()` returns a valid location; otherwise the game continues with its safe procedural fallback and retries on a later connected launch.

Because the discovery flags are part of the server snapshot, a returning player can sign in on another device and recover the same world progress. The actual downloaded bytes remain local to each device and are independently rehydrated by Play as that device requests the discovered sectors.

## Release checklist

Build and distribute the signed AAB for the automatic Play path. Test the first launch from a Play internal-test track, move the player across each discovery boundary, verify the corresponding on-demand pack request and installed-size increase, test a restart during a download, test Wi-Fi-to-mobile-data transition, and test a low-storage device. Keep the standalone OBB artifact for private APK channels and device-lab compatibility only.

## References

[1]: https://developer.android.com/guide/playcore/asset-delivery/integrate-java "Android Developers: Integrate asset delivery (Kotlin and Java)"
[2]: https://developer.android.com/reference/com/google/android/play/core/assetpacks/AssetPackManager "Android Developers: AssetPackManager API reference"
[3]: https://developer.android.com/google/play/expansion-files "Android Developers: APK Expansion Files"
