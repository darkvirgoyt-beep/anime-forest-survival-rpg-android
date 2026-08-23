# Aethelgard online service security and deployment

The service accepts only a single-use Google Play Games server authorization code over HTTPS. The Android client receives the game-server web OAuth client ID but never the OAuth client secret. The backend exchanges the code at Google, verifies the authenticated Play Games player, records only a SHA-256 receipt of the code, and rejects a second use of the same receipt.

The backend issues a signed access token with a short default lifetime of 15 minutes and a random refresh token with a default lifetime of 30 days. Only hashes of access and refresh tokens are stored. Each refresh rotates the refresh token and revokes the prior session. A second use of a rotated refresh token revokes the account's active sessions as a defensive compromise response.

## Required configuration

| Variable | Purpose | Storage rule |
|---|---|---|
| `DATABASE_URL` | PostgreSQL connection string | Deployment secret only |
| `DATABASE_SSL` | Set `true` when the database requires TLS | Deployment configuration |
| `GOOGLE_GAME_SERVER_CLIENT_ID` | Game-server web OAuth client ID | Deployment configuration; may also be present in Android resources |
| `GOOGLE_GAME_SERVER_CLIENT_SECRET` | OAuth client secret for code exchange | Backend secret only; never commit or put in an APK |
| `GAME_SESSION_JWT_SECRET` | HMAC key for Aethelgard access tokens | Backend secret only; at least 32 characters |
| `ALLOWED_ORIGIN` | Exact HTTPS web/control origin, if any | Deployment configuration |
| `GAME_ACCESS_TOKEN_TTL_SECONDS` | Access-token lifetime, 60–3600 seconds | Deployment configuration |
| `GAME_REFRESH_TOKEN_TTL_SECONDS` | Refresh-token lifetime, 1 hour–90 days | Deployment configuration |

Apply `sql/001_init.sql` and then `sql/002_hardened_sessions.sql` before starting the hardened service. Use a managed PostgreSQL service with TLS, private credentials, backups, and least-privilege database access. Do not deploy with placeholder configuration or a wildcard browser origin.

The online service is not a dedicated gameplay server. It authenticates players, validates sessions, persists account and world metadata, and allocates co-op worlds. Authoritative real-time combat and world simulation must run in separately deployed dedicated game servers.
