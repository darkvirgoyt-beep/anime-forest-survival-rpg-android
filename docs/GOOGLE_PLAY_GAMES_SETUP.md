# Google Play Games Services setup

The Android client uses **Google Play Games Services v2** for platform authentication. The client integration is present, but Google login will not authenticate until the game is registered in Google Play Console and the project ID is configured.

## 1. Create the Play Games configuration

In [Google Play Console](https://play.google.com/console/), create or open the game and enable **Play Games Services**. On the Play Games Services configuration page, create the Android credential for this application.

| Setting | Value |
|---|---|
| Android package name | `com.darkvirgoyt.forestslice` |
| Game services project ID | The numeric ID shown by Play Console |
| Minimum Android version | API 26 |
| Supported architectures | `arm64-v8a`, `x86_64` |

Replace the placeholder in `app/src/main/res/values/strings.xml` with the numeric project ID from Play Console:

```xml
<string name="game_services_project_id" translatable="false">REPLACE_WITH_PLAY_GAMES_PROJECT_ID</string>
```

Do not put OAuth client secrets, service-account keys, or backend administrator credentials in this repository or in the APK.

## 2. Configure signing fingerprints

Register the SHA-1 and SHA-256 certificate fingerprints for every signing certificate used to test or distribute the app. At minimum, configure the debug certificate for local testing and the protected release certificate for Play distribution. A build signed with an unregistered certificate can install successfully but still fail Play Games authentication.

## 3. Add test users

Before publishing, add the Google accounts used for testing as Play Games testers in Play Console. Install the application from the matching internal-test or closed-test track when testing release behavior. A locally sideloaded release build may not authenticate unless its package name, signing certificate, Play Games project, and tester account all match the configured values.

## 4. Client behavior

`ForestSliceApplication` initializes the Play Games SDK. `MainActivity` checks the asynchronous authentication state during launch and blocks online world entry until the state is `AUTHENTICATED`. The **GOOGLE PLAY SIGN-IN** button calls `GamesSignInClient.signIn()` when automatic authentication did not succeed.

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
