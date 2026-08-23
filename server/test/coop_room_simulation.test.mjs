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
  const worldSaves = new Map();
  const buildings = new Map();
  const targets = new Map();
  const companions = new Map();
  const camps = new Map();
  let nextRoomId = 1;
  let nextBuildingId = 1;
  let nextCompanionId = 1;
  let nextCampId = 1;

  const key = (roomId, accountId) => `${roomId}:${accountId}`;
  const targetKey = (roomId, creatureId) => `${roomId}:${creatureId}`;
  const roomByCode = code => [...rooms.values()].find(room => room.code === code);
  const memberFor = (roomId, accountId) => members.get(key(roomId, accountId));
  const participantRows = roomId => [...members.values()]
    .filter(member => member.roomId === roomId && member.isActive && Date.now() - member.lastSeenAt < 20_000)
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
      const accountId = ["account-a", "account-b", "account-c", "account-d", "account-e"][Math.max(0, Number(params[0]) - 1)] || "account-a";
      return { rowCount: 1, rows: [{ id: Number(params[0]), account_id: accountId }] };
    }
    if (normalized.startsWith("UPDATE sessions SET last_seen_at")) return { rowCount: 1, rows: [] };

    if (normalized.startsWith("INSERT INTO coop_rooms")) {
      const hasWorldName = normalized.includes("world_name");
      const room = {
        id: `room-${nextRoomId++}`,
        code: params[0],
        region: params[1],
        worldName: hasWorldName ? params[2] : "Aethelgard Shared World",
        createdBy: hasWorldName ? params[3] : params[2],
        maxPlayers: 4,
        worldTime: 0,
        towerRevision: 0,
        bossHealth: 100,
        combatRevision: 0
      };
      rooms.set(room.id, room);
      return { rowCount: 1, rows: [{ id: room.id }] };
    }
    if (normalized.startsWith("INSERT INTO coop_members (room_id, account_id, is_active)")) {
      const memberKey = key(params[0], params[1]);
      const current = members.get(memberKey) || {
        roomId: params[0], accountId: params[1], playerX: -0.55, playerY: -0.08,
        atTower: false, towerRevision: 0, lastSeenAt: Date.now(), isActive: true, wood: 12, fiber: 8,
        stone: 4, inventoryRevision: 0, emberKit: false, itemState: { wood: 12, fiber: 8, stone: 4, emberKit: false, items: [] },
        progressionState: { level: 1, xp: 0, unlocked: [] }, memberRevision: 0, lastActionAt: null
      };
      current.isActive = true;
      current.lastSeenAt = Date.now();
      members.set(memberKey, current);
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("SELECT id, code, region, max_players FROM coop_rooms")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id, code: room.code, region: room.region, max_players: room.maxPlayers }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT COUNT(*)::int AS count FROM coop_members")) {
      const count = [...members.values()].filter(member => member.roomId === params[0] && member.accountId !== params[1] && member.isActive && Date.now() - member.lastSeenAt < 20_000).length;
      return { rowCount: 1, rows: [{ count }] };
    }
    if (normalized.startsWith("SELECT id, code, region, world_name, created_by, max_players, world_time, tower_revision, boss_health, combat_revision FROM coop_rooms")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id, code: room.code, region: room.region, world_name: room.worldName, created_by: room.createdBy, max_players: room.maxPlayers, world_time: room.worldTime, tower_revision: room.towerRevision, boss_health: room.bossHealth, combat_revision: room.combatRevision }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT 1 FROM coop_members")) {
      return memberFor(params[0], params[1]) ? { rowCount: 1, rows: [{}] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_members SET last_seen_at")) {
      const member = memberFor(params[0], params[1]);
      if (member) member.lastSeenAt = Date.now();
      return { rowCount: 1, rows: [] };
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
    if (normalized.startsWith("INSERT INTO coop_world_saves")) {
      worldSaves.set(params[0], { worldState: { schemaVersion: 1, buildings: [], claimedResources: [], quests: {} }, saveRevision: 0, updatedBy: params[1], updatedAt: null });
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("INSERT INTO coop_creature_targets")) {
      const target = { id: `target-${params[0]}-${params[1]}`, roomId: params[0], creature_id: params[1], position_x: Number(params[2]), position_y: Number(params[3]), health_fraction: Number(params[4]), active: true, revision: 0, captured_by: null };
      targets.set(targetKey(target.roomId, target.creature_id), target);
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("SELECT companion_id, creature_id, display_name, command, bond, health_fraction, revision, captured_at, updated_at FROM coop_companions")) {
      const companion = companions.get(key(params[0], params[1]));
      return companion ? { rowCount: 1, rows: [companion] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT id, recipe_id, transform, state, revision, created_at, updated_at FROM coop_camps WHERE room_id")) {
      const camp = camps.get(key(params[0], params[1]));
      return camp ? { rowCount: 1, rows: [camp] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT id, creature_id, position_x, position_y, health_fraction, revision FROM coop_creature_targets")) {
      if (normalized.includes("AND creature_id = $2")) {
        const target = targets.get(targetKey(params[0], params[1]));
        return target && target.active ? { rowCount: 1, rows: [{ ...target }] } : { rowCount: 0, rows: [] };
      }
      const roomId = params[0];
      return { rowCount: [...targets.values()].filter(target => target.roomId === roomId && target.active).length, rows: [...targets.values()].filter(target => target.roomId === roomId && target.active).sort((a, b) => a.creature_id.localeCompare(b.creature_id)).map(({ id, creature_id, position_x, position_y, health_fraction, revision }) => ({ id, creature_id, position_x, position_y, health_fraction, revision })) };
    }
    if (normalized.startsWith("SELECT m.item_state, m.progression_state, m.member_revision, r.created_by, r.world_name")) {
      const member = memberFor(rooms.get(roomByCode(params[0])?.id)?.id, params[1]);
      const room = roomByCode(params[0]);
      return member && room ? { rowCount: 1, rows: [{ item_state: member.itemState, progression_state: member.progressionState, member_revision: member.memberRevision, created_by: room.createdBy, world_name: room.worldName }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_members SET item_state")) {
      const room = roomByCode(params[0]);
      const member = room ? memberFor(room.id, params[1]) : null;
      if (!member || member.memberRevision !== params[4]) return { rowCount: 0, rows: [] };
      member.itemState = params[2];
      member.progressionState = params[3];
      member.memberRevision += 1;
      member.lastSeenAt = Date.now();
      return { rowCount: 1, rows: [{ member_revision: member.memberRevision, item_state: member.itemState, progression_state: member.progressionState }] };
    }
    if (normalized.startsWith("SELECT world_state, save_revision, updated_at FROM coop_world_saves")) {
      const save = worldSaves.get(params[0]);
      return save ? { rowCount: 1, rows: [{ world_state: save.worldState, save_revision: save.saveRevision, updated_at: save.updatedAt }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT id, building_type, placed_by, transform, state, created_at, updated_at FROM coop_buildings")) {
      return { rowCount: 0, rows: [...buildings.values()].filter(building => building.roomId === params[0]).map(({ roomId, ...building }) => building) };
    }
    if (normalized.startsWith("SELECT id, created_by, world_name FROM coop_rooms WHERE code = $1")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id, created_by: room.createdBy, world_name: room.worldName }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT id, created_by FROM coop_rooms WHERE code = $1")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id, created_by: room.createdBy }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_world_saves SET world_state")) {
      const save = worldSaves.get(params[0]);
      if (!save || save.saveRevision !== params[3]) return { rowCount: 0, rows: [] };
      save.worldState = params[1];
      save.updatedBy = params[2];
      save.saveRevision += 1;
      return { rowCount: 1, rows: [{ save_revision: save.saveRevision, updated_at: save.updatedAt }] };
    }
    if (normalized.startsWith("INSERT INTO coop_buildings (room_id")) {
      const room = roomByCode(params[0]);
      const member = room ? memberFor(room.id, params[2]) : null;
      if (!room || !member || !member.isActive) return { rowCount: 0, rows: [] };
      const building = { id: `building-${nextBuildingId++}`, roomId: room.id, building_type: params[1], placed_by: params[2], transform: params[3], state: params[4], created_at: null, updated_at: null };
      buildings.set(building.id, building);
      const { roomId, ...row } = building;
      return { rowCount: 1, rows: [row] };
    }
    if (normalized.startsWith("UPDATE coop_members SET is_active")) {
      const room = rooms.get(params[0]) || roomByCode(params[0]);
      const member = room ? memberFor(room.id, params[1]) : null;
      if (member) { member.isActive = normalized.includes("is_active = TRUE"); member.lastSeenAt = Date.now(); }
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
    if (normalized.startsWith("SELECT id, creature_id, position_x, position_y, health_fraction, revision FROM coop_creature_targets WHERE room_id")) {
      const target = targets.get(targetKey(params[0], params[1]));
      return target && target.active ? { rowCount: 1, rows: [{ ...target }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT 1 FROM coop_companions")) {
      return companions.has(key(params[0], params[1])) ? { rowCount: 1, rows: [{}] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT companion_id, creature_id, display_name, command, bond, health_fraction, revision, captured_at, updated_at FROM coop_companions")) {
      const companion = companions.get(key(params[0], params[1]));
      return companion ? { rowCount: 1, rows: [{ ...companion }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT id, recipe_id, transform, state, revision, created_at, updated_at FROM coop_camps WHERE id")) {
      const camp = [...camps.values()].find(candidate => candidate.id === params[0]);
      return camp ? { rowCount: 1, rows: [{ ...camp }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT id, recipe_id, transform, state, revision, created_at, updated_at FROM coop_camps WHERE room_id")) {
      const camp = camps.get(key(params[0], params[1]));
      return camp ? { rowCount: 1, rows: [{ ...camp }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("INSERT INTO coop_companions")) {
      const companion = { companion_id: `companion-${nextCompanionId++}`, creature_id: params[2], display_name: params[3], command: "follow", bond: 0, health_fraction: 0.75, revision: 1, captured_at: null, updated_at: null };
      companions.set(key(params[0], params[1]), companion);
      return { rowCount: 1, rows: [{ ...companion }] };
    }
    if (normalized.startsWith("UPDATE coop_creature_targets SET active")) {
      const target = [...targets.values()].find(candidate => candidate.id === params[0]);
      if (target) { target.active = false; target.captured_by = params[1]; target.revision += 1; }
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_companions SET command")) {
      const companion = companions.get(key(params[0], params[1]));
      if (!companion) return { rowCount: 0, rows: [] };
      companion.command = params[2];
      companion.revision += 1;
      return { rowCount: 1, rows: [{ ...companion }] };
    }
    if (normalized.startsWith("INSERT INTO coop_camps")) {
      const camp = { id: `camp-${nextCampId++}`, recipe_id: params[2], transform: params[3], state: {}, revision: 1, created_at: null, updated_at: null };
      camps.set(key(params[0], params[1]), camp);
      return { rowCount: 1, rows: [{ ...camp }] };
    }
    if (normalized.startsWith("DELETE FROM coop_camps")) {
      const camp = [...camps.entries()].find(([, value]) => value.id === params[0]);
      if (camp) camps.delete(camp[0]);
      return { rowCount: 1, rows: [] };
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
      const result = params.length >= 6 ? params[5] : params[4] ?? params[3];
      receipts.set(`${params[0]}:${params[1]}:${params[2]}`, result);
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("SELECT id FROM coop_rooms WHERE code = $1 FOR UPDATE")) {
      const room = roomByCode(params[0]);
      return room ? { rowCount: 1, rows: [{ id: room.id }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT player_x, player_y, wood, fiber, stone, ember_kit, inventory_revision, member_revision, is_active")) {
      const member = memberFor(params[0], params[1]);
      return member ? { rowCount: 1, rows: [{ player_x: member.playerX, player_y: member.playerY, wood: member.wood, fiber: member.fiber, stone: member.stone, ember_kit: member.emberKit, inventory_revision: member.inventoryRevision, member_revision: member.memberRevision, is_active: member.isActive }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("SELECT player_x, player_y, wood, fiber, stone, inventory_revision, ember_kit")) {
      const member = memberFor(params[0], params[1]);
      return member ? { rowCount: 1, rows: [{ player_x: member.playerX, player_y: member.playerY, wood: member.wood, fiber: member.fiber, stone: member.stone, inventory_revision: member.inventoryRevision, ember_kit: member.emberKit }] } : { rowCount: 0, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_members SET fiber = $3, inventory_revision = $4, member_revision = $5")) {
      const member = memberFor(params[0], params[1]);
      if (member) { member.fiber = params[2]; member.inventoryRevision = params[3]; member.memberRevision = params[4]; member.lastSeenAt = Date.now(); }
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_members SET wood = $3, fiber = $4, inventory_revision = $5, member_revision = $6")) {
      const member = memberFor(params[0], params[1]);
      if (member) { member.wood = params[2]; member.fiber = params[3]; member.inventoryRevision = params[4]; member.memberRevision = params[5]; member.lastSeenAt = Date.now(); }
      return { rowCount: 1, rows: [] };
    }
    if (normalized.startsWith("UPDATE coop_members SET wood")) {
      const member = memberFor(params[0], params[1]);
      if (member) { member.wood = params[2]; member.fiber = params[3]; member.stone = params[4]; member.emberKit = params[5]; member.inventoryRevision = params[6]; member.itemState = params[7]; member.memberRevision = params[8]; member.lastSeenAt = Date.now(); }
      return { rowCount: 1, rows: [] };
    }
    if (normalized === "BEGIN" || normalized === "COMMIT" || normalized === "ROLLBACK") return { rowCount: 0, rows: [] };
    throw new Error(`Unhandled simulation query: ${normalized}`);
  }

  return {
    query,
    connect: async () => ({ query, release() {} }),
    setTargetHealth(code, creatureId, healthFraction) {
      const room = roomByCode(code);
      const target = room ? targets.get(targetKey(room.id, creatureId)) : null;
      if (target) target.health_fraction = healthFraction;
    },
    setMemberPosition(code, accountId, playerX, playerY) {
      const room = roomByCode(code);
      const member = room ? memberFor(room.id, accountId) : null;
      if (member) { member.playerX = playerX; member.playerY = playerY; }
    },
    setMemberResources(code, accountId, wood, fiber) {
      const room = roomByCode(code);
      const member = room ? memberFor(room.id, accountId) : null;
      if (member) { member.wood = wood; member.fiber = fiber; }
    }
  };
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
const tokenC = issueAccessToken({ accountId: "account-c", sessionId: 3, secret: config.sessionSecret, now: Date.now(), ttlSeconds: 900 }).token;
const tokenD = issueAccessToken({ accountId: "account-d", sessionId: 4, secret: config.sessionSecret, now: Date.now(), ttlSeconds: 900 }).token;
const tokenE = issueAccessToken({ accountId: "account-e", sessionId: 5, secret: config.sessionSecret, now: Date.now(), ttlSeconds: 900 }).token;

try {
  const created = await request(baseUrl, tokenA, "/v1/coop/rooms", "POST", { region: "asia", worldName: "Aurora's Grove" });
  assert.equal(created.status, 201);
  const code = created.payload.room.code;
  assert.match(code, /^[A-Z0-9]{6}$/);
  assert.equal(created.payload.participants.length, 1);
  assert.equal(created.payload.room.worldName, "Aurora's Grove");
  assert.equal(created.payload.room.ownerAccountId, "account-a");

  const joined = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/join`, "POST", {});
  assert.equal(joined.status, 200);
  assert.equal(joined.payload.participants.length, 2);
  const joinedC = await request(baseUrl, tokenC, `/v1/coop/rooms/${code}/join`, "POST", {});
  const joinedD = await request(baseUrl, tokenD, `/v1/coop/rooms/${code}/join`, "POST", {});
  assert.equal(joinedC.status, 200);
  assert.equal(joinedD.status, 200);
  assert.equal(joinedD.payload.room.maxPlayers, 4);
  assert.equal(joinedD.payload.participants.length, 4);
  const full = await request(baseUrl, tokenE, `/v1/coop/rooms/${code}/join`, "POST", {});
  assert.equal(full.status, 409);

  pool.setMemberPosition(code, "account-a", -0.62, 0.42);
  const capture = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/companions/capture`, "POST", { requestId: "capture-a-001", creatureId: "moon_deer" });
  assert.equal(capture.status, 201);
  assert.equal(capture.payload.companion.creature_id, "moon_deer");
  assert.equal(capture.payload.inventory.fiber, 6);
  const duplicateCapture = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/companions/capture`, "POST", { requestId: "capture-a-001", creatureId: "moon_deer" });
  assert.equal(duplicateCapture.status, 200);
  assert.deepEqual(duplicateCapture.payload, capture.payload);

  const command = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/companions/command`, "POST", { requestId: "command-a-001", command: "stay", expectedRevision: 1 });
  assert.equal(command.status, 200);
  assert.equal(command.payload.companion.command, "stay");
  assert.equal(command.payload.companion.revision, 2);
  const staleCommand = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/companions/command`, "POST", { requestId: "command-a-stale", command: "follow", expectedRevision: 1 });
  assert.equal(staleCommand.status, 409);
  assert.equal(staleCommand.payload.companion.revision, 2);
  const retriedCommand = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/companions/command`, "POST", { requestId: "command-a-002", command: "follow", expectedRevision: 2 });
  assert.equal(retriedCommand.status, 200);
  assert.equal(retriedCommand.payload.companion.revision, 3);

  pool.setMemberPosition(code, "account-c", -0.62, 0.42);
  const rangeRejectedCapture = await request(baseUrl, tokenC, `/v1/coop/rooms/${code}/companions/capture`, "POST", { requestId: "capture-c-range", creatureId: "moon_deer" });
  assert.equal(rangeRejectedCapture.status, 404);
  pool.setMemberPosition(code, "account-d", 0.64, 0.26);
  pool.setTargetHealth(code, "canopy_fox", 0.9);
  const healthRejectedCapture = await request(baseUrl, tokenD, `/v1/coop/rooms/${code}/companions/capture`, "POST", { requestId: "capture-d-health", creatureId: "canopy_fox" });
  assert.equal(healthRejectedCapture.status, 422);
  pool.setMemberPosition(code, "account-b", -0.28, 0.40);
  pool.setMemberResources(code, "account-b", 12, 1);
  const fiberRejectedCapture = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/companions/capture`, "POST", { requestId: "capture-b-fiber", creatureId: "mossback_boar" });
  assert.equal(fiberRejectedCapture.status, 422);
  pool.setMemberResources(code, "account-b", 12, 8);

  const camp = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/camps`, "POST", { requestId: "camp-a-001", recipeId: "field_camp", expectedRevision: 0, transform: { x: -0.62, y: 0.42, z: 0, yaw: 0, scale: 1 } });
  assert.equal(camp.status, 201);
  assert.equal(camp.payload.camp.recipe_id, "field_camp");
  assert.deepEqual(camp.payload.inventory, { wood: 6, fiber: 2, stone: 4, emberKit: false });
  const duplicateCamp = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/camps`, "POST", { requestId: "camp-a-001", recipeId: "field_camp", expectedRevision: 0, transform: { x: -0.62, y: 0.42, z: 0, yaw: 0, scale: 1 } });
  assert.equal(duplicateCamp.status, 201);
  assert.deepEqual(duplicateCamp.payload, camp.payload);
  const campConflictPlacement = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/camps`, "POST", { requestId: "camp-a-stale", recipeId: "field_camp", expectedRevision: 0, transform: { x: -0.62, y: 0.42, z: 0, yaw: 0, scale: 1 } });
  assert.equal(campConflictPlacement.status, 409);
  assert.equal(campConflictPlacement.payload.camp.revision, 1);

  pool.setMemberPosition(code, "account-c", -0.55, -0.08);
  const campRangeRejected = await request(baseUrl, tokenC, `/v1/coop/rooms/${code}/camps`, "POST", { requestId: "camp-c-range", recipeId: "field_camp", expectedRevision: 0, transform: { x: 0.5, y: 0.5, z: 0, yaw: 0, scale: 1 } });
  assert.equal(campRangeRejected.status, 422);
  pool.setMemberResources(code, "account-d", 5, 8);
  const campMaterialsRejected = await request(baseUrl, tokenD, `/v1/coop/rooms/${code}/camps`, "POST", { requestId: "camp-d-materials", recipeId: "field_camp", expectedRevision: 0, transform: { x: 0.64, y: 0.26, z: 0, yaw: 0, scale: 1 } });
  assert.equal(campMaterialsRejected.status, 422);
  const campId = camp.payload.camp.id;
  const staleRemoval = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/camps/${campId}`, "DELETE", { requestId: "camp-remove-stale", expectedRevision: 0 });
  assert.equal(staleRemoval.status, 409);
  assert.equal(staleRemoval.payload.camp.revision, 1);
  const removedCamp = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/camps/${campId}`, "DELETE", { requestId: "camp-remove-001", expectedRevision: 1 });
  assert.equal(removedCamp.status, 200);
  assert.equal(removedCamp.payload.removedRevision, 2);
  const authorityState = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/companions`, "GET");
  assert.equal(authorityState.status, 200);
  assert.equal(authorityState.payload.companion.command, "follow");
  assert.equal(authorityState.payload.camp, null);
  assert.equal(authorityState.payload.targets.length, 3);

  const tower = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/heartbeat`, "POST", { playerX: -0.06, playerY: 0.28, atTower: true, towerRevision: 1 });
  assert.equal(tower.status, 200);
  assert.equal(tower.payload.room.towerRevision, 1);
  assert.ok(tower.payload.participants.some(participant => participant.accountId === "account-a" && participant.atTower));

  const reconnect = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/reconnect`, "POST", {});
  assert.equal(reconnect.status, 200);
  assert.equal(reconnect.payload.room.maxPlayers, 4);

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

  const playerBeforeSave = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/player-save`, "GET");
  assert.equal(playerBeforeSave.status, 200);
  assert.equal(playerBeforeSave.payload.ownerAccountId, "account-a");
  assert.equal(playerBeforeSave.payload.worldName, "Aurora's Grove");
  assert.equal(playerBeforeSave.payload.memberRevision, 1);
  const savedPlayer = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/player-save`, "PUT", {
    expectedRevision: 1,
    itemState: { wood: 10, fiber: 8, stone: 4, emberKit: true, items: ["ember_kit"] },
    progressionState: { level: 3, xp: 240, unlocked: ["campfire"] }
  });
  assert.equal(savedPlayer.status, 200);
  assert.equal(savedPlayer.payload.memberRevision, 2);
  const stalePlayerSave = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/player-save`, "PUT", {
    expectedRevision: 1,
    itemState: { wood: 1 },
    progressionState: { level: 1, xp: 0, unlocked: [] }
  });
  assert.equal(stalePlayerSave.status, 409);

  const leave = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/leave`, "DELETE");
  assert.equal(leave.status, 204);
  const inactiveBuild = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/buildings`, "POST", {
    buildingType: "foundation",
    transform: { x: 1, y: 2, z: 0, yaw: 0 },
    state: { tier: 1 }
  });
  assert.equal(inactiveBuild.status, 403);
  const savedAfterLeave = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/player-save`, "GET");
  assert.equal(savedAfterLeave.status, 200);
  assert.deepEqual(savedAfterLeave.payload.progressionState, { level: 3, xp: 240, unlocked: ["campfire"] });

  const reconnectAfterLeave = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/reconnect`, "POST", {});
  assert.equal(reconnectAfterLeave.status, 200);
  const building = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/buildings`, "POST", {
    buildingType: "foundation",
    transform: { x: 1, y: 2, z: 0, yaw: 0 },
    state: { tier: 1, material: "wood" }
  });
  assert.equal(building.status, 201);
  assert.equal(building.payload.building.placed_by, "account-b");

  const nonOwnerSave = await request(baseUrl, tokenB, `/v1/coop/rooms/${code}/save`, "PUT", {
    expectedRevision: 0,
    worldState: { schemaVersion: 1, quests: { firstEmber: "active" } }
  });
  assert.equal(nonOwnerSave.status, 403);
  const worldSave = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/save`, "PUT", {
    expectedRevision: 0,
    worldState: { schemaVersion: 1, quests: { firstEmber: "complete" }, claimedResources: ["forest_cache"] }
  });
  assert.equal(worldSave.status, 200);
  assert.equal(worldSave.payload.saveRevision, 1);
  const staleWorldSave = await request(baseUrl, tokenA, `/v1/coop/rooms/${code}/save`, "PUT", {
    expectedRevision: 0,
    worldState: { schemaVersion: 1, quests: { firstEmber: "active" } }
  });
  assert.equal(staleWorldSave.status, 409);
  const restoredWorld = await request(baseUrl, tokenC, `/v1/coop/rooms/${code}/save`, "GET");
  assert.equal(restoredWorld.status, 200);
  assert.equal(restoredWorld.payload.worldName, "Aurora's Grove");
  assert.equal(restoredWorld.payload.saveRevision, 1);
  assert.equal(restoredWorld.payload.worldState.quests.firstEmber, "complete");
  assert.equal(restoredWorld.payload.buildings.length, 1);

  console.log(JSON.stringify({
    ok: true,
    roomCode: code,
    checks: ["room_created", "future_room_targets_seeded", "friend_joined", "four_player_cap_validated", "fifth_player_rejected", "companion_capture_validated", "companion_capture_retry_idempotent", "companion_command_revision_conflict_rejected", "companion_command_retry_validated", "capture_range_and_health_and_fiber_rejected", "camp_materials_and_range_rejected", "camp_placement_validated", "camp_retry_idempotent", "camp_revision_conflict_rejected", "camp_removal_validated", "authority_state_reloaded", "reconnect_presence_refreshed", "tower_revision_seen", "co_op_clock_read", "combat_validated", "combat_retry_idempotent", "combat_range_rejected", "inventory_reward_validated", "inventory_retry_idempotent", "craft_validated", "player_save_persisted", "player_save_conflict_rejected", "leave_preserved_membership", "reconnect_restored_membership", "building_persisted", "world_owner_enforced", "world_save_conflict_rejected", "world_reload_includes_building"]
  }, null, 2));
} finally {
  await new Promise(resolve => server.close(resolve));
}
