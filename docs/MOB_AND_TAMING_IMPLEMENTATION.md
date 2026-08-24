# Aethelgrad Mob and Taming Pass

Aethelgrad now has an original wildlife layer alongside its humanoid enemy roster. Wildlife is presented as tamable rather than as another reskinned humanoid target, with separate silhouettes, movement rhythms, materials, glow accents, and resource costs.

| Creature | Role | Animation direction | Taming cost |
|---|---|---|---:|
| Moon Deer | Agile forest scout | Light gait, leap lift, antler shimmer, soft moon-core glow | 2 fiber |
| Mossback Boar | Durable bruiser | Heavy stomp, snort motion, moss ridge, grounded four-leg stride | 3 fiber |
| River Otter | Water companion | Low swim-like bob, tail sweep, ripple-friendly body motion | 2 fiber |
| Canopy Fox | Fast skirmisher | Pounce bounce, quick paw rhythm, raised tail, ember eye glint | 2 fiber |

## Taming loop

The player weakens a wild creature to 38 percent health or below, approaches within the interaction radius, and taps **TAME ANIMAL**. The action consumes the species’ fiber cost and converts the creature into the active companion. Only wildlife profiles are eligible; humanoid enemies cannot be captured through this button. If a companion is already active, the command button controls that companion instead of replacing it.

The active companion follows the player through the existing authoritative simulation. Tapping **COMMAND** toggles follow and stay behavior, and the HUD target line identifies a nearby tamable creature with its current health, readiness state, and fiber cost. The existing Emberling bonding system remains separate, so the player can still bond with Emberling when no captured wildlife companion is active.

## Animation contract

The Android blockout uses deterministic procedural animation so it remains lightweight and buildable without importing binary skeletal assets. Each wildlife type has a distinct motion signature while remaining driven by world time, current distance to the player, health, hit flash, captured state, and existing movement updates. This keeps presentation state subordinate to gameplay state and makes later replacement with original rigged meshes straightforward.

The Unreal production replacement should preserve the same gameplay contract and add authored skeletal assets for idle, locomotion, hit, defeat, tame/bond, follow, stay, and command feedback. The final content pass should also add species-specific interaction VFX, audio, icons, and cloud-save serialization for a multi-companion roster.
