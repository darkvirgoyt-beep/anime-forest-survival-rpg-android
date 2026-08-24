# Aethelgard release configuration lock

`config/RELEASE_LOCK.ini` is the short, human-readable source of truth for settings that must not drift. Its `[Manifest]` format is intentionally compact: values are defined once, while `tools/test_online_only_contract.py` compares them to the Android project and fails CI if they differ.

| Lock | Current value | Why it must not be changed casually |
|---|---|---|
| Android package and JNI prefix | `com.darkvirgoyt.aethelgrad` / `Java_com_darkvirgoyt_aethelgrad_` | Google Android OAuth registration, APK identity, native bridge exports, CI, and OBB naming depend on the exact identity. |
| Game authentication base | `https://aethelgard-api-v2.onrender.com/v1` | The authenticated Google exchange and refresh flow require the matching managed HTTPS backend. |
| Online mode | `online-only` | Do not add a guest or offline production bypass; cloud/session ownership is the game entry boundary. |
| High-graphics publication gate | `false` | Keep this false until real original/licensed Unreal Android cooked content has a measured signed archive or Play release. See `docs/VERIFIED_CONTENT_PUBLICATION.md`. |
| Unreal Engine source | `external-private-only` | Epic-licensed engine source stays outside this public game repository. Commit only project code and original/licensed assets. |

Change a locked value only as one reviewed migration: update the release lock, every matching code/CI/backend value, Android OAuth/hosting configuration where applicable, and the validator; then run the repository contracts and a physical-device login/content test. Never commit secrets, keystores, refresh keys, OAuth private credentials, or a private Unreal Engine checkout.
