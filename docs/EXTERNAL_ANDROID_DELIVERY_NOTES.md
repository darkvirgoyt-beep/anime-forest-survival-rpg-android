# External Android delivery notes

The official Android Play Asset Delivery guide is available at [Android Developers: Play Asset Delivery](https://developer.android.com/guide/playcore/asset-delivery). It describes Play Asset Delivery as the current large-game delivery path and a replacement for legacy APK expansion files for games larger than the base APK limit.

The official legacy expansion-file guide is available at [Android Developers: APK Expansion Files](https://developer.android.com/google/play/expansion-files). It documents the opaque `main.<versionCode>.<packageName>.obb` naming convention and the separate expansion-file storage model.

For Aethelgrad’s AAA production game, Play Asset Delivery remains the primary Google Play path. A direct APK downloaded from GitHub is only for base-client UI/native testing and cannot fetch on-demand asset packs. For local asset-pack testing outside Play, install the generated `aethelgard-local-testing.apks` with bundletool. Any standalone OBB is a private APK/device-lab compatibility artifact and must not be represented as the production Play release mechanism.

## Fixing a failed asset-pack download

The Play Asset Delivery error code `-100` is Google’s `INTERNAL_ERROR` value: it means the asset-pack request failed with an unknown internal delivery error. The most common cause for this screen is installing `app-release.apk` directly. A direct APK does not contain the on-demand asset-pack delivery relationship that Google Play establishes for the matching App Bundle.

Use one of these supported paths:

1. **Real Play delivery:** upload the matching `app-release.aab` to Play Console internal testing or internal app sharing, install it from the generated Play link, and retry the download. The test device must use the same package/version available to that Play track.
2. **Local delivery:** download `aethelgard-local-testing.apks` from the CI artifacts or release, install bundletool, uninstall the old direct APK, then run `java -jar bundletool.jar install-apks --apks=aethelgard-local-testing.apks`. Local testing requires uninstalling the previous version before installing a new APK set.
3. **Direct APK:** use `app-release.apk` only to test the base Android/native client. Do not use it to validate Play Asset Delivery.

If the app was already installed from Play and still reports `-100`, update or uninstall/reinstall from the same Play track, confirm Google Play Store and Google Play Services are enabled, use Wi-Fi, verify sufficient free storage, and confirm the requested pack is included in the uploaded AAB. Do not mix a GitHub APK with a Play-installed asset-pack session.
