# Aethelgard Crafting and Building Catalog

This document defines an original survival-RPG crafting and base-building system for **Aethelgard: Wild Horizons**. It uses familiar genre patterns—resource gathering, processing stations, equipment progression, farming, storage, shelter, and biome-specific materials—but does not copy another game’s proprietary recipes, names, UI, models, or progression.

## Design assumptions

The player begins with scavenged materials in the forest launch region, establishes a small camp, reaches the sand settlement to unlock kiln and loom processing, and finally enters the snow region for high-risk predator materials and frost upgrades. Recipes are intentionally modular so the Android prototype can implement them as data rows before the production version replaces procedural visuals with streamed assets.

The base currency is not a separate gold resource. Progression is driven by materials, station unlocks, player level, and biome access. A recipe may require a station and a minimum level, but it should never require a specific real-world service or external purchase.

## Resource families

| Family | Example materials | Primary source | Main use |
|---|---|---|---|
| Timber | Branch, wood, resin, frostwood | Forest trees, driftwood, snow deadwood | Structures, tools, weapons, fuel |
| Plant | Fiber, herb, berry, grain, cactus fiber | Forest farms, grass, desert plants | Rope, cloth, meals, medicine |
| Mineral | Stone, clay, sand, copper ore, iron ore, coal | Rocks, sand pits, caves, snow outcrops | Foundations, kiln goods, weapons, tools |
| Creature | Hide, pelt, meat, claw, antler | Wildlife and snow predators | Armor, meals, bait, rare equipment |
| Refined | Plank, brick, glass, copper bar, iron bar, frost crystal | Workbench, kiln, forge, frost altar | Advanced building and equipment |
| Utility | Water, salt, ash, ember, sap | Wells, cooking, campfires, resin trees | Cooking, alchemy, fuel, preservation |

## Crafting stations and unlocks

| Station | Unlock condition | Inputs it processes | Core outputs |
|---|---|---|---|
| Handcraft | Available at start | Branch, fiber, stone, herb | Basic tools, rope, bandage, torch |
| Field Workbench | Player level 1; forest shelter quest | Wood, fiber, stone | Planks, chests, beds, structural pieces |
| Cooking Fire | Player level 1; gather ember | Meat, grain, herb, water | Meals, broths, stamina food |
| Loom | Player level 3; collect 12 fiber | Fiber, hide, cactus fiber | Cloth, bags, light armor, banners |
| Clay Kiln | Player level 4; visit sand settlement | Clay, sand, coal, ash | Brick, glass, pottery, desert pieces |
| Stonecutter | Player level 5; collect 20 stone | Stone, brick, copper bar | Cut stone, stairs, monuments, foundations |
| Copper Forge | Player level 6; process copper ore | Copper bar, coal, wood | Copper tools, weapons, lamps, fittings |
| Iron Forge | Player level 9; enter snow route | Iron bar, coal, hide | Iron tools, weapons, reinforced structures |
| Alchemy Bench | Player level 7; recover three herbs | Herb, crystal, water, salt | Tonics, resistance items, ability charges |
| Frost Altar | Player level 12; defeat a 100 HP snow predator | Frost crystal, pelt, claw, iron bar | Frost gear, predator bait, final-tier lights |

# Complete crafting recipe list

## Handcraft, tools, and gathering

| ID | Recipe | Station | Ingredients | Output | Unlock |
|---|---|---|---|---|---|
| HC-01 | Twine Bundle | Handcraft | Fiber ×3 | Twine ×1 | Start |
| HC-02 | Rough Plank | Handcraft | Wood ×2 | Plank ×1 | Start |
| HC-03 | Stone Chip | Handcraft | Stone ×2 | Stone Chip ×1 | Start |
| HC-04 | Wood Handle | Handcraft | Branch ×2, Twine ×1 | Handle ×1 | Start |
| HC-05 | Stone Knife | Handcraft | Stone Chip ×2, Handle ×1, Twine ×1 | Stone Knife ×1 | Start |
| HC-06 | Flint Axe | Handcraft | Stone Chip ×3, Handle ×1, Twine ×1 | Flint Axe ×1 | Start |
| HC-07 | Reed Pick | Handcraft | Stone Chip ×3, Handle ×1, Fiber ×2 | Reed Pick ×1 | Start |
| HC-08 | Gathering Sickle | Handcraft | Stone Chip ×2, Handle ×1, Fiber ×2 | Sickle ×1 | Level 2 |
| HC-09 | Hunting Spear | Handcraft | Wood ×3, Stone Chip ×2, Twine ×1 | Spear ×1 | Level 2 |
| HC-10 | Simple Bow | Handcraft | Wood ×3, Twine ×2, Fiber ×2 | Bow ×1 | Level 2 |
| HC-11 | Arrow Bundle | Handcraft | Branch ×4, Stone Chip ×2, Fiber ×1 | Arrows ×8 | Level 2 |
| HC-12 | Rope Coil | Handcraft | Twine ×4 | Rope ×1 | Level 2 |
| HC-13 | Stone Hammer | Handcraft | Stone Chip ×3, Handle ×1, Twine ×1 | Hammer ×1 | Level 3 |
| HC-14 | Repair Kit | Handcraft | Plank ×2, Twine ×2, Stone Chip ×1 | Repair Kit ×1 | Level 3 |

## Survival, medicine, and cooking

| ID | Recipe | Station | Ingredients | Output | Unlock |
|---|---|---|---|---|---|
| SV-01 | Bandage Roll | Handcraft | Fiber ×3, Herb ×1 | Bandage ×2 | Start |
| SV-02 | Bitter Tonic | Handcraft | Herb ×2, Water ×1 | Tonic ×1; restores health over time | Level 2 |
| SV-03 | Berry Mash | Cooking Fire | Berry ×3, Water ×1 | Meal ×1; restores hunger | Start |
| SV-04 | Roasted Meat | Cooking Fire | Meat ×2, Salt ×1 | Meal ×1; restores hunger and health | Start |
| SV-05 | Grain Porridge | Cooking Fire | Grain ×2, Water ×2, Herb ×1 | Meal ×2; restores hunger and stamina | Level 2 |
| SV-06 | Herb Broth | Cooking Fire | Herb ×2, Water ×2, Salt ×1 | Meal ×2; grants slow regeneration | Level 3 |
| SV-07 | Smoked Meat | Cooking Fire | Meat ×3, Ash ×1, Salt ×1 | Preserved Meal ×2; long spoil timer | Level 3 |
| SV-08 | Desert Cooler | Cooking Fire | Cactus Fiber ×2, Berry ×1, Water ×2 | Drink ×1; grants heat resistance | Level 4 |
| SV-09 | Frost Stew | Cooking Fire | Meat ×2, Grain ×1, Herb ×1, Salt ×1 | Meal ×1; grants cold resistance | Level 8 |
| SV-10 | Warming Draught | Alchemy Bench | Herb ×2, Ember ×1, Water ×1 | Tonic ×1; prevents cold slowdown | Level 8 |
| SV-11 | Clearwater Flask | Loom | Fiber ×2, Hide ×1, Water ×1 | Flask ×1; carries water | Level 3 |
| SV-12 | Predator Bait | Cooking Fire | Meat ×2, Resin ×1, Pelt ×1 | Bait ×1; attracts snow predators | Level 9 |

## Processing and stations

| ID | Recipe | Station | Ingredients | Output | Unlock |
|---|---|---|---|---|---|
| PR-01 | Field Workbench | Handcraft | Wood ×8, Stone ×4, Fiber ×4 | Workbench ×1 | Start |
| PR-02 | Cooking Fire | Handcraft | Stone ×6, Branch ×4, Ember ×1 | Cooking Fire ×1 | Start |
| PR-03 | Loom | Workbench | Plank ×6, Rope ×2, Fiber ×6 | Loom ×1 | Level 3 |
| PR-04 | Clay Kiln | Workbench | Stone ×12, Clay ×10, Brick ×2 | Kiln ×1 | Level 4 |
| PR-05 | Stonecutter | Workbench | Stone ×16, Plank ×4, Iron Bar ×1 | Stonecutter ×1 | Level 5 |
| PR-06 | Copper Forge | Workbench | Stone ×16, Clay ×8, Copper Bar ×4, Coal ×2 | Copper Forge ×1 | Level 6 |
| PR-07 | Iron Forge | Copper Forge | Stone ×20, Brick ×8, Iron Bar ×6, Coal ×4 | Iron Forge ×1 | Level 9 |
| PR-08 | Alchemy Bench | Loom | Plank ×8, Glass ×2, Herb ×6, Crystal ×1 | Alchemy Bench ×1 | Level 7 |
| PR-09 | Frost Altar | Iron Forge | Cut Stone ×8, Iron Bar ×4, Frost Crystal ×3, Claw ×2 | Frost Altar ×1 | Level 12 |
| PR-10 | Rough Brick | Clay Kiln | Clay ×2, Sand ×1, Coal ×1 | Brick ×1 | Level 4 |
| PR-11 | Glass Shard | Clay Kiln | Sand ×3, Coal ×1 | Glass ×1 | Level 4 |
| PR-12 | Copper Bar | Clay Kiln | Copper Ore ×3, Coal ×1 | Copper Bar ×1 | Level 5 |
| PR-13 | Iron Bar | Iron Forge | Iron Ore ×3, Coal ×2 | Iron Bar ×1 | Level 9 |
| PR-14 | Cut Stone | Stonecutter | Stone ×3 | Cut Stone ×1 | Level 5 |
| PR-15 | Cloth Roll | Loom | Fiber ×4, Cactus Fiber ×1 | Cloth ×1 | Level 4 |
| PR-16 | Frost Crystal | Frost Altar | Crystal ×2, Ice Shard ×3 | Frost Crystal ×1 | Level 12 |

## Weapons and shields

| ID | Recipe | Station | Ingredients | Output | Unlock |
|---|---|---|---|---|---|
| WP-01 | Flint Shortblade | Workbench | Stone Chip ×4, Plank ×1, Twine ×1 | Weapon; damage 12 | Level 2 |
| WP-02 | Ranger Bow | Workbench | Plank ×4, Twine ×3, Hide ×1 | Weapon; ranged damage 10 | Level 3 |
| WP-03 | Copper Saber | Copper Forge | Copper Bar ×5, Plank ×1, Hide ×1 | Weapon; damage 22 | Level 6 |
| WP-04 | Thorn Spear | Workbench | Wood ×4, Resin ×2, Claw ×1, Twine ×2 | Weapon; damage 18 and bleed chance | Level 7 |
| WP-05 | Iron Greatblade | Iron Forge | Iron Bar ×8, Plank ×2, Hide ×2 | Weapon; damage 36; slow swing | Level 9 |
| WP-06 | Frostfang Blade | Frost Altar | Iron Bar ×4, Frost Crystal ×3, Claw ×2 | Weapon; damage 48; cold mark | Level 12 |
| WP-07 | Hide Buckler | Workbench | Plank ×3, Hide ×2, Rope ×1 | Shield; guard rating 10 | Level 3 |
| WP-08 | Copper Guardshield | Copper Forge | Copper Bar ×5, Plank ×2, Hide ×1 | Shield; guard rating 20 | Level 6 |
| WP-09 | Frostwall Shield | Frost Altar | Iron Bar ×5, Frost Crystal ×2, Pelt ×2 | Shield; guard rating 34; cold resistance | Level 12 |

## Clothing and armor

| ID | Recipe | Station | Ingredients | Output | Unlock |
|---|---|---|---|---|---|
| AR-01 | Ranger Wrap | Loom | Cloth ×2, Fiber ×3, Hide ×1 | Chest gear; armor 4 | Level 3 |
| AR-02 | Ranger Set | Loom | Cloth ×5, Hide ×3, Twine ×2 | Full light set; armor 10 | Level 4 |
| AR-03 | Desert Veil | Loom | Cloth ×2, Cactus Fiber ×3, Glass ×1 | Head gear; heat resistance | Level 5 |
| AR-04 | Sandrunner Boots | Loom | Hide ×2, Cactus Fiber ×2, Rope ×1 | Boots; movement efficiency | Level 6 |
| AR-05 | Copper Lamellar | Copper Forge | Copper Bar ×8, Hide ×3, Cloth ×2 | Chest gear; armor 20 | Level 7 |
| AR-06 | Iron Trailguard | Iron Forge | Iron Bar ×8, Hide ×4, Cloth ×2 | Full medium set; armor 32 | Level 9 |
| AR-07 | Frosthide Mantle | Frost Altar | Pelt ×4, Frost Crystal ×2, Cloth ×3 | Chest gear; armor 42; cold resistance | Level 12 |
| AR-08 | Frostclaw Set | Frost Altar | Pelt ×6, Claw ×3, Iron Bar ×6, Frost Crystal ×4 | Full heavy set; armor 60 | Level 14 |

## Utility, storage, lights, and farming

| ID | Recipe | Station | Ingredients | Output | Unlock |
|---|---|---|---|---|---|
| UT-01 | Small Storage Chest | Workbench | Plank ×8, Stone ×2, Iron Bar ×1 | Storage ×12 slots | Level 2 |
| UT-02 | Reinforced Chest | Copper Forge | Plank ×10, Copper Bar ×4, Iron Bar ×2 | Storage ×24 slots | Level 7 |
| UT-03 | Bedroll | Workbench | Fiber ×6, Cloth ×2, Hide ×1, Plank ×2 | Bedroll; respawn point | Level 2 |
| UT-04 | Timber Bed | Workbench | Plank ×10, Cloth ×3, Hide ×2 | Bed; respawn and time skip | Level 4 |
| UT-05 | Frost Bed | Frost Altar | Frostwood ×6, Pelt ×3, Cloth ×3, Frost Crystal ×1 | Bed; cold immunity while resting | Level 12 |
| UT-06 | Hand Torch | Handcraft | Branch ×1, Fiber ×1, Resin ×1 | Light source ×1 | Start |
| UT-07 | Wall Lantern | Copper Forge | Copper Bar ×1, Glass ×1, Resin ×1 | Placeable light ×1 | Level 6 |
| UT-08 | Frost Lantern | Frost Altar | Copper Bar ×1, Frost Crystal ×1, Glass ×1 | Bright cold light ×1 | Level 12 |
| UT-09 | Farm Plot | Workbench | Plank ×2, Fiber ×2, Water ×1, Soil ×4 | Farm plot ×1 | Level 2 |
| UT-10 | Irrigation Trough | Workbench | Plank ×4, Clay ×3, Copper Bar ×1 | Waters nearby plots | Level 6 |
| UT-11 | Seed Bundle | Handcraft | Grain ×2, Fiber ×1 | Seeds ×3 | Level 2 |
| UT-12 | Drying Rack | Workbench | Plank ×6, Rope ×2, Fiber ×2 | Food-processing station | Level 3 |
| UT-13 | Village Banner | Loom | Cloth ×2, Plank ×1, Fiber ×2 | Morale radius decoration | Level 4 |
| UT-14 | Water Barrel | Workbench | Plank ×6, Iron Bar ×1, Rope ×1 | Stores water ×8 | Level 4 |
| UT-15 | Waystone Marker | Stonecutter | Cut Stone ×6, Copper Bar ×2, Crystal ×1 | Fast-travel destination | Level 8 |

## Biome abilities and ability items

| ID | Ability or item | Station | Ingredients | Effect | Unlock |
|---|---|---|---|---|---|
| AB-01 | Vine Snare Charm | Loom | Fiber ×4, Resin ×2, Herb ×2 | Unlocks forest ability; roots enemies briefly | Level 3 |
| AB-02 | Sand Dash Charm | Clay Kiln | Glass ×2, Cactus Fiber ×3, Copper Bar ×1 | Unlocks sand ability; short stamina dash | Level 6 |
| AB-03 | Frost Guard Charm | Frost Altar | Frost Crystal ×2, Pelt ×2, Claw ×1 | Unlocks snow ability; reduces incoming damage | Level 12 |
| AB-04 | Ember Charge | Alchemy Bench | Ember ×1, Crystal ×1, Herb ×2 | One ability recharge | Level 7 |
| AB-05 | Smoke Bomb | Alchemy Bench | Ash ×2, Salt ×1, Resin ×1, Fiber ×1 | Breaks enemy attention for a short time | Level 5 |
| AB-06 | Iceburst Bomb | Frost Altar | Frost Crystal ×1, Salt ×1, Coal ×1 | Small area slow and cold mark | Level 12 |

# Building tiers

Building tiers are **material and station gates**, not copies of any other game’s construction catalog. Each tier increases durability, weather resistance, decoration capacity, and the number of connected building pieces the player can maintain.

| Tier | Name | Theme and use | Unlock | Core materials | Durability | Weather protection | New building capability |
|---|---|---|---|---|---:|---:|---|
| T0 | Camp | Temporary survival shelter | Start | Branch, fiber, stone | 50 | None | Bedroll, hand torch, campfire, simple storage |
| T1 | Timber Homestead | First permanent forest base | Level 2 and workbench | Plank, fiber, stone | 120 | Light rain | Floors, timber walls, door, roof, farm plots, small chest |
| T2 | Reinforced Lodge | Safe all-season forest base | Level 5 and stonecutter | Plank, cut stone, rope, iron bar | 260 | Rain and wind | Windows, stairs, ladder, beams, reinforced chest, drying rack |
| T3 | Sunbaked Compound | Sand-settlement architecture | Level 7 and kiln | Brick, glass, copper bar, cloth | 360 | Heat and sandstorm | Adobe walls, shade roof, awnings, water barrel, wall lantern, kiln room |
| T4 | Ironhold Keep | Heavy defensive base | Level 9 and iron forge | Iron bar, cut stone, brick, plank | 560 | Heavy weather | Iron-reinforced walls, gate, defensive platform, weapon rack, forge room |
| T5 | Frostspire Refuge | Snow-region endgame base | Level 12 and frost altar | Frost crystal, frostwood, iron bar, pelt | 820 | Cold, blizzard, night exposure | Frost insulation, crystal lights, elevated bridge, boss trophy room, fast-travel waystone |

## Building piece catalog by tier

The same functional piece can appear in several visual materials. The table shows the first tier at which each piece becomes available and its base cost. Higher tiers substitute the tier’s core material and apply the durability multiplier from the building tier table.

| Piece | First tier | Base cost | Function |
|---|---|---|---|
| Foundation | T0 | Stone ×2, Branch ×2 | Snaps the building grid to terrain |
| Floor tile | T1 | Plank ×2 | Walkable interior or platform |
| Half floor | T1 | Plank ×1 | Fine interior layout |
| Timber wall | T1 | Plank ×3, Fiber ×1 | Basic enclosure |
| Window wall | T2 | Plank ×2, Glass ×1 | Light and visibility |
| Door frame | T1 | Plank ×4, Rope ×1 | Entrance support |
| Timber door | T1 | Plank ×3, Iron Bar ×1 | Lockable entrance |
| Roof panel | T1 | Plank ×2, Fiber ×2 | Rain cover |
| Sloped roof | T2 | Plank ×3, Fiber ×2 | Pitched roof and attic space |
| Support beam | T2 | Plank ×2, Iron Bar ×1 | Raises structural limit |
| Stair flight | T2 | Plank ×5, Cut Stone ×1 | Vertical traversal |
| Ladder | T2 | Plank ×3, Rope ×2 | Low-cost vertical traversal |
| Fence segment | T1 | Plank ×2, Fiber ×1 | Farming and settlement boundary |
| Gate | T2 | Plank ×5, Iron Bar ×2 | Wide passable boundary |
| Adobe wall | T3 | Brick ×3, Clay ×1 | Heat-resistant wall |
| Shade awning | T3 | Cloth ×2, Plank ×2, Rope ×1 | Desert shade and decoration |
| Water trough | T3 | Brick ×2, Clay ×2, Copper Bar ×1 | Farm hydration point |
| Iron-reinforced wall | T4 | Cut Stone ×3, Iron Bar ×2 | High durability defense |
| Defensive platform | T4 | Plank ×4, Iron Bar ×2, Rope ×1 | Elevated lookout and combat point |
| Frost wall | T5 | Frostwood ×3, Frost Crystal ×1, Iron Bar ×1 | Cold-proof endgame wall |
| Crystal bridge | T5 | Frostwood ×4, Frost Crystal ×2, Iron Bar ×1 | Snow ravine traversal |
| Light post | T1 | Plank ×2, Resin ×1 | Holds torch or lantern |
| Wall lantern mount | T3 | Copper Bar ×1, Glass ×1 | Persistent area light |
| Small chest | T1 | Plank ×8, Iron Bar ×1 | 12-slot storage |
| Reinforced chest | T3 | Plank ×10, Copper Bar ×4, Iron Bar ×2 | 24-slot storage |
| Bedroll | T1 | Fiber ×6, Cloth ×2, Hide ×1 | Respawn point |
| Bed | T1 | Plank ×10, Cloth ×3, Hide ×2 | Respawn point and time skip |
| Farm plot | T1 | Plank ×2, Fiber ×2, Soil ×4 | Plant and grow crops |
| Cooking fire | T0 | Stone ×6, Branch ×4, Ember ×1 | Cook food and smoke meat |
| Workbench | T0 | Wood ×8, Stone ×4, Fiber ×4 | Unlocks structured crafting |
| Kiln | T3 | Stone ×12, Clay ×10, Brick ×2 | Refines clay, sand, and ore |
| Forge | T4 | Stone ×20, Brick ×8, Iron Bar ×6, Coal ×4 | Refines iron and advanced gear |
| Frost altar | T5 | Cut Stone ×8, Iron Bar ×4, Frost Crystal ×3, Claw ×2 | Endgame crafting and abilities |
| Waystone | T5 | Cut Stone ×6, Copper Bar ×2, Crystal ×1 | Fast travel and settlement link |

## Recommended implementation order

The Android prototype should implement the catalog in four slices. First, add the data-driven resource and recipe tables for handcraft, workbench, cooking fire, chests, bedrolls, torches, and T1 timber pieces. Second, add the kiln, loom, farming, desert pieces, lamps, and sand-processing recipes. Third, add the forge, iron structures, weapons, armor, and snow predator drops. Fourth, add the frost altar, endgame abilities, Frostspire pieces, and waystone.

A recipe should be represented by a stable ID, station ID, level requirement, biome requirement, ingredient list, output list, and optional effect payload. Building pieces should use a stable piece ID, tier, material family, snap rules, durability, placement footprint, and interaction tags such as `storage`, `rest`, `light`, `crafting`, or `farming`.

## Balance and originality guardrails

The first hour should let the player build a useful timber shelter without visiting every biome. Desert materials should provide processing and mobility advantages rather than simply larger damage numbers. Snow materials should be rare and dangerous to obtain; the first snow predator should have **100 HP**, clear attack telegraphs, and meaningful pelt/claw drops. No recipe should depend on copied names, exact item icons, recognizable UI, or ripped assets from another game.
