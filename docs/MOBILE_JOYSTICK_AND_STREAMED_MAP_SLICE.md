# AETHELGRAD Mobile Joystick and Streamed Map Slice

## Scope and honest boundary

The supplied screenshots show the lightweight Android GLES harness. That harness is useful for mobile input regression, but it is not the final Blender-quality Unreal world. This milestone improves the harness presentation and control feel while extending the Unreal production path with deterministic chunk streaming, animated chunk presentation, and a map landmark contract.

## PUBG/BGMI-style joystick behavior

The left movement control now uses a dynamic touch origin. The player can press anywhere in the lower-left movement zone rather than aiming for a fixed circle. On touch-down, the base appears under the thumb; the thumb knob follows the finger inside a bounded ring; output remains a normalized `FVector2D` equivalent in the native bridge; and the stick returns to its default resting position on release.

The implementation keeps a 10% dead zone, clamps the knob to the ring, tracks one active pointer, rejects touches outside the left movement zone, and hands right-side touches to the camera-look surface. This preserves movement/camera separation during multi-touch combat and camera operation.

## Prototype ground and top surfaces

The GLES 3D harness no longer relies on one large flat slab plus a false upper slab. It renders a 5×5 grid of terrain tiles around the player. Each tile contains a solid terrain body and a thin topsoil surface. Small deterministic height variation, a road band, a river band, and a subtle animated top-surface shimmer make the ground plane readable on phone screens.

The upper false slab was removed from the world pass. Weather remains a separate effect, so the sky is not confused with a ceiling or underground surface.

## Unreal streamed chunks

`AForestSliceProceduralForest` remains the production owner for chunk streaming. It uses the world seed and chunk coordinates to generate stable tree, rock, and resource transforms. The active radius bounds memory and draw work. Newly generated chunks begin at `PresentationAlpha = 0` and animate to `1` over time. The chunk instances scale in during that transition, creating a lightweight spawn/reveal cue while the final project can replace the cue with Niagara, material parameter animation, foliage wind, and authored LOD transitions.

The actor exposes `GetChunkPresentationAlpha`, so UMG, debug telemetry, or a future loading indicator can observe chunk reveal progress. Off-radius chunks are unloaded and the HISM instances are rebuilt from the active records.

## First playable map layout

The map uses the existing 100×100 km design contract and its three horizontal biome bands. The Unreal map coordinates are represented in kilometres in the landmark data and can be converted to metres for world placement.

| Landmark | Biome | Coordinate (km) | First-playable purpose |
|---|---|---:|---|
| Forest Camp | Forest | (10, 16) | Safe spawn, bed, Heartfire, workbench, First Ember start |
| Farming Village | Forest | (14, 40) | Farm contours, crops, cooking, early trade |
| Moss Cave | Forest | (26, 78) | Excavation entrance, minerals, early encounter |
| Sand Gate | Sand | (40, 48) | Forest-to-sand transition and road checkpoint |
| Sun Kiln | Sand | (47, 28) | Processing, bricks, glass, ore refinement |
| Oasis | Sand | (55, 69) | Water refill and heat-recovery stop |
| Frost Gate | Snow | (74, 57) | Cold tutorial and equipment check |
| Predator Basin | Snow | (84, 59) | Predator grinding zone and material drops |
| Frostclaw Arena | Snow | (88, 83) | Boss route, reward chest, and end-of-slice gate |

The initial playable Unreal test map should activate only the Forest Camp, Farming Village, and Moss Cave route. Sand and Snow remain map-visible planning regions until their content packs and terrain are authored.

## Unreal Editor setup still required

Create a Landscape with visible collision, add the procedural forest actor, assign tree and rock meshes, and set the active radius. Place an authored Forest Camp, Farming Village farm area, and Moss Cave entrance. Add an underground actor to the cave and assign it to the ground-planning component. Create UMG bindings for the virtual joystick vector, camera look delta, sprint hold, and the existing digging/farming buttons.

The final Blender-quality presentation requires authored or licensed meshes, rigged animation, material instances, foliage wind, landscape layers, water shaders, navigation, LODs, occlusion, Niagara effects, and device profiling. Those assets and the Unreal editor cook are not available in the sandbox, so this milestone does not claim a finished AAA map or a final 3D Android APK.
