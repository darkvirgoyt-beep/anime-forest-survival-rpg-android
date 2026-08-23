# AETHELGRAD Post-Install Resource Download Specification

## Goal

The installed APK is a small bootstrap client. After installation, the production AAB obtains the larger cooked game resources through Google Play Asset Delivery. The player sees a preparation screen with progress, status, downloaded/total size, retry behavior, and a clear reason when the game cannot unlock yet.

## Delivery approaches

| Approach | Tradeoffs | Cost | Setup complexity |
|---|---|---|---|
| Google Play Asset Delivery fast-follow/on-demand packs | Best fit for Android Play installation, resumable delivery and Play-managed updates; requires a signed AAB and Play internal testing or production track. | Play distribution/storage and bandwidth costs; no custom server required. | Medium. Requires correct asset-pack modules, Play Console configuration, and real cooked content. |
| Direct HTTPS resource bundles | Works outside Play and can support a custom CDN; requires implementing download, resume, checksum, extraction, versioning, and security independently. | CDN/storage/bandwidth costs and additional backend operations. | High. Must protect against corrupted or tampered bundles and manage updates. |

AETHELGRAD uses Play Asset Delivery for the online production path because the project already contains asset-pack modules and the required Play libraries. Direct APKs are bootstrap test artifacts only; they do not unlock the online world without Play-managed production content.

## State machine

The post-install flow uses the following user-visible states:

| State | Meaning | Player action |
|---|---|---|
| Checking | Inspecting pack locations, network state, and free storage. | Wait or retry if checks fail. |
| Waiting for Wi-Fi | Play is waiting for an allowed connection. | Connect to Wi-Fi or change the Play download policy, then retry. |
| Confirmation required | Google Play requires confirmation for the large download. | Confirm the Play dialog, then retry. |
| Downloading | One or more packs are downloading. | Keep the screen open; Play can resume interrupted delivery. |
| Verifying/mounting | All required packs report ready and their locations are available. | Wait for the game to unlock. |
| Ready | Every required pack has a valid Play Asset Delivery location. | Continue to account/game setup. |
| Failed | A pack fetch or preflight check failed. | Retry; the game remains locked until required production content is ready. |

## Pack policy

The current plan uses one install-time bootstrap pack and a set of production packs. `assetpack_forest` is fast-follow for the initial Forest region. Large optional or later-region packs are on-demand in the Gradle modules and should be requested by map progression in a later milestone. Packs marked `requiredBeforeStart` are the only packs included in the initial production gate.

The checked-in pack directories are intentionally small content stubs for the current client contract. They do not represent a finished multi-gigabyte Unreal world. The final AAB must replace them with real cooked Unreal `.pak` content, manifest metadata, and device-tested assets without padding files.

## Reliability rules

Play Asset Delivery owns resumable transfer and package-level update behavior. The application adds a stable UI denominator, does not reset progress when a new pack reports its total, keeps the retry action visible for cancellation and confirmation states, checks free storage before starting, and never unlocks the production game on a direct APK that cannot resolve required Play pack locations.

There is no offline preparation path. If required Play content is unavailable, the online client remains at the resource center and does not enter a local world.

## Required production setup

The production build must be a signed AAB uploaded to Google Play internal testing or a production track. The asset-pack names in the AAB, `ContentDownloadPlan`, and runtime `AssetPackManager` request must match exactly. The resource packs must contain real cooked content, and the Unreal runtime must resolve assets through the pack locations rather than assuming a permanent filesystem path.

## References

[1]: https://developer.android.com/guide/playcore/asset-delivery "Google Play Asset Delivery documentation"
[2]: https://developer.android.com/reference/com/google/android/play/core/assetpacks/AssetPackManager "AssetPackManager API reference"
[3]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android "AETHELGRAD repository"
