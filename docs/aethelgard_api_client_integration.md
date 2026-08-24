# AETHELGRAD Server API — Client Integration Guide

**API version:** v1  
**Service:** `aethelgard-online-services`  
**Audience:** Android client, dedicated-server allocator, QA automation, and tools integration

This document describes the current HTTP contract implemented by the AETHELGRAD online service. The API uses JSON over HTTPS in production. Authentication is server-side, room actions are transactionally validated, and the current co-op layer synchronizes room state without pretending to be a complete real-time combat server.

## 1. Transport and authentication

The Android client must send `Content-Type: application/json` for JSON requests and must use an `Authorization: Bearer <accessToken>` header for every authenticated endpoint. The access token is issued after guest authentication, Google ID-token exchange, or the optional Play Games exchange. The service verifies the token signature, issuer, audience, expiry, session ID, account ID, stored token hash, and revocation state before allowing access.[1] [2]

The current token contract uses issuer `aethelgard-online-services`, audience `aethelgard-android`, account subject `sub`, session ID `sid`, issued-at `iat`, and expiry `exp`. The default access-token lifetime is 900 seconds; refresh sessions default to 30 days and are rotated on use. A client should refresh before expiry and discard a refresh token after a successful rotation.

| Header | Required | Description |
| --- | --- | --- |
| `Authorization` | Authenticated routes | `Bearer <accessToken>`. |
| `Content-Type` | JSON requests | `application/json`. |
| `Origin` | Browser callers only | Must match the configured allowed origin; production origin must use HTTPS. |

## 2. Environment and deployment prerequisites

The service requires `DATABASE_URL`, `GOOGLE_ID_TOKEN_AUDIENCE`, `GAME_SESSION_JWT_SECRET`, and `ALLOWED_ORIGIN`. The session secret must contain at least 32 characters. Apply the schema export in this order: `001_init`, `002_hardened_sessions`, `003_coop_rendezvous`, `004_authoritative_gameplay`, and `005_guest_accounts`.

The Android release client rejects non-HTTPS authentication endpoints. The service should therefore be deployed behind TLS, with PostgreSQL connectivity, secret storage, rate limiting, audit logging, backups, and a migration process before external testing.

## 3. Authentication endpoints

### `POST /v1/auth/guest`

Creates or resumes an anonymous online account. The client generates a random 32-byte base64url key, stores it in platform-private storage, and sends it only over HTTPS. The server stores only the SHA-256 hash and returns the normal session bundle:

```json
{
  "guestKey": "<32-byte-base64url-key>"
}
```

A successful response is `200 OK`:

```json
{
  "accessToken": "<signed-access-token>",
  "refreshToken": "<opaque-refresh-token>",
  "tokenType": "Bearer",
  "accountId": "<uuid>",
  "accountType": "guest",
  "expiresAt": "2026-08-23T13:00:00.000Z",
  "refreshExpiresAt": "2026-09-22T12:45:00.000Z"
}
```

Malformed keys return `400 invalid_guest_key`. This endpoint documents a future/server-backed anonymous account contract; the current Android release does not call it. Android Guest mode is intentionally local-only, stores the versioned world snapshot on-device, and cannot create, join, or reconnect to hosted co-op. Google login remains the required path for cloud worlds and multiplayer.

### `POST /v1/auth/google-id-token/exchange`

Exchanges a Google ID token for an AETHELGRAD game session. The request body is:

```json
{
  "idToken": "<google-id-token>"
}
```

A successful response is `200 OK`:

```json
{
  "accessToken": "<signed-access-token>",
  "refreshToken": "<opaque-refresh-token>",
  "tokenType": "Bearer",
  "accountId": "<uuid>",
  "expiresAt": "2026-08-23T13:00:00.000Z",
  "refreshExpiresAt": "2026-09-22T12:45:00.000Z"
}
```

Malformed tokens return `400 invalid_google_id_token`. Failed server-side verification returns `401 google_id_token_authentication_failed`.

### `POST /v1/auth/play-games/exchange`

Exchanges a Play Games server authorization code when the optional backend credentials are configured. The request body is:

```json
{
  "serverAuthCode": "<server-auth-code>"
}
```

The service rejects malformed codes with `400 invalid_server_auth_code`, rejects a reused code with `409 replayed_server_auth_code`, and returns `503 play_games_not_configured` until the backend credentials exist.

### `POST /v1/auth/refresh`

Rotates an active refresh session:

```json
{
  "refreshToken": "<opaque-refresh-token>"
}
```

The response returns a new access token and refresh token. The previous refresh session is revoked. Reusing a rotated token revokes the account’s remaining active sessions and returns `401 replayed_refresh_session`.

### `GET /v1/auth/session` and `POST /v1/auth/logout`

`GET /v1/auth/session` returns the current account and session identity:

```json
{
  "accountId": "<uuid>",
  "sessionId": 123,
  "status": "authenticated"
}
```

`POST /v1/auth/logout` revokes the current session and returns `204 No Content`.

## 4. World allocation endpoints

### `GET /v1/worlds`

Returns online world records after session validation:

```json
{
  "worlds": [
    {
      "id": "<uuid>",
      "region": "asia",
      "name": "Aethelgrad Forest",
      "status": "online",
      "max_players": 4,
      "current_players": 2
    }
  ]
}
```

### `POST /v1/worlds`

Creates an allocating world record. This is a world-service contract and does not by itself allocate a live Unreal dedicated server:

```json
{
  "region": "asia",
  "name": "Aethelgrad Forest"
}
```

The default region is `asia`, and the default name is `Aethelgrad Forest`.

## 5. Co-op room endpoints

A room supports up to four active players. Presence expires after approximately 20 seconds without a heartbeat. The room clock is advanced by the service and returned to each active member. Clients derive the local weather and day-night presentation from that synchronized clock.

### `POST /v1/coop/rooms`

Creates a room and adds the authenticated account as the first member:

```json
{
  "region": "asia"
}
```

The response is `201 Created`:

```json
{
  "room": {
    "code": "F7D51F",
    "region": "asia",
    "maxPlayers": 4,
    "worldTime": 12.5,
    "towerRevision": 0,
    "bossHealth": 100,
    "combatRevision": 0
  },
  "participants": [
    {
      "accountId": "<uuid>",
      "playerX": -0.55,
      "playerY": -0.08,
      "atTower": false,
      "towerRevision": 0
    }
  ]
}
```

### `POST /v1/coop/rooms/:code/join`

Joins an existing room. Room codes are six uppercase alphanumeric characters. A full room returns `409 room_full`; an unknown room returns `404 room_not_found`.

```json
{}
```

The response has the same room snapshot shape as room creation.

### `GET /v1/coop/rooms/:code`

Returns the synchronized room clock, tower revision, shared boss state, and active participant list. The caller must already be a room member.

### `POST /v1/coop/rooms/:code/heartbeat`

Publishes bounded presence and receives the newest room snapshot:

```json
{
  "playerX": -0.06,
  "playerY": 0.28,
  "atTower": true,
  "towerRevision": 1
}
```

`playerX` is clamped to `[-0.90, 0.90]`, and `playerY` is clamped to `[-0.50, 0.52]`. A caller that has not joined receives `403 room_membership_required`. The Android client should send this request approximately every two seconds while the activity is active.

### `DELETE /v1/coop/rooms/:code/leave`

Removes the authenticated member and returns `204 No Content`. The client should call this when leaving the room, while also treating stale presence expiry as the fallback path.

## 6. Authoritative combat endpoint

### `POST /v1/coop/rooms/:code/combat`

The combat endpoint is server-authoritative for the current shared Forest Warden action. The server locks the room and member rows in a database transaction, checks membership, validates the target, checks player-to-target range, enforces an action cooldown, applies server-calculated damage, increments `combatRevision`, stores an action receipt, commits, and returns the result.

Request:

```json
{
  "requestId": "combat-1724412345-1",
  "action": "attack",
  "targetId": "forest_warden"
}
```

Allowed actions are `attack` and `heavy_attack`. The current damage values are 12 and 24 respectively. A successful response is:

```json
{
  "accepted": true,
  "action": "attack",
  "targetId": "forest_warden",
  "damage": 12,
  "bossHealth": 88,
  "combatRevision": 1
}
```

`requestId` must contain 8–80 characters matching `[A-Za-z0-9_-]`. Repeating the same request ID for the same room and account returns the original result rather than applying damage twice. Common errors are `400 invalid_combat_request`, `400 unknown_combat_target`, `403 room_membership_required`, `404 room_not_found`, `409 combat_target_out_of_range`, and `429 combat_cooldown`.

The Android client may play the attack animation immediately for responsiveness, but it must apply the returned `bossHealth` as the authoritative result. In an active co-op room, the native client disables local hit mutation and waits for the server-approved health value.

## 7. Authoritative inventory endpoint

### `POST /v1/coop/rooms/:code/inventory`

The inventory endpoint validates resource gathering and crafting against server-held member state. Gathering accepts only known resource IDs and checks proximity to the server-defined resource coordinate. Crafting checks the server-held material totals and then applies the cost in the same transaction.

Gather request:

```json
{
  "requestId": "gather-1724412345-1",
  "operation": "gather",
  "resourceId": "forest_cache"
}
```

Supported resource IDs are `forest_cache`, `root_cache`, and `warden_stone`. The server calculates the reward; the client does not submit reward quantities.

Craft request:

```json
{
  "requestId": "craft-1724412345-1",
  "operation": "craft"
}
```

A successful response is:

```json
{
  "accepted": true,
  "operation": "gather",
  "inventory": {
    "wood": 13,
    "fiber": 10,
    "stone": 4,
    "emberKit": false
  },
  "inventoryRevision": 1
}
```

The current craft recipe costs 3 wood and 2 fiber and produces the server-side `emberKit` flag. Common errors are `400 invalid_inventory_request`, `400 unknown_resource`, `403 room_membership_required`, `404 room_not_found`, `409 resource_out_of_range`, and `409 insufficient_crafting_materials`. Repeating a request ID returns the original inventory result without duplicating the reward or craft.

## 8. Client synchronization algorithm

The recommended Android loop is straightforward. When the user creates or joins a room, the client stores the returned room code and immediately applies `worldTime`, `bossHealth`, and the peer list to the native renderer. While the activity is resumed, it reads the local native position and tower revision, posts a heartbeat, applies the returned room snapshot, updates remote peer markers, and schedules the next heartbeat after roughly two seconds.

A remote tower revision greater than the client’s last observed revision is treated as a shared arrival event. The client moves its local player to the tower, records the revision, and does not increment the revision again. This prevents teleportation loops. Combat and inventory buttons remain responsive, but their shared results are applied only after the authoritative HTTP response.

## 9. Error handling and retry rules

Clients should treat `401 expired_session`, `401 invalid_session`, and `401 revoked_session` as authentication recovery events. A client should attempt one refresh-token rotation, then return the player to sign-in if refresh fails. For `429 combat_cooldown`, wait for the cooldown and do not retry immediately. For `409` range or resource errors, update the HUD and require the player to move. For network timeouts, retry with the same `requestId` for combat or inventory so the server receipt makes the retry safe.

| Status | Client action |
| --- | --- |
| `200` / `201` | Apply the returned authoritative state. |
| `204` | Clear the local session or room membership as appropriate. |
| `400` | Correct the request; do not retry unchanged. |
| `401` | Refresh once, then require sign-in if still rejected. |
| `403` | Rejoin the room or report a membership problem. |
| `404` | Refresh room/world discovery; the requested resource may no longer exist. |
| `409` | Show the state conflict and require a player action, except duplicate request receipts which are safe to accept. |
| `429` | Respect the server cooldown. |
| `503` | Retry with backoff and show the service as temporarily unavailable. |

## 10. Current authority boundary

The current service provides authoritative room actions for shared boss health and co-op inventory mutations. It does not yet validate every movement frame, hitbox, projectile, inventory item type, disconnect reconciliation, or low-latency combat event. Those responsibilities belong in a future dedicated game server that can consume the same account, room, world-clock, combat-revision, and inventory-revision contracts.

## References

[1]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/src/security.mjs "AETHELGRAD session and runtime security contract"

[2]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/src/server.mjs "AETHELGRAD HTTP API implementation"

[3]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/sql/001_init.sql "AETHELGRAD base PostgreSQL schema"

[4]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/sql/003_coop_rendezvous.sql "AETHELGRAD co-op room schema"

[5]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/sql/004_authoritative_gameplay.sql "AETHELGRAD authoritative gameplay schema"
