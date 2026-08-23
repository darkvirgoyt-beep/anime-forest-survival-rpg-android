import assert from "node:assert/strict";
import http from "node:http";
import { performance } from "node:perf_hooks";
import { createOnlineService } from "../src/server.mjs";
import { issueAccessToken, loadRuntimeConfig } from "../src/security.mjs";

const ROOM_COUNT = 50;
const PLAYERS_PER_ROOM = 4;
const SYNTHETIC_DB_LATENCY_MS = Number(process.env.SYNTHETIC_DB_LATENCY_MS ?? 1);
const config = loadRuntimeConfig({
  DATABASE_URL: "postgres://stress-simulation",
  GOOGLE_ID_TOKEN_AUDIENCE: "stress.apps.googleusercontent.com",
  GAME_SESSION_JWT_SECRET: "stress-session-secret-which-is-long-enough",
  ALLOWED_ORIGIN: "https://stress.aethelgard.example",
  GAME_ACCESS_TOKEN_TTL_SECONDS: "900",
  GAME_REFRESH_TOKEN_TTL_SECONDS: "2592000"
});

const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

function createStressPool() {
  const rooms = new Map();
  const members = new Map();
  const receipts = new Map();
  const sessions = new Map();
  let nextRoomId = 1;
  let queryCount = 0;
  let transactionCount = 0;
  const key = (roomId, accountId) => `${roomId}:${accountId}`;
  const roomByCode = code => [...rooms.values()].find(room => room.code === code);
  const memberFor = (roomId, accountId) => members.get(key(roomId, accountId));
  const participantRows = roomId => [...members.values()]
    .filter(member => member.roomId === roomId && Date.now() - member.lastSeenAt < 20_000)
    .map(member => ({ account_id: member.accountId, player_x: member.playerX, player_y: member.playerY, at_tower: member.atTower, tower_revision: member.towerRevision }));

  async function query(sql, params = []) {
    const normalized = sql.replace(/\s+/g, " ").trim();
    queryCount += 1;
    if (SYNTHETIC_DB_LATENCY_MS > 0 && (normalized === "BEGIN" || normalized === "COMMIT" || normalized.startsWith("UPDATE") || normalized.startsWith("INSERT"))) await sleep(SYNTHETIC_DB_LATENCY_MS);
    if (normalized === "BEGIN") transactionCount += 1;
    if (normalized.includes("FROM sessions") && normalized.includes("token_hash")) {
      const accountId = sessions.get(String(params[0]));
      return accountId ? { rowCount: 1, rows: [{ id: Number(params[0]), account_id: accountId }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("UPDATE sessions SET last_seen_at")) return { rowCount: 1, rows: [] };
    if (normalized.startsWith("INSERT INTO coop_rooms")) {
      const room = { id: `room-${nextRoomId++}`, code: params[0], region: params[1], maxPlayers: 4, worldTime: 0, towerRevision: 0, bossHealth: 100, combatRevision: 0 };
      rooms.set(room.id, room);
      return { rowCount: 1, rows: [{ id: room.id }] };
    }
    if (normalized.startsWith("INSERT INTO coop_members (room_id, account_id)")) {
      const memberKey = key(params[0], params[1]);
      const current = members.get(memberKey) ?? { roomId: params[0], accountId: params[1], playerX: -0.55, playerY: -0.08, atTower: false, towerRevision: 0, lastSeenAt: Date.now(), wood: 12, fiber: 8, stone: 4, inventoryRevision: 0, emberKit: false, lastActionAt: null };
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
    if (normalized.startsWith("SELECT 1 FROM coop_members")) return memberFor(params[0], params[1]) ? { rowCount: 1, rows: [{}] } : { rowCount: 0, rows: [] };
    if (normalized.startsWith("UPDATE coop_rooms SET world_time")) {
      const room = rooms.get(params[0]);
      if (room) room.worldTime += 1;
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("SELECT account_id, player_x, player_y, at_tower, tower_revision")) return { rowCount: participantRows(params[0]).length, rows: participantRows(params[0]) };
    if (normalized.startsWith("SELECT world_time, tower_revision, boss_health, combat_revision FROM coop_rooms")) {
      const room = rooms.get(params[0]);
      return room ? { rowCount: 1, rows: [{ world_time: room.worldTime, tower_revision: room.towerRevision, boss_health: room.bossHealth, combat_revision: room.combatRevision }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT id FROM coop_rooms WHERE code = $1 FOR UPDATE")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT id FROM coop_rooms")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("INSERT INTO coop_members (room_id, account_id, player_x")) {
      const member = memberFor(params[0], params[1]);
      if (member) { member.playerX = params[2]; member.playerY = params[3]; member.atTower = params[4]; member.towerRevision = params[5]; member.lastSeenAt = Date.now(); }
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
    throw new Error(`Unhandled stress query: ${normalized}`);
  }

  return { query, connect: async () => ({ query, release() {} }), sessions, roomCodes: () => [...rooms.values()].map(room => room.code), metrics: () => ({ queryCount, transactionCount, roomCount: rooms.size, memberCount: members.size }) };
}

async function request(baseUrl, token, method, path, body) {
  const started = performance.now();
  const response = await fetch(`${baseUrl}${path}`, { method, headers: { Authorization: `Bearer ${token}`, "Content-Type": "application/json" }, body: body === undefined ? undefined : JSON.stringify(body) });
  const payload = response.status === 204 ? null : await response.json();
  return { status: response.status, payload, latencyMs: performance.now() - started };
}

function summarize(results) {
  const latencies = results.map(result => result.latencyMs).sort((a, b) => a - b);
  const percentile = p => latencies[Math.min(latencies.length - 1, Math.floor(latencies.length * p))];
  const statusCounts = results.reduce((counts, result) => { counts[result.status] = (counts[result.status] ?? 0) + 1; return counts; }, {});
  return { requests: results.length, statusCounts, minMs: Number(latencies[0].toFixed(3)), p50Ms: Number(percentile(0.50).toFixed(3)), p95Ms: Number(percentile(0.95).toFixed(3)), maxMs: Number(latencies[latencies.length - 1].toFixed(3)) };
}

const pool = createStressPool();
const app = createOnlineService({ pool, config });
const server = http.createServer(app);
await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
const baseUrl = `http://127.0.0.1:${server.address().port}`;
const tokens = [];
for (let i = 0; i < ROOM_COUNT * PLAYERS_PER_ROOM; i += 1) {
  const sessionId = i + 1;
  pool.sessions.set(String(sessionId), `stress-account-${i}`);
  tokens.push(issueAccessToken({ accountId: `stress-account-${i}`, sessionId, secret: config.sessionSecret, now: Date.now(), ttlSeconds: 900 }).token);
}

const allResults = [];
const phases = [];
const phase = async (name, jobs, expectedStatus) => {
  const started = performance.now();
  const results = await Promise.all(jobs);
  const summary = { name, elapsedMs: Number((performance.now() - started).toFixed(3)), ...summarize(results) };
  assert.equal(summary.statusCounts[expectedStatus], results.length, `${name} did not return ${expectedStatus} for every request`);
  allResults.push(...results);
  phases.push(summary);
  return { ...summary, results };
};

try {
  const created = await phase("create_rooms", Array.from({ length: ROOM_COUNT }, (_, roomIndex) => request(baseUrl, tokens[roomIndex * PLAYERS_PER_ROOM], "POST", "/v1/coop/rooms", { region: roomIndex % 2 === 0 ? "asia" : "eu" })), 201);
  const roomCodes = created.results.map(result => result.payload.room.code);
  assert.equal(new Set(roomCodes).size, ROOM_COUNT);

  await phase("join_members", Array.from({ length: ROOM_COUNT * (PLAYERS_PER_ROOM - 1) }, (_, index) => {
    const roomIndex = Math.floor(index / (PLAYERS_PER_ROOM - 1));
    const playerSlot = (index % (PLAYERS_PER_ROOM - 1)) + 1;
    return request(baseUrl, tokens[roomIndex * PLAYERS_PER_ROOM + playerSlot], "POST", `/v1/coop/rooms/${roomCodes[roomIndex]}/join`, {});
  }), 200);

  await phase("heartbeats", Array.from({ length: ROOM_COUNT * PLAYERS_PER_ROOM }, (_, index) => {
    const roomIndex = Math.floor(index / PLAYERS_PER_ROOM);
    const playerSlot = index % PLAYERS_PER_ROOM;
    const position = playerSlot === 0 ? [-0.18, -0.08, false, 0] : playerSlot === 1 ? [-0.06, 0.28, true, 1] : playerSlot === 2 ? [-0.56, -0.28, false, 0] : [-0.45, -0.08, false, 0];
    return request(baseUrl, tokens[index], "POST", `/v1/coop/rooms/${roomCodes[roomIndex]}/heartbeat`, { playerX: position[0], playerY: position[1], atTower: position[2], towerRevision: position[3] });
  }), 200);

  await phase("combat", roomCodes.map((code, roomIndex) => request(baseUrl, tokens[roomIndex * PLAYERS_PER_ROOM], "POST", `/v1/coop/rooms/${code}/combat`, { requestId: `stress-combat-${roomIndex}`, action: "attack", targetId: "forest_warden" })), 200);
  await phase("gather", roomCodes.map((code, roomIndex) => request(baseUrl, tokens[roomIndex * PLAYERS_PER_ROOM + 2], "POST", `/v1/coop/rooms/${code}/inventory`, { requestId: `stress-gather-${roomIndex}`, operation: "gather", resourceId: "forest_cache" })), 200);
  await phase("craft", roomCodes.map((code, roomIndex) => request(baseUrl, tokens[roomIndex * PLAYERS_PER_ROOM + 2], "POST", `/v1/coop/rooms/${code}/inventory`, { requestId: `stress-craft-${roomIndex}`, operation: "craft" })), 200);

  const elapsedMs = phases.reduce((sum, current) => sum + current.elapsedMs, 0);
  const aggregate = summarize(allResults);
  const report = {
    ok: true,
    scenario: { rooms: ROOM_COUNT, playersPerRoom: PLAYERS_PER_ROOM, totalPlayers: ROOM_COUNT * PLAYERS_PER_ROOM, syntheticDbLatencyMs: SYNTHETIC_DB_LATENCY_MS, databaseMode: "in-memory SQL-pattern simulation", note: "This is not a PostgreSQL capacity benchmark." },
    phases,
    aggregate: { ...aggregate, elapsedMs: Number(elapsedMs.toFixed(3)), throughputRps: Number((allResults.length / (elapsedMs / 1000)).toFixed(2)) },
    databaseSimulation: pool.metrics(),
    validation: { expectedStatuses: { createRooms: 201, allOtherRequests: 200 }, unexpectedFailures: 0 }
  };
  console.log(JSON.stringify(report, null, 2));
} finally {
  await new Promise(resolve => server.close(resolve));
}
