# Aurora Vale Unreal Presentation Integration

The Unreal production tree now contains `UForestSlicePresentationComponent`, a presentation-only component attached to `AForestSliceCharacter`. It consumes combat and weapon delegates, plays configured animation montages, attaches short-lived Niagara sword trails, spawns hit and weapon-switch bursts, and plays sound cues. Damage, stamina, hit confirmation, and replication remain owned by the combat component.

## Character Blueprint setup

Add the default Aurora skeletal mesh and humanoid animation blueprint to the character Blueprint. Confirm the skeleton contains `hand_r_socket` and the presentation sockets listed in `docs/CHARACTER_01_3D_CONFIG.md`. Assign these soft references in the `PresentationComponent` details panel.

| Cue | Suggested asset path | Required sections or behavior |
|---|---|---|
| Light attack montage | `/Game/Characters/Aurora/Animations/AM_Aurora_LightCombo` | Sections `Light_01`, `Light_02`, `Light_03` |
| Heavy attack montage | `/Game/Characters/Aurora/Animations/AM_Aurora_HeavyFinisher` | Section `Heavy_01` |
| Dodge, slide, jump | `/Game/Characters/Aurora/Animations/AM_Aurora_Dodge`, `AM_Aurora_Slide`, `AM_Aurora_Jump` | One montage per movement action |
| Sword trail | `/Game/VFX/Aurora/NS_Aurora_SwordTrail` | Attached to `hand_r_socket`; keep lifetime under 0.5 seconds |
| Hit burst | `/Game/VFX/Aurora/NS_Aurora_SwordHitBurst` | Spawned at the forward hit presentation point |
| Weapon-switch burst | `/Game/VFX/Aurora/NS_Aurora_WeaponSwitchBurst` | Spawned at the character location |
| Sword whoosh | `/Game/Audio/SFX/sfx_attack_sword` | Plays when an attack event begins |
| Sword hit | `/Game/Audio/SFX/sfx_aurora_sword_hit` | Plays only on `HitConfirmed` |
| Magic pulse | `/Game/Audio/SFX/sfx_aurora_magic_pulse` | Plays on weapon-switch presentation |
| Dodge whoosh | `/Game/Audio/SFX/sfx_slide` | Plays on dodge and slide |

## Combat event flow

The authoritative combat component now replicates `CurrentAttackId`. On the owning server, the existing attack phase and hit-resolution logic remain unchanged. On remote clients, `OnRep_AttackPresentation` broadcasts the attack ID to the presentation component, which selects the montage and VFX without applying client-side damage. `HitConfirmed` remains emitted by the authoritative hit sweep.

## Animation Blueprint contract

Use a locomotion base state machine with upper-body montage layering. The presentation component exposes normalized locomotion speed, sprint state, falling state, and movement triggers to Blueprint. Keep animation notifies named `AttackWindowOpen`, `AttackWindowClose`, `HitConfirm`, `Footstep`, `WeaponTrailOn`, `WeaponTrailOff`, and `AbilitySpawn` for the authored animation pass. The C++ combat timing remains the gameplay source of truth; notifies are for presentation and optional local anticipation only.

## Asset status

The repository includes the generated original one-shot files `assets/audio/sfx_aurora_magic_pulse.mp3` and `assets/audio/sfx_aurora_sword_hit.mp3`. Existing sword, slide, and forest-footstep assets remain available. The final Aurora skeletal mesh, montages, Niagara systems, materials, and cooked Unreal assets still need to be imported or authored in the Unreal project; the code intentionally uses soft references so missing content does not prevent the module from loading.

## Mobile validation

Validate the presentation on the target Android tier with one character and then with four replicated players. Check that montage playback does not alter authoritative hit timing, the sword trail is pooled or short-lived, hit bursts do not allocate every frame, soft references are preloaded before combat, and low-quality profiles reduce VFX density while preserving readable attack feedback.
