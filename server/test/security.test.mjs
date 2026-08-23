import assert from "node:assert/strict";
import http from "node:http";
import test from "node:test";
import {
  createOpaqueToken,
  createReplayGuard,
  hashSecret,
  issueAccessToken,
  loadRuntimeConfig,
  validateGoogleIdToken,
  validateGuestKey,
  validateServerAuthCode,
  verifyAccessToken
} from "../src/security.mjs";
import { createOnlineService } from "../src/server.mjs";

const validConfig = {
  DATABASE_URL: "postgres://example",
  GOOGLE_ID_TOKEN_AUDIENCE: "web-client.apps.googleusercontent.com",
  GAME_SESSION_JWT_SECRET: "a-very-long-session-signing-secret-for-tests",
  ALLOWED_ORIGIN: "https://control.aethelgard.example",
  GAME_ACCESS_TOKEN_TTL_SECONDS: "900",
  GAME_REFRESH_TOKEN_TTL_SECONDS: "2592000"
};

test("runtime configuration rejects placeholders, missing values, and insecure production origins", () => {
  assert.throws(() => loadRuntimeConfig({ ...validConfig, DATABASE_URL: "" }), /missing_required_configuration/);
  assert.throws(() => loadRuntimeConfig({ ...validConfig, GOOGLE_ID_TOKEN_AUDIENCE: "REPLACE_ME" }), /missing_required_configuration/);
  assert.throws(() => loadRuntimeConfig({ ...validConfig, NODE_ENV: "production", ALLOWED_ORIGIN: "http://localhost" }), /ALLOWED_ORIGIN must use HTTPS/);
  assert.equal(loadRuntimeConfig(validConfig).accessTtlSeconds, 900);
});

test("server auth code validator rejects invalid code and replay guard rejects a second receipt", () => {
  const replayGuard = createReplayGuard();
  assert.equal(validateServerAuthCode("short"), false);
  assert.equal(validateServerAuthCode("x".repeat(16)), true);
  assert.equal(replayGuard("x".repeat(16)), true);
  assert.equal(replayGuard("x".repeat(16)), false);
});

test("Google ID token validator rejects malformed values before verification", () => {
  assert.equal(validateGoogleIdToken("short"), false);
  assert.equal(validateGoogleIdToken(`${"a".repeat(34)}.${"b".repeat(34)}.${"c".repeat(34)}`), true);
  assert.equal(validateGoogleIdToken(`${"a".repeat(34)}.${"b".repeat(34)}.not valid`), false);
});

test("guest key validator accepts only opaque base64url keys", () => {
  assert.equal(validateGuestKey("short"), false);
  assert.equal(validateGuestKey("g".repeat(32)), true);
  assert.equal(validateGuestKey("g".repeat(31)), false);
  assert.equal(validateGuestKey("g".repeat(32) + "/"), false);
});

test("guest authentication upserts an anonymous account and issues a normal game session", async () => {
  let nextSessionId = 1;
  let receivedGuestHash = null;
  const pool = {
    async query(sql, params = []) {
      if (sql.includes("INSERT INTO accounts")) {
        receivedGuestHash = params[0];
        return { rowCount: 1, rows: [{ id: "guest-account-1" }] };
      }
      if (sql.includes("INSERT INTO sessions")) return { rowCount: 1, rows: [{ id: nextSessionId++ }] };
      return { rowCount: 1, rows: [] };
    }
  };
  const app = createOnlineService({ pool, config: loadRuntimeConfig(validConfig) });
  const server = http.createServer(app);
  await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
  const url = `http://127.0.0.1:${server.address().port}/v1/auth/guest`;
  try {
    const invalid = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ guestKey: "short" }) });
    assert.equal(invalid.status, 400);

    const guestKey = "g".repeat(43);
    const accepted = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ guestKey }) });
    const payload = await accepted.json();
    assert.equal(accepted.status, 200);
    assert.equal(payload.accountId, "guest-account-1");
    assert.equal(payload.accountType, "guest");
    assert.ok(payload.accessToken);
    assert.ok(payload.refreshToken);
    assert.equal(receivedGuestHash, hashSecret(guestKey));
  } finally {
    await new Promise(resolve => server.close(resolve));
  }
});

test("access tokens expire and tampered tokens are rejected", () => {
  const secret = validConfig.GAME_SESSION_JWT_SECRET;
  const issued = issueAccessToken({ accountId: "account-1", sessionId: 7, secret, now: 1_000_000, ttlSeconds: 60 });
  assert.equal(verifyAccessToken(issued.token, secret, 1_030_000).sub, "account-1");
  assert.throws(() => verifyAccessToken(issued.token, secret, 1_061_000), /expired_session/);
  assert.throws(() => verifyAccessToken(`${issued.token}x`, secret, 1_030_000), /invalid_session/);
  assert.ok(createOpaqueToken().length >= 64);
});

test("Play Games exchange rejects an invalid code and a replayed code", async () => {
  const codeReceipts = new Set();
  let nextSessionId = 1;
  const pool = {
    async query(sql, params = []) {
      if (sql.includes("INSERT INTO authorization_code_receipts")) {
        const isNew = !codeReceipts.has(params[0]);
        if (isNew) codeReceipts.add(params[0]);
        return { rowCount: isNew ? 1 : 0, rows: isNew ? [{ code_hash: params[0] }] : [] };
      }
      if (sql.includes("INSERT INTO accounts")) return { rowCount: 1, rows: [{ id: "account-1" }] };
      if (sql.includes("INSERT INTO sessions")) return { rowCount: 1, rows: [{ id: nextSessionId++ }] };
      return { rowCount: 1, rows: [] };
    }
  };
  const fetchImpl = async url => {
    if (url.includes("oauth2.googleapis.com")) return { ok: true, json: async () => ({ access_token: "google-access" }) };
    return { ok: true, json: async () => ({ playerId: "gpg-player-1", displayName: "Aethel" }) };
  };
  const app = createOnlineService({
    pool,
    config: loadRuntimeConfig({ ...validConfig, GOOGLE_GAME_SERVER_CLIENT_ID: "client.apps.googleusercontent.com", GOOGLE_GAME_SERVER_CLIENT_SECRET: "server-secret" }),
    fetchImpl
  });
  const server = http.createServer(app);
  await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
  const url = `http://127.0.0.1:${server.address().port}/v1/auth/play-games/exchange`;
  try {
    const invalid = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ serverAuthCode: "bad" }) });
    assert.equal(invalid.status, 400);

    const body = JSON.stringify({ serverAuthCode: "x".repeat(16) });
    const first = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body });
    const firstPayload = await first.json();
    assert.equal(first.status, 200);
    assert.equal(firstPayload.accountId, "account-1");
    assert.ok(firstPayload.accessToken);
    assert.ok(firstPayload.refreshToken);

    const replay = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body });
    assert.equal(replay.status, 409);
    assert.equal((await replay.json()).error, "replayed_server_auth_code");
  } finally {
    await new Promise(resolve => server.close(resolve));
  }
});

test("Google ID-token exchange verifies identity server-side and issues a game session", async () => {
  let nextSessionId = 1;
  const pool = {
    async query(sql) {
      if (sql.includes("INSERT INTO accounts")) return { rowCount: 1, rows: [{ id: "google-account-1" }] };
      if (sql.includes("INSERT INTO sessions")) return { rowCount: 1, rows: [{ id: nextSessionId++ }] };
      return { rowCount: 1, rows: [] };
    }
  };
  let receivedToken = null;
  const app = createOnlineService({
    pool,
    config: loadRuntimeConfig(validConfig),
    verifyGoogleIdTokenImpl: async idToken => {
      receivedToken = idToken;
      return { subject: "google-subject-1", displayName: "Aethel" };
    }
  });
  const server = http.createServer(app);
  await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
  const url = `http://127.0.0.1:${server.address().port}/v1/auth/google-id-token/exchange`;
  try {
    const invalid = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ idToken: "invalid" }) });
    assert.equal(invalid.status, 400);

    const idToken = `${"a".repeat(34)}.${"b".repeat(34)}.${"c".repeat(34)}`;
    const accepted = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ idToken }) });
    const payload = await accepted.json();
    assert.equal(accepted.status, 200);
    assert.equal(receivedToken, idToken);
    assert.equal(payload.accountId, "google-account-1");
    assert.ok(payload.accessToken);
    assert.ok(payload.refreshToken);
  } finally {
    await new Promise(resolve => server.close(resolve));
  }
});

test("refresh rotation rejects reuse of the prior refresh session", async () => {
  const initialRefreshToken = "r".repeat(64);
  const current = {
    id: 1,
    account_id: "account-1",
    refresh_token_hash: hashSecret(initialRefreshToken),
    refresh_expires_at: new Date(Date.now() + 60_000).toISOString(),
    revoked_at: null
  };
  let nextSessionId = 2;
  const client = {
    async query(sql, params = []) {
      if (sql === "BEGIN" || sql === "COMMIT" || sql === "ROLLBACK") return { rowCount: 0, rows: [] };
      if (sql.includes("FROM sessions") && sql.includes("FOR UPDATE")) {
        return params[0] === current.refresh_token_hash ? { rowCount: 1, rows: [current] } : { rowCount: 0, rows: [] };
      }
      if (sql.includes("INSERT INTO sessions")) return { rowCount: 1, rows: [{ id: nextSessionId++ }] };
      if (sql.includes("revoked_reason = 'rotated'")) {
        current.revoked_at = new Date().toISOString();
        return { rowCount: 1, rows: [] };
      }
      if (sql.includes("refresh_reuse_detected")) return { rowCount: 1, rows: [] };
      return { rowCount: 1, rows: [] };
    },
    release() {}
  };
  const pool = { connect: async () => client, query: async () => ({ rowCount: 0, rows: [] }) };
  const app = createOnlineService({ pool, config: loadRuntimeConfig(validConfig) });
  const server = http.createServer(app);
  await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
  const url = `http://127.0.0.1:${server.address().port}/v1/auth/refresh`;
  try {
    const body = JSON.stringify({ refreshToken: initialRefreshToken });
    const first = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body });
    assert.equal(first.status, 200);
    assert.ok((await first.json()).refreshToken);

    const replay = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body });
    assert.equal(replay.status, 401);
    assert.equal((await replay.json()).error, "replayed_refresh_session");
  } finally {
    await new Promise(resolve => server.close(resolve));
  }
});


test("co-op room routes reject malformed tower room codes after session verification", async () => {
  const issued = issueAccessToken({ accountId: "account-1", sessionId: 9, secret: validConfig.GAME_SESSION_JWT_SECRET, now: Date.now(), ttlSeconds: 900 });
  const pool = {
    async query(sql) {
      if (sql.includes("FROM sessions") && sql.includes("token_hash")) {
        return { rowCount: 1, rows: [{ id: 9, account_id: "account-1" }] };
      }
      return { rowCount: 1, rows: [] };
    }
  };
  const app = createOnlineService({ pool, config: loadRuntimeConfig(validConfig) });
  const server = http.createServer(app);
  await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
  try {
    const response = await fetch(`http://127.0.0.1:${server.address().port}/v1/coop/rooms/not-valid/join`, {
      method: "POST",
      headers: { Authorization: `Bearer ${issued.token}`, "Content-Type": "application/json" },
      body: "{}"
    });
    assert.equal(response.status, 400);
    assert.equal((await response.json()).error, "invalid_room_code");
  } finally {
    await new Promise(resolve => server.close(resolve));
  }
});
