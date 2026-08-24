# Rootback Grazer and Duskmaw Prowler Blueprint implementation guide

This guide creates the first two original living creatures for **Aethelgard: Wild Horizons – Crafting**. It assumes Unreal Engine 5.6+, the repository's `AForestSliceWildCreature` base class, and `AForestSliceBiomeSpawnDirector` have already been compiled in an Unreal-capable environment. The base class is now an `ACharacter`, so child Blueprints inherit a capsule, skeletal-mesh slot, character movement component, navigation-capable movement, replicated movement, and automatic AI possession for placed or spawned instances.

> The spawning director owns *where and when* a creature exists. The creature Blueprint owns presentation and configured behavior. The authoritative game/session path owns damage, drops, capture approval, progression, and saved state.

## 1. Prerequisites and safe content folders

Create the following project-local content structure. All meshes, textures, rigs, animation clips, effects, and sounds must be original work or appropriately licensed; record the source, creator, license, import settings, LOD policy, Android target memory, and replacement plan in `Unreal/ASSETS.md` before a player-facing build.

| Folder | Assets to create | Purpose |
|---|---|---|
| `Content/Aethelgard/Creatures/RootbackGrazer` | `BP_RootbackGrazer`, `BP_RootbackGrazerAI`, `BB_RootbackGrazer`, `BT_RootbackGrazer`, optional idle/walk/flee animations | Herd animal, wildlife role, skittish behavior |
| `Content/Aethelgard/Creatures/DuskmawProwler` | `BP_DuskmawProwler`, `BP_DuskmawProwlerAI`, `BB_DuskmawProwler`, `BT_DuskmawProwler`, optional alert/run/attack animations | Hostile predator, server-approved combat hook |
| `Content/Aethelgard/Creatures/Common` | shared collision profiles, material instances, sound cues, optional Behavior Tree tasks/services | Prevent duplicated settings across creatures |

Create a `NavMeshBoundsVolume` covering the Verdant Veil test level and press **P** in the editor to confirm navigable ground is visible. The spawn director already ground-traces against world-static geometry, but AI movement also requires a valid navmesh. Behavior Trees evaluate logic through Blackboard keys, while AI Perception can provide sensory data that changes those keys.[1] [2]

## 2. Shared Blueprint rules

Create both creature Blueprints by right-clicking `AForestSliceWildCreature` and selecting **Create Blueprint Class Based on…**. Do **not** call `InitializeFromSpawn` from a Blueprint: the authoritative biome spawn director calls it after validating location and budget. `OnSpawnStateReady` is the Blueprint event that receives the replicated state on the server and on clients.

| Rule | Required implementation |
|---|---|
| Spawn authority | Only `AForestSliceBiomeSpawnDirector` creates creatures. Do not place gameplay spawners in a Level Blueprint or client widget. |
| Decision authority | AI controller and Behavior Tree decision branches execute on the server. Clients consume replicated movement/state and play cosmetic animation/audio only. |
| Damage and drops | A Behavior Tree can request a C++/server combat action; it must not subtract health, grant loot, or persist progression locally in Blueprint. |
| State guard | At the start of gameplay logic in `OnSpawnStateReady`, use `Switch Has Authority`. The **Authority** pin may start/refresh AI state; the **Remote** pin may update animation or cosmetic state only. |
| Player filtering | Give player characters a project-defined gameplay tag or Actor Tag such as `Aethelgard.Player`. Perception controllers must reject other creatures, props, and friendlies before setting a target. |

In both creatures, configure the inherited **Capsule Component** for `Pawn` collision and the inherited **Mesh** component with an original skeletal mesh, correct Anim Class, and a shadow/collision setup appropriate to the asset. Keep **Replicates** and **Replicate Movement** enabled on the class defaults. Unreal replication synchronizes state and procedure calls between clients and the server; gameplay mutations must therefore originate on authority.[3]

## 3. BP_RootbackGrazer — skittish herd animal

Create `BP_RootbackGrazer` from `AForestSliceWildCreature`. Rootback is a non-hostile wildlife animal for the Verdant Veil. Its purpose in the first playable slice is to make the forest feel alive, react to nearby danger, and demonstrate stable herd spawning without becoming a combat or capture system implementation.

### Class defaults and components

| Setting | Value or intent |
|---|---|
| Blueprint parent | `AForestSliceWildCreature` |
| Creature state expected from spawn profile | `SpeciesId=RootbackGrazer`, `Disposition=Skittish`, `Role=Wildlife`, `bCaptureCandidate=false`, `bElite=false` |
| Movement | Walking movement mode; set speed values from a data asset or exposed variables rather than hard-coding level logic. Begin conservatively: a slow roam speed and a visibly higher flee speed. |
| Mesh and collision | Original/licensed quadruped mesh, capsule sized to the actual model, blocking world static/navigation, no client-only collision rules. |
| AI controller class | `BP_RootbackGrazerAI` |
| Auto Possess AI | Inherited **Placed in World or Spawned**; leave it enabled. |

Add these Blueprint variables to the creature: `HomeAnchor` (`Vector`), `RoamRadius` (`Float`, editor exposed), `FleeDistance` (`Float`, editor exposed), and `bIsFleeing` (`Boolean`, read-only to AnimBP). Do not treat these as cloud-save values. The spawn director gives the actor its valid spawn transform; `HomeAnchor` becomes the local, server-owned return point for the current loaded creature instance.

### Rootback initialization graph

Implement the event graph as this sequence.

1. Add **Event On Spawn State Ready** and break the incoming `FForestSliceWildCreatureState`.
2. Verify `SpeciesId` equals `RootbackGrazer`. On a mismatch, emit a development-only warning and return; this catches a wrong spawn profile/Blueprint pairing.
3. Use **Switch Has Authority**. On **Authority**, set `HomeAnchor` from **Get Actor Location**, clear `bIsFleeing`, and obtain the controller as `BP_RootbackGrazerAI`.
4. Call a Blueprint-interface or custom controller event such as `InitializeRootback(HomeAnchor, RoamRadius, FleeDistance)`. Keep this event idempotent because a replicated spawn state can be observed more than once.
5. On **Remote**, update only animation-facing booleans or locally audible cosmetic cues. Do not call `Move To`, set Blackboard target keys, or create gameplay effects from this path.

### Rootback AI controller, Blackboard, and Behavior Tree

Create `BP_RootbackGrazerAI` from `AIController`, add **AIPerception**, and configure `AIHearing` plus `AIDamage` first. Sight is optional for the first slice; hearing/damage is enough for a readable flee response without creating a predator detector. AI Perception broadcasts target-perception updates that can update the variables used by Behavior Tree branches.[2]

Create `BB_RootbackGrazer` with the following keys.

| Blackboard key | Type | Owned by | Meaning |
|---|---|---|---|
| `HomeAnchor` | Vector | Server controller | Spawn-time return location for this loaded creature instance |
| `ThreatActor` | Object / Actor | Server controller | Valid player or hostile source that caused a current threat |
| `LastThreatLocation` | Vector | Server controller | Fallback location used to calculate the flee destination |
| `IsThreatened` | Bool | Server controller | Gates the high-priority flee branch |
| `RoamPoint` | Vector | Server service/task | Next reachable local wander location |

In `BP_RootbackGrazerAI`, use **Event On Possess** to cast the pawn to `BP_RootbackGrazer`, run `BT_RootbackGrazer`, and write `HomeAnchor` to the Blackboard. Bind **On Target Perception Updated**. For a successful stimulus, only accept a player-tagged actor or a confirmed hostile source; set `ThreatActor`, `LastThreatLocation`, and `IsThreatened=true`. For an expired/unsensed stimulus, do not immediately clear the threat—use a short server-side Behavior Tree cooldown, then clear the actor and set `IsThreatened=false`.

Set the `BT_RootbackGrazer` root selector in this priority order:

1. **Flee sequence**: decorator `IsThreatened=true`; calculate a reachable point away from `LastThreatLocation`; `Move To` that point; set `bIsFleeing=true`; wait briefly.
2. **Recover sequence**: after the threat cooldown, move toward a random reachable point around `HomeAnchor`; set `bIsFleeing=false`.
3. **Roam sequence**: choose a random reachable point inside `RoamRadius`; `Move To`; wait for a short random period; repeat.

Use **Move To** against Blackboard vectors and calculate reachable positions through Navigation System nodes or a small original Blueprint task. Avoid per-frame `Event Tick` roaming logic. Behavior Trees are designed to select behavior branches using Blackboard-held values, including simple flee-versus-roam decisions.[1]

## 4. BP_DuskmawProwler — hostile predator

Create `BP_DuskmawProwler` from `AForestSliceWildCreature`. Duskmaw is an original hostile predator for Verdant Veil and Emberfall Hollow. Its first implementation should pursue and face a valid player target, then ask the authoritative combat system to resolve an attack. It must not become a fully finished boss, loot, or stealth system in this step.

### Class defaults and components

| Setting | Value or intent |
|---|---|
| Blueprint parent | `AForestSliceWildCreature` |
| Creature state expected from spawn profile | `SpeciesId=DuskmawProwler`, `Disposition=Hostile`, `Role=Predator`, `bCaptureCandidate=false`, `bElite=false` |
| Movement | Walking/running values exposed as balancing variables; do not replicate speed independently every frame. |
| Mesh and collision | Original/licensed predator mesh, capsule tuned from actual dimensions, melee socket only after an original attack animation is available. |
| AI controller class | `BP_DuskmawProwlerAI` |
| Auto Possess AI | Inherited **Placed in World or Spawned**; leave it enabled. |

Add `HomeAnchor` (`Vector`), `CombatTarget` (`Actor` reference), `AttackRange` (`Float`, editor exposed), `ChaseLeashDistance` (`Float`, editor exposed), `bIsAlert` (`Boolean`), and `bIsAttacking` (`Boolean`). The two booleans may drive animation presentation on clients; target acquisition and attack requests remain server decisions.

### Duskmaw initialization graph

Use **Event On Spawn State Ready**, verify `SpeciesId=DuskmawProwler`, and branch via **Switch Has Authority**. On authority, set `HomeAnchor`, clear `CombatTarget`, clear alert/attack presentation state, and call `InitializeProwler(HomeAnchor, AttackRange, ChaseLeashDistance)` on `BP_DuskmawProwlerAI`. On remote, update only animation-state variables that were already replicated or derived from replicated movement/state.

### Duskmaw AI controller, Blackboard, and Behavior Tree

Create `BP_DuskmawProwlerAI` from `AIController`, then add **AIPerception** with `AISight`, `AIHearing`, and `AIDamage`. Configure ranges from exposed balancing variables and use **Detect Neutrals** plus the `Aethelgard.Player` tag filter in Blueprint; Blueprint affiliation settings are limited, so tag filtering keeps the first slice explicit.[2]

Create `BB_DuskmawProwler` with the following keys.

| Blackboard key | Type | Meaning |
|---|---|---|
| `HomeAnchor` | Vector | Spawn-time home point for current loaded instance |
| `TargetActor` | Object / Actor | Valid visible or recently perceived player target |
| `LastKnownTargetLocation` | Vector | Investigation/chase fallback if the target is lost |
| `HasTarget` | Bool | Enables pursuit/attack branches |
| `InAttackRange` | Bool | Enables an authoritative attack request branch |
| `ReturnHome` | Bool | Indicates leash breach or lost target recovery |

In `BP_DuskmawProwlerAI`, **On Possess** runs `BT_DuskmawProwler`. **On Target Perception Updated** accepts only a player-tagged, alive target. A successful sight/damage/hearing stimulus sets `TargetActor`, `LastKnownTargetLocation`, and `HasTarget=true`; loss of sight preserves the last known location for a brief investigation timeout. If target distance exceeds `ChaseLeashDistance`, clear `TargetActor`, set `ReturnHome=true`, and use the recovery branch. The controller should never select targets or call move commands on a client.

Set the `BT_DuskmawProwler` root selector in this priority order:

1. **Attack sequence**: `HasTarget=true` and `InAttackRange=true`; stop movement; face target; invoke a Blueprint event such as `RequestAuthoritativeProwlerAttack(TargetActor)`; wait for the replicated combat result or cooldown.
2. **Chase sequence**: `HasTarget=true`; `Move To` `TargetActor` with an acceptance radius smaller than `AttackRange`; update alert presentation.
3. **Investigate sequence**: recent `LastKnownTargetLocation`; `Move To` it; wait; clear the temporary investigation state.
4. **Return sequence**: `ReturnHome=true`; `Move To` `HomeAnchor`; clear alert/return state on arrival.
5. **Idle patrol sequence**: select reachable points near `HomeAnchor`; move and wait.

`RequestAuthoritativeProwlerAttack` is a deliberately named integration boundary, not a completed damage implementation. Connect it later to the existing C++ combat/health path so that hit validation, health changes, rewards, and effects are resolved once by authority and replicated as compact outcomes. Do not use an Anim Notify alone to subtract player health in a client Blueprint.

## 5. Configure the biome spawn director

In `L_VerdantVeil`, place one `AForestSliceBiomeSpawnDirector`. Add the two profiles below. Blueprint classes are assigned in the Details panel after they exist; the values are proposed first-slice defaults and must be tuned in real playtests.

| Field | Rootback Grazer profile | Duskmaw Prowler profile |
|---|---|---|
| `SpeciesId` | `RootbackGrazer` | `DuskmawProwler` |
| `CreatureClass` | `BP_RootbackGrazer` | `BP_DuskmawProwler` |
| `AllowedBiomes` | `VerdantVeil` | `VerdantVeil`, `EmberfallHollow` |
| `Disposition` | `Skittish` | `Hostile` |
| `Role` | `Wildlife` | `Predator` |
| `bCaptureCandidate` | `false` for this first slice | `false` |
| `bElite` | `false` | `false` |
| `GroupSize` | `3` | `1` |
| `Weight` | `3.0` | `0.8` |

The authoritative game mode/session must keep `SetAuthoritativePlayerAnchors` current for real server-owned player positions and call `RefreshSpawnBudget` when time, weather, or graphics-quality policy changes. The director uses the existing encounter budget, respects its active creature limit, and keeps player-distance-safe deterministic candidates. Do not create a second Blueprint timer or local random spawning path.

## 6. Validation checklist

| Test | Expected result |
|---|---|
| Dedicated/listen server with one client | Server owns spawned creatures and AI; client sees replicated movement/state without duplicate local spawns. |
| Four-player co-op test | Active creature count never exceeds the encounter budget; Rootback packs are bounded and Prowlers remain single-member profiles. |
| Rootback threat test | Valid threat stimulus sets a server Blackboard key; grazer flees, then returns to roaming after its cooldown. |
| Prowler perception test | Player-tagged target is chased; an untagged prop or wildlife actor is ignored. |
| Lost target and leash test | Prowler investigates last known location briefly, then returns home after target loss/leash breach. |
| Client attack test | Client cannot directly inflict damage or grant drops by invoking a Blueprint event. |
| Navigation test | Both creatures stay on navmesh and do not continuously issue movement requests when no valid path exists. |
| Android profiling test | Measure creature count, animation cost, draw calls, memory, thermals, and frame pacing on the actual reference devices before raising weights or budgets. |

The repository has no Unreal Engine 5.6+ compiler/editor runner in this environment. Therefore, this guide and the base-class update were statically validated only; Behavior Tree execution, navmesh movement, replication, assets, animations, combat integration, and Android performance must be verified in Unreal on a UE-capable machine.

## References

[1] [Epic Games, *Behavior Trees in Unreal Engine*](https://dev.epicgames.com/documentation/unreal-engine/behavior-trees-in-unreal-engine?lang=en-US)

[2] [Epic Games, *AI Perception in Unreal Engine*](https://dev.epicgames.com/documentation/unreal-engine/ai-perception-in-unreal-engine?lang=en-US)

[3] [Epic Games, *Networking and Multiplayer in Unreal Engine*](https://dev.epicgames.com/documentation/unreal-engine/networking-and-multiplayer-in-unreal-engine?lang=en-US)
