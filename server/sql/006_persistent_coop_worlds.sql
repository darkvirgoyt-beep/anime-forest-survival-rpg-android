-- Persistent creator-owned co-op world state.
-- Existing coop_rooms remain the invite/access surface, but leaving no longer
-- destroys a player's saved inventory or progression.
ALTER TABLE coop_rooms
    ADD COLUMN IF NOT EXISTS world_name TEXT NOT NULL DEFAULT 'Aethelgrad Shared World';

ALTER TABLE coop_members
    ADD COLUMN IF NOT EXISTS is_active BOOLEAN NOT NULL DEFAULT true,
    ADD COLUMN IF NOT EXISTS item_state JSONB NOT NULL DEFAULT '{"wood":12,"fiber":8,"stone":4,"emberKit":false,"items":[]}'::jsonb,
    ADD COLUMN IF NOT EXISTS progression_state JSONB NOT NULL DEFAULT '{"level":1,"xp":0,"unlocked":[]}'::jsonb,
    ADD COLUMN IF NOT EXISTS member_revision BIGINT NOT NULL DEFAULT 0 CHECK (member_revision >= 0);

CREATE INDEX IF NOT EXISTS coop_members_active_idx ON coop_members(room_id, is_active, last_seen_at);

CREATE TABLE IF NOT EXISTS coop_world_saves (
    room_id UUID PRIMARY KEY REFERENCES coop_rooms(id) ON DELETE CASCADE,
    world_state JSONB NOT NULL DEFAULT '{"schemaVersion":1,"buildings":[],"claimedResources":[],"quests":{}}'::jsonb,
    save_revision BIGINT NOT NULL DEFAULT 0 CHECK (save_revision >= 0),
    updated_by UUID REFERENCES accounts(id) ON DELETE SET NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS coop_buildings (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    room_id UUID NOT NULL REFERENCES coop_rooms(id) ON DELETE CASCADE,
    building_type TEXT NOT NULL CHECK (building_type <> ''),
    placed_by UUID NOT NULL REFERENCES accounts(id) ON DELETE RESTRICT,
    transform JSONB NOT NULL,
    state JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS coop_buildings_room_idx ON coop_buildings(room_id, created_at);

INSERT INTO coop_world_saves (room_id, world_state, updated_by)
SELECT id, '{"schemaVersion":1,"buildings":[],"claimedResources":[],"quests":{}}'::jsonb, created_by
FROM coop_rooms
ON CONFLICT (room_id) DO NOTHING;

UPDATE coop_members
SET item_state = jsonb_build_object(
        'wood', wood,
        'fiber', fiber,
        'stone', stone,
        'emberKit', ember_kit,
        'items', '[]'::jsonb
    )
WHERE item_state = '{"wood":12,"fiber":8,"stone":4,"emberKit":false,"items":[]}'::jsonb
   OR item_state IS NULL;
