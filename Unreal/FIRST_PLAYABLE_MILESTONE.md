# First playable: Forest Arrival

## Scope

This milestone is an **original Aethelgrad** vertical slice, not a claim that the full open-world game or final AAA asset set is complete. It keeps the existing `app/` Kotlin/OpenGL Android game as the active harness and adds Unreal only under `Unreal/`.

| Player-visible step | C++ owner | Authority and save boundary |
|---|---|---|
| Sign in and account gate | `UForestSliceAccountSubsystem` and `UForestSliceFirstPlayableSubsystem` | The platform bridge and backend must verify identity; no raw provider string authenticates a player. |
| Recover or create an owned world | `UForestSliceWorldSessionSubsystem` and first-playable subsystem | Backend authorizes world ownership and revisions; the client only presents the recovered manifest. |
| Character confirmation | `UForestSliceCharacterProfileComponent` | Profile validation and cloud persistence remain server-owned. |
| Forest arrival and movement | `AForestSliceCharacter`, `AForestSliceProceduralForest`, `UForestSliceMobileHUD` | Input intent is local; replicated world mutations require server validation. |
| Gather and craft a camp | Interaction, inventory, tool-loadout, building, and survival components | Resource claims and building mutations must be validated by the dedicated server. |
| Place bed and advance tutorial | `AForestSliceBed`, `AForestSliceWorldClock`, first-playable subsystem | Sleep and time advancement are authoritative; co-op policy remains a later gate. |

## State flow

`Boot → AccountGate → WorldRecovery → CharacterSetup → ForestArrival → CampTutorial → Complete`

`UForestSliceFirstPlayableSubsystem` implements this presentation progression and rejects invalid order transitions. It is not a replacement for authenticated backend exchange, cloud-save ownership, dedicated-server authority, Unreal UMG assets, or authored world content.

## Required local Unreal work

Use an Unreal Engine 5.6+ source or installed build, generate project files from `ForestSlice.uproject`, compile the `ForestSlice` module, create original Blueprint/UMG assets, assign the project game mode, and run an Android device profile. The sandbox does not have Unreal Engine, generated headers, production assets, or an Unreal-capable CI runner; compilation and device packaging remain pending.

## Completion evidence

- [ ] The subsystem compiles in Unreal Engine 5.6+.
- [ ] A Blueprint/UMG flow binds `OnPhaseChanged` to original screens and tutorial prompts.
- [ ] An authenticated backend session feeds `BeginAuthenticatedSession` only after server verification.
- [ ] A real owned-world manifest feeds `ResolveOwnedWorld`.
- [ ] A player completes movement, gathering, camp construction, bed placement, and save/reload on an Android target device.
- [ ] Original/licensed asset entries are added to `Unreal/ASSETS.md` before shipping content.
