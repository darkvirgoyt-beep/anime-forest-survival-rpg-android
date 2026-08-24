# Aethelgrad audio manifest

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
| `sfx_aurora_magic_pulse.mp3` | Effects | Original generated one-shot via ElevenLabs sound generation | Crystalline violet magic pulse for weapon-switch and sword VFX. |
| `sfx_aurora_sword_hit.mp3` | Effects | Original generated one-shot via ElevenLabs sound generation | Confirmed sword impact with metallic crack and magic tail. |

The Android harness routes the current bank through `GameAudio` and persists Master, Music, Effects, Ambience, and Mute settings. The Unreal production path uses `UForestSliceAudioSubsystem` as the settings boundary. Final production content should replace procedural fallback sounds with recorded or licensed Foley, authored creature vocalizations, layered ambience, dynamic music states, and platform-compressed assets. Keep source WAVs outside shipping packages when the runtime only needs compressed derivatives.

The Aurora magic-pulse and sword-hit one-shots were generated as original game sound effects and stored in this directory. The forest music and existing combat bank remain available as separate runtime assets. Final production audio should still be mixed, normalized, loop-tested, and device-profiled before release.
