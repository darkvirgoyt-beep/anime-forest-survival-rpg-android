-- Aethelgard inventory and progression schema
-- Target: PostgreSQL 15+ for cloud authority.
-- The Android local store mirrors the same logical entities in SQLite.

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TYPE item_category AS ENUM ('weapon', 'armor', 'consumable', 'material', 'quest');
CREATE TYPE item_rarity AS ENUM ('common', 'uncommon', 'rare', 'epic', 'legendary');
CREATE TYPE inventory_mutation_status AS ENUM ('pending', 'committed', 'rejected', 'superseded');
CREATE TYPE quest_state AS ENUM ('locked', 'active', 'completed', 'claimed');

CREATE TABLE player_profiles (
    profile_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    account_id UUID NOT NULL,
    display_name VARCHAR(32) NOT NULL,
    schema_version INTEGER NOT NULL DEFAULT 1 CHECK (schema_version > 0),
    inventory_revision BIGINT NOT NULL DEFAULT 0 CHECK (inventory_revision >= 0),
    progression_revision BIGINT NOT NULL DEFAULT 0 CHECK (progression_revision >= 0),
    last_played_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE UNIQUE INDEX ux_player_profiles_account ON player_profiles(account_id);

CREATE TABLE item_definitions (
    definition_id VARCHAR(120) PRIMARY KEY,
    content_version INTEGER NOT NULL CHECK (content_version > 0),
    display_name_key VARCHAR(160) NOT NULL,
    description_key VARCHAR(160) NOT NULL,
    category item_category NOT NULL,
    sub_type VARCHAR(48) NOT NULL,
    rarity item_rarity NOT NULL,
    max_stack INTEGER NOT NULL DEFAULT 1 CHECK (max_stack > 0),
    icon_key VARCHAR(160) NOT NULL,
    equip_slot VARCHAR(32),
    required_level INTEGER NOT NULL DEFAULT 1 CHECK (required_level > 0),
    tags JSONB NOT NULL DEFAULT '[]'::jsonb,
    effects JSONB NOT NULL DEFAULT '[]'::jsonb,
    sell_value INTEGER NOT NULL DEFAULT 0 CHECK (sell_value >= 0),
    drop_policy VARCHAR(24) NOT NULL DEFAULT 'normal',
    enabled BOOLEAN NOT NULL DEFAULT true,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX ix_item_definitions_category ON item_definitions(category, rarity);

CREATE TABLE profile_progression (
    profile_id UUID PRIMARY KEY REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    level INTEGER NOT NULL DEFAULT 1 CHECK (level > 0),
    experience BIGINT NOT NULL DEFAULT 0 CHECK (experience >= 0),
    experience_to_next BIGINT NOT NULL DEFAULT 100 CHECK (experience_to_next > 0),
    active_quest_id VARCHAR(120),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE profile_currencies (
    profile_id UUID NOT NULL REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    currency_code VARCHAR(32) NOT NULL,
    amount BIGINT NOT NULL DEFAULT 0 CHECK (amount >= 0),
    revision BIGINT NOT NULL DEFAULT 0 CHECK (revision >= 0),
    PRIMARY KEY (profile_id, currency_code)
);

CREATE TABLE item_instances (
    instance_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    profile_id UUID NOT NULL REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    definition_id VARCHAR(120) NOT NULL REFERENCES item_definitions(definition_id),
    quantity INTEGER NOT NULL DEFAULT 1 CHECK (quantity > 0),
    durability NUMERIC(5,4) CHECK (durability IS NULL OR durability BETWEEN 0 AND 1),
    roll_seed BIGINT,
    favorite BOOLEAN NOT NULL DEFAULT false,
    locked BOOLEAN NOT NULL DEFAULT false,
    custom_data JSONB NOT NULL DEFAULT '{}'::jsonb,
    acquired_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX ix_item_instances_profile ON item_instances(profile_id, updated_at DESC);
CREATE INDEX ix_item_instances_definition ON item_instances(profile_id, definition_id);
CREATE UNIQUE INDEX ux_unique_equipment_instance_per_profile ON item_instances(profile_id, instance_id);

CREATE TABLE inventory_containers (
    container_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    profile_id UUID NOT NULL REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    container_type VARCHAR(24) NOT NULL DEFAULT 'backpack',
    capacity INTEGER NOT NULL DEFAULT 24 CHECK (capacity > 0),
    revision BIGINT NOT NULL DEFAULT 0 CHECK (revision >= 0),
    UNIQUE (profile_id, container_type)
);

CREATE TABLE inventory_slots (
    container_id UUID NOT NULL REFERENCES inventory_containers(container_id) ON DELETE CASCADE,
    slot_index INTEGER NOT NULL CHECK (slot_index >= 0),
    instance_id UUID REFERENCES item_instances(instance_id) ON DELETE SET NULL,
    PRIMARY KEY (container_id, slot_index),
    UNIQUE (container_id, instance_id)
);

CREATE INDEX ix_inventory_slots_instance ON inventory_slots(instance_id);

CREATE TABLE equipment_loadouts (
    loadout_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    profile_id UUID NOT NULL REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    loadout_name VARCHAR(32) NOT NULL,
    is_active BOOLEAN NOT NULL DEFAULT false,
    revision BIGINT NOT NULL DEFAULT 0 CHECK (revision >= 0),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (profile_id, loadout_name)
);

CREATE UNIQUE INDEX ux_one_active_loadout_per_profile
    ON equipment_loadouts(profile_id) WHERE is_active = true;

CREATE TABLE equipment_slots (
    loadout_id UUID NOT NULL REFERENCES equipment_loadouts(loadout_id) ON DELETE CASCADE,
    slot_code VARCHAR(32) NOT NULL,
    instance_id UUID REFERENCES item_instances(instance_id) ON DELETE SET NULL,
    PRIMARY KEY (loadout_id, slot_code),
    UNIQUE (loadout_id, instance_id)
);

CREATE INDEX ix_equipment_slots_instance ON equipment_slots(instance_id);

CREATE TABLE quest_definitions (
    quest_id VARCHAR(120) PRIMARY KEY,
    content_version INTEGER NOT NULL CHECK (content_version > 0),
    display_name_key VARCHAR(160) NOT NULL,
    description_key VARCHAR(160) NOT NULL,
    objectives JSONB NOT NULL DEFAULT '[]'::jsonb,
    reward_table JSONB NOT NULL DEFAULT '{}'::jsonb,
    enabled BOOLEAN NOT NULL DEFAULT true,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE quest_progress (
    profile_id UUID NOT NULL REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    quest_id VARCHAR(120) NOT NULL REFERENCES quest_definitions(quest_id),
    state quest_state NOT NULL DEFAULT 'locked',
    current_objective_index INTEGER NOT NULL DEFAULT 0 CHECK (current_objective_index >= 0),
    objective_state JSONB NOT NULL DEFAULT '{}'::jsonb,
    accepted_at TIMESTAMPTZ,
    completed_at TIMESTAMPTZ,
    claimed_at TIMESTAMPTZ,
    revision BIGINT NOT NULL DEFAULT 0 CHECK (revision >= 0),
    PRIMARY KEY (profile_id, quest_id)
);

CREATE INDEX ix_quest_progress_active ON quest_progress(profile_id, state);

CREATE TABLE progression_ledger (
    ledger_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    profile_id UUID NOT NULL REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    idempotency_key VARCHAR(160) NOT NULL,
    source_type VARCHAR(48) NOT NULL,
    source_id VARCHAR(120),
    experience_delta BIGINT NOT NULL DEFAULT 0,
    level_before INTEGER NOT NULL,
    level_after INTEGER NOT NULL,
    payload JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (profile_id, idempotency_key)
);

CREATE INDEX ix_progression_ledger_profile ON progression_ledger(profile_id, created_at DESC);

CREATE TABLE world_mutations (
    profile_id UUID NOT NULL REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    mutation_key VARCHAR(180) NOT NULL,
    mutation_type VARCHAR(48) NOT NULL,
    payload JSONB NOT NULL DEFAULT '{}'::jsonb,
    revision BIGINT NOT NULL DEFAULT 0 CHECK (revision >= 0),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (profile_id, mutation_key)
);

CREATE TABLE save_snapshots (
    snapshot_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    profile_id UUID NOT NULL REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    schema_version INTEGER NOT NULL CHECK (schema_version > 0),
    inventory_revision BIGINT NOT NULL CHECK (inventory_revision >= 0),
    progression_revision BIGINT NOT NULL CHECK (progression_revision >= 0),
    payload JSONB NOT NULL,
    checksum CHAR(64) NOT NULL,
    is_current BOOLEAN NOT NULL DEFAULT false,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX ix_save_snapshots_profile_time ON save_snapshots(profile_id, created_at DESC);
CREATE UNIQUE INDEX ux_current_save_snapshot ON save_snapshots(profile_id) WHERE is_current = true;

CREATE TABLE inventory_mutations (
    mutation_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    profile_id UUID NOT NULL REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    request_id VARCHAR(160) NOT NULL,
    operation VARCHAR(48) NOT NULL,
    base_inventory_revision BIGINT NOT NULL CHECK (base_inventory_revision >= 0),
    result_inventory_revision BIGINT,
    status inventory_mutation_status NOT NULL DEFAULT 'pending',
    request_payload JSONB NOT NULL,
    result_payload JSONB,
    error_code VARCHAR(64),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    committed_at TIMESTAMPTZ,
    UNIQUE (profile_id, request_id)
);

CREATE INDEX ix_inventory_mutations_profile_time ON inventory_mutations(profile_id, created_at DESC);

CREATE TABLE sync_cursors (
    profile_id UUID PRIMARY KEY REFERENCES player_profiles(profile_id) ON DELETE CASCADE,
    device_id VARCHAR(160) NOT NULL,
    last_uploaded_revision BIGINT NOT NULL DEFAULT 0 CHECK (last_uploaded_revision >= 0),
    last_downloaded_revision BIGINT NOT NULL DEFAULT 0 CHECK (last_downloaded_revision >= 0),
    last_sync_at TIMESTAMPTZ,
    conflict_count INTEGER NOT NULL DEFAULT 0 CHECK (conflict_count >= 0)
);

-- Transaction rule: an equip/unequip/use/split/drop request must lock the profile,
-- container, relevant item instances, and active loadout in one database transaction.
-- Validate every foreign key, requirement, capacity, lock flag, and revision before
-- writing. Increment the relevant revision exactly once on success, then append an
-- inventory_mutations row and an optional progression_ledger row with the same request_id.
