import express from "express";
import pg from "pg";
import { OAuth2Client } from "google-auth-library";
import { pathToFileURL } from "node:url";
import path from "node:path";
import { randomBytes } from "node:crypto";
import {
  createOpaqueToken,
  hashSecret,
  issueAccessToken,
  loadRuntimeConfig,
  validateGoogleIdToken,
  validateGuestKey,
  validateServerAuthCode,
  verifyAccessToken
} from "./security.mjs";
import {
  COMPANION_TARGET_SEEDS,
  companionProfile,
  validateCampPlacement,
  validateCaptureRequest
} from "./companion-camp-authority.mjs";

const { Pool } = pg;
const googleIdTokenVerifier = new OAuth2Client();
const MAX_COOP_PLAYERS = 4;

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

  app.get("/v1/content/high/manifest", (_req, res) => {
    if (!config.privateContentManifestPath || !path.isAbsolute(config.privateContentManifestPath)) {
      return res.status(503).json({ error: "private_content_not_configured" });
    }
    res.setHeader("Cache-Control", "no-store");
    return res.type("application/json").sendFile(config.privateContentManifestPath, { dotfiles: "deny" }, (error) => {
      if (error && !res.headersSent) res.status(error.statusCode || 500).json({ error: "private_content_manifest_unavailable" });
    });
  });

  app.get("/v1/content/high/archive", (_req, res) => {
    if (!config.privateContentArchivePath || !path.isAbsolute(config.privateContentArchivePath)) {
      return res.status(503).json({ error: "private_content_not_configured" });
    }
    res.setHeader("Cache-Control", "no-store");
    res.setHeader("Accept-Ranges", "bytes");
    return res.sendFile(config.privateContentArchivePath, { dotfiles: "deny" }, (error) => {
      if (error && !res.headersSent) res.status(error.statusCode || 500).json({ error: "private_content_archive_unavailable" });
    });
  });

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

  app.post("/v1/auth/guest", async (req, res) => {
    const guestKey = req.body?.guestKey;
    if (!validateGuestKey(guestKey)) return res.status(400).json({ error: "invalid_guest_key" });

    try {
      const account = await upsertGuestAccount(pool, guestKey);
      const bundle = await createSessionBundle(pool, account.id, config);
      res.status(200).json({
        accessToken: bundle.accessToken,
        refreshToken: bundle.refreshToken,
        tokenType: "Bearer",
        accountId: account.id,
        accountType: "guest",
        expiresAt: new Date(bundle.accessExpiresAt * 1000).toISOString(),
        refreshExpiresAt: new Date(bundle.refreshExpiresAt * 1000).toISOString()
      });
    } catch (error) {
      console.error("guest_authentication_failed", safeErrorCode(error));
      res.status(503).json({ error: "guest_authentication_unavailable" });
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
    const name = typeof req.body?.name === "string" ? req.body.name.trim().slice(0, 64) : "Aethelgrad Forest";
    if (!name) return res.status(400).json({ error: "world_name_required" });
    const result = await pool.query(
      "INSERT INTO worlds (region, name, status, max_players, current_players) VALUES ($1, $2, 'allocating', 4, 0) RETURNING id, region, name, status, max_players, current_players",
      [region, name]
    );
    res.status(201).json({ world: result.rows[0] });
  });

  app.post("/v1/coop/rooms", requireSession, async (req, res) => {
    const region = typeof req.body?.region === "string" ? req.body.region.trim().slice(0, 32) : "asia";
    const worldName = typeof req.body?.worldName === "string" ? req.body.worldName.trim().slice(0, 64) : "Aethelgrad Shared World";
    if (!worldName) return res.status(400).json({ error: "world_name_required" });
    const code = await createCoOpRoom(pool, req.accountId, region, worldName);
    const room = await getCoOpRoom(pool, code, req.accountId);
    res.status(201).json(room);
  });

  app.post("/v1/coop/rooms/:code/join", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    const roomResult = await pool.query("SELECT id, code, region, max_players FROM coop_rooms WHERE code = $1", [code]);
    if (roomResult.rowCount !== 1) return res.status(404).json({ error: "room_not_found" });
    const room = roomResult.rows[0];
    const active = await pool.query("SELECT COUNT(*)::int AS count FROM coop_members WHERE room_id = $1 AND account_id <> $2 AND is_active = TRUE AND last_seen_at > now() - interval '20 seconds'", [room.id, req.accountId]);
    if (Number(active.rows[0]?.count || 0) >= MAX_COOP_PLAYERS) return res.status(409).json({ error: "room_full", maxPlayers: MAX_COOP_PLAYERS });
    await pool.query(
      `INSERT INTO coop_members (room_id, account_id, is_active) VALUES ($1, $2, TRUE)
       ON CONFLICT (room_id, account_id) DO UPDATE SET is_active = TRUE, last_seen_at = now()`,
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

  app.post("/v1/coop/rooms/:code/reconnect", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    const roomResult = await pool.query("SELECT id FROM coop_rooms WHERE code = $1", [code]);
    if (roomResult.rowCount !== 1) return res.status(404).json({ error: "room_not_found" });
    const roomId = roomResult.rows[0].id;
    const membership = await pool.query("SELECT 1 FROM coop_members WHERE room_id = $1 AND account_id = $2", [roomId, req.accountId]);
    if (membership.rowCount !== 1) return res.status(403).json({ error: "room_membership_required" });
    const active = await pool.query("SELECT COUNT(*)::int AS count FROM coop_members WHERE room_id = $1 AND account_id <> $2 AND is_active = TRUE AND last_seen_at > now() - interval '20 seconds'", [roomId, req.accountId]);
    if (Number(active.rows[0]?.count || 0) >= MAX_COOP_PLAYERS) return res.status(409).json({ error: "room_full", maxPlayers: MAX_COOP_PLAYERS });
    await pool.query("UPDATE coop_members SET is_active = TRUE, last_seen_at = now() WHERE room_id = $1 AND account_id = $2", [roomId, req.accountId]);
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
    await pool.query("UPDATE coop_members SET is_active = FALSE, last_seen_at = now() WHERE room_id = (SELECT id FROM coop_rooms WHERE code = $1) AND account_id = $2", [code, req.accountId]);
    res.status(204).end();
  });

  app.get("/v1/coop/rooms/:code/player-save", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    const result = await pool.query(
      `SELECT m.item_state, m.progression_state, m.member_revision, r.created_by, r.world_name
       FROM coop_members m JOIN coop_rooms r ON r.id = m.room_id
       WHERE r.code = $1 AND m.account_id = $2`,
      [code, req.accountId]
    );
    if (result.rowCount !== 1) return res.status(403).json({ error: "world_membership_required" });
    const row = result.rows[0];
    res.json({
      ownerAccountId: row.created_by,
      worldName: row.world_name,
      memberRevision: Number(row.member_revision || 0),
      itemState: row.item_state || {},
      progressionState: row.progression_state || {}
    });
  });

  app.put("/v1/coop/rooms/:code/player-save", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    const expectedRevision = Number.isInteger(req.body?.expectedRevision) ? req.body.expectedRevision : -1;
    const itemState = req.body?.itemState;
    const progressionState = req.body?.progressionState;
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    if (expectedRevision < 0 || !isPlainObject(itemState) || !isPlainObject(progressionState)) return res.status(400).json({ error: "invalid_player_save" });
    if (JSON.stringify(itemState).length > 64_000 || JSON.stringify(progressionState).length > 64_000) return res.status(413).json({ error: "player_save_too_large" });
    const result = await pool.query(
      `UPDATE coop_members SET item_state = $3, progression_state = $4, member_revision = member_revision + 1, last_seen_at = now()
       WHERE room_id = (SELECT id FROM coop_rooms WHERE code = $1) AND account_id = $2 AND member_revision = $5
       RETURNING member_revision, item_state, progression_state`,
      [code, req.accountId, itemState, progressionState, expectedRevision]
    );
    if (result.rowCount !== 1) return res.status(409).json({ error: "player_save_revision_conflict" });
    const row = result.rows[0];
    res.json({ memberRevision: Number(row.member_revision), itemState: row.item_state, progressionState: row.progression_state });
  });

  app.get("/v1/coop/rooms/:code/save", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    const room = await pool.query("SELECT id, created_by, world_name FROM coop_rooms WHERE code = $1", [code]);
    if (room.rowCount !== 1) return res.status(404).json({ error: "world_not_found" });
    const membership = await pool.query("SELECT 1 FROM coop_members WHERE room_id = $1 AND account_id = $2", [room.rows[0].id, req.accountId]);
    if (membership.rowCount !== 1) return res.status(403).json({ error: "world_membership_required" });
    const save = await pool.query("SELECT world_state, save_revision, updated_at FROM coop_world_saves WHERE room_id = $1", [room.rows[0].id]);
    const buildings = await pool.query("SELECT id, building_type, placed_by, transform, state, created_at, updated_at FROM coop_buildings WHERE room_id = $1 ORDER BY created_at", [room.rows[0].id]);
    const row = save.rows[0] || { world_state: {}, save_revision: 0, updated_at: null };
    res.json({ ownerAccountId: room.rows[0].created_by, worldName: room.rows[0].world_name, saveRevision: Number(row.save_revision || 0), updatedAt: row.updated_at, worldState: row.world_state || {}, buildings: buildings.rows });
  });

  app.put("/v1/coop/rooms/:code/save", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    const expectedRevision = Number.isInteger(req.body?.expectedRevision) ? req.body.expectedRevision : -1;
    const worldState = req.body?.worldState;
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    if (!isPlainObject(worldState) || expectedRevision < 0) return res.status(400).json({ error: "invalid_world_save" });
    if (JSON.stringify(worldState).length > 256_000) return res.status(413).json({ error: "world_save_too_large" });
    const owner = await pool.query("SELECT id, created_by FROM coop_rooms WHERE code = $1", [code]);
    if (owner.rowCount !== 1) return res.status(404).json({ error: "world_not_found" });
    const membership = await pool.query("SELECT 1 FROM coop_members WHERE room_id = $1 AND account_id = $2", [owner.rows[0].id, req.accountId]);
    if (membership.rowCount !== 1) return res.status(403).json({ error: "world_membership_required" });
    if (owner.rows[0].created_by !== req.accountId) return res.status(403).json({ error: "world_owner_required" });
    const saved = await pool.query(
      `UPDATE coop_world_saves SET world_state = $2, save_revision = save_revision + 1, updated_by = $3, updated_at = now()
       WHERE room_id = $1 AND save_revision = $4
       RETURNING save_revision, updated_at`,
      [owner.rows[0].id, worldState, req.accountId, expectedRevision]
    );
    if (saved.rowCount !== 1) return res.status(409).json({ error: "world_save_revision_conflict" });
    res.json({ saveRevision: Number(saved.rows[0].save_revision), updatedAt: saved.rows[0].updated_at });
  });

  app.post("/v1/coop/rooms/:code/buildings", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    const buildingType = typeof req.body?.buildingType === "string" ? req.body.buildingType.trim().slice(0, 64) : "";
    const transform = req.body?.transform;
    const state = req.body?.state && isPlainObject(req.body.state) ? req.body.state : {};
    if (!code || !buildingType || !isPlainObject(transform)) return res.status(400).json({ error: "invalid_building" });
    if (JSON.stringify(transform).length > 8_000 || JSON.stringify(state).length > 16_000) return res.status(413).json({ error: "building_state_too_large" });
    const result = await pool.query(
      `INSERT INTO coop_buildings (room_id, building_type, placed_by, transform, state)
       SELECT r.id, $2, $3, $4, $5 FROM coop_rooms r JOIN coop_members m ON m.room_id = r.id AND m.account_id = $3 AND m.is_active = TRUE WHERE r.code = $1
       RETURNING id, building_type, placed_by, transform, state, created_at, updated_at`,
      [code, buildingType, req.accountId, transform, state]
    );
    if (result.rowCount !== 1) return res.status(403).json({ error: "active_world_membership_required" });
    res.status(201).json({ building: result.rows[0] });
  });

  app.get("/v1/coop/rooms/:code/companions", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    const room = await pool.query("SELECT id FROM coop_rooms WHERE code = $1", [code]);
    if (room.rowCount !== 1) return res.status(404).json({ error: "room_not_found" });
    const membership = await pool.query("SELECT 1 FROM coop_members WHERE room_id = $1 AND account_id = $2 AND is_active = TRUE", [room.rows[0].id, req.accountId]);
    if (membership.rowCount !== 1) return res.status(403).json({ error: "room_membership_required" });
    const [companion, camp, targets] = await Promise.all([
      pool.query("SELECT companion_id, creature_id, display_name, command, bond, health_fraction, revision, captured_at, updated_at FROM coop_companions WHERE room_id = $1 AND account_id = $2", [room.rows[0].id, req.accountId]),
      pool.query("SELECT id, recipe_id, transform, state, revision, created_at, updated_at FROM coop_camps WHERE room_id = $1 AND account_id = $2", [room.rows[0].id, req.accountId]),
      pool.query("SELECT id, creature_id, position_x, position_y, health_fraction, revision FROM coop_creature_targets WHERE room_id = $1 AND active = TRUE ORDER BY creature_id", [room.rows[0].id])
    ]);
    res.json({
      companion: companion.rows[0] || null,
      camp: camp.rows[0] || null,
      targets: targets.rows
    });
  });

  app.post("/v1/coop/rooms/:code/companions/capture", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    const requestId = normalizeRequestId(req.body?.requestId);
    const creatureId = typeof req.body?.creatureId === "string" ? req.body.creatureId.trim().toLowerCase() : "";
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    if (!requestId || !creatureId) return res.status(400).json({ error: "invalid_capture_request" });
    const client = await pool.connect();
    try {
      await client.query("BEGIN");
      const roomResult = await client.query("SELECT id FROM coop_rooms WHERE code = $1 FOR UPDATE", [code]);
      if (roomResult.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(404).json({ error: "room_not_found" });
      }
      const roomId = roomResult.rows[0].id;
      const memberResult = await client.query("SELECT player_x, player_y, wood, fiber, stone, ember_kit, inventory_revision, member_revision, is_active FROM coop_members WHERE room_id = $1 AND account_id = $2 FOR UPDATE", [roomId, req.accountId]);
      if (memberResult.rowCount !== 1 || !memberResult.rows[0].is_active) {
        await client.query("ROLLBACK");
        return res.status(403).json({ error: "active_world_membership_required" });
      }
      const duplicate = await client.query("SELECT result FROM coop_action_receipts WHERE room_id = $1 AND account_id = $2 AND request_id = $3", [roomId, req.accountId, requestId]);
      if (duplicate.rowCount === 1) {
        await client.query("COMMIT");
        return res.json(duplicate.rows[0].result);
      }
      const targetResult = await client.query("SELECT id, creature_id, position_x, position_y, health_fraction, revision FROM coop_creature_targets WHERE room_id = $1 AND creature_id = $2 AND active = TRUE FOR UPDATE", [roomId, creatureId]);
      if (targetResult.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(404).json({ error: "creature_target_not_found" });
      }
      const target = targetResult.rows[0];
      const existing = await client.query("SELECT 1 FROM coop_companions WHERE room_id = $1 AND account_id = $2", [roomId, req.accountId]);
      const profile = companionProfile(target.creature_id);
      const member = memberResult.rows[0];
      const capture = validateCaptureRequest({
        creatureId: target.creature_id,
        distanceMeters: Math.hypot(Number(member.player_x) - Number(target.position_x), Number(member.player_y) - Number(target.position_y)),
        healthFraction: Number(target.health_fraction),
        fiber: Number(member.fiber),
        hasCompanion: existing.rowCount !== 0
      });
      if (!capture.accepted) {
        await client.query("ROLLBACK");
        return res.status(capture.error === "companion_slot_full" ? 409 : 422).json({ error: capture.error });
      }
      const companion = await client.query(
        `INSERT INTO coop_companions (room_id, account_id, creature_id, display_name, command, bond, health_fraction, revision)
         VALUES ($1, $2, $3, $4, 'follow', 0, 0.75, 1)
         RETURNING companion_id, creature_id, display_name, command, bond, health_fraction, revision, captured_at, updated_at`,
        [roomId, req.accountId, target.creature_id, profile.displayName]
      );
      const inventoryRevision = Number(member.inventory_revision || 0) + 1;
      const memberRevision = Number(member.member_revision || 0) + 1;
      await client.query("UPDATE coop_members SET fiber = $3, inventory_revision = $4, member_revision = $5, last_seen_at = now() WHERE room_id = $1 AND account_id = $2", [roomId, req.accountId, capture.remainingFiber, inventoryRevision, memberRevision]);
      await client.query("UPDATE coop_creature_targets SET active = FALSE, captured_by = $2, revision = revision + 1, updated_at = now() WHERE id = $1", [target.id, req.accountId]);
      const result = {
        accepted: true,
        companion: companion.rows[0],
        inventory: { wood: Number(member.wood || 0), fiber: capture.remainingFiber, stone: Number(member.stone || 0), emberKit: Boolean(member.ember_kit) },
        inventoryRevision,
        memberRevision,
        targetRevision: Number(target.revision || 0) + 1
      };
      await client.query("INSERT INTO coop_action_receipts (room_id, account_id, request_id, action_type, action_version, result) VALUES ($1, $2, $3, 'companion', 1, $4)", [roomId, req.accountId, requestId, result]);
      await client.query("COMMIT");
      return res.status(201).json(result);
    } catch (error) {
      await client.query("ROLLBACK");
      throw error;
    } finally {
      client.release();
    }
  });

  app.post("/v1/coop/rooms/:code/companions/command", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    const requestId = normalizeRequestId(req.body?.requestId);
    const command = typeof req.body?.command === "string" ? req.body.command.trim().toLowerCase() : "";
    const expectedRevision = Number.isInteger(req.body?.expectedRevision) ? req.body.expectedRevision : -1;
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    if (!requestId || !["follow", "stay"].includes(command) || expectedRevision < 0) return res.status(400).json({ error: "invalid_companion_command" });
    const client = await pool.connect();
    try {
      await client.query("BEGIN");
      const roomResult = await client.query("SELECT id FROM coop_rooms WHERE code = $1 FOR UPDATE", [code]);
      if (roomResult.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(404).json({ error: "room_not_found" });
      }
      const roomId = roomResult.rows[0].id;
      const membership = await client.query("SELECT 1 FROM coop_members WHERE room_id = $1 AND account_id = $2 AND is_active = TRUE", [roomId, req.accountId]);
      if (membership.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(403).json({ error: "active_world_membership_required" });
      }
      const duplicate = await client.query("SELECT result FROM coop_action_receipts WHERE room_id = $1 AND account_id = $2 AND request_id = $3", [roomId, req.accountId, requestId]);
      if (duplicate.rowCount === 1) {
        await client.query("COMMIT");
        return res.json(duplicate.rows[0].result);
      }
      const current = await client.query("SELECT companion_id, creature_id, display_name, command, bond, health_fraction, revision, captured_at, updated_at FROM coop_companions WHERE room_id = $1 AND account_id = $2 FOR UPDATE", [roomId, req.accountId]);
      if (current.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(404).json({ error: "companion_not_found" });
      }
      const companion = current.rows[0];
      if (Number(companion.revision) !== expectedRevision) {
        await client.query("ROLLBACK");
        return res.status(409).json({ error: "companion_revision_conflict", companion });
      }
      const updated = await client.query(
        `UPDATE coop_companions SET command = $3, revision = revision + 1, updated_at = now()
         WHERE room_id = $1 AND account_id = $2
         RETURNING companion_id, creature_id, display_name, command, bond, health_fraction, revision, captured_at, updated_at`,
        [roomId, req.accountId, command]
      );
      const result = { accepted: true, companion: updated.rows[0] };
      await client.query("INSERT INTO coop_action_receipts (room_id, account_id, request_id, action_type, action_version, result) VALUES ($1, $2, $3, 'companion', 1, $4)", [roomId, req.accountId, requestId, result]);
      await client.query("COMMIT");
      res.json(result);
    } catch (error) {
      await client.query("ROLLBACK");
      throw error;
    } finally {
      client.release();
    }
  });

  app.post("/v1/coop/rooms/:code/camps", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    const requestId = normalizeRequestId(req.body?.requestId);
    const recipeId = typeof req.body?.recipeId === "string" ? req.body.recipeId.trim().toLowerCase() : "";
    const expectedRevision = Number.isInteger(req.body?.expectedRevision) ? req.body.expectedRevision : -1;
    if (!code) return res.status(400).json({ error: "invalid_room_code" });
    if (!requestId || !recipeId || expectedRevision < 0 || !isPlainObject(req.body?.transform)) return res.status(400).json({ error: "invalid_camp_request" });
    const client = await pool.connect();
    try {
      await client.query("BEGIN");
      const roomResult = await client.query("SELECT id FROM coop_rooms WHERE code = $1 FOR UPDATE", [code]);
      if (roomResult.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(404).json({ error: "room_not_found" });
      }
      const roomId = roomResult.rows[0].id;
      const memberResult = await client.query("SELECT player_x, player_y, wood, fiber, stone, ember_kit, inventory_revision, member_revision, is_active FROM coop_members WHERE room_id = $1 AND account_id = $2 FOR UPDATE", [roomId, req.accountId]);
      if (memberResult.rowCount !== 1 || !memberResult.rows[0].is_active) {
        await client.query("ROLLBACK");
        return res.status(403).json({ error: "active_world_membership_required" });
      }
      const duplicate = await client.query("SELECT result FROM coop_action_receipts WHERE room_id = $1 AND account_id = $2 AND request_id = $3", [roomId, req.accountId, requestId]);
      if (duplicate.rowCount === 1) {
        await client.query("COMMIT");
        return res.status(201).json(duplicate.rows[0].result);
      }
      const member = memberResult.rows[0];
      const existing = await client.query("SELECT id, recipe_id, transform, state, revision, created_at, updated_at FROM coop_camps WHERE room_id = $1 AND account_id = $2 FOR UPDATE", [roomId, req.accountId]);
      const currentCamp = existing.rows[0] || null;
      const currentRevision = currentCamp ? Number(currentCamp.revision || 0) : 0;
      if (expectedRevision !== currentRevision) {
        await client.query("ROLLBACK");
        return res.status(409).json({ error: "camp_revision_conflict", camp: currentCamp });
      }
      const placement = validateCampPlacement({
        recipeId,
        transform: req.body.transform,
        playerPosition: { x: Number(member.player_x), y: Number(member.player_y) },
        slope: 0,
        existingCampCount: existing.rowCount
      });
      if (!placement.accepted) {
        await client.query("ROLLBACK");
        return res.status(422).json({ error: placement.error });
      }
      if (Number(member.wood) < placement.recipe.woodCost || Number(member.fiber) < placement.recipe.fiberCost) {
        await client.query("ROLLBACK");
        return res.status(422).json({ error: "insufficient_camp_materials" });
      }
      const camp = await client.query(
        `INSERT INTO coop_camps (room_id, account_id, recipe_id, transform, state, revision)
         VALUES ($1, $2, $3, $4, '{}'::jsonb, 1)
         RETURNING id, recipe_id, transform, state, revision, created_at, updated_at`,
        [roomId, req.accountId, recipeId, placement.transform]
      );
      const wood = Number(member.wood) - placement.recipe.woodCost;
      const fiber = Number(member.fiber) - placement.recipe.fiberCost;
      const inventoryRevision = Number(member.inventory_revision || 0) + 1;
      const memberRevision = Number(member.member_revision || 0) + 1;
      await client.query("UPDATE coop_members SET wood = $3, fiber = $4, inventory_revision = $5, member_revision = $6, last_seen_at = now() WHERE room_id = $1 AND account_id = $2", [roomId, req.accountId, wood, fiber, inventoryRevision, memberRevision]);
      const result = { accepted: true, camp: camp.rows[0], inventory: { wood, fiber, stone: Number(member.stone || 0), emberKit: Boolean(member.ember_kit) }, inventoryRevision, memberRevision };
      await client.query("INSERT INTO coop_action_receipts (room_id, account_id, request_id, action_type, action_version, result) VALUES ($1, $2, $3, 'camp', 1, $4)", [roomId, req.accountId, requestId, result]);
      await client.query("COMMIT");
      res.status(201).json(result);
    } catch (error) {
      await client.query("ROLLBACK");
      throw error;
    } finally {
      client.release();
    }
  });

  app.delete("/v1/coop/rooms/:code/camps/:campId", requireSession, async (req, res) => {
    const code = normalizeRoomCode(req.params.code);
    const requestId = normalizeRequestId(req.body?.requestId || req.get("x-request-id"));
    const expectedRevision = Number.isInteger(req.body?.expectedRevision) ? req.body.expectedRevision : -1;
    if (!code || !requestId || expectedRevision < 0) return res.status(400).json({ error: "invalid_camp_delete_request" });
    const client = await pool.connect();
    try {
      await client.query("BEGIN");
      const roomResult = await client.query("SELECT id FROM coop_rooms WHERE code = $1 FOR UPDATE", [code]);
      if (roomResult.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(404).json({ error: "room_not_found" });
      }
      const roomId = roomResult.rows[0].id;
      const membership = await client.query("SELECT 1 FROM coop_members WHERE room_id = $1 AND account_id = $2 AND is_active = TRUE", [roomId, req.accountId]);
      if (membership.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(403).json({ error: "active_world_membership_required" });
      }
      const duplicate = await client.query("SELECT result FROM coop_action_receipts WHERE room_id = $1 AND account_id = $2 AND request_id = $3", [roomId, req.accountId, requestId]);
      if (duplicate.rowCount === 1) {
        await client.query("COMMIT");
        return res.json(duplicate.rows[0].result);
      }
      const current = await client.query("SELECT id, recipe_id, transform, state, revision, created_at, updated_at FROM coop_camps WHERE id = $1 AND room_id = $2 AND account_id = $3 FOR UPDATE", [req.params.campId, roomId, req.accountId]);
      if (current.rowCount !== 1) {
        await client.query("ROLLBACK");
        return res.status(404).json({ error: "camp_not_found" });
      }
      if (Number(current.rows[0].revision) !== expectedRevision) {
        await client.query("ROLLBACK");
        return res.status(409).json({ error: "camp_revision_conflict", camp: current.rows[0] });
      }
      await client.query("DELETE FROM coop_camps WHERE id = $1", [req.params.campId]);
      const result = { accepted: true, campId: req.params.campId, removedRevision: expectedRevision + 1 };
      await client.query("INSERT INTO coop_action_receipts (room_id, account_id, request_id, action_type, action_version, result) VALUES ($1, $2, $3, 'camp', 1, $4)", [roomId, req.accountId, requestId, result]);
      await client.query("COMMIT");
      res.json(result);
    } catch (error) {
      await client.query("ROLLBACK");
      throw error;
    } finally {
      client.release();
    }
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
      const memberResult = await client.query("SELECT player_x, player_y, wood, fiber, stone, inventory_revision, ember_kit, item_state, progression_state, member_revision FROM coop_members WHERE room_id = $1 AND account_id = $2 FOR UPDATE", [roomId, req.accountId]);
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
      const itemState = isPlainObject(member.item_state) ? { ...member.item_state } : {};
      const progressionState = isPlainObject(member.progression_state) ? member.progression_state : {};
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
      const memberRevision = Number(member.member_revision || 0) + 1;
      itemState.wood = wood;
      itemState.fiber = fiber;
      itemState.stone = stone;
      itemState.emberKit = emberKit;
      const result = { accepted: true, operation, inventory: { wood, fiber, stone, emberKit }, inventoryRevision, memberRevision };
      await client.query("UPDATE coop_members SET wood = $3, fiber = $4, stone = $5, ember_kit = $6, inventory_revision = $7, item_state = $8, member_revision = $9, last_seen_at = now() WHERE room_id = $1 AND account_id = $2", [roomId, req.accountId, wood, fiber, stone, emberKit, inventoryRevision, itemState, memberRevision]);
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

function isPlainObject(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

async function createCoOpRoom(pool, accountId, region, worldName = "Aethelgrad Shared World") {
  for (let attempt = 0; attempt < 5; attempt += 1) {
    const code = randomBytes(3).toString("hex").toUpperCase();
    try {
      const created = await pool.query(
        "INSERT INTO coop_rooms (code, region, world_name, created_by, max_players) VALUES ($1, $2, $3, $4, $5) RETURNING id",
        [code, region, worldName, accountId, MAX_COOP_PLAYERS]
      );
      if (created.rowCount === 1) {
        await pool.query("INSERT INTO coop_members (room_id, account_id, is_active) VALUES ($1, $2, TRUE)", [created.rows[0].id, accountId]);
        await pool.query("INSERT INTO coop_world_saves (room_id, updated_by) VALUES ($1, $2) ON CONFLICT (room_id) DO NOTHING", [created.rows[0].id, accountId]);
        for (const [creatureId, target] of Object.entries(COMPANION_TARGET_SEEDS)) {
          await pool.query(
            "INSERT INTO coop_creature_targets (room_id, creature_id, position_x, position_y, health_fraction) VALUES ($1, $2, $3, $4, $5) ON CONFLICT (room_id, creature_id) DO NOTHING",
            [created.rows[0].id, creatureId, target.x, target.y, target.healthFraction]
          );
        }
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
    `SELECT id, code, region, world_name, created_by, max_players, world_time, tower_revision, boss_health, combat_revision
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
     FROM coop_members WHERE room_id = $1 AND is_active = TRUE AND last_seen_at > now() - interval '20 seconds'
     ORDER BY last_seen_at DESC
     LIMIT 4`,
    [room.id]
  );
  const refreshed = await pool.query("SELECT world_time, tower_revision, boss_health, combat_revision FROM coop_rooms WHERE id = $1", [room.id]);
  return {
    room: {
      code: room.code,
      region: room.region,
      worldName: room.world_name || "Aethelgrad Shared World",
      ownerAccountId: room.created_by,
      maxPlayers: MAX_COOP_PLAYERS,
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

async function upsertGuestAccount(db, guestKey) {
  const guestKeyHash = hashSecret(guestKey);
  const result = await db.query(
    `INSERT INTO accounts (provider, provider_player_id, display_name)
     VALUES ('guest', $1, 'Guest Wayfarer')
     ON CONFLICT (provider, provider_player_id)
     DO UPDATE SET updated_at = now()
     RETURNING id`,
    [guestKeyHash]
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
    ssl: config.databaseSsl ? { rejectUnauthorized: config.databaseSslVerify } : undefined
  });
  const port = Number(process.env.PORT || 8080);
  createOnlineService({ pool, config }).listen(port, () => {
    console.log(`Aethelgrad online services listening on :${port}`);
  });
}
