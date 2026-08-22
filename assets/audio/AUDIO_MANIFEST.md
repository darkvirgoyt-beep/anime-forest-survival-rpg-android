# Aethelgard audio manifest

The audio bank is organized by runtime bus rather than by screen. `Music` contains exploration and future combat/boss tracks. `Effects` contains attacks, bow release, gathering, crafting, slide, footsteps, and UI feedback. `Ambience` is reserved for forest wind, rain, insects, caves, water, and biome beds. `Voice` is reserved for original character and companion performances.

| Asset | Bus | Current source | Runtime use |
|---|---|---|---|
| `aethelgard_forest_exploration.wav` | Music | Original generated instrumental | Loopable forest exploration bed. |
| `sfx_footsteps_forest.wav` | Effects | Original procedural fallback | Forest walking cadence. |
| `sfx_sprint_loop.wav` | Effects | Original procedural fallback | Sprint movement layer. |
| `sfx_slide.wav` | Effects | Original procedural fallback | Slide burst and dodge texture. |
| `sfx_attack_sword.wav` | Effects | Original procedural fallback | Light/heavy melee feedback seed. |
| `sfx_bow_release.wav` | Effects | Original procedural fallback | Ranged weapon release seed. |
| `sfx_gather_resource.wav` | Effects | Original procedural fallback | Harvest confirmation. |
| `sfx_craft_workbench.wav` | Effects | Original procedural fallback | Crafting station confirmation. |
| `sfx_animal_companion_call.wav` | Ambience | Original procedural fallback | Companion call seed. |
| `sfx_boss_roar.wav` | Effects | Original procedural fallback | Boss encounter cue seed. |
| `sfx_ui_click.wav` | Effects | Original procedural fallback | Menu and HUD feedback. |

The Android harness routes the current bank through `GameAudio` and persists Master, Music, Effects, Ambience, and Mute settings. The Unreal production path uses `UForestSliceAudioSubsystem` as the settings boundary. Final production content should replace procedural fallback sounds with recorded or licensed Foley, authored creature vocalizations, layered ambience, dynamic music states, and platform-compressed assets. Keep source WAVs outside shipping packages when the runtime only needs compressed derivatives.

The ElevenLabs generation route was attempted but unavailable in the current environment because the service redirected to a regional-access page. The forest music was generated successfully through the built-in music route. The icon is a deterministic original fallback because the built-in image quota was exhausted and the configured image connector had insufficient balance. These limitations are recorded so they are not confused with final AAA art or audio.
