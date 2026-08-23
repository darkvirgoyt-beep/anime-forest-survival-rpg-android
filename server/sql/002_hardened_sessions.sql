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
