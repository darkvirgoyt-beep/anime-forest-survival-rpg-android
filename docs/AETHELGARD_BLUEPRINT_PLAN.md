# Aethelgrad: Wild Horizons – Crafting

## Blueprint 01: account to first world

This blueprint defines the first playable product path. It deliberately separates screen flow, server authority, cloud persistence, and world gameplay so each boundary can be implemented and tested independently.

```text
BOOT
  ↓
TITLE / LEGAL / VERSION CHECK
  ↓
ACCOUNT GATE ───────────────→ AUTHENTICATED PROFILE
          │
     GOOGLE PLAY BRIDGE
          ↓
     BACKEND SESSION EXCHANGE
          ↓
      SERVER DIRECTORY
 region • status • capacity • live ping
          ↓
       CLOUD SAVE PREFLIGHT
 authenticated account • newest revision • conflict policy
          ↓
       CO-OP SESSION GATE
 invite-only party • version • capacity • reconnect
          ↓
       SELECT / CREATE WORLD
              seed • privacy

                         ↓
               CHARACTER CREATION
     eyebrow • hair • outfit • name validation
                         ↓
                 WORLD BOOTSTRAP
    forest spawn • camp • cave entrance • map
                         ↓
              MISSION / NAVIGATION HUD
    objective • marker • distance • discovery
                         ↓
       GATHER → CRAFT → BUILD → SURVIVE
                         ↓
           CREATURE OBSERVE / BOND / TAME
                         ↓
          CO-OP PARTY / BOSS / WORLD SAVE
```

## Screen and authority map

| Screen or state | Player-facing responsibilities | Server/backend responsibility | Current foundation |
|---|---|---|---|
| Boot | Load version, legal text, local settings, asset-pack status | Version compatibility and maintenance status | Android activity and Unreal project descriptor |
| Account | Choose Google for cloud/co-op or Guest for local worlds; show session status and scope | Exchange short-lived provider credential for Google backend session; no backend call for Guest | `AccountSessionManager`, `UForestSliceAccountSubsystem` boundary; Android exposes both explicit paths with separate authority scopes |
| Server directory | Show region, status, capacity, measured ping, selected row | Return signed directory and perform real health/ping checks | `ServerDirectory.kt`, `UForestSliceServerDirectorySubsystem` |
| World lobby | Google: create private world, invite friends, join by code, set privacy. Guest: enter the local world directly; hosted lobby is unavailable | Allocate/destroy authoritative session and verify party permissions for Google accounts | `UForestSliceWorldSessionSubsystem` contract; local guest world uses the native versioned snapshot |
| Cloud save | Show last saved version, conflict warning, retry state | Versioned save, checksum, conflict resolution, migration, backup | Planned cloud-save service boundary |
| Character creation | Choose original style parameters and valid name | Validate name, reserve identity, replicate profile | `CharacterCreationState`, `UForestSliceCharacterProfileComponent` |
| World bootstrap | Spawn at safe camp, load nearby chunk, display quest | Own seed, spawn, mutations, quest state, inventory | Procedural forest and gameplay component foundations |
| Mission HUD | Track current objective and navigate to marker | Authoritative quest state and completion event | Planned mission/navigation subsystem |
| Cave | Enter/exit, light, gather, encounter, loot | Stream cave cell, own hazards, loot claims, save mutations | Planned cave contract |
| Creature bonding | Observe, approach, feed, bond, command, mount when eligible | Validate distance, item use, bond progression, ownership | `UForestSliceCreatureComponent` |
| World save | Save after meaningful mutation and on safe checkpoints | Serialize world/player/party state atomically | Planned versioned save subsystem |

## Data ownership rules

The client owns presentation state, local input, camera feel, menu navigation, and cached read-only directory data. The server owns damage, health, stamina outcomes, inventory mutation, crafting, building, creature bonding, AI, loot claims, missions, world mutations, sleep/time, and co-op permissions. The backend owns account exchange, cloud-save storage, conflict resolution, entitlements, telemetry, and service health. Provider secrets and Play signing credentials never ship in the APK.

## First account/server/character milestone

The dual-entry release milestone is complete when a player can launch the Android build and choose either Google or Guest. Google sign-in goes through the platform bridge, receives a backend session, shows live server health and measured latency, passes cloud-save preflight, and can join or create an invite-only co-op session. Guest entry makes a local world available without Gmail or server login and preserves the same character, exploration, combat, gathering, crafting, companion, and camp gameplay path, while hosted multiplayer and cloud worlds remain disabled. Both paths enter character setup and the Google button must never fake an authenticated session.

## First world milestone

The first world is intentionally narrow: one authored forest clearing, one safe camp, one bed, one build frame, one harvestable node, one cave entrance, one passive creature, one hostile creature, one mission chain, and one saveable mutation. The map and mission system should guide the player without requiring a complete continent. The cave is a streamed interior cell with a separate lighting, audio, navigation, loot, and encounter budget.

## Co-op milestone order

Co-op begins as invite-only four-player sessions on a dedicated authoritative server. Google players create or select a world; invited players join after version and entitlement checks. Guest players remain single-player and see an explicit hosted-multiplayer-unavailable state rather than a fake room or LAN fallback. The server owns the shared seed, party membership, world mutations, loot claims, quest progression policy, revive rules, and sleep-time policy. Reconnect and conflict-safe save are mandatory before public matchmaking.

## Cloud-save contract

Every save has a `schemaVersion`, `worldId`, `playerId`, `revision`, `updatedAt`, `contentHash`, and separate player/world mutation sections. Google saves upload idempotently through the authenticated backend, and the server rejects stale revisions unless the conflict resolver can merge disjoint mutations. Guest saves use the same versioned native snapshot format in app-private local storage and never silently merge into a Google account. Save migration is tested before each release; switching from Guest to Google is an explicit account path, not an automatic conversion.

## Taming and creature contract

Creatures are not copied from any reference title. Each species defines a role, perception radius, food preferences, fear/aggression rules, bond threshold, command set, mount eligibility, health profile, animation set, and biome habitat. The first implementation uses a server-authoritative bond-progress contract. Final taming minigames, creature AI, rigs, locomotion, saddles, and animation assets are separate milestones.

## Blueprint acceptance gates

The flow must have no hidden provider credential, no client-authoritative world mutation, no fake ping or online status, no unvalidated character name, no save overwrite without revision checking, and no UI that exposes a feature as live before its backend and device tests pass. Each milestone produces a public commit, host-test result, CI result, and explicit list of unavailable Unreal/device dependencies.
