# Reference Audit: etopuz/Survival-Game

## Source

Reference repository: [etopuz/Survival-Game](https://github.com/etopuz/Survival-Game). The repository is a Unity 2018.4.26f1 prototype, last updated on 19 October 2021, with 57 commits, 6 stars, and no repository license metadata or tracked LICENSE/COPYING/NOTICE file. Because the repository does not declare a license, AETHELGRAD must use it as a behavioral and architectural reference only; its C# source, Unity assets, package content, and third-party assets will not be copied into the Unreal project.

## Relevant reference features

| Reference feature | Reference implementation | AETHELGRAD adaptation decision |
|---|---|---|
| Health, hunger, stamina | `PlayerStatsController.cs` owns normalized survival values, stamina drain/recovery, hunger drain, starvation damage, death state, and UI bars. | Keep AETHELGRAD’s existing replicated `UForestSliceSurvivalComponent`, which already owns health, hunger, thirst, stamina, temperature, injury, shelter, stamina consumption, damage, and sleep restoration. Add only missing behavior if tests show a gap. |
| Focus interaction | `PlayerInteractive.cs` raycasts forward, identifies a tagged object, displays an action prompt, and confirms a collect action. | Reuse AETHELGRAD’s existing typed `UForestSliceInteractionComponent`, which ranks registered candidates by distance and priority and broadcasts candidate/confirmation events. Add a focused resource-node bridge only if needed by the map slice. |
| Collectables | `Collectable.cs` calls inventory add and destroys the object after a successful pickup. | Implement a server-authoritative resource pickup contract that grants a typed item ID/quantity, awards gathering XP, and records a stable mutation rather than copying the Unity component. |
| Inventory stacking | `Inventory.cs` uses item identity, stackability, max stack size, finite capacity, and change callbacks. | Extend the existing replicated quick-slot/item contract or add a small authoritative inventory component with stack limits and change events; do not duplicate the quick-slot system unnecessarily. |
| Power-ups | `PowerUp.cs`, `SpeedPowerUp.cs`, and `JumpPowerUp.cs` rotate pickups and apply temporary player modifiers. | Adapt as data-driven temporary status effects in the Unreal survival/presentation path only after the first resource loop is stable. No Unity asset or code reuse. |
| UI separation | Reference inventory and survival UI update from gameplay scripts. | Keep AETHELGRAD’s UMG as a command/presentation surface; C++ components remain owners of state and server validation. |

## Important limitations of the reference

The reference README explicitly marks attacking, crafting, mining/lumbering, building, crouching, and saving as unfinished. It also cites tutorial and third-party asset sources. Its inventory and interaction patterns are useful for a first prototype but are not suitable as production code for AETHELGRAD without server authority, replication, save migration, stable IDs, mobile input, and asset governance.

## Selected first adaptation

The safest high-value adaptation is a **resource-node pickup loop**: a focusable resource candidate displays a prompt; confirmation validates distance and authority; the inventory stack is increased if capacity allows; the resource mutation is persisted by stable ID; gathering XP is awarded; and the resource is visually removed or disabled. This directly complements AETHELGRAD’s existing deterministic procedural forest, interaction component, quick slots, progression, mobile HUD, and save systems.

The implementation should use original AETHELGRAD types and assets. The reference repository receives attribution as a design reference, not as a code or asset dependency.

## References

[1]: https://github.com/etopuz/Survival-Game "etopuz/Survival-Game repository"
[2]: https://github.com/etopuz/Survival-Game/blob/main/README.md "Survival-Game README"
[3]: https://github.com/etopuz/Survival-Game/blob/main/Assets/Scripts/Player/PlayerStatsController.cs "Reference survival-stat controller"
[4]: https://github.com/etopuz/Survival-Game/blob/main/Assets/Scripts/Player/PlayerInteractive.cs "Reference interaction controller"
[5]: https://github.com/etopuz/Survival-Game/blob/main/Assets/Scripts/InteractableObjects/Collectable.cs "Reference collectable behavior"
[6]: https://github.com/etopuz/Survival-Game/blob/main/Assets/Scripts/Inventory/Inventory.cs "Reference inventory behavior"
