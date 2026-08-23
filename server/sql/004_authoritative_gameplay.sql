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
