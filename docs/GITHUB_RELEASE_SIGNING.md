# Stable GitHub Android Signing for Google OAuth

The Android OAuth client must recognize the package and certificate fingerprint of the exact APK installed on the phone. GitHub-hosted runners generate a new debug key when no persistent key is supplied, so their SHA-1 cannot be used as a permanent Google OAuth identity.

The Android build workflow now supports a protected reusable release signing key. The repository does **not** contain this key, any password, or its Base64 encoding.

## Required GitHub Actions Secrets

Create these repository-level Actions secrets in **Settings → Secrets and variables → Actions**:

| Secret name | Purpose |
|---|---|
| `AETHELGARD_RELEASE_KEYSTORE_BASE64` | Base64 of the private `.jks` keystore file. |
| `AETHELGARD_RELEASE_STORE_PASSWORD` | Keystore password. |
| `AETHELGARD_RELEASE_KEY_ALIAS` | Key alias. |
| `AETHELGARD_RELEASE_KEY_PASSWORD` | Key password. |

After a workflow run, download the `aethelgard-android-signing-certificate` artifact and register its **SHA1** value on the Android OAuth client in Google Cloud Console, together with package `com.darvirgoyt.aethelgrad`. Keep an encrypted offline backup of the keystore outside GitHub; losing it means future production APKs must use a new Android OAuth SHA-1.

If these secrets are absent, CI continues with debug signing for compilation-only validation. Do not distribute those changing debug-signed CI APKs as a permanent Google sign-in build.
