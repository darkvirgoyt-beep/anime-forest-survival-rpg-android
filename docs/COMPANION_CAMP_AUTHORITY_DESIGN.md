# AETHELGRAD Companion and Camp Authority Design

## Objective

Move companion capture/commands and camp building from the local Android slice into the existing authenticated Express/PostgreSQL service without breaking guest login, co-op rooms, or cloud-save revision behavior. The server owns every shared outcome. The client may animate a predicted request, but it must apply the server response as the source of truth.

## State ownership

| State | Owner | Client behavior |
|---|---|---|
| Companion species, capture status, bond, health fraction, command, revision | Server | Display approved state; do not award capture or assist damage locally while online. |
| Camp identity, owner, recipe, transform, condition, revision | Server | Show placement preview locally, submit a bounded request, then render only the committed building. |
| Inventory materials and deductions | Server | Send an action with a stable request ID; never send client totals as authority. |
| Camera, touch input, animation prediction, foliage, particles, local interpolation | Device | Remain responsive and cosmetic. |
| Room membership, shared world clock, world save revision, receipts | Server | Reconnect with the same request IDs and reconcile revisions after a conflict. |

## Migration 007

`coop_companions` stores at most one active companion per account in a room. It uses `(room_id, account_id)` as the key, a stable `creature_id`, a server-generated `companion_id`, a command enum, bond, health fraction, and a monotonically increasing revision. `coop_camps` stores one account-owned field camp per room, with a server-generated ID, bounded transform JSON, a recipe ID, condition, and revision. Existing rooms receive no companion or camp automatically; all new fields have safe defaults.

## Endpoints

| Method | Route | Purpose |
|---|---|---|
| `GET` | `/v1/coop/rooms/:code/companions` | Return the caller’s committed companion and current camp state. |
| `POST` | `/v1/coop/rooms/:code/companions/capture` | Capture a known server-side creature target using range, health, tameability, Fiber, and membership checks. |
| `POST` | `/v1/coop/rooms/:code/companions/command` | Toggle or set `follow`/`stay`; return the new companion revision. |
| `POST` | `/v1/coop/rooms/:code/camps` | Place one camp after validating recipe, transform, range, slope, occupancy, materials, and request receipt. |
| `DELETE` | `/v1/coop/rooms/:code/camps/:campId` | Remove an owner’s camp using an idempotent request ID and return the committed revision. |

Every mutation requires a request ID matching the existing receipt format. A duplicate request returns the original JSON result without applying the mutation again. A stale expected revision returns `409` with the current state and revision so the client can reload rather than overwrite another player’s outcome.

## Server validation

Capture accepts only a server-known target ID, a tameable creature profile, a player who is an active room member, a player position within the configured capture radius, a target health fraction at or below `0.38`, and enough server-held Fiber for the target’s profile cost. The server chooses the resulting companion ID, display name, bond, health, and revision.

Camp placement accepts only the allow-listed recipe `field_camp` in this slice. The server clamps and validates `x`, `y`, `z`, yaw, and scale, checks the player’s server-known position and placement distance, limits slope and overlap, enforces one active camp per account per room, and subtracts 6 Wood plus 4 Fiber inside the same transaction. The server never trusts client-supplied material totals, health values, or world coordinates outside its bounded map contract.

## Revision and retry behavior

The companion revision and camp revision increment only after a successful transaction. The action receipt is written in the same transaction as the state mutation. Network failures may be retried with the same request ID. Validation failures are not retried unchanged. A `401` refreshes the session once through the existing account manager; a `409` reloads the committed state; a `429` respects the server cooldown if one is added later.

## Migration path to real-time multiplayer

The HTTPS implementation is appropriate for low-frequency shared actions such as capture, commands, and construction. It is not yet a full authoritative movement/combat server. A future WebSocket or dedicated game-server layer can subscribe to these same committed companion/camp revisions and request IDs without changing the persistence model.

## Integration examples

The client sends a stable request ID for every mutation and never sends client-owned health, inventory totals, or target damage. A capture request is:

```http
POST /v1/coop/rooms/ABC123/companions/capture
Authorization: Bearer <access-token>
Content-Type: application/json

{"requestId":"capture-20260824-0001","creatureId":"moon_deer"}
```

A successful response contains the server-created companion, the deducted inventory, and the revisions that the Android client must apply:

```json
{
  "accepted": true,
  "companion": {
    "companion_id": "server-uuid",
    "creature_id": "moon_deer",
    "display_name": "Moon Deer",
    "command": "follow",
    "bond": 0,
    "health_fraction": 0.75,
    "revision": 1
  },
  "inventory": {"wood": 12, "fiber": 6, "stone": 4, "emberKit": false},
  "inventoryRevision": 1,
  "memberRevision": 1,
  "targetRevision": 1
}
```

Companion commands require the last committed companion revision:

```json
{"requestId":"command-20260824-0002","command":"stay","expectedRevision":1}
```

A field-camp placement starts at expected revision `0` when no camp exists. The server accepts only `field_camp`, deducts **6 Wood and 4 Fiber**, and returns the bounded transform it stored:

```json
{
  "requestId":"camp-20260824-0003",
  "recipeId":"field_camp",
  "expectedRevision":0,
  "transform":{"x":-0.62,"y":0.42,"z":0,"yaw":0,"scale":1}
}
```

A stale mutation returns `409` and includes the committed state. For example, a stale command returns `{"error":"companion_revision_conflict","companion":{...}}`; a stale camp placement or removal returns `{"error":"camp_revision_conflict","camp":{...}}`. The client reloads `GET /v1/coop/rooms/:code/companions` and retries with a new request ID and the returned revision. Retrying a request with the same request ID returns the original receipt result without deducting materials twice.

## Deployment order

Deploy migration `007_companion_camp_authority.sql` after the existing co-op persistence migrations, especially `006_persistent_coop_worlds.sql`, and complete it successfully before activating the new routes. The migration creates empty companion/camp state for existing rooms, seeds the four server-known creature targets using the native world coordinates, and expands the receipt action-type constraint. New room creation also seeds those targets, so rooms created after deployment are covered without a second backfill.

The older `POST /v1/coop/rooms/:code/buildings` endpoint remains a legacy generic persistence route for existing world-save compatibility. It is **not** used by field-camp placement and does not provide the recipe, material, receipt, or revision guarantees described here. Production client code must use `/camps`; the generic endpoint should be deprecated or separately hardened before it is presented as an authoritative construction API.

The current camp route passes a deterministic slope value of `0` because the Express service has no terrain sampler in this slice. Transform bounds, server-known player position, placement distance, and the slope contract are enforced, but full terrain collision/occupancy is deferred to a future dedicated world simulation layer. This HTTPS state layer authorizes low-frequency capture, commands, and construction; it does not claim authoritative real-time movement or combat.

## Stable world identifiers

The server-known target positions mirror the original Android creature profiles: Moon Deer `(-0.62, 0.42)`, Mossback Boar `(-0.28, 0.40)`, River Otter `(0.42, -0.34)`, and Canopy Fox `(0.64, 0.26)`. These are AETHELGRAD-specific gameplay coordinates, not copied commercial-game assets, names, maps, UI, or code.
