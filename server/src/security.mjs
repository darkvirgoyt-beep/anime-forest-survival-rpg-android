import crypto from "node:crypto";

export const SESSION_ISSUER = "aethelgard-online-services";
export const SESSION_AUDIENCE = "aethelgard-android";

export function loadRuntimeConfig(env = process.env) {
  const required = [
    "DATABASE_URL",
    "GOOGLE_ID_TOKEN_AUDIENCE",
    "GAME_SESSION_JWT_SECRET",
    "ALLOWED_ORIGIN"
  ];
  const missing = required.filter(key => !env[key] || env[key].startsWith("REPLACE_"));
  if (missing.length > 0) throw new Error(`missing_required_configuration:${missing.join(",")}`);
  if (env.GAME_SESSION_JWT_SECRET.length < 32) throw new Error("GAME_SESSION_JWT_SECRET must be at least 32 characters");
  if (env.NODE_ENV === "production" && !env.ALLOWED_ORIGIN.startsWith("https://")) {
    throw new Error("ALLOWED_ORIGIN must use HTTPS in production");
  }

  return {
    databaseUrl: env.DATABASE_URL,
    databaseSsl: env.DATABASE_SSL === "true",
    googleIdTokenAudience: env.GOOGLE_ID_TOKEN_AUDIENCE,
    googlePlayGamesClientId: optionalConfigValue(env.GOOGLE_GAME_SERVER_CLIENT_ID),
    googlePlayGamesClientSecret: optionalConfigValue(env.GOOGLE_GAME_SERVER_CLIENT_SECRET),
    sessionSecret: env.GAME_SESSION_JWT_SECRET,
    allowedOrigin: env.ALLOWED_ORIGIN,
    accessTtlSeconds: boundedPositiveInt(env.GAME_ACCESS_TOKEN_TTL_SECONDS, 900, 60, 3600),
    refreshTtlSeconds: boundedPositiveInt(env.GAME_REFRESH_TOKEN_TTL_SECONDS, 60 * 60 * 24 * 30, 3600, 60 * 60 * 24 * 90)
  };
}

export function validateServerAuthCode(value) {
  return typeof value === "string" && value.length >= 16 && value.length <= 4096 && !/[\u0000-\u001f]/.test(value);
}

export function validateGoogleIdToken(value) {
  return typeof value === "string" && value.length >= 100 && value.length <= 16_384 && /^[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+$/.test(value);
}

export function hashSecret(value) {
  return crypto.createHash("sha256").update(value).digest("hex");
}

export function createOpaqueToken() {
  return crypto.randomBytes(48).toString("base64url");
}

export function issueAccessToken({ accountId, sessionId, secret, now = Date.now(), ttlSeconds = 900 }) {
  const issuedAt = Math.floor(now / 1000);
  const expiresAt = issuedAt + ttlSeconds;
  const header = encodeJson({ alg: "HS256", typ: "JWT" });
  const payload = encodeJson({
    iss: SESSION_ISSUER,
    aud: SESSION_AUDIENCE,
    sub: accountId,
    sid: String(sessionId),
    iat: issuedAt,
    exp: expiresAt
  });
  const signingInput = `${header}.${payload}`;
  const signature = crypto.createHmac("sha256", secret).update(signingInput).digest("base64url");
  return { token: `${signingInput}.${signature}`, expiresAt };
}

export function verifyAccessToken(token, secret, now = Date.now()) {
  if (typeof token !== "string") throw new Error("missing_session");
  const parts = token.split(".");
  if (parts.length !== 3) throw new Error("invalid_session");

  const expected = crypto.createHmac("sha256", secret).update(`${parts[0]}.${parts[1]}`).digest();
  const actual = Buffer.from(parts[2], "base64url");
  if (actual.length !== expected.length || !crypto.timingSafeEqual(expected, actual)) throw new Error("invalid_session");

  let payload;
  try {
    payload = JSON.parse(Buffer.from(parts[1], "base64url").toString("utf8"));
  } catch {
    throw new Error("invalid_session");
  }
  const nowSeconds = Math.floor(now / 1000);
  if (
    payload.iss !== SESSION_ISSUER ||
    payload.aud !== SESSION_AUDIENCE ||
    typeof payload.sub !== "string" ||
    typeof payload.sid !== "string" ||
    !Number.isInteger(payload.exp) ||
    payload.exp <= nowSeconds
  ) {
    throw new Error("expired_session");
  }
  return payload;
}

export function createReplayGuard() {
  const seen = new Set();
  return code => {
    const digest = hashSecret(code);
    if (seen.has(digest)) return false;
    seen.add(digest);
    return true;
  };
}

function boundedPositiveInt(raw, fallback, min, max) {
  const parsed = Number(raw ?? fallback);
  if (!Number.isInteger(parsed) || parsed < min || parsed > max) {
    throw new Error("invalid_session_ttl_configuration");
  }
  return parsed;
}

function optionalConfigValue(value) {
  return !value || value.startsWith("REPLACE_") ? null : value;
}

function encodeJson(value) {
  return Buffer.from(JSON.stringify(value)).toString("base64url");
}
