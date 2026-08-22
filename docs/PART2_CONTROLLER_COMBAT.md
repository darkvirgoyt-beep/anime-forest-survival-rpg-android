# Part 2 — Third-Person Controller and Advanced Combat

## Design goal

Part 2 establishes a production-shaped controller and combat boundary that can later accept a rigged anime character, authored terrain, animation clips, and a full 3D renderer. The current OpenGL ES scene remains intentionally lightweight, but the gameplay timing and state ownership are no longer tied to individual draw calls.

## System ownership

| System | C++ responsibility | OpenGL ES responsibility | Kotlin responsibility |
|---|---|---|---|
| Controller | Input interpretation, acceleration, sprint, jump, dodge, health, stamina, hitstun, invulnerability, and locomotion state. | Later: skinning pose, facing, motion trails, and camera matrices. | Touch joystick, camera drag, button edge events, lifecycle. |
| Camera | Orbit yaw/pitch, distance limits, obstruction distance smoothing. | Later: view/projection matrix, frustum, depth-tested scene. | Look gesture sensitivity and camera reset action. |
| Combat | Attack definitions, startup/active/recovery phases, combo buffer, hitbox generation, damage event, hit-stop. | Slash arc, weapon trail, impact particles, hit flash, camera shake. | Attack/dodge/jump button events only. |
| Physics | Fixed-step body movement, collision resolution, grounded state, world bounds. | Debug collider overlays in development builds. | No gameplay rules. |

## Fixed-step update

The renderer may run at different rates on Android devices, so gameplay advances in fixed 60 Hz steps. The render callback adds elapsed time to an accumulator and consumes at most eight simulation steps per frame. If a device stalls for too long, excess time is discarded deliberately to avoid a spiral of death. The next production version should expose a debug counter for simulation steps and dropped time.

```text
accumulator += clamp(frameDelta, 0, 100 ms)
while accumulator >= 16.666 ms and steps < 8:
    controller.tick(input, 16.666 ms)
    combat.tick(16.666 ms)
    resolve hitboxes and damage events
    accumulator -= 16.666 ms
render(interpolated state)
```

## Third-person controller

`ThirdPersonController` owns the authoritative body, camera, locomotion state, stamina, health, hitstun, and dodge invulnerability. Input is camera-relative: the normalized joystick vector is rotated by camera yaw before it reaches the physics body. Movement accelerates toward a target speed, friction is applied when the stick is released, and facing rotates toward the movement vector. Sprint consumes stamina and changes the target speed; regeneration is slower during strenuous actions.

The controller state machine is:

```text
Idle -> Walk -> Sprint
  |       |       |
  +---- Jump ---- Fall
  |
  +---- Dodge -> Idle
  |
  +---- Hitstun -> Idle
  |
  +---- Dead
```

A jump is accepted only while grounded and consumes stamina. Dodge applies a short burst velocity, cancels the current attack request, and grants a bounded invulnerability window. Damage is rejected during invulnerability, otherwise it subtracts health, applies knockback, and enters hitstun. Every state transition should eventually emit a gameplay event for animation and audio.

## Combat timing model

Each light attack is defined by data rather than a chain of conditionals:

| Combo step | Startup | Active | Recovery | Damage | Knockback |
|---|---:|---:|---:|---:|---:|
| 1 | 80 ms | 100 ms | 260 ms | 0.10 | 0.20 |
| 2 | 100 ms | 110 ms | 280 ms | 0.13 | 0.25 |
| 3 | 140 ms | 140 ms | 380 ms | 0.20 | 0.38 |

An attack request during startup is ignored. A request during the recovery combo window is buffered and advances to the next definition. The active phase creates one hitbox event per target, preventing multiple damage applications from one swing. A confirmed hit produces a 60 ms hit-stop request; production VFX should consume the same event rather than polling visual state.

The combat pipeline is:

```text
Touch attack
  -> requestAttack()
  -> Startup
  -> Active
  -> build oriented hitbox from facing
  -> overlap target hurtboxes
  -> damage + knockback + hit-stop event
  -> Recovery / combo window
```

Production combat should add attack cancel rules, buffered input timestamps, target filtering, friendly-fire policy, armor/poise, elemental status, stagger thresholds, interrupt priority, invulnerability frames, camera shake, and deterministic replay identifiers. Those additions should extend the event model rather than bypassing it.

## OpenGL ES rendering path

The current renderer draws procedural shapes with a GLES 3 shader so the controller can be tested without large assets. The production replacement should use a depth-tested 3D pipeline:

1. Upload static meshes into VBOs and VAOs; do not use client-side vertex arrays for final content.
2. Use a perspective camera with a spring-arm obstruction ray or capsule cast.
3. Use a skinned mesh path for the hero and animals, with animation pose selection driven by `LocomotionState` and combat events.
4. Batch foliage through instancing and stream only nearby region assets.
5. Use a toon-compatible material with base color, normal, roughness, rim-light, and shadow settings selected by device tier.
6. Render hitboxes, attack arcs, and navigation probes only in development builds.

The first authoring target is one original hero with idle, walk, sprint, jump, fall, dodge, attack-1, attack-2, attack-3, hit, and death clips. The renderer should consume animation events from the gameplay layer; gameplay must not depend on an animation callback to apply damage.

## Mobile input contract

The left joystick is continuous input and can be sampled every simulation tick. The `SPRINT / SLIDE` button is a press-and-hold control: press begins sprint intent, while release clears sprint and requests a grounded slide burst. Attack, jump, dodge, gather, and craft are edge events and should be queued exactly once. Right-side drags update camera orbit on the GL thread. All JNI calls touching native state are issued through `GLSurfaceView.queueEvent`; no UI-thread call should mutate gameplay directly.

Gyro aiming is capability-gated. Android checks for `Sensor.TYPE_GYROSCOPE` at runtime. When present, the button toggles `GYRO: ON` and streams sensor rates through the GL queue with a sensitivity multiplier. When absent, the button text becomes `GYRO: UNSUPPORTED`, its alpha is reduced, and `isEnabled` is false. The unsupported path is a deliberate product state, not an exception or a silent no-op.

## Testing strategy

Host-side tests cover the controller and combat modules without Android dependencies. Required tests include: acceleration reaches a bounded speed, world collision removes outward velocity, grounded jump cannot double-fire, dodge grants invulnerability, damage applies knockback outside invulnerability, attack phases transition at the intended timing, combo input buffers only during the combo window, and a hitbox confirms damage once per swing.

Device tests must then verify GLES context recreation, pause/resume, touch event ordering, 60/90/120 Hz behavior, screen aspect ratios, and sustained thermal performance. A debug HUD should display frame time, simulation steps, active state, combo index, stamina, and draw calls.

## Next Part 2.1 tasks

The next increment should add actual 3D camera matrices, depth-tested placeholder meshes, camera collision probes, animation event records, a target dummy with a hurtbox, and a debug combat overlay. Only after those are stable should the project add original character assets and larger combat VFX.
