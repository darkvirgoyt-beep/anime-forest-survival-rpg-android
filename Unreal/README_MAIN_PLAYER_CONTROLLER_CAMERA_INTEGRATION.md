# Main player-controller camera integration

The repository currently contains Unreal C++ source and mobile-input contracts but no binary `.uasset` or `.umap` files. Apply these steps in the real Player Character/Controller Blueprint derived from `AForestSliceCharacter` and `APlayerController`.

## Ownership

Keep the camera state in the player controller or a dedicated camera component. The character owns the spring arm and camera components; the controller owns the look command and smooth target values. The Android UMG look pad must call the existing `SetVirtualLook(FVector2D)` path, while desktop Enhanced Input `IA_Look` calls the same function. Do not create a second independent camera path in the widget.

## Character Blueprint components

Add `SpringArm_Camera` to the main player character and attach `Camera_ThirdPerson` to its socket. Set `Target Arm Length = 420`, `Probe Size = 12`, `Do Collision Test = true`, `Enable Camera Lag = true`, `Camera Lag Speed = 12`, `Enable Camera Rotation Lag = true`, and `Camera Rotation Lag Speed = 14`. Disable `Use Pawn Control Rotation` on both the spring arm and camera if the Blueprint explicitly sets the spring-arm rotation each tick.

Create these controller variables:

| Variable | Type | Default |
|---|---:|---:|
| `TargetYaw` / `CurrentYaw` | Float | `0` |
| `TargetPitch` / `CurrentPitch` | Float | `-14` |
| `TargetArmLength` / `CurrentArmLength` | Float | `420` |
| `TouchSensitivity` | Float | `0.22` |
| `TouchPitchScale` | Float | `0.16` |
| `YawInterpSpeed` | Float | `12` |
| `PitchInterpSpeed` | Float | `14` |
| `MinPitch` / `MaxPitch` | Float | `-87` / `87` |
| `LookPointerId` | Integer | `-1` |

## Event graph

On `BeginPlay`, cache the possessed `AForestSliceCharacter`, the spring arm, and the camera. Initialize the current values from the target values. On `Tick`, run `FInterp To` for yaw, pitch, and arm length using `Delta Seconds`. Clamp pitch before applying it. Build a rotator with `Roll = 0`, `Pitch = CurrentPitch`, and `Yaw = FMod(CurrentYaw, 360)`. Set the spring arm relative rotation and target arm length.

The yaw is allowed to wrap continuously. Crossing `359` to `0` must not cause a visible snap. The valid vertical range is nearly a hemisphere: `-87` through `87` degrees. This is the intended 540°-class orbit, meaning 360° horizontal travel plus nearly 180° vertical travel; it is not a 540° field of view.

## Look-pad event nodes

In the right-side UMG look pad, implement these nodes:

```text
OnTouchStarted
  → Branch (LookPointerId == -1)
  → Set LookPointerId = PointerEvent.PointerIndex
  → Set LastScreenPosition = PointerEvent.ScreenSpacePosition
  → Capture Pointer
  → Return Handled

OnTouchMoved
  → Branch (PointerEvent.PointerIndex == LookPointerId)
  → Delta = ScreenSpacePosition - LastScreenPosition
  → Call AddLookDelta(Delta.X * TouchSensitivity,
                      -Delta.Y * TouchPitchScale)
  → Set LastScreenPosition = ScreenSpacePosition
  → Return Handled

OnTouchEnded / OnTouchCancelled
  → Branch (PointerIndex == LookPointerId)
  → Set LookPointerId = -1
  → Release Pointer Capture
  → Return Handled
```

`AddLookDelta` should add yaw to `TargetYaw` and clamp pitch to `MinPitch`/`MaxPitch`. Add a small dead zone and clamp an unusually large single-frame delta to protect against touch-driver spikes. Keep the left virtual joystick independent and route it to `SetVirtualMove`.

## Existing project hooks

Map the UMG controls according to `README_MOBILE_INPUT.md`: the right look pad calls `SetVirtualLook`, the left joystick calls `SetVirtualMove`, and the gyro toggle calls `SetGyroEnabled`. The existing Android bridge already exposes native orbit and gyro calls for the prototype; the Unreal Blueprint path should converge on the same command semantics.

## Acceptance test

Test continuous rotation across the ±180 boundary, upward and downward pitch limits, camera collision against a wall, two-finger input, pointer cancellation, Android rotation, pause/resume, gyro unsupported, and low-FPS behavior. The camera must remain smooth because all interpolation uses `Delta Seconds`, and movement must remain camera-relative without directly teleporting the character.
