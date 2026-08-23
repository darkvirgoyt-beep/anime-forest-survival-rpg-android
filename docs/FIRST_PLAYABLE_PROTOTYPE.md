# AETHELGRAD First Playable Prototype

## Purpose

This milestone is the small playable foundation to validate the game loop before production art, large-world streaming, multiplayer, and the full realistic-graphics build. It is intentionally compact and uses the verified Android C++/Kotlin harness. The production Unreal branch remains separate so the prototype can be tested without turning the unfinished engine migration into the game’s only build.

## Prototype build

The `prototype` Android build type is an offline development variant. It hides the online authentication screen, does not request the fast-follow Play Asset Delivery pack, and starts directly in the built-in forest slice. It uses the same movement, combat, physics, water, weather, progression, and HUD code as the release path. The release build still keeps the online-only session boundary.

Build it with:

```bash
gradle assemblePrototype
```

The expected APK is:

```text
app/build/outputs/apk/prototype/app-prototype.apk
```

This variant uses the package suffix `.prototype` so it can be installed beside the release package. It is for local testing and is not a store release.

## First playable loop

The player moves an original hero through a compact three-biome scene containing forest, sand, snow, and a shallow stream. Movement includes acceleration, friction, jump, dodge, slide, stamina, water wading/swimming, collision bounds, and spring-driven hair/cloth presentation. The hero can gather nearby resources, craft the ember kit, fight the forest warden, gain experience, level up, and observe the objective update in the HUD.

| Prototype element | Acceptance check |
|---|---|
| Hero | The game starts directly in the prototype build with a controllable hero and readable HUD. |
| Movement | Joystick movement, sprint/slide, jump, dodge, collision, stamina, and camera orbit respond without authentication. |
| Environment | Forest, sand, snow, campfire, village/settlement shapes, water, ripples, weather, and day/night presentation are visible. |
| Gathering | `GATHER` collects nearby caches and updates wood, fiber, stone, quest progress, and XP. |
| Crafting | `CRAFT` consumes resources and unlocks the warden objective. |
| Combat | `ATTACK` runs the combo/combat state machine, shows hit feedback, damages the warden, and grants XP. |
| Progression | Level, XP, health, stamina, hunger, inventory, warden health, and quest state remain visible. |
| Prototype safety | The prototype does not depend on Google login, remote packs, or a network connection. |

## Controls

Use the left virtual joystick to move and right-side drag input to orbit the camera. Use `SPRINT / SLIDE`, `ATTACK`, `JUMP`, `DODGE`, `GATHER`, and `CRAFT` to exercise the loop. Gyro input is enabled only when the device reports a gyroscope. The graphics settings surface changes the prototype quality tier and target frame rate.

## What comes next

After this slice is stable on a reference Android device, the next milestone replaces placeholder geometry with original or licensed Unreal assets, adds authored animation graphs and material instances, moves the forest into a streamed 3D micro-region, and adds one real camp/bed interaction. Only after those checks pass should the project add additional regions, advanced hair/cloth, boss behaviors, co-op authority, cinematics, and the full 10 GiB asset-pack content set.

The prototype is not presented as finished AAA graphics or a complete RPG. Its purpose is to prove the controls, game loop, data contracts, and build path before expensive production content is authored.


## Mobile controls test plan

Use [`MOBILE_CONTROL_TEST_PLAN.md`](MOBILE_CONTROL_TEST_PLAN.md) to validate joystick response, multi-touch separation, camera orbit, sprint/slide, dodge, jump, water movement, pause/resume, and frame pacing on the reference phone.
