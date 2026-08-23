# AETHELGRAD 3D Digging and Farming Slice

## Current boundary

The existing Android prototype remains a lightweight 2D GLES harness for input and gameplay regression. The true 3D path is the Unreal Engine branch. This milestone adds the gameplay contracts required for a 3D third-person slice without pretending that the Android harness has become a 3D renderer.

## Camera contract

The Unreal character uses a spring arm and follow camera. Horizontal orbit is bounded to **±270 degrees**, providing a total **540-degree orbit envelope** around the character. The camera pitch is clamped between **-18 degrees and +58 degrees** so the player cannot rotate the view underneath the ground. Spring-arm collision testing keeps the camera from passing through nearby geometry, while camera lag smooths touch and gyro input.

The 540-degree value is an authored orbit envelope, not an unrestricted camera that can expose the underside of the world. The camera still respects collision and pitch safety. If the design later needs a seamless full-circle orbit, the limit can be raised to 360 degrees per side while retaining the pitch clamp and collision probe.

## Underground visibility contract

A level designer assigns an `UndergroundContentActor` to `UForestSliceGroundPlanningComponent`. The actor is hidden and collision-disabled at `BeginPlay`. It remains hidden until excavation reaches `UndergroundRevealDepth`. Every accepted shovel or pickaxe action increases excavation progress only inside the planned ground area. At full progress, the component reveals the actor and enables its collision.

The component does not generate arbitrary underground geometry at runtime. It provides a deterministic visibility and collision gate for authored caves, roots, ore, foundations, and underground rooms. This is safer for mobile performance and lets the production branch use authored Nanite-compatible or mobile-optimized content later.

## Ground planning and farm contours

The gameplay order is:

1. `PlanGround` creates a bounded planning area and resets its contour state.
2. `CreateFarmContour` defines the contour center, size, and height.
3. The player selects a shovel or pickaxe and calls `DigAtLocation` at the forward interaction point.
4. The topsoil flag becomes active after partial excavation; underground content becomes visible only at full excavation progress.
5. `PlantSeed` succeeds only after a planned contour has exposed topsoil and completed the underground reveal gate.

The first slice uses safe size limits of 100–1200 Unreal units per side. The contour stores moisture, seed state, height, and size so a future farming simulation can add irrigation, soil fertility, crop growth, and terrain deformation without changing the mobile-facing API.

## Mobile UMG wiring

The mobile HUD should bind buttons to the character’s Blueprint-callable functions: `SetActiveGroundTool`, `TriggerVirtualPlanGround`, `TriggerVirtualCreateFarmContour`, `TriggerVirtualDig`, and `TriggerVirtualPlantSeed`. A recommended first layout has a `SHOVEL`, `PICKAXE`, `PLAN`, `CONTOUR`, `DIG`, and `PLANT` action group on the right side, separate from movement and camera touch regions.

The Android 2D harness does not expose these Unreal-only actions. The next device milestone should be run from an Unreal Android package after the editor has assigned input assets and an authored test map.

## Editor setup checklist

Create a test map with a walkable landscape, a shallow water volume, a planned soil patch, and an underground actor containing a small cellar or ore chamber. Add the ground-planning component to the player, assign the underground actor, and connect UMG buttons to the Blueprint-callable functions. Use a visible debug material or outline for the planned contour during testing, then replace it with the final farming presentation.

## Acceptance checks

| Area | Required result |
|---|---|
| Camera | 540-degree horizontal envelope, no under-ground pitch, spring-arm collision, smooth touch/gyro response. |
| Planning | A contour can be planned only within the authored ground workflow and has bounded dimensions. |
| Tools | Shovel and pickaxe advance excavation; no tool means no excavation. |
| Reveal | Underground content starts hidden and becomes visible/collidable only after full excavation progress. |
| Farming | Seed planting fails before topsoil and reveal conditions, then succeeds after the contour is prepared. |
| Mobile | UMG actions do not share the movement joystick pointer and remain large enough for touch input. |
