# Aethelgard online services

This service is the first production boundary for **Aethelgard: Wild Horizons – Crafting**. It does not simulate combat or the world. It verifies a player’s Google account identity on the server, creates an internal account, issues a short-lived game session, and exposes the beginning of the world-service contract.

## Local development

1. Install Node.js 22 and PostgreSQL 16, or run the included Docker Compose file.
2. Copy `.env.example` to `.env`. Set `GOOGLE_ID_TOKEN_AUDIENCE` to the **Web OAuth client ID** from Google Cloud and create a random `GAME_SESSION_JWT_SECRET` of at least 32 characters. Never commit `.env`.
3. Start PostgreSQL and apply `sql/001_init.sql`, then `sql/002_hardened_sessions.sql`.
4. Install dependencies and start the service:

```bash
npm install
npm start
```

The health endpoint is `GET http://localhost:8080/healthz`. The Android app intentionally rejects non-HTTPS authentication endpoints in release-style builds, so use a real HTTPS domain for a device test.

## Standard Google account authentication

The Android app uses Credential Manager to receive a Google ID token for the configured **Web OAuth client ID**. It sends that token to `POST /v1/auth/google-id-token/exchange`. The service verifies the token signature, issuer, expiry, and audience using Google’s official Node authentication library before it associates the Google `sub` identifier with an internal account.

The backend returns a short-lived game access token plus a rotating refresh token. Google ID tokens and provider profile fields are never accepted as an Aethelgard session without this server-side verification.

## Future Play Games authentication

When a Play Console game and Game Server credential exist, add `GOOGLE_GAME_SERVER_CLIENT_ID` and `GOOGLE_GAME_SERVER_CLIENT_SECRET` only to the backend deployment. The service can then enable `POST /v1/auth/play-games/exchange` without changing the session or world contracts. Until both values are configured, that endpoint deliberately returns `play_games_not_configured`.

## Production requirements

Before public release, replace the development setup with a managed PostgreSQL instance, secret storage, TLS termination, rate limiting, structured audit logs, database backups and migrations, token revocation, abuse controls, and a dedicated-server allocator. The HTTP service must be deployed independently from the Unreal dedicated-server fleet. The game client must never receive a Google client secret or database credentials.

The initial API is deliberately narrow:

| Endpoint | Purpose |
|---|---|
| `GET /healthz` | Liveness and database readiness check. |
| `POST /v1/auth/google-id-token/exchange` | Verify a Google ID token server-side and create an internal game session. |
| `POST /v1/auth/refresh` | Rotate an active game refresh session. |
| `POST /v1/auth/play-games/exchange` | Optional future Play Games server-code exchange. |
| `GET /v1/worlds` | List online worlds after session validation. |
| `POST /v1/worlds` | Allocate a world record for later dedicated-server placement. |

World allocation is only a database contract at this stage. It must not be presented as a live multiplayer fleet until a real Unreal dedicated server, allocator, health reporting, and reconnect path are deployed.
