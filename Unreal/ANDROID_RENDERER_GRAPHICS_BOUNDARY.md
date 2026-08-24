# Android renderer graphics boundary

The downloadable Android client uses a real OpenGL ES 3 renderer in `app/src/main/cpp/forest_game.cpp`. This revision adds weather-aware material wetness, direct-light specular response, quality-bounded ambient occlusion, depth fog, moving water sheen, animated water streaks, dynamic time-of-day lighting, vegetation, VFX, and reflection accents. These are GPU calculations executed by the Android renderer, not a loading-screen mock-up.

| Present in the Android renderer | Still required for an Unreal-quality production release |
|---|---|
| OpenGL ES 3 third-/first-person scene, procedural terrain, water, weather, fog, dynamic lighting, original placeholder characters/creatures, quality tiers, and real byte-verified asset delivery gates | Original/licensed 3D character and creature meshes, PBR texture sets, animation, authored maps, material instances, UE 5.6+ Android cook, shader/performance captures, and an actual signed content archive |

The native renderer intentionally caps unavailable high-resource effects through `effectiveGraphicsQuality()`. The app must not unlock texture-heavy or premium content just because a user selected a high setting; it can unlock that path only after the signed cooked archive or Play Asset Delivery packs are mounted and measured.

The next art milestone is not to copy other games. It is to create or license original character, animal, environment, and audio assets; record them in `Unreal/ASSETS.md`; cook them in Unreal Engine 5.6+; then publish the measured Android payload described in `docs/VERIFIED_CONTENT_PUBLICATION.md`.
