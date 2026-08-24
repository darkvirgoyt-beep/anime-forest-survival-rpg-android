# Aethelgrad online service security and deployment

The default pre-Play-Console login route is `POST /v1/auth/google-id-token/exchange`. The Android app obtains a Google-issued ID token through Android Credential Manager and sends it over HTTPS. The backend verifies the ID token with Google’s official Node library, including signature, expiry, issuer, and configured web-client audience, before using the stable Google `sub` identifier to upsert an Aethelgrad account. The backend does not trust client-provided account IDs, names, or email addresses.

The service issues a signed Aethelgrad access token with a short default lifetime of 15 minutes and a random refresh token with a default lifetime of 30 days. Only hashes of access and refresh tokens are stored. Each refresh rotates the refresh token and revokes the prior session. A second use of a rotated refresh token revokes the account’s active sessions as a defensive compromise response.

## Required configuration

| Variable | Purpose | Storage rule |
|---|---|---|
| `DATABASE_URL` | PostgreSQL connection string | Deployment secret only |
| `DATABASE_SSL` | Set `true` when the database requires TLS | Deployment configuration |
| `GOOGLE_ID_TOKEN_AUDIENCE` | Web OAuth client ID expected in Google ID-token `aud` | Deployment configuration and Android resource; not a secret |
| `GAME_SESSION_JWT_SECRET` | HMAC key for Aethelgrad access tokens | Backend secret only; at least 32 characters |
| `ALLOWED_ORIGIN` | Exact HTTPS web/control origin, if any | Deployment configuration |
| `GAME_ACCESS_TOKEN_TTL_SECONDS` | Access-token lifetime, 60–3600 seconds | Deployment configuration |
| `GAME_REFRESH_TOKEN_TTL_SECONDS` | Refresh-token lifetime, 1 hour–90 days | Deployment configuration |

The future Play Games endpoint remains available but intentionally returns `play_games_not_configured` unless both `GOOGLE_GAME_SERVER_CLIENT_ID` and `GOOGLE_GAME_SERVER_CLIENT_SECRET` are set. These are server-only secrets and must never be committed or put into the APK.

Apply `sql/001_init.sql` and then `sql/002_hardened_sessions.sql` before starting the service. Use a managed PostgreSQL service with TLS, private credentials, backups, and least-privilege database access. Do not deploy with placeholder configuration or a wildcard browser origin.

The online service is not a dedicated gameplay server. It authenticates players, validates sessions, persists account and world metadata, and allocates co-op worlds. Authoritative real-time combat and world simulation must run in separately deployed dedicated game servers.
