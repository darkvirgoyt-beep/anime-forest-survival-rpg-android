# Aethelgard Inventory & Equipment UI System

## 1. Purpose and product intent

This document defines the player-facing inventory and equipment management system for **Aethelgard: Wild Horizons**. It is designed for the existing Android landscape application, which uses a Kotlin activity and touch HUD around a C++17 gameplay core with local-first progression state.[1] The system should feel like a natural extension of the forest-survival RPG loop: gathering creates material pressure, crafting turns materials into useful tools, equipment changes how the hero survives the wilds, and the inventory makes those decisions legible without forcing players through a spreadsheet-like menu.

The design targets a **playable vertical slice first** and a production-ready data contract second. The initial implementation must be practical on Android phones and tablets, work offline, avoid third-party UI dependencies, and remain portable to the planned Unreal production path. The UI is therefore data-driven, transaction-based, and independent from the renderer. The Android layer presents state and gathers intent; the native gameplay layer validates and commits inventory and equipment mutations.

> **Design principle:** the menu should answer three questions in under five seconds: “What do I have?”, “What can I use right now?”, and “What improves my build?”

The design also follows the project’s existing mobile assumptions: landscape presentation, a continuous movement joystick, right-side camera orbit, edge-triggered actions, and future support for rebindable touch layouts.[2]

## 2. Scope and non-goals

The first release of this system covers the player backpack, item categories, stack management, equipment slots, item inspection, comparison, quick-use actions, crafting handoff, sorting, filtering, favorite/lock flags, and local persistence. It does not attempt to implement a full multiplayer trading economy, auction house, shared storage, appearance transmog, or server-backed entitlement system.

| Area | Vertical-slice target | Production extension |
|---|---|---|
| Backpack | 24 slots, four rows of six, stackable materials and consumables | Expandable capacity, storage chests, loadouts, controller navigation |
| Equipment | Main hand, off hand, head, chest, hands, legs, feet, two accessories | Set bonuses, durability, sockets, cosmetics, class restrictions |
| Item detail | Name, rarity, level, quantity, description, effects, comparison | Full stat breakdown, provenance, crafting sources, market value |
| Actions | Equip, use, split, favorite, lock, drop, discard confirmation | Salvage, repair, compare against loadouts, batch actions |
| Crafting | Inventory screen can open the existing craft action with a selected recipe | Recipe browser, stations, queue, discovery, technology tiers |
| Persistence | Versioned local profile save | Cloud save, migration, reconnect-safe transactions |
| Multiplayer | Offline authoritative prototype | Server-authoritative inventory and anti-duplication validation |

## 3. Entry points and navigation model

Inventory should be reachable from a new **INVENTORY** button in the gameplay action stack and from the pause panel. It must not be opened accidentally by a short camera drag or by pressing an action button. The recommended entry gesture is a single tap on the button; a long press is reserved for future quick-access customization.

Opening inventory pauses the local simulation, stops stamina and hunger drain, and suppresses gameplay actions while the menu is active. Audio should crossfade to a low-volume menu bed or retain the exploration track with a soft ducking effect. The pause behavior is local-only in the current offline slice; the future multiplayer contract must replace this with a safe menu state that does not pause the authoritative world.

The top-level navigation has three destinations:

| Destination | Purpose | Default selection |
|---|---|---|
| **Inventory** | Inspect, sort, filter, use, drop, and equip items | Last selected category, otherwise All |
| **Equipment** | Inspect the paper-doll layout and manage equipped items | Hero overview |
| **Craft** | Hand off the selected material or recipe to the craft flow | Last selected recipe or current quest recipe |

The navigation is a segmented control in the upper-left menu header. It uses text plus an icon and an active underline; color is not the only active-state signal. The Android back button, a visible **CLOSE** button, and the menu button itself all return to gameplay. The back button first closes an open item action sheet, then a comparison panel, then the inventory screen.

## 4. Landscape layout

The screen is designed for a minimum usable landscape canvas of **854 × 480 dp**. The UI must remain functional on wider tablets and on smaller landscape phones by scaling the grid before shrinking text. It must not rely on a fixed pixel size or on content extending beneath system cutouts. The manifest currently locks the activity to landscape and uses immersive full-screen presentation.[3]

The menu uses a three-column composition. The left rail establishes location and filters, the center area gives the inventory a dense but readable grid, and the right panel provides detail without requiring a second screen.

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│ AETHELGARD  /  INVENTORY     [INVENTORY] [EQUIPMENT] [CRAFT]      [CLOSE]     │
├───────────────┬──────────────────────────────────────┬───────────────────────┤
│ FILTERS        │ BACKPACK  14 / 24    [SEARCH] [SORT]│ ITEM DETAIL           │
│                │                                      │                       │
│ ALL            │  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ │ Moonleaf Tonic    │
│ WEAPONS        │  │icon│ │icon│ │icon│ │icon│ │icon│ │icon│ │ Rare • Consumable │
│ ARMOR          │  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ │                   │
│ CONSUMABLES    │  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ │ Restores 25 HP    │
│ MATERIALS      │  │icon│ │icon│ │icon│ │icon│ │icon│ │icon│ │                   │
│ QUEST          │  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ │ [USE] [FAVORITE]  │
│ FAVORITES      │  ...                                     │ [DROP]            │
│                │  [PREV]                         [NEXT]   │                       │
├───────────────┴──────────────────────────────────────┴───────────────────────┤
│ HP 100  STA 82  LV 2       Carry weight 18 / 40       The First Ember        │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 4.1 Header and footer

The header contains the Aethelgard wordmark, the current menu destination, three destination tabs, and a clear close control. The footer repeats only high-value context: current health, stamina, level, carry load, and the active quest objective. It deliberately does not duplicate the full gameplay HUD.

The footer’s carry-load field is initially a slot count, such as **14 / 24**. The production data model supports weight, but weight should remain hidden until the game has enough item variety for it to create meaningful decisions. Showing both slots and weight in the first slice adds cognitive load without improving choice quality.

### 4.2 Filter rail

The filter rail contains **All, Weapons, Armor, Consumables, Materials, Quest, and Favorites**. It is a vertical list with a minimum 48 dp touch target per row, a selected-state underline, and an item count when useful. The rail scrolls only on compact devices; on a typical landscape phone all filters fit without scrolling.

Selecting a filter keeps the currently selected item if that item still belongs to the filter. Otherwise, selection moves to the first visible item. Empty filters show a short explanation and one contextual action, such as “Gather materials in the forest” or “No favorites yet — long-press an item to add one.”

### 4.3 Backpack grid

The backpack contains 24 slots in the initial slice. Each slot is a square button with an icon, quantity, rarity edge, favorite marker, lock marker, and a small state badge where needed. The entire slot is the touch target; tapping the icon is not required.

The grid rules are as follows:

| Rule | Specification |
|---|---|
| Columns | Six at the target landscape width; four on compact fallback |
| Rows | Four initial rows; future capacity adds rows rather than changing slot size |
| Touch target | Minimum 48 × 48 dp, with an 8 dp visual gap |
| Selection | One strong focus outline; selected item also appears in the detail panel |
| Empty slot | Low-contrast frame, no decorative placeholder art |
| Quantity | Bottom-right number; “999+” for overflow display |
| Rarity | Edge treatment plus rarity name in detail panel; never color alone |
| Locked item | Small lock glyph and “Locked” spoken label |
| Favorite item | Small star glyph and “Favorite” spoken label |
| Sorting | Does not change the authoritative item order; it changes a view projection |

The grid supports tap, long press, and optional drag-and-drop. Drag-and-drop is a convenience, not the only way to equip something. Every action available through dragging must also be available through a button or action sheet so the system works with one hand, keyboard/gamepad navigation in the future, and accessibility services.

## 5. Item detail panel

The detail panel is always present on landscape screens. With no selection it shows a neutral “Select an item” state. With a selection it displays the item icon, name, rarity, type, quantity, description, requirements, effects, and actions. The panel must keep a stable width so selecting different items does not make the grid jump.

A detail panel for an equippable item contains a comparison block:

```text
IRONBLOOM SABER
Rare • One-handed weapon • Level 2

POWER                 18       +4 ▲
CRITICAL CHANCE        6%       +2% ▲
STAMINA COST           9        -1 ▼

Trait: Ember Edge
Basic attacks apply a brief warm-light hit flash.

[ EQUIP ]  [ FAVORITE ]  [ LOCK ]
```

Positive and negative comparisons use both an arrow and text such as **+4** or **-1**. Green and red may reinforce the result, but are not the only signal. If the item cannot be equipped, the action is disabled with an explanation rather than simply hidden. Examples include “Requires level 3”, “Requires main-hand slot”, or “This slot is locked during the tutorial.”

The action set is contextual:

| Item type | Primary action | Secondary actions |
|---|---|---|
| Weapon or armor | Equip | Compare, favorite, lock, drop |
| Consumable | Use | Split, favorite, lock, drop |
| Material | Inspect | Split, favorite, lock, drop |
| Quest item | Inspect | Favorite; drop is unavailable with explanation |
| Equipped item | Unequip | Compare, favorite, lock |

Tapping an action performs it immediately when it is reversible and low-risk. **Drop, discard, split, and unequip** use a confirmation or quantity step when the action could cause loss or leave the hero without a valid weapon. The detail panel shows a success toast, such as “Moonleaf Tonic used: +25 HP”, for no more than two seconds.

## 6. Equipment screen

The equipment screen is a focused paper-doll view rather than a second inventory grid. The hero silhouette sits in the center, with equipment slots arranged around it. Each slot is a large button that displays the equipped icon, item name on focus, and slot label. The layout is readable even when the hero render is unavailable; the silhouette is a UI illustration and is not coupled to the GLES scene.

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│ EQUIPMENT                                      [COMPARE: CURRENT / SELECTED] │
├───────────────┬──────────────────────────┬───────────────────────────────────┤
│ EQUIPPED       │        HERO             │ LOADOUT SUMMARY                   │
│ [HEAD]         │      [silhouette]       │ Power                 42           │
│ [CHEST]        │                         │ Defense               31           │
│ [HANDS]        │ [MAIN]       [OFF]      │ Max HP               100           │
│ [LEGS]         │                         │ Max stamina            82           │
│ [FEET]         │                         │                                   │
│ [ACCESSORY 1]  │                         │ [UNEQUIP SELECTED]                 │
│ [ACCESSORY 2]  │                         │ [OPEN INVENTORY]                   │
└───────────────┴──────────────────────────┴───────────────────────────────────┘
```

The initial equipment slots are **Main Hand, Off Hand, Head, Chest, Hands, Legs, Feet, Accessory 1, and Accessory 2**. The equipment data model supports additional slots, but the first UI should not show empty slots for systems that do not exist yet. A locked future slot may appear only in a progression screen, not in the active equipment layout.

Equipment changes are previewed before commitment. Selecting an inventory item and choosing **Compare** opens a split panel with current and proposed values. Choosing **Equip** commits one atomic transaction. If the item occupies two slots or conflicts with an off-hand item, the UI explains the resolution before applying it.

## 7. Item data model

The game should distinguish between an immutable **item definition** and a mutable **item instance**. Definitions describe what an item is; instances describe the player’s quantity, durability, random roll, lock state, favorite state, and ownership. This avoids duplicating static text and balancing values in every save slot.

### 7.1 Definition schema

```text
ItemDefinition {
  id: string                    // "weapon.ironbloom_saber"
  displayNameKey: string        // localization key
  descriptionKey: string
  category: Weapon | Armor | Consumable | Material | Quest
  subType: string               // sword, chest, tonic, fiber, quest_relic
  rarity: Common | Uncommon | Rare | Epic | Legendary
  maxStack: int
  iconKey: string
  equipSlot: MainHand | OffHand | Head | Chest | Hands | Legs | Feet | Accessory | None
  requiredLevel: int
  tags: string[]                // fire, wood, healing, quest, one_handed
  effects: EffectDefinition[]
  sellValue: int
  dropPolicy: Never | Normal | QuestLocked
}
```

### 7.2 Instance schema

```text
ItemInstance {
  instanceId: uint64            // stable within a profile
  definitionId: string
  quantity: int
  durability: float             // 0..1, optional in the first slice
  rollSeed: uint32              // deterministic affix seed, optional
  favorite: bool
  locked: bool
  acquiredAt: int64             // local epoch seconds, optional
}
```

Stackable materials and consumables share one instance until a split action or a distinct roll requires separation. Equipment instances are always quantity one. The UI must never invent an item from an icon tap; it requests the native layer to resolve the selected `instanceId`.

### 7.3 Equipment and derived stats

```text
EquipmentState {
  mainHand: uint64?
  offHand: uint64?
  head: uint64?
  chest: uint64?
  hands: uint64?
  legs: uint64?
  feet: uint64?
  accessoryOne: uint64?
  accessoryTwo: uint64?
}

DerivedStats {
  level: int
  maxHealth: int
  maxStamina: int
  attackPower: int
  defense: int
  gatheringPower: int
  movementSpeed: float
  elementalResistances: map<string, float>
}
```

Derived stats are calculated by the native gameplay layer from the base hero and equipped definitions. The UI receives the result as a snapshot. Kotlin must not recalculate combat values from display text, because this would create divergent rules between the menu and the game.

## 8. Transaction and interaction rules

Every mutation uses a request/response contract. The UI sends an intent with an item or slot identifier, the native layer validates the request, applies it atomically, and returns a new inventory snapshot plus a result code. A failed request must leave the inventory unchanged.

| Intent | Required fields | Success result | Failure examples |
|---|---|---|---|
| `EquipItem` | instance ID, target slot | Equipment updated, old item returned to backpack | Level too low, invalid slot, item missing |
| `UnequipItem` | target slot | Item returned to backpack | Backpack full, slot empty |
| `UseItem` | instance ID, quantity | Effect applied, quantity reduced | Item not usable, hero at full health |
| `SplitStack` | instance ID, quantity | Two valid stacks | Invalid quantity, item not stackable |
| `DropItem` | instance ID, quantity | Quantity removed | Quest-locked, invalid quantity |
| `ToggleFavorite` | instance ID | Flag changed | Item missing |
| `ToggleLock` | instance ID | Flag changed | Item missing |
| `SortInventory` | sort mode | View order changed | None; sorting is local view state |

The request ID is monotonically increasing within the activity session. The response includes the request ID so late results cannot overwrite a newer selection. This is important because the existing Kotlin activity queues native calls on the GL thread while the HUD renders on the Android thread.

Example response shape:

```text
InventoryResult {
  requestId: uint64
  success: bool
  code: Success | NotFound | InvalidQuantity | RequirementsNotMet |
        InventoryFull | Locked | QuestProtected | InvalidSlot
  messageKey: string
  changedInstanceIds: uint64[]
  inventoryRevision: uint64
  equipmentRevision: uint64
}
```

The UI should optimistically highlight the requested action but must not permanently update quantities or equipped slots until the native response is accepted. A brief pending state prevents double taps. If a response fails, the original snapshot remains visible and the message explains what the player can do next.

## 9. Sorting, filtering, and search

Sorting is a presentation operation and must not rewrite the authoritative inventory order. The initial sort modes are **Recommended, Category, Rarity, Name, and Quantity**. Recommended places quest items and usable upgrades first, then materials relevant to the active quest, then remaining items. The recommendation algorithm must be deterministic and must not silently hide items.

Search is optional on small phones and always available on tablets or when a hardware keyboard is connected. It searches localized display names and tags. The search field must be a real accessible text field, not a painted canvas control. The clear button is always visible once text is entered.

## 10. Touch, gesture, and input behavior

A single tap selects an item. A second tap on the already selected item opens the action sheet, which reduces accidental use during fast menu navigation. Long press opens the action sheet directly. Dragging an item toward an equipment slot previews whether the slot accepts it; releasing commits only after native validation.

The system must provide a non-drag path for every action. A player with limited dexterity should be able to select an item, tap **Equip**, select a slot if needed, and confirm. Multi-touch is not required. The menu ignores camera orbit gestures while open.

Haptics are optional and respect the existing vibration setting boundary. A light tap confirms selection, a medium pulse confirms equip/use, and a distinct error pulse accompanies a rejected transaction. Haptics never carry information that is unavailable visually or through TalkBack.

## 11. Accessibility and readability

Accessibility is part of the interaction contract rather than a later polish pass.

| Requirement | Implementation |
|---|---|
| TalkBack | Every slot has a spoken label: “Moonleaf Tonic, rare consumable, quantity 3, favorite” |
| State announcement | Successful mutations announce the result through an accessibility event and visible toast |
| Color independence | Rarity includes text; comparison includes arrows and signed values; lock/favorite use glyphs and labels |
| Text scaling | Support Android font scaling up to 1.3× without clipping; allow the detail panel to scroll vertically |
| Touch size | Interactive controls are at least 48 dp; visual icons may be smaller inside the target |
| Focus order | Header tabs → filter rail → grid left-to-right/top-to-bottom → detail actions → footer controls |
| Contrast | Use the existing dark teal surface with warm gold accent, but verify text against the actual background |
| Motion | Selection and panel transitions are short and skippable when reduced-motion is enabled |
| One-handed reach | Put high-frequency actions in the right detail panel; do not require cross-screen dragging |
| Error recovery | Never clear a selection after a failed action; keep the error explanation adjacent to the action |

The UI should use normal Android text views and buttons for semantics. Decorative icons and item artwork may be custom-drawn, but the accessibility tree must expose the same meaning without the artwork.

## 12. Persistence and save compatibility

Inventory and equipment belong to the local player profile and must be saved through a versioned envelope. The menu may request a save after a successful mutation, but it must not write a partial inventory before equipment validation completes.

```text
ProfileSave {
  schemaVersion: 1
  profileId: string
  lastSavedAt: int64
  inventoryRevision: uint64
  inventory: ItemInstance[]
  equipment: EquipmentState
  activeLoadout: string
  unlockedEquipmentSlots: string[]
  currencies: map<string, int>
}
```

The first migration must tolerate missing inventory and equipment fields by creating an empty backpack plus the starter equipment. Unknown item definitions are preserved as `orphanedDefinitionId` records rather than deleted; the UI displays them as “Unavailable item” and prevents use. This protects a save when content data is updated before a migration is shipped.

Saving should use a temporary file followed by an atomic rename. A failed write must retain the previous valid save. The game should expose the profile’s inventory revision to future cloud synchronization, but cloud services are outside the vertical-slice implementation.

## 13. Kotlin and native implementation boundary

The Android implementation should introduce an `InventoryView` or `InventoryPanel` controller that owns navigation, selection, filtering, sorting, action sheets, and accessibility semantics. It should not own item definitions or derived combat math. The Kotlin layer requests snapshots and submits intents through a narrow bridge.

Recommended bridge surface:

```kotlin
external fun openInventory()
external fun getInventorySnapshot(): String
external fun getEquipmentSnapshot(): String
external fun submitInventoryIntent(request: String): String
external fun saveProfile(): Boolean
```

The existing compact string snapshot can be used for the first prototype, but a versioned JSON payload is preferred once the item count and equipment rules grow. If JSON is introduced, the schema version must be carried in every snapshot. The native layer should expose a read-only snapshot after each mutation and at a bounded polling rate while the menu is open; the gameplay HUD does not need to poll the full inventory.

The native C++ module should own:

1. Item definitions and lookup.
2. Inventory stack merging and capacity validation.
3. Equipment slot compatibility.
4. Derived stat calculation.
5. Transaction validation and revision numbers.
6. Profile serialization and migration.

Kotlin should own:

1. Screen layout and navigation.
2. Touch, focus, and accessibility behavior.
3. Filtering, sorting, and transient selection state.
4. Toasts, dialogs, action sheets, and audio/haptic presentation.
5. Serialization of UI-only preferences such as the last filter and grid sort.

## 14. Visual language

The UI should reuse the gameplay palette instead of creating a separate fantasy menu. Surfaces use deep teal-black, primary text uses warm ivory, active controls use ember gold, and system feedback uses restrained cyan or rose accents. Item rarity is represented by a left edge marker and a label in the detail panel. Do not use texture-heavy panels that reduce legibility on low-resolution displays.

Item icons should be simple, high-contrast silhouettes with a consistent 64 dp canvas, transparent background, and a shared rim treatment. The first slice may use original procedural or generated icons for the saber, cloth armor, moonleaf tonic, wood, fiber, stone, ember kit, and quest relic. Each icon should have a text fallback in the accessibility label and a manifest entry before it is committed as production content.

## 15. Edge cases and failure states

The following states must be designed before implementation rather than handled with generic exceptions:

| State | Required behavior |
|---|---|
| Backpack full while unequipping | Block action and explain that a slot is required; offer a drop/manage-items route |
| Item selected but removed by another system | Clear selection only after showing “Item no longer available”; refresh snapshot |
| Player tries to use a full-health tonic | Explain “Health is already full”; do not consume the item |
| Quest item selected | Show objective context and disable drop with a clear reason |
| Equipment requirement not met | Show the unmet level/tag requirement and keep current gear equipped |
| Double tap on equip | Process one request; subsequent tap sees pending state and does nothing |
| Save failure | Keep current in-memory state, show non-blocking warning, retry on next safe point |
| Unknown item definition | Preserve instance, show unavailable state, disallow mutation |
| Activity rotation/configuration event | Recreate UI from the latest native snapshot without duplicating transactions |
| Accessibility service enabled | Preserve the same action order and do not require drag gestures |

## 16. Implementation sequence

### Phase A — data and bridge

Create `inventory/` and `equipment/` native modules with item definitions, instance state, slot validation, snapshot serialization, and regression tests. Add starter items that support the existing survival loop: wood, fiber, stone, a healing tonic, a starter saber, and a simple chest piece. Add a JNI read-only snapshot before creating the full UI.

### Phase B — inventory surface

Build the landscape inventory container with the header, filter rail, 24-slot grid, detail panel, and footer. Implement selection, filter, sort, quantity display, and empty states with static original icons first. Wire only low-risk actions initially: favorite, lock, and inspect.

### Phase C — equipment and transactions

Add the paper-doll screen, equip/unequip validation, comparison panel, and pending/error states. Add native transaction IDs and inventory/equipment revisions. Verify that moving gear between backpack and equipment is atomic and survives reopening the menu.

### Phase D — consumables and crafting handoff

Add use, split, drop confirmation, and the connection from a selected material or the craft tab into the existing craft flow. Ensure the **The First Ember** quest can show its required materials directly from the detail panel.

### Phase E — persistence and QA

Add versioned profile serialization, save migration tests, Android lifecycle tests, TalkBack labels, font scaling checks, safe-area checks, and device testing at the smallest supported landscape resolution. Profile menu open/close, snapshot parsing, and icon memory use on a mid-range Android phone.

## 17. Acceptance criteria

The system is ready for the next gameplay slice when a player can open inventory without moving or attacking, locate a material through filters, inspect it, equip a valid weapon, compare a replacement, use a tonic, split a stack, favorite and lock an item, and close the menu without losing state. The player must be able to complete all actions without dragging, with an accessible spoken label for every item and result.

The implementation must also pass these technical checks:

| Check | Expected result |
|---|---|
| Native transaction tests | Invalid actions do not mutate inventory or equipment |
| Save round-trip | Inventory, equipment, flags, and revisions are identical after reload |
| Migration test | Version 0 starter profile upgrades to version 1 without data loss |
| Lifecycle test | Pause/resume or recreation does not duplicate requests |
| Visual test | No clipped text or hidden buttons at minimum landscape size |
| Accessibility test | TalkBack can reach every filter, item, action, and close control |
| Gameplay test | Menu pauses local hunger/stamina simulation and resumes cleanly |

## 18. Unreal migration notes

The Android UI contract should remain engine-neutral. `ItemDefinition`, `ItemInstance`, `EquipmentState`, transaction codes, and profile schema should map directly to Unreal data assets and replicated server-side components later. The Android menu should not encode assumptions about a 2D renderer, because the production path calls for a real 3D character, authored equipment, and server-authoritative inventory.[4]

The safest long-term boundary is to keep item identifiers, slot names, effect tags, and transaction result codes stable while allowing Android and Unreal to render different presentations. This gives the prototype a credible progression surface without pretending that a local menu is already a multiplayer-safe economy.

## References

[1]: ../README.md "Aethelgard repository README"
[2]: GDD_IMPLEMENTATION_MAP.md "GDD implementation map and mobile control contract"
[3]: ../app/src/main/AndroidManifest.xml "Android manifest and landscape/GLES requirements"
[4]: AAA_PRODUCTION_ARCHITECTURE.md "AAA production architecture"
