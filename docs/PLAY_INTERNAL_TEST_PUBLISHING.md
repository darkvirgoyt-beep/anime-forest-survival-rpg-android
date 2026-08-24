# Play Asset Delivery upload and internal testing

`tools/publish_internal_test.py` automates the Google Play Publishing API edit workflow for a signed Android App Bundle. By default it performs a local dry run and makes no network request. A live release requires both `--commit` and `--yes`.

## One-time Play setup

Create or select a Google Cloud project, enable the Google Play Developer API, create a service account, and invite that service-account email in Play Console with only the release permissions needed for the target app. Keep the JSON key outside the repository and outside the APK/AAB. Google's official setup documentation recommends service-account access for server-to-server publishing and requires secure credential handling.

Create the **Internal testing** track in Play Console and configure its tester list or Google Group. The Publishing API track name for internal testing is `qa`. The script distributes the AAB to that already-configured track; it does not manage tester email addresses.

Install the optional local dependency for service-account authentication:

```bash
python3 -m pip install google-auth
```

## Dry run

Run this from the repository root after the Android/Unreal build has produced a signed AAB:

```bash
python3 tools/publish_internal_test.py \
  --aab path/to/app-release.aab \
  --package-name com.darkvirgoyt.aethelgrand \
  --track qa \
  --release-name "Aethelgard internal build" \
  --release-notes "Resource center and 540-class camera test build"
```

The default output shows the AAB path, size, package ID, track, and the four planned API operations: insert edit, upload bundle, update `qa`, and commit. It does not load credentials and does not contact Play.

## Live internal release

After checking the dry-run output, supply the service account file from a secure path and explicitly confirm the commit:

```bash
python3 tools/publish_internal_test.py \
  --aab path/to/app-release.aab \
  --package-name com.darkvirgoyt.aethelgrand \
  --track qa \
  --service-account /secure/credentials/play-publisher.json \
  --release-name "Aethelgard internal build" \
  --release-notes "Resource center and 540-class camera test build" \
  --commit --yes
```

The script creates one edit, uploads the AAB, reads the returned version code, assigns it to the `qa` track with status `completed`, and commits the edit. If an operation fails, do not retry blindly while another Play Console edit may be open; inspect the printed edit ID and Play Console state first.

For CI, prefer short-lived workload identity or an encrypted secret that is materialized only during the job. Never commit `play-publisher.json`, print its contents, or place it in an artifact. Keep the live publish step manual or protected by an environment approval.

## Tester flow

After the release is committed, share the internal-test Play link or opt-in link with the configured testers. Each tester must use a Google account included in the internal-testing configuration. The Play Store, not a sideloaded APK, is required to exercise real Play Asset Delivery pack installation. Test clean install, update with cached packs, Wi-Fi confirmation, insufficient storage, retry, and the resource center’s transition from downloading to ready.

## Safety model

The script accepts only known release tracks, defaults to `qa`, rejects non-`.aab` files and suspiciously small bundles, and refuses live publishing unless both `--commit` and `--yes` are present. It uses HTTPS and the official Android Publisher API. It does not create testers, change store listings, publish to production by accident, or upload a direct APK.

## References

1. [Google Play Developer API — Getting Started](https://developers.google.com/android-publisher/getting_started)
2. [Google Play Developer API — Edits](https://developers.google.com/android-publisher/edits)
3. [Google Play Developer API — APKs and Tracks](https://developers.google.com/android-publisher/tracks)
4. [Play Console Help — Set up an open, closed, or internal test](https://support.google.com/googleplay/android-developer/answer/9845334?hl=en)
