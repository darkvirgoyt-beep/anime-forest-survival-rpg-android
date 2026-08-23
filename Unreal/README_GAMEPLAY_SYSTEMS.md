# Gameplay systems implementation

## Combat and weapons

`UForestSliceCombatComponent` is the first server-ready combat boundary. It exposes light and heavy attack requests, a three-step light combo, startup/active/recovery phases, combo buffering, active-hit events, replicated phase state, and replicated equipped weapon index. The first attack definitions are data-shaped so animation montages, stamina costs, ranges, damage, poise, knockback, and cue identifiers can move into data assets later.

`UForestSliceWeaponComponent` owns the weapon slot list and validated switching. The initial slots are Blade, Greatblade, Bow, and GatheringTool. A switch cancels queued attacks, resets combo state, starts a switch cooldown, replicates the selected slot, and broadcasts a weapon-changed event. Replace the first-slice server-owned hit-window event with an authoritative capsule or sphere sweep against hurtbox components before a networked alpha. Attack startup now consumes stamina through `UForestSliceSurvivalComponent`, so combat cannot create a second stamina authority.

## Procedural forest

`AForestSliceProceduralForest` derives a stable chunk seed from the world seed and chunk coordinate. It generates tree and rock transforms into hierarchical instanced static mesh components, stores resource locations, keeps a bounded active-radius record set, unloads distant chunk records, and rebuilds instance buffers after unloads. This is a deterministic streaming foundation. Production expansion should add per-chunk instance handles, authored biome masks, nav/AI budgets, persistent harvest/build mutations, terrain height sampling, and World Partition integration.

## Survival and stamina

`UForestSliceSurvivalComponent` owns replicated health, hunger, thirst, stamina, temperature, shelter, and injury state. It drains hunger/thirst and recovers stamina on the authoritative side, applies starvation damage, exposes explicit stamina spending, and restores configured values after sleep. Character actions now call `ConsumeStamina` through this component for sprint drain, slide, and dodge. The character no longer owns a second stamina value; `GetStaminaNormalized` reads the replicated survival state. Heavy/light combat startup uses the same authority.

## Contextual interaction and quick-select

`UForestSliceInteractionComponent` stores nearby candidates for resource nodes, build frames, beds, mounts, stations, chests, quest objects, and NPCs. It deterministically selects the highest-priority valid candidate, using distance as a tie-breaker, then broadcasts candidate changes and confirmed actions for UMG, audio, animation, and server-owned interaction handlers.

`UForestSliceQuickSlotComponent` provides nine replicated slots for weapons, tools, food, potions, and other usable items. It tracks item IDs, quantities, combat usability, active selection, and consumption events. It is intentionally a data contract first; the final Blueprint HUD will supply the thumb-accessible grid/radial presentation.

## Day/night and beds

`AForestSliceWorldClock` replicates a 24-hour server clock, exposes day alpha and night detection, and validates bed sleep by checking nighttime, proximity, movement, and authority. `AForestSliceBed` adds occupancy protection, asks the clock to advance to morning, and restores survival state. The next co-op pass should require party policy approval before advancing a shared clock and should cancel sleep on damage, combat, disconnect, or boss activity.

## UMG mobile HUD

`UForestSliceMobileHUD` is a C++ base widget for a Blueprint-authored UMG layout. It forwards joystick, look, sprint/slide, attack, heavy attack, jump, dodge, weapon switch, and gyro commands to the controlled character and its components. The UMG Blueprint should include a safe-area-aware root, top survival bar, left joystick, right look pad, action cluster, quick slots, interaction prompt, settings entry, and gyro toggle.

The gyro toggle must bind to `SetGyroSensorSupport`. If false, set `GYRO: UNSUPPORTED`, disable the button, reduce alpha, and keep the C++ gate closed. If true, allow the player to enable gyro and forward filtered sensor samples through `PushGyroSample`.

## Google Play account boundary

`UForestSliceAccountSubsystem` defines guest, signing-in, authenticated, signed-out, and error states. It intentionally leaves the provider bridge and backend exchange behind `StartGooglePlaySignIn` and `HandleGooglePlayCredential`. The final service must use a short-lived Google Play credential exchange, keep provider secrets off-device, support token expiry and logout, and add consent, account linking, deletion, and cloud-save conflict handling.

## Current verification boundary

The repository CI can validate the project descriptor, required source layout, Android prototype build, and native prototype tests. It cannot compile Unreal C++ without the Unreal Engine editor/toolchain and generated headers. Use a real Unreal 5.6+ runner to generate project files, compile the module, create input/UMG assets, cook content, and run device tests.
