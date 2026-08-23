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
