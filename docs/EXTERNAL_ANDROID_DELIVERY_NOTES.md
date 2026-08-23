# External Android delivery notes

The official Android Play Asset Delivery guide is available at [Android Developers: Play Asset Delivery](https://developer.android.com/guide/playcore/asset-delivery). It describes Play Asset Delivery as the current large-game delivery path and a replacement for legacy APK expansion files for games larger than the base APK limit.

The official legacy expansion-file guide is available at [Android Developers: APK Expansion Files](https://developer.android.com/google/play/expansion-files). It documents the opaque `main.<versionCode>.<packageName>.obb` naming convention and the separate expansion-file storage model.

For this prototype, Play Asset Delivery remains the primary Google Play path. Any standalone OBB is a private APK/device-lab compatibility artifact and must not be represented as the production Play release mechanism.
