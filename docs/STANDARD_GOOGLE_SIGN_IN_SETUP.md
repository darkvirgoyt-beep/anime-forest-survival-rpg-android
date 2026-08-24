# Standard Google Sign-In setup without Play Console

This test-login path uses **standard Google account sign-in**, not Google Play Games Services. It permits development before acquiring a Play Console account but does not provide Play Games player identities, achievements, leaderboards, internal-test tracks, or Google Play publishing.

## Create the two correct OAuth client types

Create the Android OAuth client you already made with this identity:

| Setting | Value |
|---|---|
| Application type | Android |
| Package name | `com.darkvirgoyt.aethelgrad` |
| Signing SHA-1 | The SHA-1 for the exact installed APK |

Then create a separate **Web application** OAuth client in the same Google Cloud project. Its **client ID** is the required `GOOGLE_ID_TOKEN_AUDIENCE` and Android `google_web_client_id`. The client ID is public configuration, not a secret. This Credential Manager ID-token flow does not use a custom URL scheme or Android redirect URI. Do not copy the Web client secret into the APK or repository.

## Configure Android and backend

After the backend has a permanent HTTPS domain, put the non-secret values in Android resources:

```xml
<string name="google_web_client_id" translatable="false">YOUR_WEB_CLIENT_ID.apps.googleusercontent.com</string>
<string name="auth_exchange_url" translatable="false">https://api.example.com/v1/auth/google-id-token/exchange</string>
<string name="auth_refresh_url" translatable="false">https://api.example.com/v1/auth/refresh</string>
```

Configure the deployment environment with the matching `GOOGLE_ID_TOKEN_AUDIENCE`, a non-placeholder `DATABASE_URL`, a random 32+ character `GAME_SESSION_JWT_SECRET`, and the exact allowed HTTPS control origin if browser access is used. The mobile app sends a Google ID token only to the HTTPS exchange route; the backend independently verifies it before creating the game session.

## Deployment prerequisite

The repository contains a deployable Node/PostgreSQL backend source tree, but no live deployment, database, domain, or user OAuth identifiers have been configured. An APK remains configuration-blocked until all three values above point to a deployed HTTPS backend and the matching Web client ID.

For the current managed development backend, the Android test build uses the non-secret Web OAuth audience `1062428369173-q4thoceukcd15r0cni75gfct25j1fk04.apps.googleusercontent.com` and the temporary HTTPS service base `https://3000-i6040xmjz90odduzrpnt3-1b82c801.sg1.manus.computer`. This test domain must be replaced with the final published service URL before public release.

## Later Play Games upgrade

Once a Play Console account and Play Games Services game configuration exist, add the game-server Web OAuth client ID and secret only to the backend deployment. The existing `/v1/auth/play-games/exchange` route can then be enabled as a provider-specific upgrade. Keep standard Google account identifiers and Play Games identifiers separate; they are distinct provider records by design.

## Official references

1. [Android Credential Manager Sign in with Google implementation](https://developer.android.com/identity/sign-in/credential-manager-siwg-implementation)
2. [Verify Google ID tokens on a backend](https://developers.google.com/identity/gsi/web/guides/verify-google-id-token)
