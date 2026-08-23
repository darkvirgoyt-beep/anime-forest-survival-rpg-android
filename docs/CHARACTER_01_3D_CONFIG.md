# Character 01 — Aurora Vale 3D Configuration

Aurora Vale is the first original adult female hero for the full 3D production path. She is a disciplined frontier warden who connects the safe forest launch region to the snow route. Her visual direction uses a silver-white hair silhouette, cool blue and warm gold armor accents, a practical travel skirt and leggings, a rose-shaped sword guard, and a calm, readable heroic posture. These are broad project-authored traits, not a reproduction of any supplied or commercial character.

The Android prototype remains a lightweight procedural GLES slice. This configuration targets the planned Unreal-style 3D pipeline and gives the art, animation, physics, and gameplay teams a shared contract.

## Identity and gameplay role

| Field | Configuration |
|---|---|
| Character ID | `hero_aurora_vale_01` |
| Age category | Adult; exact age is intentionally not gameplay-relevant |
| Role | Warden / sword-and-guard explorer |
| Primary weapon | One-handed frost-rose longsword |
| Secondary tool | Compact field bow for gathering and ranged utility |
| Core strengths | Guard timing, cold resistance, stable footing, balanced stamina |
| Core weakness | Lower burst damage than a heavy weapon specialist |
| Starting biome | Forest camp |
| Unlock route | Character creation; snow mastery upgrades arrive later |
| Silhouette anchors | Long silver hair, short mantle, asymmetric shoulder guard, blue-gold chest mark, rose-shaped sword guard |
| Skin presentation | Natural warm-light skin tone with a neutral material response; provide multiple original tone variants without changing anatomy or hitboxes |

## Model budgets

The base hero should be authored as a clean, deformation-friendly real-time model. The high-resolution sculpt is retained for baking, while runtime meshes are split into body, hair, armor/clothing, weapon, and accessories so equipment can be swapped without duplicating the full character.

| Asset | Target budget | Notes |
|---|---:|---|
| Hero body LOD0 | 45,000–65,000 triangles | Close gameplay and photo mode; clean facial topology |
| Hero body LOD1 | 24,000–35,000 triangles | Normal gameplay distance |
| Hero body LOD2 | 9,000–15,000 triangles | Mid-distance exploration |
| Hero body LOD3 | 3,000–6,000 triangles | Far distance and crowds |
| Hair LOD0 | 18,000–28,000 triangles | Strand-card or groom-derived shell with controlled silhouette |
| Hair LOD1 | 8,000–14,000 triangles | Gameplay distance |
| Armor and clothing | 20,000–35,000 triangles | Modular mantle, cuirass, skirt, leggings, gloves, boots |
| Weapon | 4,000–8,000 triangles | Separate socketed asset |
| Facial morph set | 24–36 targets | Blink, squint, brow, mouth, jaw, pain, smile, shout |
| Runtime material slots | 5–7 | Skin, hair, cloth, leather, metal, gem, weapon |

## Rig and skeleton

Use a humanoid skeleton compatible with retargeting, but keep Aurora’s facial and hair controls in project-owned extensions. The skeleton should be authored at a neutral A-pose with a consistent unit scale and an explicit root at the ground contact.

| Bone group | Required bones | Purpose |
|---|---|---|
| Root and locomotion | `root`, `pelvis`, `spine_01`, `spine_02`, `chest`, `neck`, `head` | Root motion, balance, upper-body layering |
| Arms | clavicle, upper arm, forearm, hand, 5-finger chains on both sides | Sword, bow, guard, and interaction poses |
| Legs | thigh, calf, foot, ball, toe on both sides | Locomotion, slopes, stairs, jump landings |
| Mantle and skirt | 10–16 helper bones | Cloth fallback and authored motion |
| Hair | 18–30 strand-chain bones | Low-cost secondary motion with spring constraints |
| Face | jaw, eyes, eyelids, brows, lips, cheeks | Expressions and combat readability |
| Equipment sockets | `hand_r`, `hand_l`, `back`, `hip`, `head`, `chest_fx` | Weapon, tool, quiver, lantern, and ability effects |

### Animation layers

The animation blueprint should use a locomotion base layer, an upper-body combat layer, a full-body reaction layer, and an additive secondary-motion layer. A montage or state-machine transition must be interruptible only at explicit notify windows so attacks and dodge invulnerability remain deterministic.

```text
FinalPose = LocomotionBase
          + UpperBodyCombatLayer
          + AdditiveAimOrLook
          + AdditiveHairAndCloth
          + FullBodyReactionWhenActive
```

## Materials and cel-shaded look

Use physically plausible base values but present them through a controlled toon response. The face and armor should maintain clear light and shadow planes, with a soft rim only when the player is backlit or a special ability is active.

| Material | Base response | Detail |
|---|---|---|
| Skin | Warm neutral albedo, medium roughness | Gentle cheek and nose variation; no plastic shine |
| Silver hair | Cool pale albedo, high roughness | Two-tone shadow ramp and narrow highlight band |
| White armor | Bright neutral albedo, medium roughness | Blue shadow plane and gold edge accents |
| Blue cloth | Saturated blue-violet albedo, high roughness | Fabric normal detail, low specular |
| Gold trim | Warm metallic, controlled reflection | Use only as a readability accent |
| Sword steel | Cool metallic, sharp highlight | Rose guard and blue ability socket |

The toon ramp should use three primary bands: lit, midtone, and shadow. Avoid gradients that erase the silhouette. Outline thickness should be distance-aware, beginning near 1.5 px at gameplay distance and falling toward 0.6 px at far distance; use a material or post-process outline rather than duplicated geometry where possible.

## Core animation set

| Animation ID | Duration | Loop | Root motion | Notes |
|---|---:|---|---|---|
| `aurora_idle_breath` | 2.8 s | Yes | No | Subtle weight shift and breathing |
| `aurora_idle_alert` | 1.4 s | Yes | No | Snow-route combat readiness |
| `aurora_walk` | 0.90 s | Yes | Optional | Moderate forward travel |
| `aurora_run` | 0.62 s | Yes | Optional | Mantle and hair lag behind hips |
| `aurora_sprint` | 0.52 s | Yes | Optional | Stronger forward lean, stamina-linked |
| `aurora_start` | 0.36 s | No | No | Acceleration anticipation |
| `aurora_stop` | 0.30 s | No | No | Foot plant and friction response |
| `aurora_jump_start` | 0.28 s | No | No | Crouch and arm preparation |
| `aurora_fall` | 0.42 s | Yes | No | Airborne loop |
| `aurora_land_soft` | 0.38 s | No | No | Small knees-and-mantle compression |
| `aurora_land_hard` | 0.56 s | No | No | Camera shake and short recovery |
| `aurora_dodge` | 0.42 s | No | Yes | Invulnerability window in first 0.30 s |
| `aurora_slide` | 0.50 s | No | Yes | Low silhouette; cloth collision enabled |
| `aurora_hit_light` | 0.22 s | No | No | Directional flinch |
| `aurora_hit_heavy` | 0.48 s | No | No | Knockback and stagger |
| `aurora_death` | 1.30 s | No | No | Weapon release and controlled fall |

## Combat animation set

| Animation ID | Duration | Active window | Gameplay parameter |
|---|---:|---:|---|
| `aurora_light_01` | 0.44 s | 0.18–0.28 s | Damage 12; combo opener |
| `aurora_light_02` | 0.49 s | 0.21–0.32 s | Damage 16; combo continuation |
| `aurora_heavy_finisher` | 0.66 s | 0.31–0.45 s | Damage 24; stagger value 18 |
| `aurora_charged_guardbreak` | 1.10 s | 0.62–0.78 s | Damage 38; armor break |
| `aurora_guard_raise` | 0.18 s | 0.08–0.48 s | Reduces frontal damage while stamina holds |
| `aurora_guard_counter` | 0.72 s | 0.30–0.42 s | Requires timed impact window |
| `aurora_bow_fire` | 0.60 s | 0.40 s | Spawns swept projectile |
| `aurora_vine_snare_cast` | 0.66 s | 0.24–0.40 s | Forest ability; roots enemies |
| `aurora_frost_guard_cast` | 0.82 s | 0.26–0.56 s | Snow ability; damage reduction |

Attack notifies should be named `AttackWindowOpen`, `AttackWindowClose`, `HitConfirm`, `Footstep`, `WeaponTrailOn`, `WeaponTrailOff`, and `AbilitySpawn`. The combat system—not animation timing alone—remains authoritative for damage and durability consumption.

## Hair secondary motion

Aurora’s hair is divided into front locks, side locks, and a rear braid/tail. Use a low-cost bone-spring solver first, with cloth or groom simulation reserved for close cameras. Hair should follow head rotation with a damped delay, collide with the shoulders and upper back, and settle quickly after a dodge or landing.

| Parameter | Front locks | Side locks | Rear braid |
|---|---:|---:|---:|
| Spring stiffness | 0.72 | 0.58 | 0.84 |
| Damping | 0.18 | 0.22 | 0.26 |
| Gravity scale | 0.62 | 0.72 | 0.88 |
| Max angular deviation | 18° | 26° | 14° |
| Collision radius | 2.0 cm | 2.5 cm | 3.0 cm |
| Teleport reset distance | 35 cm | 35 cm | 35 cm |
| Simulation update | 60 Hz fixed step | 60 Hz fixed step | 60 Hz fixed step |

On low-end devices, reduce the chain count and update hair at 30 Hz while preserving the same rest pose and maximum deviation. Never allow hair physics to alter combat hitboxes.

## Clothing, mantle, and chest-garment secondary motion

The chest area should be handled as **supported garment and torso secondary motion**, not as a camera-focused effect. The armor, cloth, and body must remain properly fitted during running, jumping, attacks, and impacts. Use collision-aware cloth constraints and limited amplitude so the motion reads as believable equipment response.

| Parameter | Mantle | Chest cloth/armor panel | Skirt panels |
|---|---:|---:|---:|
| Simulation type | Spring bones or cloth | Low-amplitude cloth/soft constraint | Cloth or spring chains |
| Stiffness | 0.42 | 0.78 | 0.34 |
| Damping | 0.32 | 0.38 | 0.40 |
| Gravity scale | 0.72 | 0.10 | 0.84 |
| Max translation | 7 cm | 1.5 cm | 9 cm |
| Max rotation | 22° | 8° | 28° |
| Body collision | Back, shoulders, hips | Torso cage and armor shell | Legs and ground proxy |
| Reset condition | Teleport or respawn | Teleport or ragdoll exit | Teleport or ragdoll exit |

The torso cage should be a non-rendered low-poly collision volume. Cloth constraints must be tested against sprint starts, hard landings, sword swings, dodge rolls, slopes, and camera cuts. If the simulation becomes unstable, blend toward the authored animation rather than allowing clipping or explosive motion.

## Physics and performance budgets

Use fixed-step gameplay physics at 60 Hz. Secondary motion may run at 30 or 60 Hz depending on device profile, but it must be deterministic enough for replay and must not change damage, movement, or hitbox outcomes.

| Device profile | Hair chains | Cloth simulation | Draw-call goal | Character memory target |
|---|---:|---|---:|---:|
| Performance | 8–12 | Spring bones only | ≤ 6 visible hero calls | ≤ 35 MB |
| Balanced | 16–24 | Mantle and skirt springs | ≤ 8 visible hero calls | ≤ 55 MB |
| Quality | 24–36 | Limited cloth on mantle and garments | ≤ 10 visible hero calls | ≤ 80 MB |

Use LOD hysteresis to prevent rapid switching. Freeze secondary motion when the character is farther than the simulation distance or fully occluded. Pool cloth proxies and ability effects; do not allocate physics objects during an attack frame.

## Validation checklist

The asset is ready for integration when it passes neutral-pose, retargeting, locomotion, attack notify, dodge invulnerability, hit reaction, weapon socket, hair collision, cloth stability, slope traversal, stair traversal, low-end device, and respawn-reset tests. All final meshes, textures, rigs, animations, and audio must be original or properly licensed.

## References

This is an original production configuration based on the project’s existing Android prototype, user-provided visual direction, and the repository’s combat/physics specification. Supplied images are treated as visual references only; they are not copied into runtime assets.
