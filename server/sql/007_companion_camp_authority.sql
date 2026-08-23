-- Server-authoritative companion and camp state for persistent co-op rooms.
-- Existing rooms receive safe empty state; no local prototype state is trusted.

CREATE TABLE IF NOT EXISTS coop_companions (
    room_id UUID NOT NULL REFERENCES coop_rooms(id) ON DELETE CASCADE,
    account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    companion_id UUID NOT NULL DEFAULT gen_random_uuid(),
    creature_id TEXT NOT NULL CHECK (creature_id <> ''),
    display_name TEXT NOT NULL CHECK (display_name <> ''),
    command TEXT NOT NULL DEFAULT 'follow' CHECK (command IN ('follow', 'stay')),
    bond INTEGER NOT NULL DEFAULT 0 CHECK (bond >= 0 AND bond <= 100),
    health_fraction NUMERIC(5,4) NOT NULL DEFAULT 0.75 CHECK (health_fraction >= 0 AND health_fraction <= 1),
    revision BIGINT NOT NULL DEFAULT 0 CHECK (revision >= 0),
    captured_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (room_id, account_id),
    UNIQUE (companion_id)
);

CREATE INDEX IF NOT EXISTS coop_companions_room_idx ON coop_companions(room_id, updated_at);

CREATE TABLE IF NOT EXISTS coop_creature_targets (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    room_id UUID NOT NULL REFERENCES coop_rooms(id) ON DELETE CASCADE,
    creature_id TEXT NOT NULL CHECK (creature_id <> ''),
    position_x NUMERIC(8,4) NOT NULL,
    position_y NUMERIC(8,4) NOT NULL,
    health_fraction NUMERIC(5,4) NOT NULL DEFAULT 0.28 CHECK (health_fraction >= 0 AND health_fraction <= 1),
    active BOOLEAN NOT NULL DEFAULT TRUE,
    captured_by UUID REFERENCES accounts(id) ON DELETE SET NULL,
    revision BIGINT NOT NULL DEFAULT 0 CHECK (revision >= 0),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (room_id, creature_id)
);

CREATE INDEX IF NOT EXISTS coop_creature_targets_room_idx ON coop_creature_targets(room_id, active);

INSERT INTO coop_creature_targets (room_id, creature_id, position_x, position_y, health_fraction)
SELECT r.id, seed.creature_id, seed.position_x, seed.position_y, seed.health_fraction
FROM coop_rooms r
CROSS JOIN (VALUES
    ('moon_deer', -0.62::numeric, 0.42::numeric, 0.28::numeric),
    ('mossback_boar', -0.28::numeric, 0.40::numeric, 0.34::numeric),
    ('river_otter', 0.42::numeric, -0.34::numeric, 0.25::numeric),
    ('canopy_fox', 0.64::numeric, 0.26::numeric, 0.30::numeric)
) AS seed(creature_id, position_x, position_y, health_fraction)
ON CONFLICT (room_id, creature_id) DO NOTHING;

CREATE TABLE IF NOT EXISTS coop_camps (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    room_id UUID NOT NULL REFERENCES coop_rooms(id) ON DELETE CASCADE,
    account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    recipe_id TEXT NOT NULL CHECK (recipe_id <> ''),
    transform JSONB NOT NULL,
    state JSONB NOT NULL DEFAULT '{}'::jsonb,
    revision BIGINT NOT NULL DEFAULT 0 CHECK (revision >= 0),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (room_id, account_id)
);

CREATE INDEX IF NOT EXISTS coop_camps_room_idx ON coop_camps(room_id, updated_at);

ALTER TABLE coop_action_receipts
    DROP CONSTRAINT IF EXISTS coop_action_receipts_action_type_check;

ALTER TABLE coop_action_receipts
    ADD COLUMN IF NOT EXISTS action_version INTEGER NOT NULL DEFAULT 1 CHECK (action_version >= 1),
    ADD COLUMN IF NOT EXISTS created_at TIMESTAMPTZ NOT NULL DEFAULT now();

ALTER TABLE coop_action_receipts
    ADD CONSTRAINT coop_action_receipts_action_type_check
    CHECK (action_type IN ('combat', 'inventory', 'companion', 'camp'));

CREATE INDEX IF NOT EXISTS coop_action_receipts_created_idx ON coop_action_receipts(created_at);
