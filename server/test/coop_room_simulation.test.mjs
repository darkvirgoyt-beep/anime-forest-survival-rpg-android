import assert from "node:assert/strict";
import http from "node:http";
import { createOnlineService } from "../src/server.mjs";
import { issueAccessToken, loadRuntimeConfig, hashSecret } from "../src/security.mjs";

const config = loadRuntimeConfig({
  DATABASE_URL: "postgres://simulation",
  GOOGLE_ID_TOKEN_AUDIENCE: "simulation.apps.googleusercontent.com",
  GAME_SESSION_JWT_SECRET: "simulation-session-secret-which-is-long-enough",
  ALLOWED_ORIGIN: "https://simulation.aethelgard.example",
  GAME_ACCESS_TOKEN_TTL_SECONDS: "900",
  GAME_REFRESH_TOKEN_TTL_SECONDS: "2592000"
});

function createMemoryPool() {
  const rooms = new Map();
  const members = new Map();
  const receipts = new Map();
  let nextRoomId = 1;

  const key = (roomId, accountId) => `${roomId}:${accountId}`;
  const roomByCode = code => [...rooms.values()].find(room => room.code === code);
  const memberFor = (roomId, accountId) => members.get(key(roomId, accountId));
  const participantRows = roomId => [...members.values()]
    .filter(member => member.roomId === roomId && Date.now() - member.lastSeenAt < 20_000)
    .map(member => ({
      account_id: member.accountId,
      player_x: member.playerX,
      player_y: member.playerY,
      at_tower: member.atTower,
      tower_revision: member.towerRevision
    }));

  async function query(sql, params = []) {
    const normalized = sql.replace(/\s+/g, " ").trim();
    if (normalized.includes("FROM sessions") && normalized.includes("token_hash")) {
      const accountId = Number(params[0]) === 2 ? "account-b" : "account-a";
      return { rowCount: 1, rows: [{ id: Number(params[0]), account_id: accountId }] };
    }
    if (normalized.startsWith("UPDATE sessions SET last_seen_at")) return { rowCount: 1, rows: [] };

    if (normalized.startsWith("INSERT INTO coop_rooms")) {
      const room = {
        id: `room-${nextRoomId++}`,
        code: params[0],
        region: params[1],
        maxPlayers: 4,
        worldTime: 0,
        towerRevision: 0,
        bossHealth: 100,
        combatRevision: 0
      };
      rooms.set(room.id, room);
      return { rowCount: 1, rows: [{ id: room.id }] };
    }
    if (normalized.startsWith("INSERT INTO coop_members (room_id, account_id)")) {
      const memberKey = key(params[0], params[1]);
      const current = members.get(memberKey) || {
        roomId: params[0], accountId: params[1], playerX: -0.55, playerY: -0.08,
        atTower: false, towerRevision: 0, lastSeenAt: Date.now(), wood: 12, fiber: 8,
        stone: 4, inventoryRevision: 0, emberKit: false, lastActionAt: null
      };
      current.lastSeenAt = Date.now();
      members.set(memberKey, current);
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("SELECT id, code, region, max_players FROM coop_rooms")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id, code: room.code, region: room.region, max_players: room.maxPlayers }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT COUNT(*)::int AS count FROM coop_members")) {
      const count = [...members.values()].filter(member => member.roomId === params[0] && member.accountId !== params[1] && Date.now() - member.lastSeenAt < 20_000).length;
      return { rowCount: 1, rows: [{ count }] };
    }
    if (normalized.startsWith("SELECT id, code, region, max_players, world_time, tower_revision, boss_health, combat_revision FROM coop_rooms")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id, code: room.code, region: room.region, max_players: room.maxPlayers, world_time: room.worldTime, tower_revision: room.towerRevision, boss_health: room.bossHealth, combat_revision: room.combatRevision }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT 1 FROM coop_members")) {
      return memberFor(params[0], params[1]) ? { rowCount: 1, rows: [{}] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_rooms SET world_time")) {
      const room = rooms.get(params[0]);
      if (room) room.worldTime += 1;
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("SELECT account_id, player_x, player_y, at_tower, tower_revision")) {
      return { rowCount: participantRows(params[0]).length, rows: participantRows(params[0]) };
    }
    if (normalized.startsWith("SELECT world_time, tower_revision, boss_health, combat_revision FROM coop_rooms")) {
      const room = rooms.get(params[0]);
      return room ? { rowCount: 1, rows: [{ world_time: room.worldTime, tower_revision: room.towerRevision, boss_health: room.bossHealth, combat_revision: room.combatRevision }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT id FROM coop_rooms")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("INSERT INTO coop_members (room_id, account_id, player_x")) {
      const member = memberFor(params[0], params[1]);
      if (member) {
        member.playerX = params[2];
        member.playerY = params[3];
        member.atTower = params[4];
        member.towerRevision = params[5];
        member.lastSeenAt = Date.now();
      }
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_rooms SET tower_revision")) {
      const room = rooms.get(params[0]);
      if (room) room.towerRevision = Math.max(room.towerRevision, params[1]);
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("DELETE FROM coop_members")) {
      const room = roomByCode(params[0]);
      if (room) members.delete(key(room.id, params[1]));
      return { rowCount: 1, rows: [] };
    }

    if (normalized.startsWith("SELECT id, boss_health, combat_revision FROM coop_rooms")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id, boss_health: room.bossHealth, combat_revision: room.combatRevision }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT player_x, player_y, last_action_at FROM coop_members")) {
      const member = memberFor(params[0], params[1]);
      return member ? { rowCount: 1, rows: [{ player_x: member.playerX, player_y: member.playerY, last_action_at: member.lastActionAt }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT result FROM coop_action_receipts")) {
      const receipt = receipts.get(`${params[0]}:${params[1]}:${params[2]}`);
      return receipt ? { rowCount: 1, rows: [{ result: receipt }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_rooms SET boss_health")) {
      const room = rooms.get(params[0]);
      if (room) { room.bossHealth = params[1]; room.combatRevision = params[2]; }
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_members SET last_action_at")) {
      const member = memberFor(params[0], params[1]);
      if (member) { member.lastActionAt = new Date().toISOString(); member.lastSeenAt = Date.now(); }
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("INSERT INTO coop_action_receipts")) {
      receipts.set(`${params[0]}:${params[1]}:${params[2]}`, params[3]);
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("SELECT id FROM coop_rooms WHERE code = $1 FOR UPDATE")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT player_x, player_y, wood, fiber, stone, inventory_revision, ember_kit")) {
      const member = memberFor(params[0], params[1]);
      return member ? { rowCount: 1, rows: [{ player_x: member.playerX, player_y: member.playerY, wood: member.wood, fiber: member.fiber, stone: member.stone, inventory_revision: member.inventoryRevision, ember_kit: member.emberKit }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_members SET wood")) {
      const member = memberFor(params[0], params[1]);
      if (member) { member.wood = params[2]; member.fiber = params[3]; member.stone = params[4]; member.emberKit = params[5]; member.inventoryRevision = params[6]; member.lastSeenAt = Date.now(); }
      return { rowCount: 1, rows: [] };
    }
    if (normalized === "BEGIN" || normalized === "COMMIT" || normalized === "ROLLBACK") return { rowCount: 0, rows: [] };
    throw new Error(`Unhandled simulation query: ${normalized}`);
  }

  return { query, connect: async () => ({ query, release() {} }) };
}

async function request(baseUrl, token, path, method, body) {
  const response = await fetch(`${baseUrl}${path}`, {
    method,
    headers: { Authorization: `Bearer ${token}`, "Content-Type": "application/json" },
    body: body === undefined ? undefined : JSON.stringify(body)
  });
  const payload = response.status === 204 ? null : await response.json();
  return { status: response.status, payload };
}

const pool = createMemoryPool();
const app = createOnlineService({ pool, config });
const server = http.createServer(app);
await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
const baseUrl = `http://127.0.0.1:${server.address().port}`;
const tokenA = issueAccessToken({ accountId: "account-a", sessionId: 1, secret: config.sessionSecret, now: Date.now(), ttlSeconds: 900 }).token;
const tokenB = issueAccessToken({ accountId: "account-b", sessionId: 2, secret: config.sessionSecret, now: Date.now(), ttlSeconds: 900 }).token;

try {
  const created = await request(baseUrl, tokenA, "/v1/coop/rooms", "POST", { region: "asia" });
  assert.equal(created.status, 201);
  const code = created.payload.room.code;
  assert.match(code, /^[A-Z0-9]{6}$/);
  assert.equal(created.payload.participants.length, 1);

  const joined = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/join`, "POST", {});
  assert.equal(joined.status, 200);
  assert.equal(joined.payload.participants.length, 2);

  const tower = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/heartbeat`, "POST", { playerX: -0.06, playerY: 0.28, atTower: true, towerRevision: 1 });
  assert.equal(tower.status, 200);
  assert.equal(tower.payload.room.towerRevision, 1);
  assert.ok(tower.payload.participants.some(participant => participant.accountId === "account-a" && participant.atTower));

  const friendView = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}`, "GET");
  assert.equal(friendView.status, 200);
  const remoteTower = friendView.payload.participants.find(participant => participant.accountId === "account-a");
  assert.equal(remoteTower.atTower, true);
  assert.equal(remoteTower.towerRevision, 1);

  await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/heartbeat`, "POST", { playerX: -0.18, playerY: -0.08, atTower: false, towerRevision: 0 });
  const attack = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/combat`, "POST", { requestId: "combat-001", action: "attack", targetId: "forest_warden" });
  assert.equal(attack.status, 200);
  assert.equal(attack.payload.bossHealth, 88);
  const duplicateAttack = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/combat`, "POST", { requestId: "combat-001", action: "attack", targetId: "forest_warden" });
  assert.equal(duplicateAttack.status, 200);
  assert.deepEqual(duplicateAttack.payload, attack.payload);

  await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/heartbeat`, "POST", { playerX: 0.80, playerY: 0.40, atTower: false, towerRevision: 0 });
  const outOfRange = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/combat`, "POST", { requestId: "combat-002", action: "attack", targetId: "forest_warden" });
  assert.equal(outOfRange.status, 409);

  await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/heartbeat`, "POST", { playerX: -0.56, playerY: -0.28, atTower: false, towerRevision: 0 });
  const gather = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/inventory`, "POST", { requestId: "gather-001", operation: "gather", resourceId: "forest_cache" });
  assert.equal(gather.status, 200);
  assert.deepEqual(gather.payload.inventory, { wood: 13, fiber: 10, stone: 4, emberKit: false });
  const duplicateGather = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/inventory`, "POST", { requestId: "gather-001", operation: "gather", resourceId: "forest_cache" });
  assert.deepEqual(duplicateGather.payload, gather.payload);
  const craft = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/inventory`, "POST", { requestId: "craft-001", operation: "craft" });
  assert.equal(craft.status, 200);
  assert.deepEqual(craft.payload.inventory, { wood: 10, fiber: 8, stone: 4, emberKit: true });

  console.log(JSON.stringify({
    ok: true,
    roomCode: code,
    checks: ["room_created", "friend_joined", "tower_revision_seen", "co_op_clock_read", "combat_validated", "combat_retry_idempotent", "combat_range_rejected", "inventory_reward_validated", "inventory_retry_idempotent", "craft_validated"]
  }, null, 2));
} finally {
  await new Promise(resolve => server.close(resolve));
}
