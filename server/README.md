# Aethelgard online services

This service is the first production boundary for **Aethelgard: Wild Horizons – Crafting**. It does not simulate combat or the world. It authenticates a player through Google Play Games Services, verifies the player on the server, creates an internal account, issues a short-lived game session, and exposes the beginning of the world-service contract.

## Local development

1. Install Node.js 22 and PostgreSQL 16, or run the included Docker Compose file.
2. Copy `.env.example` to `.env` and set the Google game-server OAuth client ID and secret. Never commit `.env`.
3. Start PostgreSQL and apply `sql/001_init.sql`.
4. Install dependencies and start the service:

```bash
npm install
npm start
```

The health endpoint is `GET http://localhost:8080/healthz`. The Android emulator can reach a host-local service at `http://10.0.2.2:8080`, but the Android client intentionally rejects non-HTTPS endpoints in release-style builds. Use an HTTPS tunnel only for controlled development; use a real HTTPS domain in production.

## Google Play server authentication

In Play Console, create a **Game server** credential and select a **web OAuth client ID**. Put that client ID and its secret in the server environment. The Android app requests a single-use server auth code using the same web client ID, then sends it to `POST /v1/auth/play-games/exchange`.

The service exchanges that code with Google, calls the Play Games player endpoint to verify the returned identity, upserts the internal account using the stable Play Games player ID, and returns a short-lived session token. The token is not a substitute for a dedicated game-server connection token; the future Unreal server should validate a server-issued session or allocation ticket before admitting a player.

## Production requirements

Before public release, replace the development compose setup with a managed PostgreSQL instance, secret storage, TLS termination, rate limiting, structured audit logs, database backups and migrations, token revocation, abuse controls, and a dedicated-server allocator. The HTTP service must be deployed independently from the Unreal dedicated-server fleet. The game client must never receive the Google client secret or database credentials.

The initial API is deliberately narrow:

| Endpoint | Purpose |
|---|---|
| `GET /healthz` | Liveness and database readiness check. |
| `POST /v1/auth/play-games/exchange` | Exchange a single-use Play Games server auth code for an internal game session. |
| `GET /v1/worlds` | List online worlds after session validation. |
| `POST /v1/worlds` | Allocate a world record for later dedicated-server placement. |

World allocation is only a database contract at this stage. It must not be presented as a live multiplayer fleet until a real Unreal dedicated server, allocator, health reporting, and reconnect path are deployed.
