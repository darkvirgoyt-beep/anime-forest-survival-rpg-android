# Real online multiplayer architecture

The current Android Kotlin/C++ build is a playable prototype. It is not yet a BGMI- or ARK-scale online game because it does not include a production backend, dedicated game-server fleet, matchmaking, persistence, moderation, telemetry, or server-side Play Games credential verification. The launch screen should therefore describe the current build honestly and must not display a fake authenticated state.

## Required production flow

```text
Android client
  1. Play Games Services v2 authentication
  2. requestServerSideAccess() using a server OAuth web-client ID
  3. HTTPS POST of the single-use server auth code
        |
        v
Identity/session API
  4. Exchange code with Google and verify the player
  5. Map verified provider identity to an internal account
  6. Issue a short-lived game session token
        |
        +--> Account database and cloud-save service
        |
        +--> Matchmaking/session allocator
                    |
                    v
            Unreal dedicated game server
  7. Server validates the game token
  8. Server owns combat, inventory, crafting, creatures, survival,
     world mutations, party membership, and persistence writes
```

The Android client must never contain the Google web-client secret, service-account credentials, signing keys, or backend administrator keys. The server must not trust a player ID supplied by the client without verifying the server-side credential exchange.

## Architecture choices

| Approach | What it provides | Tradeoffs | Cost and setup |
|---|---|---|---|
| **Recommended: Unreal dedicated servers + identity/session API + database** | Authoritative real-time world simulation, server-owned combat and inventory, matchmaking, cloud saves, and a path to multi-region sessions. | Highest engineering and operations complexity; requires Unreal source build, server packaging, monitoring, and deployment. | Cloud hosting, database, logging, and bandwidth costs. Requires Play Console credentials, an HTTPS backend, and a server deployment account. |
| **Lighter prototype: one managed real-time server plus identity API** | Faster first online co-op slice with a small number of players and persistent account data. | Not suitable for a large persistent ARK-like world or competitive scale; must migrate or shard later. | Lower initial cost and setup. Still requires Play Console configuration, backend secrets, database, and a persistent host. |
| **Local development server** | Two or more clients can test replication and gameplay contracts on a local network. | Not publicly reachable, not a production service, and not a substitute for matchmaking or cloud persistence. | Lowest cost; requires a developer machine and Unreal dedicated-server build. |

A BGMI/ARK-style target should use the first approach. The Android prototype can remain as a harness while the Unreal tree becomes the production client and dedicated-server path; rewriting the prototype into a fake online layer would create security and maintenance problems.

## Identity and account requirements

The Android client uses Play Games Services v2 for platform authentication. After authentication, it must call `requestServerSideAccess()` with the **web OAuth client ID created for the game server**, not the Android OAuth client ID. The backend exchanges the single-use code with Google, verifies the returned identity, maps it to an internal account ID, and issues a game session token.

The internal account ID—not an email address and not an unverified client-provided player ID—should be the stable key for cloud saves, entitlements, characters, bans, party membership, and audit logs. Logout, token expiry, account deletion requests, privacy consent, and account recovery must be explicit product flows.

## Dedicated-server ownership

The dedicated server is headless and authoritative. Clients send validated intent such as movement input, attack requests, gather requests, and interaction requests. The server decides the resulting position, hit, damage, loot, crafting mutation, quest progress, survival transition, and save event. Clients receive replicated state and presentation events. A client must never be allowed to grant itself items, currency, experience, health, quest completion, or administrative permissions.

For the first real online milestone, implement one forest region, a four-player invite-only session, account login, server allocation, reconnect handling, a version handshake, server-side character state, a versioned save header, and a clean disconnect path. Add matchmaking, multi-region placement, anti-cheat signals, moderation, replay/telemetry, and elastic fleet scaling after the first authoritative slice is stable.

## Current repository boundary

| Area | Current status | Required before production |
|---|---|---|
| Android Play Games client | SDK integration and launch-state handling are implemented. | Play Console project ID, package credential, signing fingerprints, tester accounts, and device testing. |
| Server-side auth exchange | Not implemented. | HTTPS API, server OAuth web-client credential, code exchange, identity verification, token issuance, secret storage, rate limits, and logs. |
| Matchmaking/session allocation | Contract only. | Queue, party permissions, region selection, allocation, health checks, and reconnect handling. |
| Unreal dedicated server | Source foundation exists; no packaged production server is delivered. | UE source build, server target, cooked content, deployment image, health endpoint, rollout and rollback process. |
| Persistence | Contract and local data structures only. | Database schema, transactional save service, conflict policy, backups, migrations, and account deletion. |

## Official references

1. [Platform authentication for Android games](https://developer.android.com/games/pgs/android/android-signin)
2. [Server-side access to Google Play Games Services](https://developer.android.com/games/pgs/android/server-access)
3. [Setting Up Dedicated Servers in Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/setting-up-dedicated-servers-in-unreal-engine?lang=en-US)
