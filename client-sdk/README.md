# AETHELGRAD Client Networking Wrappers

These wrappers expose the current AETHELGRAD v1 service contract to non-Android clients. The Unity wrapper is C# and uses `UnityWebRequest`; the Godot wrapper is TypeScript and uses the runtime `fetch` API exposed by Godot TypeScript integrations.

## Files

| Client | File | Runtime assumption |
| --- | --- | --- |
| Unity | `unity/AethelgardCoopClient.cs` | Unity 2021+ with `UnityEngine.Networking` and a C# async/task-compatible runtime. |
| Godot | `godot/AethelgardCoopClient.ts` | Godot TypeScript integration with standard `fetch`; persist tokens using platform-secure storage. |

## Integration sequence

1. Construct the client with the deployed service base URL, without a trailing slash.
2. Exchange a Google ID token or Play Games server authorization code through the service and persist the returned token bundle securely.
3. Create or join a co-op room, then send a heartbeat approximately every two seconds while the scene is active.
4. Apply `worldTime`, `towerRevision`, `bossHealth`, `combatRevision`, and participant positions to the local simulation.
5. Send combat and inventory requests with a stable `requestId`. If a network retry is required, reuse the same request ID so the server receipt prevents duplicate outcomes.
6. Refresh a 401 session once. If refresh fails, clear tokens and return the player to authentication.

The current server validates room membership, combat target/range/cooldown, resource proximity, crafting costs, and action idempotency. It does not yet provide authoritative movement-frame validation, projectile simulation, lag compensation, or a full dedicated-server replication channel. See `docs/aethelgard_api_client_integration.md` for the complete contract.

## Unity example

```csharp
var client = new Aethelgard.Net.AethelgardCoopClient("https://api.example.com");
var room = await client.CreateRoomAsync("asia");
var combat = await client.CombatAsync(room.room.code, "attack");
Debug.Log($"Warden health: {combat.bossHealth}");
```

## Godot TypeScript example

```typescript
const client = new AethelgardCoopClient({ baseUrl: "https://api.example.com" });
const room = await client.createRoom("asia");
const combat = await client.combat(room.room.code, "attack");
console.log(`Warden health: ${combat.bossHealth}`);
```

Do not ship Google client secrets, database credentials, or refresh tokens in logs. Use HTTPS in production and keep the service origin aligned with the server’s allowed-origin configuration.
