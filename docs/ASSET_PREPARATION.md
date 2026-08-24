# Aethelgard Asset Preparation

## Failure fixed

The game no longer treats the optional private high-end graphics manifest as a prerequisite for sign-in or world entry. A private service response such as HTTP 503 is classified as a temporary service failure, retried a bounded number of times, and then reduced to a non-blocking starter-pack state.

The bundled starter pack is the playable baseline. High-end graphics are an optional content upgrade and must not prevent the core game from opening.

## Runtime states

```text
Idle
  → CheckingCache
  → HighEndReady

CheckingCache
  → HighEndReady          valid cached manifest
  → Downloading           no valid cache + configured URL
  → StarterPackReady      no URL configured

Downloading
  → HighEndReady          valid manifest downloaded and atomically cached
  → Retrying              HTTP 503/5xx or network failure
  → StarterPackReady     retry budget exhausted or permanent failure

Retrying
  → Downloading
  → StarterPackReady      interrupted or exhausted
```

## Recovery behavior

| Condition | User-facing result | World entry |
|---|---|---|
| Valid high-end cache | `HIGH GRAPHICS READY` | Available immediately |
| Valid remote manifest | `HIGH GRAPHICS READY` | Available after cache write |
| HTTP 503/private service outage | `STARTER PACK READY` with retry action | Always available |
| HTTP 401/403 | `STARTER PACK READY` with sign-in explanation | Always available |
| Malformed manifest | `STARTER PACK READY` | Always available |
| No network | `STARTER PACK READY` | Always available |
| No URL configured | `STARTER PACK READY` | Always available |
| Corrupt cache | Cache discarded and re-requested | Starter pack remains available |

The Compose overlay is opened from the **HIGH GRAPHICS** HUD action. It offers **RETRY ASSET PREPARATION** and **ENTER WORLD** after fallback. Android back dismisses the overlay. The game does not pause or lock the core renderer for optional content preparation.

## Configuration

The default resource `@string/high_end_manifest_url` is intentionally empty. A release flavor or private build may override that resource with an HTTPS manifest endpoint. The app requests `android.permission.INTERNET` only for this optional service.

The manager stores only the validated manifest at:

```text
/files/content-cache/high_end_manifest.json
```

Writes go to a temporary file first and then replace the cache target. The response is size-bounded at 256 KiB and must look like a JSON object containing either a `version` or `assets` field before it is accepted. This lightweight validation avoids treating an HTML error page as a successful manifest.

## Implementation boundaries

- `AssetPreparationManager.kt` owns cache, retry, HTTP status classification, and fallback state.
- `ui/AssetPreparationUi.kt` owns the non-blocking player-facing overlay.
- `MainActivity.kt` owns lifecycle wiring and the HUD entry point.
- Gameplay and the native renderer remain independent of optional asset preparation.
- A future Unreal/Play Asset Delivery implementation should preserve the same rule: base content unlocks world entry; optional high-end packs may retry in the background.
