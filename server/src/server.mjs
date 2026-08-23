import express from "express";
import pg from "pg";
import { OAuth2Client } from "google-auth-library";
import { pathToFileURL } from "node:url";
import {
  createOpaqueToken,
  hashSecret,
  issueAccessToken,
  loadRuntimeConfig,
  validateGoogleIdToken,
  validateServerAuthCode,
  verifyAccessToken
} from "./security.mjs";

const { Pool } = pg;
const googleIdTokenVerifier = new OAuth2Client();

export function createOnlineService({ pool, config, fetchImpl = fetch, verifyGoogleIdTokenImpl = verifyGoogleIdToken }) {
  const app = express();
  app.disable("x-powered-by");
  app.use(express.json({ limit: "16kb", strict: true }));
  app.use((req, res, next) => {
    const origin = req.get("origin");
    if (origin && origin !== config.allowedOrigin) return res.status(403).json({ error: "origin_not_allowed" });
    if (origin) {
      res.setHeader("Access-Control-Allow-Origin", config.allowedOrigin);
      res.setHeader("Vary", "Origin");
    }
    res.setHeader("Access-Control-Allow-Headers", "Authorization, Content-Type");
    res.setHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    if (req.method === "OPTIONS") return res.sendStatus(204);
    next();
  });

  const requireSession = createSessionGuard({ pool, config });

  app.get("/healthz", async (_req, res) => {
    try {
      await pool.query("SELECT 1");
      res.json({ ok: true, service: "aethelgard-online-services" });
    } catch {
      res.status(503).json({ ok: false, service: "aethelgard-online-services" });
    }
  });

  app.post("/v1/auth/play-games/exchange", async (req, res) => {
    if (!config.googlePlayGamesClientId || !config.googlePlayGamesClientSecret) {
      return res.status(503).json({ error: "play_games_not_configured" });
    }
    const serverAuthCode = req.body?.serverAuthCode;
    if (!validateServerAuthCode(serverAuthCode)) return res.status(400).json({ error: "invalid_server_auth_code" });

    const codeHash = hashSecret(serverAuthCode);
    const receipt = await pool.query(
      "INSERT INTO authorization_code_receipts (code_hash, exchange_state) VALUES ($1, 'received') ON CONFLICT DO NOTHING RETURNING code_hash",
      [codeHash]
    );
    if (receipt.rowCount !== 1) return res.status(409).json({ error: "replayed_server_auth_code" });

    try {
      const googleToken = await exchangePlayGamesCode(serverAuthCode, config, fetchImpl);
      const player = await verifyPlayGamesPlayer(googleToken.access_token, fetchImpl);
      const account = await upsertAccount(pool, player);
      const bundle = await createSessionBundle(pool, account.id, config);
      await pool.query("UPDATE authorization_code_receipts SET exchange_state = 'verified' WHERE code_hash = $1", [codeHash]);
      res.status(200).json({
        accessToken: bundle.accessToken,
        refreshToken: bundle.refreshToken,
        tokenType: "Bearer",
        accountId: account.id,
        expiresAt: new Date(bundle.accessExpiresAt * 1000).toISOString(),
        refreshExpiresAt: new Date(bundle.refreshExpiresAt * 1000).toISOString()
      });
    } catch (error) {
      await pool.query(
        "UPDATE authorization_code_receipts SET exchange_state = 'rejected', rejection_code = $2 WHERE code_hash = $1",
        [codeHash, safeErrorCode(error)]
      );
      console.error("play_games_exchange_failed", safeErrorCode(error));
      res.status(401).json({ error: "play_games_authentication_failed" });
    }
  });

  app.post("/v1/auth/google-id-token/exchange", async (req, res) => {
    const idToken = req.body?.idToken;
    if (!validateGoogleIdToken(idToken)) return res.status(400).json({ error: "invalid_google_id_token" });

    try {
      const identity = await verifyGoogleIdTokenImpl(idToken, config);
      const account = await upsertGoogleAccount(pool, identity);
      const bundle = await createSessionBundle(pool, account.id, config);
      res.status(200).json({
        accessToken: bundle.accessToken,
        refreshToken: bundle.refreshToken,
        tokenType: "Bearer",
        accountId: account.id,
        expiresAt: new Date(bundle.accessExpiresAt * 1000).toISOString(),
        refreshExpiresAt: new Date(bundle.refreshExpiresAt * 1000).toISOString()
      });
    } catch (error) {
      console.error("google_id_token_exchange_failed", safeErrorCode(error));
      res.status(401).json({ error: "google_id_token_authentication_failed" });
    }
  });

  app.post("/v1/auth/refresh", async (req, res) => {
    const refreshToken = req.body?.refreshToken;
    if (typeof refreshToken !== "string" || refreshToken.length < 32 || refreshToken.length > 512) {
      return res.status(400).json({ error: "invalid_refresh_token" });
    }

    const client = await pool.connect();
    try {
      await client.query("BEGIN");
      const result = await client.query(
        `SELECT id, account_id, refresh_expires_at, revoked_at
         FROM sessions
         WHERE refresh_token_hash = $1
         FOR UPDATE`,
        [hashSecret(refreshToken)]
      );
      if (result.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(401).json({ error: "invalid_refresh_session" });
      }

      const current = result.rows[0];
      if (current.revoked_at) {
        await client.query(
          "UPDATE sessions SET revoked_at = COALESCE(revoked_at, now()), revoked_reason = COALESCE(revoked_reason, 'refresh_reuse_detected') WHERE account_id = $1 AND revoked_at IS NULL",
          [current.account_id]
        );
        await client.query("COMMIT");
        return res.status(401).json({ error: "replayed_refresh_session" });
      }
      if (!current.refresh_expires_at || new Date(current.refresh_expires_at).getTime() <= Date.now()) {
        await client.query("UPDATE sessions SET revoked_at = now(), revoked_reason = 'refresh_expired' WHERE id = $1", [current.id]);
        await client.query("COMMIT");
        return res.status(401).json({ error: "expired_refresh_session" });
      }

      const next = await createSessionBundle(client, current.account_id, config, current.id);
      await client.query(
        "UPDATE sessions SET revoked_at = now(), revoked_reason = 'rotated', replaced_by_session_id = $2 WHERE id = $1",
        [current.id, next.sessionId]
      );
      await client.query("COMMIT");
      return res.status(200).json({
        accessToken: next.accessToken,
        refreshToken: next.refreshToken,
        tokenType: "Bearer",
        expiresAt: new Date(next.accessExpiresAt * 1000).toISOString(),
        refreshExpiresAt: new Date(next.refreshExpiresAt * 1000).toISOString()
      });
    } catch (error) {
      await client.query("ROLLBACK");
      console.error("refresh_failed", safeErrorCode(error));
      return res.status(500).json({ error: "session_refresh_failed" });
    } finally {
      client.release();
    }
  });

  app.get("/v1/auth/session", requireSession, (req, res) => {
    res.json({ accountId: req.accountId, sessionId: req.sessionId, status: "authenticated" });
  });

  app.post("/v1/auth/logout", requireSession, async (req, res) => {
    await pool.query(
      "UPDATE sessions SET revoked_at = now(), revoked_reason = 'player_logout' WHERE id = $1 AND account_id = $2 AND revoked_at IS NULL",
      [req.sessionId, req.accountId]
    );
    res.status(204).end();
  });

  app.get("/v1/worlds", requireSession, async (_req, res) => {
    const result = await pool.query(
      "SELECT id, region, name, status, max_players, current_players FROM worlds WHERE status = 'online' ORDER BY region, name LIMIT 50"
    );
    res.json({ worlds: result.rows });
  });

  app.post("/v1/worlds", requireSession, async (req, res) => {
    const region = typeof req.body?.region === "string" ? req.body.region.trim().slice(0, 32) : "asia";
    const name = typeof req.body?.name === "string" ? req.body.name.trim().slice(0, 64) : "Aethelgard Forest";
    if (!name) return res.status(400).json({ error: "world_name_required" });
    const result = await pool.query(
      "INSERT INTO worlds (region, name, status, max_players, current_players) VALUES ($1, $2, 'allocating', 4, 0) RETURNING id, region, name, status, max_players, current_players",
      [region, name]
    );
    res.status(201).json({ world: result.rows[0] });
  });

  app.use((_req, res) => res.status(404).json({ error: "not_found" }));
  app.use((error, _req, res, _next) => {
    console.error("unhandled_request_error", safeErrorCode(error));
    res.status(500).json({ error: "internal_server_error" });
  });
  return app;
}

export async function exchangePlayGamesCode(serverAuthCode, config, fetchImpl = fetch) {
  const body = new URLSearchParams({
    client_id: config.googlePlayGamesClientId,
    client_secret: config.googlePlayGamesClientSecret,
    code: serverAuthCode,
    grant_type: "authorization_code",
    redirect_uri: ""
  });
  const response = await fetchImpl("https://oauth2.googleapis.com/token", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body
  });
  if (!response.ok) throw new Error(`google_token_exchange_${response.status}`);
  const payload = await response.json();
  if (typeof payload.access_token !== "string") throw new Error("google_access_token_missing");
  return payload;
}

export async function verifyPlayGamesPlayer(accessToken, fetchImpl = fetch) {
  const response = await fetchImpl("https://www.googleapis.com/games/v1/players/me", {
    headers: { Authorization: `Bearer ${accessToken}`, Accept: "application/json" }
  });
  if (!response.ok) throw new Error(`google_player_verification_${response.status}`);
  const player = await response.json();
  if (typeof player.playerId !== "string" || player.playerId.length === 0) throw new Error("google_player_id_missing");
  return player;
}

export async function verifyGoogleIdToken(idToken, config, client = googleIdTokenVerifier) {
  const ticket = await client.verifyIdToken({ idToken, audience: config.googleIdTokenAudience });
  const payload = ticket.getPayload();
  if (!payload || typeof payload.sub !== "string" || payload.sub.length === 0 || payload.sub.length > 255) {
    throw new Error("google_subject_missing");
  }
  return {
    subject: payload.sub,
    displayName: typeof payload.name === "string" ? payload.name : "Wayfarer"
  };
}

async function upsertAccount(db, player) {
  const result = await db.query(
    `INSERT INTO accounts (provider, provider_player_id, display_name)
     VALUES ('google_play_games', $1, $2)
     ON CONFLICT (provider, provider_player_id)
     DO UPDATE SET display_name = EXCLUDED.display_name, updated_at = now()
     RETURNING id`,
    [player.playerId, String(player.displayName || "Player").slice(0, 80)]
  );
  return result.rows[0];
}

async function upsertGoogleAccount(db, identity) {
  const result = await db.query(
    `INSERT INTO accounts (provider, provider_player_id, display_name)
     VALUES ('google_openid', $1, $2)
     ON CONFLICT (provider, provider_player_id)
     DO UPDATE SET display_name = EXCLUDED.display_name, updated_at = now()
     RETURNING id`,
    [identity.subject, String(identity.displayName || "Wayfarer").slice(0, 80)]
  );
  return result.rows[0];
}

async function createSessionBundle(db, accountId, config, parentSessionId = null) {
  const now = Date.now();
  const refreshToken = createOpaqueToken();
  const placeholder = createOpaqueToken();
  const accessExpiresAt = Math.floor(now / 1000) + config.accessTtlSeconds;
  const refreshExpiresAt = Math.floor(now / 1000) + config.refreshTtlSeconds;
  const inserted = await db.query(
    `INSERT INTO sessions (account_id, token_hash, expires_at, refresh_token_hash, refresh_expires_at, parent_session_id)
     VALUES ($1, $2, to_timestamp($3), $4, to_timestamp($5), $6)
     RETURNING id`,
    [accountId, hashSecret(placeholder), hashSecret(refreshToken), refreshExpiresAt, parentSessionId]
  );
  const sessionId = inserted.rows[0].id;
  const access = issueAccessToken({
    accountId,
    sessionId,
    secret: config.sessionSecret,
    now,
    ttlSeconds: config.accessTtlSeconds
  });
  await db.query("UPDATE sessions SET token_hash = $2 WHERE id = $1", [sessionId, hashSecret(access.token)]);
  return {
    sessionId,
    accessToken: access.token,
    refreshToken,
    accessExpiresAt,
    refreshExpiresAt
  };
}

function createSessionGuard({ pool, config }) {
  return async (req, res, next) => {
    const authorization = req.get("authorization") || "";
    const accessToken = authorization.startsWith("Bearer ") ? authorization.slice(7) : "";
    try {
      const payload = verifyAccessToken(accessToken, config.sessionSecret);
      const active = await pool.query(
        `SELECT id, account_id
         FROM sessions
         WHERE id = $1 AND account_id = $2 AND token_hash = $3 AND expires_at > now() AND revoked_at IS NULL
         LIMIT 1`,
        [payload.sid, payload.sub, hashSecret(accessToken)]
      );
      if (active.rowCount !== 1) return res.status(401).json({ error: "revoked_session" });
      await pool.query("UPDATE sessions SET last_seen_at = now() WHERE id = $1", [payload.sid]);
      req.accountId = payload.sub;
      req.sessionId = Number(payload.sid);
      next();
    } catch (error) {
      const code = safeErrorCode(error);
      res.status(401).json({ error: code === "expired_session" ? "expired_session" : "invalid_session" });
    }
  };
}

function safeErrorCode(error) {
  return error instanceof Error && /^[a-z0-9_:-]{1,120}$/i.test(error.message) ? error.message : "unknown_error";
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const config = loadRuntimeConfig();
  const pool = new Pool({
    connectionString: config.databaseUrl,
    max: 10,
    ssl: config.databaseSsl ? { rejectUnauthorized: true } : undefined
  });
  const port = Number(process.env.PORT || 8080);
  createOnlineService({ pool, config }).listen(port, () => {
    console.log(`Aethelgard online services listening on :${port}`);
  });
}
