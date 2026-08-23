# Google Play Games Services setup

The Android client uses **Google Play Games Services v2** for platform authentication. The client integration is present, but Google login will not authenticate until the game is registered in Google Play Console and the project ID is configured.

## 1. Create the Play Games configuration

In [Google Play Console](https://play.google.com/console/), create or open the game and enable **Play Games Services**. On the Play Games Services configuration page, create the Android credential for this application.

| Setting | Value |
|---|---|
| Android package name | `com.darvirgoyt.aethelgrad` |
| Game services project ID | The numeric ID shown by Play Console |
| Minimum Android version | API 26 |
| Supported architectures | `arm64-v8a`, `x86_64` |

Replace the placeholders in `app/src/main/res/values/strings.xml` with the Play Console project ID, the **web OAuth client ID created for the game server**, and the HTTPS session-exchange endpoint:

```xml
<string name="game_services_project_id" translatable="false">123456789012</string>
<string name="play_games_server_client_id" translatable="false">123456789012-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.apps.googleusercontent.com</string>
<string name="auth_exchange_url" translatable="false">https://api.example.com/v1/auth/play-games/exchange</string>
<string name="auth_refresh_url" translatable="false">https://api.example.com/v1/auth/refresh</string>
```

The Android app sends the single-use Play Games server auth code to `POST /v1/auth/play-games/exchange`. The backend exchanges and verifies it; the app does not receive or store the web OAuth client secret. The backend returns a short-lived access token and a rotating refresh token. The current milestone keeps both in memory only; a cold app restart performs a fresh Play Games exchange instead of persisting refresh credentials on the device.

Do not put OAuth client secrets, service-account keys, or backend administrator credentials in this repository or in the APK.

## 2. Configure signing fingerprints

Register the SHA-1 and SHA-256 certificate fingerprints for every signing certificate used to test or distribute the app. At minimum, configure the debug certificate for local testing and the protected release certificate for Play distribution. A build signed with an unregistered certificate can install successfully but still fail Play Games authentication.

For the already delivered **test APK** inspected for this milestone, enter the following only when installing that exact artifact:

| Field | Copyable value |
|---|---|
| Package name | `com.darvirgoyt.aethelgrad` |
| SHA-1 | `4B:D4:D6:94:1A:B9:9B:56:60:3F:37:7C:B2:8B:B7:67:E6:94:FE:CB` |
| SHA-256 | `86989113250DB7CDDD1BCCAB3A3B7248B5BA5C981673D0B7607E32F22132E9D` |

This repository intentionally uses debug/test-distribution signing for its milestone APKs. Debug certificates can differ between machines and CI environments, so recalculate the fingerprint with `apksigner verify --print-certs <apk>` whenever the signed APK changes. When the game is enrolled in Google Play App Signing, register the **App signing certificate** SHA-1 shown in Play Console as a separate Android OAuth credential. Do not upload or commit a production keystore.

## 3. Add test users

Before publishing, add the Google accounts used for testing as Play Games testers in Play Console. Install the application from the matching internal-test or closed-test track when testing release behavior. A locally sideloaded release build may not authenticate unless its package name, signing certificate, Play Games project, and tester account all match the configured values.

## 4. Client behavior

`ForestSliceApplication` initializes the Play Games SDK. `MainActivity` checks the asynchronous platform authentication state during launch, requests a single-use server auth code, sends it to the HTTPS backend, and blocks online world entry until the backend returns a verified game session. The **GOOGLE PLAY SIGN-IN** button calls `GamesSignInClient.signIn()` when automatic authentication did not succeed.

The current client stores only a local authentication-state marker. A production online game must exchange a short-lived server-verifiable assertion with a backend over HTTPS. The backend—not the client—must own the mapping from the provider identity to the game account, cloud saves, entitlements, party membership, and permissions.

## 5. Expected failure messages

| Message | Meaning | Action |
|---|---|---|
| `Checking Google Play sign-in…` | The SDK is checking the device account. | Wait for the asynchronous result. |
| `Google Play sign-in is required to continue.` | No authenticated Play Games account is available. | Tap the sign-in button or check the device’s Google account and Play Games setup. |
| `Google Play sign-in failed or was cancelled.` | The sign-in task was cancelled or returned an error. | Check tester status, certificate fingerprints, package name, Play Games configuration, and network access. |
| `Google Play sign-in successful...` | Platform authentication succeeded. | Continue only after the backend session exchange is implemented for production services. |

## Official references

1. [Platform authentication for Android games](https://developer.android.com/games/pgs/android/android-signin)
2. [Get started with Play Games Services for Android games](https://developer.android.com/games/pgs/android/android-start)
3. [Set up Google Play services](https://developers.google.com/android/guides/setup)
