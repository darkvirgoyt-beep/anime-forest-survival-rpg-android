import crypto from "node:crypto";
import express from "express";
import pg from "pg";

const { Pool } = pg;
const app = express();
const port = Number(process.env.PORT || 8080);
const databaseUrl = process.env.DATABASE_URL;
const googleClientId = process.env.GOOGLE_GAME_SERVER_CLIENT_ID;
const googleClientSecret = process.env.GOOGLE_GAME_SERVER_CLIENT_SECRET;
const jwtSecret = process.env.GAME_SESSION_JWT_SECRET;

if (!databaseUrl || !googleClientId || !googleClientSecret || !jwtSecret) {
  throw new Error("DATABASE_URL, GOOGLE_GAME_SERVER_CLIENT_ID, GOOGLE_GAME_SERVER_CLIENT_SECRET, and GAME_SESSION_JWT_SECRET are required");
}
if (jwtSecret.length < 32) {
  throw new Error("GAME_SESSION_JWT_SECRET must be at least 32 characters");
}

const pool = new Pool({ connectionString: databaseUrl, max: 10, ssl: process.env.DATABASE_SSL === "true" ? { rejectUnauthorized: true } : undefined });
app.use(express.json({ limit: "16kb" }));
app.use((req, res, next) => {
  res.setHeader("Access-Control-Allow-Origin", process.env.ALLOWED_ORIGIN || "*");
  res.setHeader("Access-Control-Allow-Headers", "Authorization, Content-Type");
  res.setHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  if (req.method === "OPTIONS") return res.sendStatus(204);
  next();
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
  const serverAuthCode = req.body?.serverAuthCode;
  if (typeof serverAuthCode !== "string" || serverAuthCode.length < 16 || serverAuthCode.length > 4096) {
    return res.status(400).json({ error: "invalid_server_auth_code" });
  }

  try {
    const token = await exchangePlayGamesCode(serverAuthCode);
    const player = await verifyPlayGamesPlayer(token.access_token);
    const account = await upsertAccount(player);
    const session = issueSession(account.id);
    await pool.query(
      "INSERT INTO sessions (account_id, token_hash, expires_at) VALUES ($1, $2, to_timestamp($3))",
      [account.id, hash(session.token), session.expiresAt]
    );
    res.status(200).json({ sessionToken: session.token, accountId: account.id, expiresAt: new Date(session.expiresAt * 1000).toISOString() });
  } catch (error) {
    console.error("play_games_exchange_failed", error instanceof Error ? error.message : "unknown_error");
    res.status(401).json({ error: "play_games_authentication_failed" });
  }
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

app.use((error, _req, res, _next) => {
  console.error("unhandled_request_error", error);
  res.status(500).json({ error: "internal_server_error" });
});

app.listen(port, () => console.log(`Aethelgard online services listening on :${port}`));

async function exchangePlayGamesCode(serverAuthCode) {
  const body = new URLSearchParams({
    client_id: googleClientId,
    client_secret: googleClientSecret,
    code: serverAuthCode,
    grant_type: "authorization_code",
    redirect_uri: ""
  });
  const response = await fetch("https://oauth2.googleapis.com/token", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body
  });
  if (!response.ok) throw new Error(`google_token_exchange_${response.status}`);
  const payload = await response.json();
  if (typeof payload.access_token !== "string") throw new Error("google_access_token_missing");
  return payload;
}

async function verifyPlayGamesPlayer(accessToken) {
  const response = await fetch("https://www.googleapis.com/games/v1/players/me", {
    headers: { Authorization: `Bearer ${accessToken}`, Accept: "application/json" }
  });
  if (!response.ok) throw new Error(`google_player_verification_${response.status}`);
  const player = await response.json();
  if (typeof player.playerId !== "string" || player.playerId.length === 0) throw new Error("google_player_id_missing");
  return player;
}

async function upsertAccount(player) {
  const result = await pool.query(
    `INSERT INTO accounts (provider, provider_player_id, display_name)
     VALUES ('google_play_games', $1, $2)
     ON CONFLICT (provider, provider_player_id)
     DO UPDATE SET display_name = EXCLUDED.display_name, updated_at = now()
     RETURNING id`,
    [player.playerId, String(player.displayName || "Player").slice(0, 80)]
  );
  return result.rows[0];
}

function issueSession(accountId) {
  const now = Math.floor(Date.now() / 1000);
  const expiresAt = now + 60 * 60;
  const header = encode({ alg: "HS256", typ: "JWT" });
  const payload = encode({ sub: accountId, iat: now, exp: expiresAt, iss: "aethelgard-online-services" });
  const signingInput = `${header}.${payload}`;
  const signature = crypto.createHmac("sha256", jwtSecret).update(signingInput).digest("base64url");
  return { token: `${signingInput}.${signature}`, expiresAt };
}

async function requireSession(req, res, next) {
  const authorization = req.get("authorization") || "";
  const token = authorization.startsWith("Bearer ") ? authorization.slice(7) : "";
  const parts = token.split(".");
  if (parts.length !== 3) return res.status(401).json({ error: "missing_session" });
  try {
    const expected = crypto.createHmac("sha256", jwtSecret).update(`${parts[0]}.${parts[1]}`).digest("base64url");
    const actual = Buffer.from(parts[2]);
    const expectedBytes = Buffer.from(expected);
    if (actual.length !== expectedBytes.length || !crypto.timingSafeEqual(expectedBytes, actual)) {
      return res.status(401).json({ error: "invalid_session" });
    }
    const payload = JSON.parse(Buffer.from(parts[1], "base64url").toString("utf8"));
    if (payload.iss !== "aethelgard-online-services" || typeof payload.sub !== "string" || payload.exp <= Math.floor(Date.now() / 1000)) {
      return res.status(401).json({ error: "expired_session" });
    }
    const result = await pool.query(
      "SELECT account_id FROM sessions WHERE token_hash = $1 AND expires_at > now() LIMIT 1",
      [hash(token)]
    );
    if (result.rowCount !== 1) return res.status(401).json({ error: "revoked_session" });
    req.accountId = result.rows[0].account_id;
    next();
  } catch {
    res.status(401).json({ error: "invalid_session" });
  }
}

function encode(value) {
  return Buffer.from(JSON.stringify(value)).toString("base64url");
}

function hash(value) {
  return crypto.createHash("sha256").update(value).digest("hex");
}
