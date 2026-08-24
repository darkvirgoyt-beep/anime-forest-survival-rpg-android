# Aethelgard small online backend

Aethelgard uses a deliberately small Node.js and PostgreSQL HTTPS service for authentication, creator-owned persistent worlds, four-player co-op presence, server-validated combat and inventory actions, player saves, world saves, companions, camps, and village buildings. It is not the Unreal dedicated-server fleet; it is the lightweight account and co-op service used by the Android game layer.

## Free deployment shape

The repository includes `render.yaml` and `server/Dockerfile` for a free Render web service. Render provides free web-service instances with HTTPS, but the free instance can sleep after inactivity and its local filesystem is ephemeral. Persistent world data must remain in PostgreSQL, not in local files. A free Render PostgreSQL database is suitable for a short development test, but Render documents that its free PostgreSQL databases expire after 30 days. A free Supabase PostgreSQL database can be used instead when longer-lived free development storage is needed.

Create the web service from the GitHub repository `darkvirgoyt-beep/anime-forest-survival-rpg-android`, using the repository Blueprint. The service uses `server/Dockerfile`, runs on the `PORT` supplied by the host, exposes `GET /healthz`, and runs `node scripts/migrate.mjs` before startup. The migration runner applies SQL files `001_init.sql` through `007_companion_camp_authority.sql` once each and records them in `schema_migrations`.

## Required server environment

Set these values in the hosting dashboard or secret manager. Never commit a database URL or session secret.

| Variable | Value |
|---|---|
| `NODE_ENV` | `production` |
| `PORT` | Supplied by the host; the Blueprint uses `10000`. |
| `DATABASE_URL` | The PostgreSQL connection URL from the selected database provider. |
| `DATABASE_SSL` | `true` for a hosted PostgreSQL provider using TLS. |
| `GOOGLE_ID_TOKEN_AUDIENCE` | The Web OAuth client ID used by Android, currently `1062428369173-q4thoceukcd15r0cni75gfct25j1fk04.apps.googleusercontent.com`. |
| `GAME_SESSION_JWT_SECRET` | A newly generated random value of at least 32 characters. |
| `GAME_ACCESS_TOKEN_TTL_SECONDS` | `900` |
| `GAME_REFRESH_TOKEN_TTL_SECONDS` | `2592000` |
| `ALLOWED_ORIGIN` | The HTTPS service or game web origin. Android requests without a browser Origin are still accepted. |
| `GOOGLE_GAME_SERVER_CLIENT_ID` | Leave empty until Play Games Services is configured. |
| `GOOGLE_GAME_SERVER_CLIENT_SECRET` | Leave empty until Play Games Services is configured. |

After deployment, verify that `https://YOUR-SERVICE.onrender.com/healthz` returns JSON similar to `{"ok":true,"service":"aethelgard-online-services"}`. An HTML page or a `502 upstream_connect_failed` response is not a working API endpoint. If Render says the container is live but `/healthz` returns HTTP 503 with `{"ok":false,"service":"aethelgard-online-services"}`, the Node process and routes are running but its PostgreSQL `SELECT 1` readiness check failed. In that case, repair `DATABASE_URL`/`DATABASE_SSL` or the database availability and rerun the migration/startup logs; changing the Android login code cannot make a database-backed session exchange succeed.

## Android endpoint

The Android strings must point to the deployed service with `/v1` appended:

```xml
<string name="api_base_url">https://YOUR-SERVICE.onrender.com/v1</string>
<string name="auth_exchange_url">https://YOUR-SERVICE.onrender.com/v1/auth/google-id-token/exchange</string>
<string name="auth_guest_url">https://YOUR-SERVICE.onrender.com/v1/auth/guest</string>
<string name="auth_refresh_url">https://YOUR-SERVICE.onrender.com/v1/auth/refresh</string>
```

The current repository default points to `https://aethelgard-api-v2.onrender.com/v1`, which matches the included Blueprint name. If the host assigns a different service hostname, update the four Android URLs and publish a new Android version.

## Google sign-in certificate

Standard Google account sign-in does not require Play Console. The Android OAuth client must contain the package name `com.darvirgoyt.aethelgrad` and the SHA-1 certificate for the exact APK being installed. The GitHub debug-signed test artifact uses the certificate fingerprint printed by the workflow signing-certificate artifact. If a different local keystore signs the APK, add that keystore's SHA-1 as another fingerprint on the same Android OAuth client. The Web OAuth client ID must remain the audience configured in both Android resources and `GOOGLE_ID_TOKEN_AUDIENCE`.

## Four-player service rule

The online service enforces exactly four active players per creator-owned world. The creator occupies one slot and at most three other players can be active. Join and reconnect both return `room_full` when four active members are present. Leaving does not delete a member's item or progression state; reconnect becomes possible after a slot opens.

## Verification

Use the following checks before publishing a server or Android build:

```bash
python3 tools/test_online_only_contract.py
python3 tools/test_persistent_multiplayer_contract.py
(cd server && npm test)
```

The service is small enough for free development testing, but a public AAA production launch still requires durable PostgreSQL backups, monitoring, abuse controls, rate limiting, and a dedicated authoritative Unreal server fleet.

## References

1. [Render — Deploy for Free](https://render.com/docs/free)
2. [Render — Web Services](https://render.com/docs/web-services)
3. [Render — Create and Connect to Render Postgres](https://render.com/docs/postgresql-creating-connecting)
