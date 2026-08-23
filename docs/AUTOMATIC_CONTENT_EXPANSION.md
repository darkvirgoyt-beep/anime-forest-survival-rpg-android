# Automatic Aethelgard Content Expansion

Aethelgard now uses a two-path large-content delivery design.

| Distribution path | Expansion behavior | Player action |
|---|---|---|
| Google Play signed AAB | The core pack is install-time and the remaining game packs are `fast-follow`; Play begins delivery after installation or update and the app resumes it on every launch. | Open the game, allow the large download when prompted, and wait for the progress screen to finish. |
| Standalone APK plus OBB | The workflow still produces `main.<versionCode>.<package>.obb`. The app detects the standard OBB location automatically when an installer or device lab has placed the file there. | Copy the matching OBB to `Android/obb/com.darvirgoyt.aethelgrad/` or use the provided installer. |

A raw APK cannot silently install a separate OBB into Android’s protected expansion directory. For a PUBG/BGMI-style experience, the signed Google Play App Bundle with Play Asset Delivery is the supported path: Play manages the download, resume, storage, and updates, while the app shows an in-game progress surface and does not enter the world before required content is ready.

## Startup behavior

On first launch, the app automatically chooses the highest resource tier that passes the storage preflight. It persists that choice, starts the Play Asset Delivery request without showing a manual resource-tier chooser, and monitors all required packs. If the user restarts the app, the state is checked again and an in-progress or paused request is resumed. If Play requires consent for a large mobile-data download, the app opens the Play confirmation dialog and resumes after acceptance. If the user declines, the download remains paused and the resume button stays available.

If the standard OBB is present at `Context.obbDir`, the app treats the expansion as available and continues to the normal content-ready path. The OBB must match the APK version code and package name; mismatched files are not accepted by the standalone workflow verifier.

## Release checklist

Build and distribute the signed AAB for the automatic Play path. Test the first launch from a Play internal-test track, test a restart while the packs are downloading, test Wi-Fi-to-mobile-data transition, and test a low-storage device. Keep the standalone OBB artifact for private APK channels and device-lab compatibility only.

## References

[1]: https://developer.android.com/guide/playcore/asset-delivery/integrate-java "Android Developers: Integrate asset delivery (Kotlin and Java)"
[2]: https://developer.android.com/reference/com/google/android/play/core/assetpacks/AssetPackManager "Android Developers: AssetPackManager API reference"
[3]: https://developer.android.com/google/play/expansion-files "Android Developers: APK Expansion Files"
