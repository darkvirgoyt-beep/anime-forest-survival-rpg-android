# AETHELGRAD Prototype Mobile Control Test Plan

## Test build

Install the direct `app-prototype.apk` from the `v0.3.0-prototype` prerelease. This is the offline prototype variant and can be tested without Google login or remote asset packs.

## Control acceptance matrix

| Test | Action | Expected result |
|---|---|---|
| Joystick dead zone | Rest a finger near the joystick center, then move slowly outward | The hero remains still inside the center dead zone and begins moving smoothly after the threshold. |
| Analog range | Drag the joystick to 25%, 50%, and 100% of its radius | Walk speed changes continuously without a sudden jump at the edge. |
| Diagonal input | Hold a diagonal joystick direction | Movement remains bounded and does not become faster than a cardinal direction. |
| Joystick release | Lift the joystick finger | The hero decelerates to rest and does not continue drifting. |
| Camera orbit | Drag on the right half of the screen | Camera yaw and pitch respond smoothly; pitch remains bounded. |
| Two-finger separation | Hold the joystick with one finger and orbit with a second finger | Both controls remain independent; joystick movement does not cancel camera orbit. |
| Sprint | Hold `SPRINT / SLIDE` while moving | The hero accelerates into sprint and stamina decreases. |
| Slide | Release `SPRINT / SLIDE` while grounded and moving | A directional slide occurs once; releasing while airborne or in water does not incorrectly slide. |
| Dodge | Tap `DODGE` while moving in a direction | Dodge follows the last movement direction and consumes stamina. |
| Jump | Tap `JUMP` while grounded, then tap again in air | First tap jumps; the second tap is ignored. |
| Water | Move into the stream | HUD changes from `DRY` to `WADING` or `SWIMMING`, movement slows, and ripples appear. |
| Pause/resume | Background the app during movement, then return | The hero does not teleport or receive a large physics jump; controls resume normally. |
| Rotation | Rotate the device or re-open the app in landscape | The game remains landscape-safe and the touch regions remain usable. |
| Frame pacing | Test 60 Hz and a supported higher refresh rate from graphics settings | Motion remains stable without an obvious catch-up burst after a frame drop. |

## Device notes

Record the phone model, Android version, display refresh rate, selected graphics tier, average frame rate, and whether the phone becomes hot after ten minutes. Also note any missed taps, joystick drift, camera jumps, accidental action presses, or movement that feels too slow or too slippery. A short screen recording is useful for tuning acceleration, friction, camera sensitivity, and button placement.

## Current implementation safeguards

The joystick now uses a density-aware touch target, a 12% dead zone, normalized analog response, active-pointer tracking, and explicit release handling. The camera uses a separate active pointer, clamps per-event deltas, ignores tiny noise, and resets on pause. The renderer resets its frame clock on resume so lifecycle gaps are not converted into large frame deltas. Native regression tests cover walk acceleration, sprint stamina consumption, and water-state transitions.
