# AETHELGRAD Multiplayer Workflow

## Current playable workflow

AETHELGRAD uses an invite-only, four-player **persistent creator-owned world** flow for the first multiplayer slice. The Android client signs the player into the configured game session, lets the host create a named world with a six-character access code, and lets friends join with that code. The world remains in the database when players leave; the creator remains the owner, each member keeps a durable item/progression save, and the shared world keeps its state and placed buildings. The room response reports the region, maximum party size, owner, active participants, synchronized world time, tower revision, Warden health, and combat revision.

The flow is intentionally staged rather than pretending to be a complete public matchmaking service:

| Stage | Client behavior | Server authority |
|---|---|---|
| Account entry | Guest-first development entry or Google-backed session, depending on build/configuration. | Session token and account identity are validated by the backend. |
| Region selection | Player selects an available region before creating a room. | Server directory and room region are recorded. |
| Host room | Player creates an invite-only room and receives a six-character code. | Room is created with a maximum of four members and the host becomes the first member. |
| Friend join | Friends enter the code and join the room. | Active membership is checked and the fourth active slot is the hard limit. |
| Invite sharing | Host can share the room code through Android’s share sheet. | The code itself grants no authority; membership still requires authenticated server admission. |
| Presence | Client sends movement/tower state heartbeat approximately every two seconds. | Server bounds coordinates, refreshes presence, and returns the current room snapshot. |
| Reconnect | After a heartbeat failure, the client retries with bounded exponential backoff. | The reconnect endpoint refreshes the member’s presence only if the account is already a member. |
| Shared actions | Combat, gathering, and crafting requests carry request IDs. | Server checks range, cooldowns, inventory state, and duplicate request receipts before mutating state. |
| Leave | Player leaves explicitly or is no longer shown after the presence timeout. | The member is marked inactive; saved items, progression, and world membership remain recoverable. |
| Persistence | Client loads the member save and shared world save on entry and autosaves during play. | Revision-checked item/progression saves and creator-only world saves prevent silent overwrites. |

## Reconnect behavior

`POST /v1/coop/rooms/:code/reconnect` is an authenticated recovery operation. It does not let an arbitrary account enter a world. The server first normalizes the access code, confirms that the persistent world exists, confirms that the account already has a membership row, reactivates that membership, refreshes `last_seen_at`, and returns the same authoritative snapshot used by normal world polling. `GET /player-save` restores that member’s items and progression, while `GET /save` restores the creator-owned shared world and placed buildings.

The Android client attempts reconnect at most once at a time. Its delay grows from one second to a maximum of eight seconds. A successful snapshot resets the attempt counter. If the server reports that the room no longer exists or membership has expired, the client stops polling and returns to the co-op entry state rather than creating an uncontrolled duplicate session.

## Four-player cap

Room creation uses `max_players = 4`. Joining counts other active members whose `last_seen_at` is within the 20-second presence window. A fifth active join receives HTTP 409 with `room_full`. Stale disconnected members do not permanently block the room, but a reconnect still requires an existing membership row.

## Idempotent authoritative actions

Combat and inventory operations include a client-generated request ID. The server stores action receipts under `(room_id, account_id, request_id)`. If a mobile retry repeats the same request after a timeout, the stored result is returned instead of applying damage, gathering, or crafting twice. The Android client mirrors accepted boss health and inventory revisions into the local presentation only after the authoritative response arrives.

## CI failure diagnosis

The reported output showed that the asset-budget validator and all seven resource-center checks passed. The exit code came from a later shell source-contract command in the same production-foundation step, not from asset-budget overflow or missing resource content. The multiplayer regression now includes explicit checks for the reconnect endpoint, four-player cap, fifth-player rejection, and server-side action idempotency so future failures identify the multiplayer contract directly.

## Honest boundary

This is an authenticated persistent-world and authoritative-action workflow. It is not yet a dedicated Unreal realtime simulation server with replicated movement, server-side AI, network prediction, lag compensation, or public matchmaking. Those systems require the Unreal networking layer, a deployed dedicated-server build, backend admission integration, and device/network testing. The current workflow is appropriate for the first playable persistent co-op slice and keeps saved gameplay mutations on the server.

## References

[1]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android "AETHELGRAD repository"
[2]: https://github.com/darkvirgoyt-beep/anime-forest-survival-rpg-android/blob/main/server/README.md "AETHELGRAD online service overview"
