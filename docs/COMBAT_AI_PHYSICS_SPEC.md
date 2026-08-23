# Combat, Durability, Enemy AI, and Physics Specification

This is an original systems design for **Aethelgard: Wild Horizons**. It is inspired by broad survival-RPG patterns but does not copy another game’s characters, assets, animations, names, maps, UI, or proprietary behavior.

## System goals

Combat should feel readable on a phone, reward timing rather than button mashing, and connect directly to survival and crafting. Every attack needs a visible startup, an active hit window, and recovery. Every enemy needs a recognizable intent, a fair response window, and a reason to use movement, crafting, or a biome ability.

The simulation should remain deterministic and testable. Native C++ owns combat, AI, durability, damage, survival meters, and physics. Kotlin displays snapshots and sends input requests; it must not mutate combat state directly.

## Combat loop

The core loop is **observe → position → commit → confirm → recover**. The player reads an enemy telegraph, moves into a favorable angle, commits to a light or heavy action, confirms the hit or misses, and then manages recovery. Dodging cancels the player’s attack at a stamina cost but provides a short invulnerability window.

### Player combat states

| State | Purpose | Transition in | Transition out | Input behavior |
|---|---|---|---|---|
| Idle | Neutral exploration state | Spawn, recovery complete | Move, attack, dodge, gather | All valid actions |
| Walk/Sprint | Positioning and pursuit | Movement input | No input, attack, jump, dodge | Attack keeps facing direction |
| Startup | Attack wind-up | Attack accepted | Active window or dodge cancel | Cannot start another attack |
| Active | Hitbox is live | Startup complete | Recovery or confirmed hit | One hit per target per swing |
| Recovery | Attack commitment ends | Active window complete | Idle or queued combo | Queue next light attack inside combo window |
| Dodge | Evasive movement | Dodge accepted | Idle after duration | Invulnerable; attack input is ignored |
| Hitstun | Reaction to damage | Damage received | Idle, dodge unavailable briefly, or Dead | Movement damped; no attack |
| Dead | Knockout state | Health reaches zero | Respawn or session reset | No combat input |

### Attack classes

| Attack | Startup | Active | Recovery | Stamina cost | Base damage | Design use |
|---|---:|---:|---:|---:|---:|---|
| Light 1 | 0.08 s | 0.10 s | 0.26 s | 0 | 12 | Fast opener and safe poke |
| Light 2 | 0.10 s | 0.11 s | 0.28 s | 0 | 16 | Combo continuation |
| Heavy finisher | 0.14 s | 0.14 s | 0.38 s | 0.05 | 24 | High commitment and stagger chance |
| Charged strike | 0.42 s | 0.16 s | 0.52 s | 0.12 | 38 | Armor break; cancel if released early |
| Bow shot | 0.28 s | instant projectile | 0.34 s | 0.03 | 18 | Ranged pressure and weak-point play |
| Ability cast | 0.24 s | ability-specific | 0.42 s | 0.12–0.25 | Effect-specific | Biome abilities and crowd control |

The current prototype’s three-step combo can map directly to Light 1, Light 2, and Heavy finisher. A hit-confirmed strike applies 0.06 seconds of hit-stop to the attacker and target presentation, while the target receives a short hit flash and optional stagger.

### Damage calculation

Use a simple deterministic model that is easy to balance:

```text
rawDamage = weaponPower + attackPower + bonusDamage
mitigation = targetArmor / (targetArmor + 100)
finalDamage = max(1, round(rawDamage * (1 - mitigation)))
criticalDamage = finalDamage * criticalMultiplier when a critical roll succeeds
```

Heavy attacks can add a stagger value rather than only increasing damage. A target enters stagger when its accumulated stagger exceeds its threshold. Stagger should interrupt a normal attack but not always cancel a boss phase ability.

### Hit reactions and telegraphs

A readable attack uses three visual signals. The enemy first changes pose and emits a warm or cold tell, then places an impact marker on the ground or in front of its body, and finally performs the active strike. The impact marker should be visible for at least 0.25 seconds on normal attacks and 0.60–1.00 seconds on boss attacks.

| Attack type | Telegraph | Player response | Failure result |
|---|---|---|---|
| Claw swipe | Shoulder pulls back; short orange arc | Step backward or dodge through | Damage plus brief hitstun |
| Pounce | Target ring locks to player; enemy crouches | Move laterally; dodge at launch | Damage plus knockback |
| Frost cone | Chest crystal brightens; cone appears | Exit cone or use Frost Guard | Damage plus slow |
| Sand burrow | Dust circle expands around enemy | Leave circle before eruption | Launch and stamina loss |
| Roar | Screen-edge vignette and expanding ring | Keep distance or interrupt with heavy hit | Stagger and temporary fear slow |

## Weapon durability

Durability is a pressure mechanic, not a punishment for normal play. A weapon should last long enough for a deliberate expedition, but the player should return to a workbench or forge before every long route.

### Durability model

Each weapon has `maxDurability`, `currentDurability`, `conditionBand`, and `repairMaterialClass`. Durability decreases only when an attack connects with an enemy, a projectile is fired, or a tool harvest action completes. Missed swings do not consume durability.

```text
condition = currentDurability / maxDurability
0.75–1.00  Excellent: full damage and normal stamina cost
0.40–0.74  Worn: full damage, visible wear cue
0.15–0.39  Damaged: -10% damage, +5% stamina cost
0.01–0.14  Critical: -25% damage, +15% stamina cost, red warning
0.00       Broken: cannot attack until repaired or replaced
```

### Durability values

| Weapon tier | Example | Max durability | Durability loss per hit | Repair station |
|---|---|---:|---:|---|
| T0 | Stone Knife | 30 | 1 | Handcraft |
| T1 | Flint Shortblade | 70 | 1 | Workbench |
| T2 | Ranger Bow | 90 | 1 per shot | Workbench |
| T3 | Copper Saber | 130 | 1 | Copper Forge |
| T4 | Iron Greatblade | 190 | 2 | Iron Forge |
| T5 | Frostfang Blade | 260 | 1 | Frost Altar |

Repair restores durability up to the original maximum and consumes 25% of the weapon’s recipe materials, rounded up. A repair kit can restore 20% of maximum durability in the field but cannot repair a broken weapon from zero. Rare materials should improve efficiency, not create an unavoidable repair tax.

### Equipment feedback

The HUD should show the equipped weapon icon, condition bar, and a short `WORN`, `DAMAGED`, or `BROKEN` label when the condition band changes. A low-durability weapon emits a brief amber flash after each successful hit. The player should be warned before entering a biome if the equipped weapon is below 25% condition.

## Survival interaction

Combat and survival should influence one another without making the game tedious.

| Survival factor | Gameplay effect | Combat consequence | Recovery |
|---|---|---|---|
| Hunger above 60 | Normal stamina recovery | No penalty | Cooked meals |
| Hunger 30–59 | Slight stamina recovery reduction | Longer dodge recovery | Any food |
| Hunger 1–29 | Health drains slowly | -10% movement acceleration | Warm meal or rest |
| Cold exposure | Builds in snow at night | Slow, stamina drain, frost buildup | Warming draught, fire, frost gear |
| Heat exposure | Builds in sand afternoon | Water drain and reduced sprint | Desert cooler, shade, water barrel |
| Rested at bed | Clears minor fatigue | Temporary stamina recovery bonus | Bed or bedroll |

A bed acts as a respawn point and can advance time to the next phase when the player chooses to rest. Resting should not skip a day automatically unless the player confirms it. Storage chests preserve materials between expeditions and should be safe from normal enemy damage unless a future raid mode explicitly enables structural damage.

## Enemy AI architecture

Enemies use a small behavior-tree runtime evaluated at a fixed cadence, such as 10–15 decisions per second, while movement and hit detection continue at the fixed simulation step. Decisions are data-driven so the same tree can be reused with different perception ranges, cooldowns, health thresholds, and abilities.

### Shared perception model

| Sensor | Range or rule | Output |
|---|---|---|
| Vision cone | Forward cone with obstruction checks | Player visible or hidden |
| Hearing | Triggered by sprint, attack, or resource interaction | Last noise position and intensity |
| Damage memory | 6-second attacker memory | Threat target and revenge state |
| Territory | Biome or encounter bounds | Home position and leash limit |
| Health | Current/max ratio | Phase selector and retreat decision |
| Ally signal | Nearby compatible enemies | Alert group and pack target |

### Shared behavior tree

```text
ROOT: EnemyController
└── Selector
    ├── Dead
    │   └── PlayDefeatState → DropLoot → DespawnOrRespawn
    ├── Disabled or Stunned
    │   └── MaintainReaction → Recover
    ├── HasTarget
    │   └── Selector
    │       ├── TargetOutsideLeash → ReturnHome → RegenerateIfAllowed
    │       ├── LowHealthAndCanRetreat → RetreatToCover → Recover
    │       ├── SpecialAttackReadyAndTelegraphSafe → Telegraph → ExecuteSpecial → Recover
    │       ├── TargetInPrimaryRange → ChooseAttack → Telegraph → Attack → Recover
    │       ├── TargetInSecondaryRange → ChaseWithAvoidance
    │       └── TargetTooFar → SearchLastKnownPosition
    ├── HeardNoise
    │   └── InvestigateNoise → LookAround → ResumePatrol
    └── Patrol
        └── ChooseWaypoint → Move → Pause → Repeat
```

### Forest boar tree

The boar is a territorial melee creature. It patrols slowly, becomes alert when the player gathers nearby, and uses a short charge that can be sidestepped.

```text
BoarRoot
└── Selector
    ├── Dead
    ├── Hitstun
    ├── TargetDetected
    │   └── Selector
    │       ├── ChargeReadyAndDistance 0.25–0.70 → SnortTelegraph → Charge → SkidRecovery
    │       ├── Distance < 0.24 → Bite → Backstep
    │       ├── Distance < 0.90 → ApproachWithStrafe
    │       └── LastKnownPosition → Search
    ├── NoiseHeard → Investigate
    └── GrazePatrol
```

### Snow wolf tree

The snow wolf is faster than the boar and uses pack logic. A lone wolf can retreat and call another predator if it loses health.

```text
SnowWolfRoot
└── Selector
    ├── Dead → DropPeltAndClaw
    ├── Frozen or Stunned → Recover
    ├── TargetDetected
    │   └── Selector
    │       ├── PackCallReadyAndAlliesMissing → HowlTelegraph → CallPack
    │       ├── PounceReadyAndDistance 0.50–1.20 → CrouchTell → Pounce → TurnBack
    │       ├── Distance < 0.22 → SnapBite → CirclePlayer
    │       ├── PackAdvantage → FlankTarget
    │       └── ApproachWithZigZag
    ├── LowHealth → RetreatShortDistance → Reacquire
    └── SnowPatrol
```

### Frostclaw boss tree

The Frostclaw Sovereign is a late-game snow boss with 100 HP in the prototype encounter and a scalable production health value later. It should be approximately ten times the hero’s visual height in the final presentation, but its attack hitboxes must remain readable and avoid filling the entire playable space.

```text
FrostclawRoot
└── Selector
    ├── Dead → BossDefeat → RewardChest → ArenaExitUnlock
    ├── PhaseTransition
    │   └── RoarTelegraph → ArmorBurst → SelectNextPhase
    ├── Staggered → VulnerableWindow → Recover
    ├── TargetDetected
    │   └── Selector
    │       ├── Health > 66%
    │       │   ├── ClawComboReady → SwipeTell → Swipe1 → Swipe2 → Recover
    │       │   ├── PounceReady → IceRingTell → Pounce → Shockwave
    │       │   └── CloseDistance → HeavyStepApproach
    │       ├── Health 33–66%
    │       │   ├── FrostConeReady → ChestCharge → ConeTelegraph → FrostCone
    │       │   ├── CrystalShardReady → RaiseClaw → GroundShardPattern
    │       │   └── CloseDistance → EnragedSwipeCombo
    │       └── Health < 33%
    │           ├── BlizzardReady → ArenaDarken → SafeZoneMarkers → BlizzardCast
    │           ├── RoarReady → FearRingTelegraph → Roar
    │           └── TargetInRange → DesperationCombo
    └── ReturnToArena
```

### AI fairness rules

Enemies cannot attack from outside their visible telegraph, cannot rotate instantly during an active hit window, and cannot chain the same special attack twice without a recovery or decision interval. A boss must expose at least one safe response—movement, dodge, interrupt, line-of-sight break, or biome ability—during every special attack.

## Physics model

The current prototype already uses fixed-step simulation, acceleration, friction, gravity, grounded state, jump impulses, stamina-gated dodge movement, world bounds, and axis-separated AABB collision. The following additions improve feel without requiring a large third-party physics dependency.

### Fixed-step contract

Use a simulation step of 1/60 second. Accumulate render time, execute at most eight simulation steps per frame, and clamp a large backlog to prevent a spiral of death. Render interpolation may smooth presentation, but gameplay state must be advanced only by fixed steps.

### Movement and gravity

```text
horizontalVelocity = approach(horizontalVelocity, targetVelocity, acceleration * dt)
if no input:
    horizontalVelocity = approach(horizontalVelocity, 0, friction * dt)
verticalVelocity += gravity * dt
position += velocity * dt
resolve static collisions on the smallest penetration axis
```

Recommended starting constants for the prototype world are a walk speed of `0.55`, sprint speed of `0.78`, acceleration of `5.5`, friction of `7.0`, gravity of `-2.8`, and jump velocity of `1.05`. These values are tunable and should be tested on the lowest supported phone refresh rate.

### Better collision response

Static obstacles should use AABBs in the prototype, with an eventual migration path to capsule-vs-heightfield collision for authored terrain. Resolve horizontal and vertical penetration separately, clear only the velocity component that collided, and set `grounded` only when the contact normal points upward. A small skin width of `0.005` world units prevents jitter at corners.

### Slopes and steps

For authored 3D terrain, support a maximum walkable slope of 42 degrees. A step-up probe may move the body upward by up to `0.18` world units when a forward collision is detected and there is clear space above the step. If the slope exceeds the limit, project movement onto the surface or slide down it instead of stopping abruptly.

### Knockback and hit reactions

Knockback is an impulse applied after damage confirmation:

```text
knockbackVelocity += normalized(attackerToTarget) * impulseStrength
knockbackVelocity = clampMagnitude(knockbackVelocity, maxKnockbackSpeed)
```

Use separate horizontal and vertical components. Normal attacks should apply a small horizontal impulse, heavy attacks a larger impulse with brief stagger, and boss attacks may add a controlled launch. During hitstun, reduce input influence to 0.15 rather than freezing the body completely.

### Projectiles and area effects

Projectiles use swept-circle or ray checks between the previous and current position so fast arrows cannot tunnel through targets. Area effects use a circle or capsule query with a per-target cooldown key. Fire, frost, smoke, and sand effects should be pooled rather than allocated every frame.

### Moving props and breakables

Beds, chests, stations, lights, and structural pieces are static gameplay objects by default. Breakable props use a lightweight health value and an `impactMaterial` tag. A broken prop becomes non-collidable after its defeat animation completes, and dropped resources are spawned once with a stable interaction ID.

## Mobile implementation plan

The first implementation slice should extend the existing native combat system with weapon state and a small enemy controller interface. The second slice should add knockback, swept projectile checks, hit reaction timing, and the boar/wolf trees. The third slice should add building collision tags, resting, storage, lights, and biome survival modifiers. The final slice should add Frostclaw phase transitions and boss arena rules.

The Kotlin layer should receive a compact snapshot with player health, stamina, hunger, active biome, time phase, in-game day, equipped weapon condition, active ability cooldown, enemy health, and the current interaction prompt. The UI should never infer AI state from animation timing; it should display only the authoritative native snapshot.

## Acceptance criteria

| Area | Pass condition |
|---|---|
| Combat | Every attack has startup, active, recovery, hit confirmation, and readable feedback. |
| Durability | Missed attacks do not consume durability; broken equipment cannot be used until repaired. |
| Survival | Hunger, heat, cold, rest, and food influence combat without creating unavoidable damage loops. |
| AI | Enemies use perception, telegraphs, cooldowns, leash limits, recovery, and fair response windows. |
| Boss | Frostclaw starts at 100 HP in the prototype and exposes safe responses to every special attack. |
| Physics | Movement is deterministic, collision-safe, framerate-independent, and supports knockback without tunneling. |
| Mobile | Touch controls remain responsive and the HUD stays readable in landscape orientation. |

## References

This is an original design specification based on the game’s existing prototype goals and the user-provided visual direction. No external game assets or proprietary implementation details are included.
