# Reference-Inspired Gathering and Inventory Slice

AETHELGRAD now adapts the useful gameplay ideas from [etopuz/Survival-Game](https://github.com/etopuz/Survival-Game) without importing its Unity code, assets, packages, or third-party content. The reference project demonstrates a simple loop in which a forward interaction check identifies a collectable, the inventory accepts the item if capacity permits, and the world object is removed after a successful pickup. AETHELGRAD implements the same player-facing idea using original Unreal C++ contracts, replication, stable IDs, and the existing progression system.

## Inventory behavior

`UForestSliceInventoryComponent` is attached to `AForestSliceCharacter` beside the existing quick-slot component. It stores replicated stacks with an `ItemId`, `Quantity`, and `StackLimit`. `AddItem` first fills compatible existing stacks, then creates new stacks only when the configured capacity allows. `RemoveItem` consumes from the end of matching stacks and removes empty stacks. The default capacity is 20 stacks and the default stack limit is 10 items, matching the reference prototype’s broad behavior while remaining configurable in Unreal.

Inventory mutations are authoritative. A client cannot directly add or remove items because the component checks `GetOwner()->HasAuthority()`. Replication sends the resulting stack array to clients, and `InventoryChanged` allows UMG or other presentation systems to refresh after local replication.

## Resource-node behavior

`UForestSliceResourceNodeComponent` can be placed on an authored tree, stone, root cache, herb, ore node, or other resource actor. The level designer assigns a stable resource ID, item ID, quantity, stack limit, interaction distance, and gathering XP value. The component exposes a typed interaction candidate with the action ID `Gather` and a readable prompt.

When collection is requested, the server validates that the node is not depleted, the stable ID and item ID are valid, the collector is within range, and the collector has enough inventory capacity. Only after `AddItem` succeeds does the node become depleted, award gathering XP, disable collision, and hide its presentation. The replicated depletion state causes clients to remove the node visually as well.

The default gathering reward is **12 integer XP**, matching the existing AETHELGRAD grinding reward contract. The reward is awarded only after a successful inventory insertion, preventing XP farming from failed or full-inventory attempts.

## Mobile collection path

`UForestSliceMobileHUD::GatherPressed` forwards to `AForestSliceCharacter::TriggerVirtualCollect`. The character performs a short forward visibility trace from the player’s upper-body interaction point. If the hit actor owns a resource-node component, collection is attempted. In a networked session, the mobile action calls `ServerTriggerVirtualCollect`; the server performs the authoritative trace and collection operation. In standalone mode, the same helper executes locally because the standalone character has authority.

This follows the reference project’s forward-ray interaction idea but avoids Unity tags and `Input.GetKeyDown`. AETHELGRAD uses typed Unreal components, UMG edge-triggered commands, and server-side validation.

## Integration boundaries

The new inventory is intentionally separate from the existing quick-slot component. Quick slots represent immediately usable equipped entries, while the inventory stores the complete stack collection. A future equipment bridge can move or reference inventory stacks into quick slots without making the HUD the owner of item state.

The new resource node also remains separate from procedural chunk generation. Procedural forest records can later create stable resource actors from their `ResourceLocations`; authored actors can use the same component immediately in the first playable Forest Camp, Farming Village, and Moss Cave map. The stable resource ID is the key for future save mutations so a depleted node stays depleted after streaming out and back in.

## Reference and licensing boundary

The reference repository has no declared repository license or tracked LICENSE/COPYING/NOTICE file, and its README attributes several systems and assets to tutorials and third-party sources. Therefore, AETHELGRAD uses the repository only as a design reference. No reference C# file, Unity package, model, texture, icon, scene, or third-party asset is copied into the Unreal project. New AETHELGRAD assets must be original or separately licensed and recorded in the project asset manifest.

## Validation status

The Android/native regression suite remains the validation path available in the sandbox. Unreal Engine 5.6 editor compilation, Blueprint wiring, authored resource placement, cooked Android packaging, and physical-device multiplayer testing still require the Unreal toolchain and device environment. CI source-contract checks verify that the new inventory, resource-node, character RPC, and mobile HUD bindings remain present.

## References

[1]: https://github.com/etopuz/Survival-Game "Reference Survival-Game repository"
[2]: https://github.com/etopuz/Survival-Game/blob/main/README.md "Reference README and roadmap"
[3]: https://github.com/etopuz/Survival-Game/blob/main/Assets/Scripts/Player/PlayerInteractive.cs "Reference forward interaction flow"
[4]: https://github.com/etopuz/Survival-Game/blob/main/Assets/Scripts/InteractableObjects/Collectable.cs "Reference collectable flow"
[5]: https://github.com/etopuz/Survival-Game/blob/main/Assets/Scripts/Inventory/Inventory.cs "Reference stack and capacity flow"
