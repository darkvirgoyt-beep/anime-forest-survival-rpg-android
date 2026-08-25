# Aethelgard release configuration lock

`config/RELEASE_LOCK.ini` is the short, human-readable source of truth for settings that must not drift. Its `[Manifest]` format is intentionally compact: values are defined once, while `tools/test_online_only_contract.py` compares them to the Android project and fails CI if they differ.

| Lock | Current value | Why it must not be changed casually |
|---|---|---|
| Android package and JNI prefix | `com.darvirgoyt.aethelgrad` / `Java_com_darvirgoyt_aethelgrad_` | Google Android OAuth registration, APK identity, native bridge exports, CI, and OBB naming depend on the exact identity. |
| Current launcher label | `AETHELGARD: Wild Horizons` | It identifies the current locked-package build; an older icon with the former short label is a separately installed historical package and must be removed once from Android settings. |
| Android minimum version | Android 11 / API 30 | The downloadable release is deliberately Android 11+; app-scoped OBB download storage needs no broad shared-storage permission. |
| Android game category | `android:appCategory="game"` | Helps Android and compatible launchers identify the package as a game. OEM Game Space enrollment remains controlled by the device/user and cannot be forced by the app. |
| Game authentication base | `https://aethelservs-g7pzbnwp.manus.space/api/game-auth` | The authenticated Google exchange and refresh flow require the matching managed HTTPS backend. |
| Entry mode | `dual-entry` | Google accounts own cloud restore and hosted co-op. Guest mode is device-local only: it has no cloud restoration, shared saves, or hosted co-op until the player explicitly switches to Google. |
| High-graphics publication gate | `false` | Keep this false until real original/licensed Unreal Android cooked content has a measured signed archive or Play release. See `docs/VERIFIED_CONTENT_PUBLICATION.md`. |
| Unreal Engine source | `external-private-only` | Epic-licensed engine source stays outside this public game repository. Commit only project code and original/licensed assets. |

Change a locked value only as one reviewed migration: update the release lock, every matching code/CI/backend value, Android OAuth/hosting configuration where applicable, and the validator; then run the repository contracts and a physical-device login/content test. Never commit secrets, keystores, refresh keys, OAuth private credentials, or a private Unreal Engine checkout.

## One-time duplicate-icon cleanup

Android can install two apps with the same visible icon when their package identities differ. It cannot safely remove the other package from an update to this game. On the phone, open **Settings → Apps**, select the older short-label Aethelgard icon, verify it is not the current `AETHELGARD: Wild Horizons` build, and uninstall only that older app. Then install future releases over the current labeled app; the locked package contract prevents new duplicate identities.
