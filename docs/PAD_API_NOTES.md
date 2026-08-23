# Play Asset Delivery API notes

The official Android Play Asset Delivery guide states that the Play Store automatically starts `fast-follow` packs after install or update, but the app must check their state on every launch, monitor progress with a listener, and call `fetch()` to resume paused or cancelled downloads. It also states that `getPackLocation()` returns a usable asset root only when a pack is ready.

For large downloads, Play may pause on mobile data or require explicit consent. The current AssetPackManager API offers `showConfirmationDialog(ActivityResultLauncher<IntentSenderRequest>)` and a deprecated-compatible `showConfirmationDialog(Activity)` task overload. The app should expose this only for the Play AAB delivery path; raw APK plus manually copied OBB files cannot self-install an expansion file through Android’s normal package installer.

Source: https://developer.android.com/guide/playcore/asset-delivery/integrate-java
Source: https://developer.android.com/reference/com/google/android/play/core/assetpacks/AssetPackManager
