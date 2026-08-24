# Google Sign-In Device Troubleshooting

The required `SIGN IN WITH GOOGLE` control uses Android Credential Manager and the explicit Google button flow. AETHELGRAD does not create an automatic guest session; a verified Google account is required to protect the cloud world and continue online. A visible cancellation can be a genuine user dismissal, but Android documents that unexpected cancellations can also reflect a relying-party configuration problem. The app shows a safe action message rather than exposing token data or raw account information.

| Player-facing diagnostic | Most likely next action |
|---|---|
| No usable Google account | Add or reauthenticate a Google account on the phone, then retry. |
| Google credential services unavailable | Update Google Play services and the Aethelgrad APK. |
| Google sign-in ended before the game server | Copy the package name and SHA-1 fingerprint shown in the app’s error message into the Android OAuth client in Google Cloud Console, then retry. Do not enable the custom URI scheme. |
| Interrupted | Re-open the app and retry once. |

The backend cannot fix a failed Android credential request because that failure occurs before an ID token is issued or sent to `/v1/auth/google-id-token/exchange`. The backend health endpoint only proves that server configuration is ready. The new diagnostic computes the public signing certificate SHA-1 from the installed APK at runtime; it never displays a token, account identifier, or private key.

## Release-Certificate Requirement

Android OAuth binds an installed application to both its package name and certificate fingerprint. Any APK signed with a different key must have its SHA-1 registered on the Android OAuth client. This includes GitHub CI artifacts when their signing key differs from the locally installed build. Use a protected, reusable release keystore for production builds; do not add keystore files or passwords to the repository.

## Why This Can Repeat With GitHub APKs

The current build workflow falls back to an Android debug certificate when protected release-signing secrets are absent. A debug certificate created on a GitHub runner can differ from the certificate for an older artifact, so an Android OAuth client that contains an old SHA-1 will reject the newer APK before the backend receives an ID token. For every test APK, download its `aethelgard-android-signing-certificate` artifact and register that SHA-1; for a permanent test/release channel, configure the protected reusable signing key described in `GITHUB_RELEASE_SIGNING.md`.

## Use the Correct GitHub Artifact

Install the matching AETHELGRAD AAB through Play internal testing, or use the workflow’s bundletool local-testing APK set when testing Play Asset Delivery. The release package is `com.darkvirgoyt.aethelgrad`; it requires Google sign-in and the selected full resource tier before world entry. Register the SHA-1 from the `aethelgard-android-signing-certificate` artifact against the same package in the Android OAuth client. A direct APK is limited to installation and configuration smoke checks because it cannot fetch Play Asset Delivery packs.

## Mobile Control Direction

The native input bridge mirrors the horizontal thumb axis once before applying camera-relative movement. A rightward joystick drag now moves the visible hero rightward and a leftward drag moves the hero leftward. The correction is intentionally native-side so every mobile control surface uses the same coordinate convention.

## Official References

1. [Android Credential Manager troubleshooting guide](https://developer.android.com/identity/sign-in/credential-manager-troubleshooting-guide)
2. [Implement Sign in with Google using Credential Manager](https://developer.android.com/identity/sign-in/credential-manager-siwg-implementation)
3. [GetCredentialException API reference](https://developer.android.com/reference/androidx/credentials/exceptions/GetCredentialException)
