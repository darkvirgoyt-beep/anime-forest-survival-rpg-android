-- Guest identities are anonymous but resumable: the API stores only a SHA-256 hash
-- of the client-held guest key in accounts.provider_player_id.
CREATE INDEX IF NOT EXISTS accounts_guest_provider_lookup_idx
    ON accounts(provider, provider_player_id)
    WHERE provider = 'guest';

COMMENT ON COLUMN accounts.provider_player_id IS
    'Stable provider subject; for provider=guest this is a SHA-256 hash of the client-held guest key.';

COMMENT ON COLUMN accounts.display_name IS
    'Server-controlled display name; guest accounts start as Guest Wayfarer and may later be profiled.';


-- End of migration 005.
