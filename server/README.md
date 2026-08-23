# Aethelgard online services

This service is the online boundary for **Aethelgard: Wild Horizons – Crafting**. It supports guest-first entry, optional Google account authentication, internal sessions, co-op room state, and server-validated shared combat and inventory actions. The current Android release opens directly into an online guest session; full authoritative movement, hit detection, and high-frequency replication still belong in a dedicated game server.

## Local development

1. Install Node.js 22 and PostgreSQL 16, or run the included Docker Compose file.
2. Copy `.env.example` to `.env`. Set `GOOGLE_ID_TOKEN_AUDIENCE` to the **Web OAuth client ID** from Google Cloud and create a random `GAME_SESSION_JWT_SECRET` of at least 32 characters. Never commit `.env`.
3. Start PostgreSQL and apply `sql/001_init.sql`, `sql/002_hardened_sessions.sql`, `sql/003_coop_rendezvous.sql`, `sql/004_authoritative_gameplay.sql`, `sql/005_guest_accounts.sql`, and `sql/006_persistent_coop_worlds.sql` in order.
4. Install dependencies and start the service:

```bash
npm install
npm start
```

The health endpoint is `GET http://localhost:8080/healthz`. The Android app intentionally rejects non-HTTPS authentication endpoints in release-style builds, so use a real HTTPS domain for a device test.

## Guest-first authentication

The Android client generates a cryptographically random guest key on first launch and stores it only in the app’s private preferences. It sends the key over HTTPS to `POST /v1/auth/guest`; the server stores only its SHA-256 hash, upserts a normal internal account with provider `guest`, and returns the same short-lived access token plus rotating refresh token used by every other client. Reinstalling the app creates a new guest identity unless the platform backup restores the private preference. A guest can play online immediately without a Gmail account or account picker.

Guest keys are bearer credentials for the anonymous profile, so they must never be logged, placed in URLs, or sent over HTTP. Production deployments should add rate limiting, abuse controls, and an account-linking flow before exposing guest progression to public users.

## Standard Google account authentication

The Android app uses Credential Manager to receive a Google ID token for the configured **Web OAuth client ID**. It sends that token to `POST /v1/auth/google-id-token/exchange`. The service verifies the token signature, issuer, expiry, and audience using Google’s official Node authentication library before it associates the Google `sub` identifier with an internal account.

The backend returns a short-lived game access token plus a rotating refresh token. Google ID tokens and provider profile fields are never accepted as an Aethelgard session without this server-side verification.

## Future Play Games authentication

When a Play Console game and Game Server credential exist, add `GOOGLE_GAME_SERVER_CLIENT_ID` and `GOOGLE_GAME_SERVER_CLIENT_SECRET` only to the backend deployment. The service can then enable `POST /v1/auth/play-games/exchange` without changing the session or world contracts. Until both values are configured, that endpoint deliberately returns `play_games_not_configured`.

## Co-op tower rendezvous

Migration `sql/003_coop_rendezvous.sql` adds authenticated tower rooms and short-lived member presence. The Android client can create or join a six-character room code, then sends a heartbeat every two seconds with its local position and tower-arrival revision. The service advances one room clock and returns it to every member, so the existing deterministic weather cycle and day-night cycle stay aligned. When one member activates the tower, the room revision is observed by the other members and they teleport to the same tower landmark.

The current implementation is a lightweight HTTPS rendezvous layer with authoritative room actions: it synchronizes the shared clock, positions, tower event, boss health, and inventory mutations. Combat actions are server-locked, range-checked, cooldown-limited, and idempotent; inventory rewards and crafting costs are calculated from server-side state. Co-op worlds are creator-owned and persistent: leaving marks a member inactive instead of deleting their saved items or progression, the world save and placed buildings remain attached to the creator’s room, and a returning member can reconnect and load both world and player state. A production combat server can later consume the same room identity and world-clock contract while owning movement validation, hit detection, and low-latency replication.

| Endpoint | Purpose |
| --- | --- |
| `POST /v1/coop/rooms` | Create a room for the authenticated player’s selected region. |
| `POST /v1/coop/rooms/:code/join` | Join a room using its six-character code. |
| `GET /v1/coop/rooms/:code` | Read the synchronized room clock and active participants. |
| `POST /v1/coop/rooms/:code/heartbeat` | Publish position and tower-arrival state and receive the current room snapshot. |
| `DELETE /v1/coop/rooms/:code/leave` | Leave the room explicitly; inactive presence also expires after 20 seconds. |
| `POST /v1/coop/rooms/:code/combat` | Validate an attack target, range, cooldown, damage, boss health, and idempotent request ID inside a transaction. |
| `POST /v1/coop/rooms/:code/inventory` | Validate gather/craft proximity, material costs, inventory limits, inventory revision, and idempotent request ID inside a transaction. |
| `GET /v1/coop/rooms/:code/player-save` | Load the authenticated member’s durable items and progression. |
| `PUT /v1/coop/rooms/:code/player-save` | Save member items and progression with optimistic revision protection. |
| `GET /v1/coop/rooms/:code/save` | Load the creator-owned shared world state and placed buildings. |
| `PUT /v1/coop/rooms/:code/save` | Save shared world state; creator ownership is required. |
| `POST /v1/coop/rooms/:code/buildings` | Persist a validated placed village building for an active member. |

## Production requirements

Before public release, replace the development setup with a managed PostgreSQL instance, secret storage, TLS termination, rate limiting, structured audit logs, database backups and migrations, token revocation, abuse controls, and a dedicated-server allocator. The HTTP service must be deployed independently from the Unreal dedicated-server fleet. The game client must never receive a Google client secret or database credentials.

The initial API is deliberately narrow:

| Endpoint | Purpose |
|---|---|
| `GET /healthz` | Liveness and database readiness check. |
| `POST /v1/auth/guest` | Create or resume an anonymous online account from a hashed guest key. |
| `POST /v1/auth/google-id-token/exchange` | Verify a Google ID token server-side and create an internal game session. |
| `POST /v1/auth/refresh` | Rotate an active game refresh session. |
| `POST /v1/auth/play-games/exchange` | Optional future Play Games server-code exchange. |
| `GET /v1/worlds` | List online worlds after session validation. |
| `POST /v1/worlds` | Allocate a world record for later dedicated-server placement. |

World allocation is only a database contract at this stage. It must not be presented as a live multiplayer fleet until a real Unreal dedicated server, allocator, health reporting, reconnect path, and authoritative gameplay layer are deployed.
