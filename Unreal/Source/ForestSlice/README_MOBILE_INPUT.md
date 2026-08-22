# Unreal mobile input contract

## Touch widget wiring

Create a UMG HUD with a left virtual joystick, a right look pad, and action buttons for attack, jump, dodge, sprint/slide, and gyro. The joystick should output a normalized `FVector2D`. On every touch move, call `SetVirtualMove`; on touch release, call `SetVirtualMove(FVector2D::ZeroVector)`. The right look pad calls `SetVirtualLook` with a sensitivity-scaled delta.

The sprint/slide button is a hold interaction. On press, call `SetVirtualSprintHeld(true)`. On release or cancel, call `SetVirtualSprintHeld(false)`. The C++ character stops sprinting and requests a slide if the character was sprinting and is grounded. A separate slide button may call `TriggerVirtualSlide` when the design requires independent slide input.

| UMG control | C++ call | Input semantics |
|---|---|---|
| Left joystick | `SetVirtualMove(FVector2D)` | Continuous camera-relative movement. |
| Right look pad | `SetVirtualLook(FVector2D)` | Continuous orbit/look delta. |
| Sprint/slide | `SetVirtualSprintHeld(bool)` | Press to sprint; release may slide. |
| Jump | `TriggerVirtualJump()` | Edge-triggered jump request. |
| Dodge | `TriggerVirtualDodge()` | Edge-triggered stamina action. |
| Attack | `TriggerVirtualLightAttack()` | Edge-triggered ability request. |
| Gyro toggle | `SetGyroEnabled(bool)` | Enabled only if capability is supported. |

## Gyro unsupported state

At Android startup, a platform bridge should query `Sensor.TYPE_GYROSCOPE` and call `SetDeviceGyroscopeSupport(true)` or `SetDeviceGyroscopeSupport(false)` on the possessed character. The UMG gyro toggle should bind to `HasGyroscopeSupport()`. If false, set the button label to `GYRO: UNSUPPORTED`, disable interaction, and reduce opacity to a gray disabled style. If true, the toggle may call `SetGyroEnabled` and stream sensor values through `ApplyGyroInput`.

The character rejects gyro input if support is absent or the toggle is off. This makes the unsupported path safe even when a UI bug or stale setting attempts to enable gyro.

## Enhanced Input assets

Create these assets under `/Game/Input`:

```text
IA_Move         Axis2D
IA_Look         Axis2D
IA_Sprint       Digital
IA_Slide        Digital
IA_Dodge        Digital
IA_LightAttack  Digital
IA_Jump         Digital
IA_GyroLook     Axis2D
IMC_Player      Input Mapping Context
```

Bind `IA_Move` to gamepad left stick and keyboard for desktop testing. Bind `IA_Look` to gamepad right stick and mouse for desktop testing. Mobile UMG calls the same character methods directly, so UI testing does not depend on a physical gamepad. The production input router should later centralize both paths in a player input component so prediction, replay, and accessibility remapping use one command stream.

## Movement details

The character uses an Unreal capsule movement component with camera-relative acceleration, braking, slope and step limits, jump, sprint stamina, slide impulse, dodge impulse, and a spring-arm camera with obstruction testing. It does not directly set the actor transform. This preserves collision, navigation, replication, and server authority.

## Production additions

The next layer should move attack, dodge, slide invulnerability, and damage into Gameplay Ability System abilities. It should also add input buffering timestamps, animation montage sections, camera shake cues, hit-stop cues, replicated movement prediction, and an authoritative dedicated-server validation path.
