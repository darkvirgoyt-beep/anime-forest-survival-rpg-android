# Unreal Mobile Controls and Graphics Settings

## Runtime ownership

`AForestSlicePlayerController` creates `UForestSliceMobileHUD` for the local player and binds it to the possessed `AForestSliceCharacter`. The HUD is a command surface: movement, look, sprint, attack, heavy attack, jump, dodge, gathering, and weapon requests converge on the character’s existing UMG-callable methods. The character movement component remains the owner of collision, acceleration, stamina, water movement, and server validation.

## Touch controls

The runtime HUD builds two independent `UForestSliceVirtualJoystick` widgets. The left stick emits a clamped normalized `FVector2D` to `SetVirtualMove`; the right stick emits a camera look vector to `SetVirtualLook`. Each stick tracks its own pointer index, so multi-touch movement and look input do not share mutable pointer state. Sprint is press/release driven, while attack, heavy attack, jump, dodge, and gather are edge-triggered buttons.

The runtime control surface is landscape-safe and uses anchored canvas slots. The settings button is anchored to the top-right. Action buttons are anchored to the bottom-right, and the two sticks are anchored to the bottom corners. Projects that provide a custom Blueprint HUD can retain the same public HUD functions and replace only the visual layer.

## Graphics settings

`UForestSliceGraphicsSettingsSubsystem` loads and saves a bounded quality level from 0 through 4 in `GGameUserSettingsIni`. Changes apply live to the Unreal scalability groups for view distance, anti-aliasing, shadows, post-processing, textures, effects, foliage, and shading. The subsystem broadcasts `GraphicsQualityChanged` after a changed value is applied. The runtime settings panel exposes a slider and a live `LOW`, `MEDIUM`, `HIGH`, `ULTRA`, or `MAX` label.

The quality setting is intentionally bounded for mobile thermal and memory budgets. It is not a claim that final production art, shaders, frame pacing, or thermals have been validated on a physical phone.

## Verification boundary

Repository contracts validate the source ownership, widget spawn path, graphics subsystem, Android package, Render v2 API base, and existing native/server tests. A full Unreal compile and device touch/performance test still require Unreal Engine 5.6 and a reference Android device. The first device test should confirm left-stick movement, right-stick camera orbit, simultaneous touch, sprint release, settings-slider persistence after restart, and the existing water/hair/cloth presentation systems.
