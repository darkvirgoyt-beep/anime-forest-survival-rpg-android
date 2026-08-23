import express from "express";
import pg from "pg";
import { OAuth2Client } from "google-auth-library";
import { pathToFileURL } from "node:url";
import { randomBytes } from "node:crypto";
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
    res.setHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
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

  app.post("/v1/coop/rooms", requireSession, async (req, res) => {
    const region = typeof req.body?.region === "string" ? req.body.region.trim().slice(0, 32) : "asia";
    const code = await createCoOpRoom(pool, req.accountId, region);
    const room = await getCoOpRoom(pool, code, req.accountId);
    res.status(201).json(room);
  });

  app.post("/v1/coop/rooms/:code/join", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    const roomResult = await pool.query("SELECT id, code, region, max_players FROM coop_rooms WHERE code = $1", [code]);
    if (roomResult.rowCount !== 1) return res.status(404).json({ error: "room_not_found" });
    const room = roomResult.rows[0];
    const active = await pool.query("SELECT COUNT(*)::int AS count FROM coop_members WHERE room_id = $1 AND account_id <> $2 AND last_seen_at > now() - interval '20 seconds'", [room.id, req.accountId]);
    if (Number(active.rows[0]?.count || 0) >= room.max_players) return res.status(409).json({ error: "room_full" });
    await pool.query(
      `INSERT INTO coop_members (room_id, account_id) VALUES ($1, $2)
       ON CONFLICT (room_id, account_id) DO UPDATE SET last_seen_at = now()`,
      [room.id, req.accountId]
    );
    res.json(await getCoOpRoom(pool, code, req.accountId));
  });

  app.get("/v1/coop/rooms/:code", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    const room = await getCoOpRoom(pool, code, req.accountId);
    if (!room) return res.status(404).json({ error: "room_not_found" });
    res.json(room);
  });

  app.post("/v1/coop/rooms/:code/heartbeat", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    const x = boundedNumber(req.body?.playerX, -0.90, 0.90, -0.55);
    const y = boundedNumber(req.body?.playerY, -0.50, 0.52, -0.08);
    const towerRevision = Math.max(0, Math.min(Number(req.body?.towerRevision) || 0, 2_000_000_000));
    const atTower = req.body?.atTower === true;
    const roomResult = await pool.query("SELECT id FROM coop_rooms WHERE code = $1", [code]);
    if (roomResult.rowCount !== 1) return res.status(404).json({ error: "room_not_found" });
    const roomId = roomResult.rows[0].id;
    const membership = await pool.query("SELECT 1 FROM coop_members WHERE room_id = $1 AND account_id = $2", [roomId, req.accountId]);
    if (membership.rowCount !== 1) return res.status(403).json({ error: "room_membership_required" });
    await pool.query(
      `INSERT INTO coop_members (room_id, account_id, player_x, player_y, at_tower, tower_revision, last_seen_at)
       VALUES ($1, $2, $3, $4, $5, $6, now())
       ON CONFLICT (room_id, account_id) DO UPDATE SET player_x = EXCLUDED.player_x, player_y = EXCLUDED.player_y,
         at_tower = EXCLUDED.at_tower, tower_revision = EXCLUDED.tower_revision, last_seen_at = now()`,
      [roomId, req.accountId, x, y, atTower, towerRevision]
    );
    if (atTower && towerRevision > 0) {
      await pool.query("UPDATE coop_rooms SET tower_revision = GREATEST(tower_revision, $2), updated_at = now() WHERE id = $1", [roomId, towerRevision]);
    }
    res.json(await getCoOpRoom(pool, code, req.accountId));
  });

  app.delete("/v1/coop/rooms/:code/leave", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    await pool.query("DELETE FROM coop_members WHERE room_id = (SELECT id FROM coop_rooms WHERE code = $1) AND account_id = $2", [code, req.accountId]);
    res.status(204).end();
  });

  app.post("/v1/coop/rooms/:code/combat", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    const requestId = normalizeRequestId(req.body?.requestId);
    const action = typeof req.body?.action === "string" ? req.body.action : "";
    const targetId = typeof req.body?.targetId === "string" ? req.body.targetId : "forest_warden";
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    if (!requestId || !["attack", "heavy_attack"].includes(action)) return res.status(400).json({ error: "invalid_combat_request" });
    const client = await pool.connect();
    try {
      await client.query("BEGIN");
      const roomResult = await client.query("SELECT id, boss_health, combat_revision FROM coop_rooms WHERE code = $1 FOR UPDATE", [code]);
      if (roomResult.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(404).json({ error: "room_not_found" });
      }
      const room = roomResult.rows[0];
      const memberResult = await client.query("SELECT player_x, player_y, last_action_at FROM coop_members WHERE room_id = $1 AND account_id = $2 FOR UPDATE", [room.id, req.accountId]);
      if (memberResult.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(403).json({ error: "room_membership_required" });
      }
      const duplicate = await client.query("SELECT result FROM coop_action_receipts WHERE room_id = $1 AND account_id = $2 AND request_id = $3", [room.id, req.accountId, requestId]);
      if (duplicate.rowCount === 1) {
        await client.query("COMMIT");
        return res.json(duplicate.rows[0].result);
      }
      if (targetId !== "forest_warden") {
        await client.query("ROLLBACK");
        return res.status(400).json({ error: "unknown_combat_target" });
      }
      const member = memberResult.rows[0];
      const distance = Math.abs(Number(member.player_x) - (-0.18)) + Math.abs(Number(member.player_y) - (-0.08));
      if (distance > 0.52) {
        await client.query("ROLLBACK");
        return res.status(409).json({ error: "combat_target_out_of_range" });
      }
      const cooldownMs = action === "heavy_attack" ? 900 : 350;
      const lastActionMs = member.last_action_at ? new Date(member.last_action_at).getTime() : 0;
      if (lastActionMs > 0 && Date.now() - lastActionMs < cooldownMs) {
        await client.query("ROLLBACK");
        return res.status(429).json({ error: "combat_cooldown" });
      }
      const damage = action === "heavy_attack" ? 24 : 12;
      const bossHealth = Math.max(0, Number(room.boss_health) - damage);
      const combatRevision = Number(room.combat_revision || 0) + 1;
      const result = { accepted: true, action, targetId, damage, bossHealth, combatRevision };
      await client.query("UPDATE coop_rooms SET boss_health = $2, combat_revision = $3, updated_at = now() WHERE id = $1", [room.id, bossHealth, combatRevision]);
      await client.query("UPDATE coop_members SET last_action_at = now(), last_seen_at = now() WHERE room_id = $1 AND account_id = $2", [room.id, req.accountId]);
      await client.query("INSERT INTO coop_action_receipts (room_id, account_id, request_id, action_type, result) VALUES ($1, $2, $3, 'combat', $4)", [room.id, req.accountId, requestId, result]);
      await client.query("COMMIT");
      res.json(result);
    } catch (error) {
      await client.query("ROLLBACK");
      throw error;
    } finally {
      client.release();
    }
  });

  app.post("/v1/coop/rooms/:code/inventory", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    const requestId = normalizeRequestId(req.body?.requestId);
    const operation = typeof req.body?.operation === "string" ? req.body.operation : "";
    const resourceId = typeof req.body?.resourceId === "string" ? req.body.resourceId : "";
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    if (!requestId || !["gather", "craft"].includes(operation)) return res.status(400).json({ error: "invalid_inventory_request" });
    const client = await pool.connect();
    try {
      await client.query("BEGIN");
      const roomResult = await client.query("SELECT id FROM coop_rooms WHERE code = $1 FOR UPDATE", [code]);
      if (roomResult.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(404).json({ error: "room_not_found" });
      }
      const roomId = roomResult.rows[0].id;
      const memberResult = await client.query("SELECT player_x, player_y, wood, fiber, stone, inventory_revision, ember_kit FROM coop_members WHERE room_id = $1 AND account_id = $2 FOR UPDATE", [roomId, req.accountId]);
      if (memberResult.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(403).json({ error: "room_membership_required" });
      }
      const duplicate = await client.query("SELECT result FROM coop_action_receipts WHERE room_id = $1 AND account_id = $2 AND request_id = $3", [roomId, req.accountId, requestId]);
      if (duplicate.rowCount === 1) {
        await client.query("COMMIT");
        return res.json(duplicate.rows[0].result);
      }
      const member = memberResult.rows[0];
      let wood = Number(member.wood);
      let fiber = Number(member.fiber);
      let stone = Number(member.stone);
      let emberKit = Boolean(member.ember_kit);
      if (operation === "gather") {
        const resources = {
          forest_cache: { x: -0.56, y: -0.28, wood: 1, fiber: 2, stone: 0 },
          root_cache: { x: -0.40, y: -0.18, wood: 1, fiber: 1, stone: 0 },
          warden_stone: { x: -0.24, y: -0.28, wood: 0, fiber: 0, stone: 2 }
        };
        const resource = resources[resourceId];
        if (!resource) {
          await client.query("ROLLBACK");
          return res.status(400).json({ error: "unknown_resource" });
        }
        const distance = Math.abs(Number(member.player_x) - resource.x) + Math.abs(Number(member.player_y) - resource.y);
        if (distance > 0.32) {
          await client.query("ROLLBACK");
          return res.status(409).json({ error: "resource_out_of_range" });
        }
        wood = Math.min(999, wood + resource.wood);
        fiber = Math.min(999, fiber + resource.fiber);
        stone = Math.min(999, stone + resource.stone);
      } else {
        if (wood < 3 || fiber < 2) {
          await client.query("ROLLBACK");
          return res.status(409).json({ error: "insufficient_crafting_materials" });
        }
        wood -= 3;
        fiber -= 2;
        emberKit = true;
      }
      const inventoryRevision = Number(member.inventory_revision || 0) + 1;
      const result = { accepted: true, operation, inventory: { wood, fiber, stone, emberKit }, inventoryRevision };
      await client.query("UPDATE coop_members SET wood = $3, fiber = $4, stone = $5, ember_kit = $6, inventory_revision = $7, last_seen_at = now() WHERE room_id = $1 AND account_id = $2", [roomId, req.accountId, wood, fiber, stone, emberKit, inventoryRevision]);
      await client.query("INSERT INTO coop_action_receipts (room_id, account_id, request_id, action_type, result) VALUES ($1, $2, $3, 'inventory', $4)", [roomId, req.accountId, requestId, result]);
      await client.query("COMMIT");
      res.json(result);
    } catch (error) {
      await client.query("ROLLBACK");
      throw error;
    } finally {
      client.release();
    }
  });

  app.use((_req, res) => res.status(404).json({ error: "not_found" }));
  app.use((error, _req, res, _next) => {
    console.error("unhandled_request_error", safeErrorCode(error));
    res.status(500).json({ error: "internal_server_error" });
  });
  return app;
}

const COOP_ROOM_CODE = /^[A-Z0-9]{6}$/;

function normalizeRoomCode(value) {
  const code = typeof value === "string" ? value.trim().toUpperCase() : "";
  return COOP_ROOM_CODE.test(code) ? code : null;
}

function normalizeRequestId(value) {
  const requestId = typeof value === "string" ? value.trim() : "";
  return /^[A-Za-z0-9_-]{8,80}$/.test(requestId) ? requestId : null;
}

function boundedNumber(value, min, max, fallback) {
  const number = Number(value);
  return Number.isFinite(number) ? Math.max(min, Math.min(max, number)) : fallback;
}

async function createCoOpRoom(pool, accountId, region) {
  for (let attempt = 0; attempt < 5; attempt += 1) {
    const code = randomBytes(3).toString("hex").toUpperCase();
    try {
      const created = await pool.query(
        "INSERT INTO coop_rooms (code, region, created_by) VALUES ($1, $2, $3) RETURNING id",
        [code, region, accountId]
      );
      if (created.rowCount === 1) {
        await pool.query("INSERT INTO coop_members (room_id, account_id) VALUES ($1, $2)", [created.rows[0].id, accountId]);
        return code;
      }
    } catch (error) {
      if (error?.code !== "23505") throw error;
    }
  }
  throw new Error("coop_room_code_generation_failed");
}

async function getCoOpRoom(pool, code, accountId) {
  const roomResult = await pool.query(
    `SELECT id, code, region, max_players, world_time, tower_revision, boss_health, combat_revision
     FROM coop_rooms WHERE code = $1`,
    [code]
  );
  if (roomResult.rowCount !== 1) return null;
  const room = roomResult.rows[0];
  const membership = await pool.query("SELECT 1 FROM coop_members WHERE room_id = $1 AND account_id = $2", [room.id, accountId]);
  if (membership.rowCount !== 1) return null;
  await pool.query(
    `UPDATE coop_rooms
     SET world_time = world_time + EXTRACT(EPOCH FROM (now() - last_tick_at)), last_tick_at = now(), updated_at = now()
     WHERE id = $1`,
    [room.id]
  );
  const current = await pool.query(
    `SELECT account_id, player_x, player_y, at_tower, tower_revision
     FROM coop_members WHERE room_id = $1 AND last_seen_at > now() - interval '20 seconds'
     ORDER BY last_seen_at DESC`,
    [room.id]
  );
  const refreshed = await pool.query("SELECT world_time, tower_revision, boss_health, combat_revision FROM coop_rooms WHERE id = $1", [room.id]);
  return {
    room: {
      code: room.code,
      region: room.region,
      maxPlayers: room.max_players,
      worldTime: Number(refreshed.rows[0]?.world_time || room.world_time || 0),
      towerRevision: Number(refreshed.rows[0]?.tower_revision || room.tower_revision || 0),
      bossHealth: Number(refreshed.rows[0]?.boss_health ?? room.boss_health ?? 100),
      combatRevision: Number(refreshed.rows[0]?.combat_revision ?? room.combat_revision ?? 0)
    },
    participants: current.rows.map(member => ({
      accountId: member.account_id,
      playerX: Number(member.player_x),
      playerY: Number(member.player_y),
      atTower: Boolean(member.at_tower),
      towerRevision: Number(member.tower_revision || 0)
    }))
  };
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
