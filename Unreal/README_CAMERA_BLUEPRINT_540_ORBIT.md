# Unreal 540°-class third-person camera Blueprint

This project has no checked-in `.uasset` or `.umap` files, so use this as the node-by-node configuration for the real Unreal content project. It defines a **540°-class orbit** as 360° horizontal yaw plus nearly 180° vertical pitch. It is not a 540° field of view.

## Components

Create or open the player character Blueprint derived from `AForestSliceCharacter`. Add:

| Component | Key settings |
|---|---|
| `SpringArm_Camera` | Target arm length `420`, probe size `12`, collision test enabled, camera lag enabled, rotation lag enabled, camera lag speed `12`, rotation lag speed `14`, use pawn control rotation disabled |
| `Camera_ThirdPerson` | Attach to the spring arm socket, use pawn control rotation disabled, field of view `70` |
| Optional `Camera_FirstPerson` | Attach near the head/eyes for the existing first-person toggle; keep the same controller yaw/pitch state |
| `LookPad` UMG widget | Anchored to the right 58% of the screen, hit-test visible, captures one active pointer ID, never sends movement input |

Use the spring arm’s collision probe for obstruction. Do not teleport the camera or directly set the character transform from the touch widget.

## Variables

Add these variables to the player controller or camera component. Keep them separate from actor movement:

```text
TargetYaw          Float   default 0
CurrentYaw         Float   default 0
TargetPitch        Float   default -14
CurrentPitch       Float   default -14
TargetArmLength    Float   default 420
CurrentArmLength   Float   default 420
TouchSensitivity   Float   default 0.22
TouchPitchScale    Float   default 0.16
YawInterpSpeed     Float   default 12
PitchInterpSpeed   Float   default 14
ArmInterpSpeed     Float   default 18
MinPitch           Float   default -87
MaxPitch           Float   default 87
MinArmLength       Float   default 140
MaxArmLength       Float   default 560
LookPointerId      Integer default -1
```

Keep `TargetYaw` unbounded if possible and normalize only when applying rotation. If the project uses a bounded value, wrap it with a modulo operation so crossing ±180 never snaps the camera.

## Blueprint event graph

On `BeginPlay`, get the player controller, set input mode to game only or game-and-UI according to the HUD design, initialize the spring-arm length, and cache the camera component. Set the camera component to use the controller rotation only if the project intentionally centralizes camera rotation there; otherwise drive the spring arm explicitly.

On `Event Tick(Delta Seconds)`:

1. Interpolate `CurrentYaw` toward `TargetYaw` with `FInterp To` and `YawInterpSpeed`.
2. Interpolate `CurrentPitch` toward `TargetPitch` with `FInterp To` and `PitchInterpSpeed`.
3. Interpolate `CurrentArmLength` toward `TargetArmLength` with `FInterp To` and `ArmInterpSpeed`.
4. Normalize the yaw for display/application. Construct `Make Rotator` with `Roll = 0`, `Pitch = CurrentPitch`, and `Yaw = NormalizedYaw`.
5. Call `Set Relative Rotation` on `SpringArm_Camera` and `Set Target Arm Length` with `CurrentArmLength`.
6. Let the spring arm collision test shorten the arm around walls. If using custom obstruction, line trace from the target socket to the desired camera location and set `TargetArmLength` to the hit distance minus a safety margin, clamped between `MinArmLength` and `MaxArmLength`.

Use the camera’s yaw to make movement camera-relative. Feed movement to `SetVirtualMove`/the existing character movement path; never rotate the character from a look-pad event unless the combat design explicitly requires it.

## Right look-pad touch flow

In the `LookPad` User Widget:

```text
OnTouchStarted
  → if LookPointerId == -1
      Set LookPointerId = PointerEvent PointerIndex
      Set LastScreenPosition = Screen Space Position
      Capture Mouse / Pointer
      Return Handled

OnTouchMoved
  → if PointerIndex == LookPointerId
      Delta = CurrentScreenPosition - LastScreenPosition
      TargetYaw = TargetYaw + Delta.X * TouchSensitivity
      TargetPitch = Clamp(TargetPitch - Delta.Y * TouchPitchScale, MinPitch, MaxPitch)
      LastScreenPosition = CurrentScreenPosition
      Return Handled

OnTouchEnded / OnTouchForceChanged / OnTouchCancelled
  → if PointerIndex == LookPointerId
      Set LookPointerId = -1
      Release Pointer Capture
      Return Handled
```

Use a dead zone of 0.5–1.0 pixels, clamp a single-frame delta to a reasonable maximum, and scale by DPI or viewport width so the same swipe distance feels similar on phones and tablets. Do not apply the same delta twice through both UMG and Enhanced Input.

For desktop testing, bind `IA_Look` to the mouse/gamepad right stick and route it to the same `Add Look Delta` function. For Android, the UMG look pad calls `SetVirtualLook(FVector2D)` from the project’s mobile input contract. The implementation should be one command path after the router, not two independent camera systems.

## Gyroscope

Query Android `Sensor.TYPE_GYROSCOPE` and call `SetDeviceGyroscopeSupport`. If unsupported, disable the gyro button and show `GYRO: UNSUPPORTED`. If supported and enabled, low-pass filter the sensor values, multiply by a user sensitivity, and add the result to `TargetYaw`/`TargetPitch`. Clamp pitch after applying gyro input. Reset the filtered values on pause/resume to avoid a jump.

## Validation checklist

Rotate continuously across the ±180 boundary and confirm there is no snap. Swipe upward and downward repeatedly and confirm the pitch stops just inside ±90°. Move behind a wall and confirm the spring arm shortens without pushing the character. Test one-finger look while the left joystick is active, then test two-finger input, pointer cancellation, rotation changes, pause/resume, gyro unsupported, and low-FPS behavior. Verify that camera smoothing uses Delta Seconds and does not change with frame rate.
