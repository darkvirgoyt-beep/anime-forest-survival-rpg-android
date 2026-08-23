-- AETHELGRAD online-service schema export
-- Apply in order: 001_init.sql, 002_hardened_sessions.sql,
-- 003_coop_rendezvous.sql, 004_authoritative_gameplay.sql.
-- PostgreSQL 16+ recommended.

CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- 001_init.sql
CREATE TABLE IF NOT EXISTS accounts (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    provider TEXT NOT NULL,
    provider_player_id TEXT NOT NULL,
    display_name TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (provider, provider_player_id)
);

CREATE TABLE IF NOT EXISTS sessions (
    id BIGSERIAL PRIMARY KEY,
    account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    token_hash TEXT NOT NULL UNIQUE,
    expires_at TIMESTAMPTZ NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS sessions_account_id_idx ON sessions(account_id);
CREATE INDEX IF NOT EXISTS sessions_expires_at_idx ON sessions(expires_at);

CREATE TABLE IF NOT EXISTS worlds (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    region TEXT NOT NULL,
    name TEXT NOT NULL,
    status TEXT NOT NULL CHECK (status IN ('allocating', 'online', 'draining', 'offline')),
    max_players INTEGER NOT NULL CHECK (max_players > 0),
    current_players INTEGER NOT NULL DEFAULT 0 CHECK (current_players >= 0),
    dedicated_server_id TEXT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS worlds_region_status_idx ON worlds(region, status);

CREATE TABLE IF NOT EXISTS characters (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    state JSONB NOT NULL DEFAULT '{}'::jsonb,
    version BIGINT NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (account_id, name)
);

-- 002_hardened_sessions.sql
CREATE TABLE IF NOT EXISTS authorization_code_receipts (
    code_hash TEXT PRIMARY KEY,
    received_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    exchange_state TEXT NOT NULL CHECK (exchange_state IN ('received', 'verified', 'rejected')),
    rejection_code TEXT
);

ALTER TABLE sessions
    ADD COLUMN IF NOT EXISTS refresh_token_hash TEXT UNIQUE,
    ADD COLUMN IF NOT EXISTS refresh_expires_at TIMESTAMPTZ,
    ADD COLUMN IF NOT EXISTS revoked_at TIMESTAMPTZ,
    ADD COLUMN IF NOT EXISTS revoked_reason TEXT,
    ADD COLUMN IF NOT EXISTS parent_session_id BIGINT REFERENCES sessions(id),
    ADD COLUMN IF NOT EXISTS replaced_by_session_id BIGINT REFERENCES sessions(id),
    ADD COLUMN IF NOT EXISTS last_seen_at TIMESTAMPTZ NOT NULL DEFAULT now();

CREATE INDEX IF NOT EXISTS sessions_refresh_lookup_idx
    ON sessions(refresh_token_hash, refresh_expires_at)
    WHERE refresh_token_hash IS NOT NULL;

CREATE INDEX IF NOT EXISTS sessions_active_account_idx
    ON sessions(account_id, revoked_at, expires_at);

-- 003_coop_rendezvous.sql
CREATE TABLE IF NOT EXISTS coop_rooms (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    code TEXT NOT NULL UNIQUE CHECK (code ~ '^[A-Z0-9]{6}$'),
    region TEXT NOT NULL,
    max_players INTEGER NOT NULL DEFAULT 4 CHECK (max_players BETWEEN 2 AND 4),
    world_time DOUBLE PRECISION NOT NULL DEFAULT 0 CHECK (world_time >= 0),
    last_tick_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    tower_revision BIGINT NOT NULL DEFAULT 0 CHECK (tower_revision >= 0),
    created_by UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS coop_members (
    room_id UUID NOT NULL REFERENCES coop_rooms(id) ON DELETE CASCADE,
    account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    player_x REAL NOT NULL DEFAULT -0.55 CHECK (player_x BETWEEN -0.90 AND 0.90),
    player_y REAL NOT NULL DEFAULT -0.08 CHECK (player_y BETWEEN -0.50 AND 0.52),
    at_tower BOOLEAN NOT NULL DEFAULT false,
    tower_revision BIGINT NOT NULL DEFAULT 0 CHECK (tower_revision >= 0),
    last_seen_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (room_id, account_id)
);

CREATE INDEX IF NOT EXISTS coop_members_presence_idx ON coop_members(room_id, last_seen_at);
CREATE INDEX IF NOT EXISTS coop_rooms_code_idx ON coop_rooms(code);

-- 004_authoritative_gameplay.sql
ALTER TABLE coop_rooms
    ADD COLUMN IF NOT EXISTS boss_health INTEGER NOT NULL DEFAULT 100 CHECK (boss_health BETWEEN 0 AND 100),
    ADD COLUMN IF NOT EXISTS combat_revision BIGINT NOT NULL DEFAULT 0 CHECK (combat_revision >= 0);

ALTER TABLE coop_members
    ADD COLUMN IF NOT EXISTS wood INTEGER NOT NULL DEFAULT 12 CHECK (wood >= 0),
    ADD COLUMN IF NOT EXISTS fiber INTEGER NOT NULL DEFAULT 8 CHECK (fiber >= 0),
    ADD COLUMN IF NOT EXISTS stone INTEGER NOT NULL DEFAULT 4 CHECK (stone >= 0),
    ADD COLUMN IF NOT EXISTS inventory_revision BIGINT NOT NULL DEFAULT 0 CHECK (inventory_revision >= 0),
    ADD COLUMN IF NOT EXISTS ember_kit BOOLEAN NOT NULL DEFAULT false,
    ADD COLUMN IF NOT EXISTS last_action_at TIMESTAMPTZ;

CREATE TABLE IF NOT EXISTS coop_action_receipts (
    id BIGSERIAL PRIMARY KEY,
    room_id UUID NOT NULL REFERENCES coop_rooms(id) ON DELETE CASCADE,
    account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    request_id TEXT NOT NULL CHECK (request_id ~ '^[A-Za-z0-9_-]{8,80}$'),
    action_type TEXT NOT NULL CHECK (action_type IN ('combat', 'inventory')),
    result JSONB NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (room_id, account_id, request_id)
);

CREATE INDEX IF NOT EXISTS coop_action_receipts_created_idx ON coop_action_receipts(created_at);
