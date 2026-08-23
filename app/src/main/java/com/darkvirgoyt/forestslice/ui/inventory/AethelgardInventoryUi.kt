@file:OptIn(androidx.compose.foundation.ExperimentalFoundationApi::class)

package com.darkvirgoyt.forestslice.ui.inventory

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.itemsIndexed
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

private val AethelgardSurface = Color(0xFF0B191A)
private val AethelgardPanel = Color(0xFF102426)
private val AethelgardPanelRaised = Color(0xFF173234)
private val AethelgardGold = Color(0xFFF5C56B)
private val AethelgardIvory = Color(0xFFFFF4D6)
private val AethelgardMuted = Color(0xFFA7BDB7)
private val AethelgardCyan = Color(0xFF8BE0D8)
private val AethelgardError = Color(0xFFF08B8B)

/** Categories are deliberately stable because they are shared by the native snapshot and UI filters. */
enum class InventoryPanel {
    INVENTORY,
    EQUIPMENT,
    CRAFT
}

enum class InventoryCategory(val label: String) {
    ALL("ALL"),
    WEAPONS("WEAPONS"),
    ARMOR("ARMOR"),
    CONSUMABLES("CONSUMABLES"),
    MATERIALS("MATERIALS"),
    QUEST("QUEST"),
    FAVORITES("FAVORITES")
}

enum class ItemRarity(val label: String, val accent: Color) {
    COMMON("Common", Color(0xFFB8C5C1)),
    UNCOMMON("Uncommon", Color(0xFF8BD49E)),
    RARE("Rare", Color(0xFF73B7F2)),
    EPIC("Epic", Color(0xFFC293F3)),
    LEGENDARY("Legendary", Color(0xFFF3AD62))
}

enum class EquipmentSlot(val label: String) {
    MAIN_HAND("MAIN HAND"),
    OFF_HAND("OFF HAND"),
    HEAD("HEAD"),
    CHEST("CHEST"),
    HANDS("HANDS"),
    LEGS("LEGS"),
    FEET("FEET"),
    ACCESSORY_ONE("ACCESSORY 1"),
    ACCESSORY_TWO("ACCESSORY 2")
}

@Immutable
data class InventoryItem(
    val instanceId: String,
    val definitionId: String,
    val name: String,
    val category: InventoryCategory,
    val rarity: ItemRarity,
    val quantity: Int = 1,
    val iconGlyph: String = "?",
    val description: String = "",
    val isFavorite: Boolean = false,
    val isLocked: Boolean = false,
    val requiredLevel: Int = 1,
    val equipSlot: EquipmentSlot? = null,
    val power: Int? = null,
    val effectSummary: String? = null
)

@Immutable
data class EquipmentState(
    val equipped: Map<EquipmentSlot, InventoryItem?> = emptyMap()
) {
    fun itemAt(slot: EquipmentSlot): InventoryItem? = equipped[slot]
}

@Composable
fun AethelgardInventoryScreen(
    inventory: List<InventoryItem?>,
    equipment: EquipmentState,
    playerLevel: Int,
    selectedItemId: String?,
    activeCategory: InventoryCategory = InventoryCategory.ALL,
    activePanel: InventoryPanel = InventoryPanel.INVENTORY,
    onPanelSelected: (InventoryPanel) -> Unit = {},
    onCategorySelected: (InventoryCategory) -> Unit,
    onItemSelected: (InventoryItem) -> Unit,
    onItemLongPressed: (InventoryItem) -> Unit,
    onEquipRequested: (InventoryItem) -> Unit,
    onUnequipRequested: (EquipmentSlot) -> Unit,
    onClose: () -> Unit,
    modifier: Modifier = Modifier
) {
    val selectedItem = inventory.asSequence()
        .filterNotNull()
        .firstOrNull { it.instanceId == selectedItemId }

    Surface(
        modifier = modifier.fillMaxSize(),
        color = AethelgardSurface,
        contentColor = AethelgardIvory
    ) {
        Column(modifier = Modifier.fillMaxSize().padding(horizontal = 20.dp, vertical = 14.dp)) {
            InventoryHeader(activePanel = activePanel, onPanelSelected = onPanelSelected, onClose = onClose)
            Spacer(Modifier.height(12.dp))
            when (activePanel) {
                InventoryPanel.INVENTORY -> Row(
                    modifier = Modifier.weight(1f).fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(14.dp)
                ) {
                    FilterRail(
                        activeCategory = activeCategory,
                        inventory = inventory,
                        onCategorySelected = onCategorySelected,
                        modifier = Modifier.width(122.dp).fillMaxHeight()
                    )
                    BackpackGrid(
                        inventory = inventory,
                        activeCategory = activeCategory,
                        selectedItemId = selectedItemId,
                        onItemSelected = onItemSelected,
                        onItemLongPressed = onItemLongPressed,
                        modifier = Modifier.weight(1.15f).fillMaxHeight()
                    )
                    ItemDetailPanel(
                        item = selectedItem,
                        playerLevel = playerLevel,
                        onEquipRequested = onEquipRequested,
                        modifier = Modifier.weight(0.85f).fillMaxHeight()
                    )
                }
                InventoryPanel.EQUIPMENT -> EquipmentPaperDoll(
                    equipment = equipment,
                    onSlotSelected = { slot -> equipment.itemAt(slot)?.let(onItemSelected) },
                    onUnequipRequested = onUnequipRequested,
                    modifier = Modifier.weight(1f).fillMaxWidth()
                )
                InventoryPanel.CRAFT -> CraftPanel(modifier = Modifier.weight(1f).fillMaxWidth())
            }
            Spacer(Modifier.height(10.dp))
            InventoryFooter(playerLevel = playerLevel, inventory = inventory)
        }
    }
}

@Composable
private fun InventoryHeader(activePanel: InventoryPanel, onPanelSelected: (InventoryPanel) -> Unit, onClose: () -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().height(46.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(
                text = "AETHELGARD",
                color = AethelgardGold,
                fontSize = 17.sp,
                fontWeight = FontWeight.Bold
            )
            Text("  /  INVENTORY", color = AethelgardMuted, fontSize = 14.sp)
        }
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), verticalAlignment = Alignment.CenterVertically) {
            MenuTab(label = "INVENTORY", selected = activePanel == InventoryPanel.INVENTORY, onClick = { onPanelSelected(InventoryPanel.INVENTORY) })
            MenuTab(label = "EQUIPMENT", selected = activePanel == InventoryPanel.EQUIPMENT, onClick = { onPanelSelected(InventoryPanel.EQUIPMENT) })
            MenuTab(label = "CRAFT", selected = activePanel == InventoryPanel.CRAFT, onClick = { onPanelSelected(InventoryPanel.CRAFT) })
            TextButtonSurface(label = "CLOSE", onClick = onClose)
        }
    }
}

@Composable
private fun MenuTab(label: String, selected: Boolean, onClick: () -> Unit) {
    Surface(
        modifier = Modifier.combinedClickable(onClick = onClick),
        color = if (selected) AethelgardPanelRaised else Color.Transparent,
        shape = RoundedCornerShape(8.dp)
    ) {
        Column(
            modifier = Modifier.padding(horizontal = 10.dp, vertical = 7.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(label, color = if (selected) AethelgardGold else AethelgardMuted, fontSize = 11.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.height(3.dp))
            Box(Modifier.width(42.dp).height(2.dp).background(if (selected) AethelgardGold else Color.Transparent))
        }
    }
}

@Composable
private fun TextButtonSurface(label: String, onClick: () -> Unit) {
    Surface(
        modifier = Modifier
            .height(38.dp)
            .combinedClickable(onClick = onClick)
            .semantics { contentDescription = label },
        color = AethelgardPanelRaised,
        shape = RoundedCornerShape(8.dp)
    ) {
        Box(Modifier.padding(horizontal = 14.dp), contentAlignment = Alignment.Center) {
            Text(label, color = AethelgardIvory, fontSize = 11.sp, fontWeight = FontWeight.Bold)
        }
    }
}

@Composable
private fun FilterRail(
    activeCategory: InventoryCategory,
    inventory: List<InventoryItem?>,
    onCategorySelected: (InventoryCategory) -> Unit,
    modifier: Modifier = Modifier
) {
    Surface(modifier = modifier, color = AethelgardPanel, shape = RoundedCornerShape(12.dp)) {
        Column(modifier = Modifier.padding(vertical = 10.dp)) {
            Text("FILTERS", color = AethelgardMuted, fontSize = 11.sp, fontWeight = FontWeight.Bold, modifier = Modifier.padding(horizontal = 12.dp, vertical = 8.dp))
            InventoryCategory.entries.forEach { category ->
                val count = if (category == InventoryCategory.ALL) {
                    inventory.count { it != null }
                } else {
                    inventory.count { item -> item != null && (item.category == category || (category == InventoryCategory.FAVORITES && item.isFavorite)) }
                }
                FilterRow(
                    category = category,
                    count = count,
                    selected = activeCategory == category,
                    onClick = { onCategorySelected(category) }
                )
            }
        }
    }
}

@Composable
private fun FilterRow(category: InventoryCategory, count: Int, selected: Boolean, onClick: () -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(42.dp)
            .combinedClickable(onClick = onClick)
            .semantics { contentDescription = "${category.label} filter, $count items" }
            .background(if (selected) AethelgardPanelRaised else Color.Transparent)
            .padding(horizontal = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(category.label, color = if (selected) AethelgardGold else AethelgardIvory, fontSize = 10.sp, fontWeight = if (selected) FontWeight.Bold else FontWeight.Normal)
        Text(count.toString(), color = AethelgardMuted, fontSize = 10.sp)
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun BackpackGrid(
    inventory: List<InventoryItem?>,
    activeCategory: InventoryCategory,
    selectedItemId: String?,
    onItemSelected: (InventoryItem) -> Unit,
    onItemLongPressed: (InventoryItem) -> Unit,
    modifier: Modifier = Modifier
) {
    val slots = remember(inventory, activeCategory) {
        val visibleItems = inventory.map { item ->
            item?.takeIf { activeCategory == InventoryCategory.ALL || it.category == activeCategory || (activeCategory == InventoryCategory.FAVORITES && it.isFavorite) }
        }
        List(24) { index -> visibleItems.getOrNull(index) }
    }

    Surface(modifier = modifier, color = AethelgardPanel, shape = RoundedCornerShape(12.dp)) {
        Column(modifier = Modifier.fillMaxSize().padding(12.dp)) {
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically) {
                Text("BACKPACK  ${slots.count { it != null }} / 24", color = AethelgardIvory, fontSize = 13.sp, fontWeight = FontWeight.Bold)
                Text("SORT: RECOMMENDED", color = AethelgardMuted, fontSize = 10.sp)
            }
            Spacer(Modifier.height(10.dp))
            BoxWithConstraints(modifier = Modifier.weight(1f).fillMaxWidth()) {
                val columns = if (maxWidth >= 520.dp) 6 else 4
                LazyVerticalGrid(
                    columns = GridCells.Fixed(columns),
                    modifier = Modifier.fillMaxSize(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    itemsIndexed(
                        items = slots,
                        key = { index, item -> item?.instanceId ?: "empty-slot-$index" }
                    ) { _, item ->
                        InventorySlot(
                            item = item,
                            selected = item?.instanceId == selectedItemId,
                            onClick = { item?.let(onItemSelected) },
                            onLongClick = { item?.let(onItemLongPressed) }
                        )
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun InventorySlot(
    item: InventoryItem?,
    selected: Boolean,
    onClick: () -> Unit,
    onLongClick: () -> Unit
) {
    val borderColor = when {
        selected -> AethelgardGold
        item != null -> item.rarity.accent.copy(alpha = 0.72f)
        else -> AethelgardMuted.copy(alpha = 0.18f)
    }
    val description = item?.let {
        buildString {
            append("${it.name}, ${it.rarity.label} ${it.category.label.lowercase()}, quantity ${it.quantity}")
            if (it.isFavorite) append(", favorite")
            if (it.isLocked) append(", locked")
        }
    } ?: "Empty backpack slot"

    Surface(
        modifier = Modifier
            .fillMaxWidth()
            .height(68.dp)
            .border(width = if (selected) 2.dp else 1.dp, color = borderColor, shape = RoundedCornerShape(8.dp))
            .combinedClickable(onClick = onClick, onLongClick = onLongClick)
            .semantics { contentDescription = description },
        color = if (selected) AethelgardPanelRaised else Color(0xFF0E2021),
        shape = RoundedCornerShape(8.dp)
    ) {
        Box(Modifier.fillMaxSize().padding(6.dp)) {
            if (item == null) {
                Text("—", modifier = Modifier.align(Alignment.Center), color = AethelgardMuted.copy(alpha = 0.32f), fontSize = 18.sp)
            } else {
                Text(item.iconGlyph, modifier = Modifier.align(Alignment.Center), color = AethelgardIvory, fontSize = 20.sp, fontWeight = FontWeight.Bold)
                Text(item.quantity.toString(), modifier = Modifier.align(Alignment.BottomEnd), color = AethelgardIvory, fontSize = 10.sp, fontWeight = FontWeight.Bold)
                if (item.isFavorite) Text("★", modifier = Modifier.align(Alignment.TopStart), color = AethelgardGold, fontSize = 10.sp)
                if (item.isLocked) Text("L", modifier = Modifier.align(Alignment.TopEnd), color = AethelgardMuted, fontSize = 9.sp, fontWeight = FontWeight.Bold)
            }
        }
    }
}

@Composable
private fun ItemDetailPanel(
    item: InventoryItem?,
    playerLevel: Int,
    onEquipRequested: (InventoryItem) -> Unit,
    modifier: Modifier = Modifier
) {
    Surface(modifier = modifier, color = AethelgardPanel, shape = RoundedCornerShape(12.dp)) {
        if (item == null) {
            Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Text("SELECT AN ITEM", color = AethelgardMuted, fontSize = 12.sp, fontWeight = FontWeight.Bold)
            }
        } else {
            Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
                Text(item.name, color = AethelgardIvory, fontSize = 16.sp, fontWeight = FontWeight.Bold)
                Text("${item.rarity.label}  •  ${item.category.label.lowercase()}", color = item.rarity.accent, fontSize = 11.sp)
                Spacer(Modifier.height(16.dp))
                Text(item.description.ifBlank { "No description available." }, color = AethelgardMuted, fontSize = 12.sp, lineHeight = 17.sp)
                Spacer(Modifier.height(14.dp))
                item.power?.let { StatLine(label = "POWER", value = it.toString(), positive = true) }
                item.effectSummary?.let { effect ->
                    Spacer(Modifier.height(8.dp))
                    Text("EFFECT", color = AethelgardGold, fontSize = 10.sp, fontWeight = FontWeight.Bold)
                    Text(effect, color = AethelgardIvory, fontSize = 12.sp, lineHeight = 17.sp)
                }
                if (item.requiredLevel > playerLevel) {
                    Spacer(Modifier.height(12.dp))
                    Text("REQUIRES LEVEL ${item.requiredLevel}", color = AethelgardError, fontSize = 10.sp, fontWeight = FontWeight.Bold)
                }
                Spacer(Modifier.weight(1f))
                val canEquip = item.equipSlot != null && item.requiredLevel <= playerLevel && !item.isLocked
                DetailActionButton(
                    label = if (canEquip) "EQUIP" else "EQUIP — UNAVAILABLE",
                    enabled = canEquip,
                    onClick = { onEquipRequested(item) },
                    modifier = Modifier.fillMaxWidth()
                )
                Spacer(Modifier.height(8.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
                    DetailActionButton(label = if (item.isFavorite) "UNFAVORITE" else "FAVORITE", enabled = true, onClick = {}, modifier = Modifier.weight(1f))
                    DetailActionButton(label = if (item.isLocked) "UNLOCK" else "LOCK", enabled = true, onClick = {}, modifier = Modifier.weight(1f))
                }
            }
        }
    }
}

@Composable
private fun StatLine(label: String, value: String, positive: Boolean) {
    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(label, color = AethelgardMuted, fontSize = 10.sp)
        Text("$value  ${if (positive) "▲" else "▼"}", color = if (positive) AethelgardCyan else AethelgardError, fontSize = 11.sp, fontWeight = FontWeight.Bold)
    }
}

@Composable
private fun DetailActionButton(label: String, enabled: Boolean, onClick: () -> Unit, modifier: Modifier = Modifier) {
    Surface(
        modifier = modifier
            .height(38.dp)
            .combinedClickable(enabled = enabled, onClick = onClick)
            .semantics { contentDescription = label },
        color = if (enabled) AethelgardGold else AethelgardPanelRaised,
        contentColor = if (enabled) AethelgardSurface else AethelgardMuted,
        shape = RoundedCornerShape(7.dp)
    ) {
        Box(contentAlignment = Alignment.Center) {
            Text(label, fontSize = 10.sp, fontWeight = FontWeight.Bold, textAlign = TextAlign.Center)
        }
    }
}

@Composable
fun EquipmentPaperDoll(
    equipment: EquipmentState,
    onSlotSelected: (EquipmentSlot) -> Unit,
    onUnequipRequested: (EquipmentSlot) -> Unit,
    modifier: Modifier = Modifier
) {
    Surface(modifier = modifier, color = AethelgardPanel, shape = RoundedCornerShape(12.dp)) {
        Column(modifier = Modifier.fillMaxSize().padding(14.dp)) {
            Text("EQUIPMENT", color = AethelgardIvory, fontSize = 13.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.height(10.dp))
            Row(modifier = Modifier.weight(1f).fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                EquipmentColumn(
                    slots = listOf(EquipmentSlot.HEAD, EquipmentSlot.CHEST, EquipmentSlot.HANDS),
                    equipment = equipment,
                    onSlotSelected = onSlotSelected,
                    onUnequipRequested = onUnequipRequested,
                    modifier = Modifier.weight(1f).fillMaxHeight()
                )
                HeroSilhouette(modifier = Modifier.weight(0.82f).fillMaxHeight())
                EquipmentColumn(
                    slots = listOf(EquipmentSlot.LEGS, EquipmentSlot.FEET, EquipmentSlot.ACCESSORY_ONE, EquipmentSlot.ACCESSORY_TWO),
                    equipment = equipment,
                    onSlotSelected = onSlotSelected,
                    onUnequipRequested = onUnequipRequested,
                    modifier = Modifier.weight(1f).fillMaxHeight()
                )
            }
            Spacer(Modifier.height(8.dp))
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
                EquipmentSlotButton(
                    slot = EquipmentSlot.MAIN_HAND,
                    item = equipment.itemAt(EquipmentSlot.MAIN_HAND),
                    onClick = { onSlotSelected(EquipmentSlot.MAIN_HAND) },
                    onUnequip = { onUnequipRequested(EquipmentSlot.MAIN_HAND) },
                    modifier = Modifier.weight(1f)
                )
                EquipmentSlotButton(
                    slot = EquipmentSlot.OFF_HAND,
                    item = equipment.itemAt(EquipmentSlot.OFF_HAND),
                    onClick = { onSlotSelected(EquipmentSlot.OFF_HAND) },
                    onUnequip = { onUnequipRequested(EquipmentSlot.OFF_HAND) },
                    modifier = Modifier.weight(1f)
                )
            }
        }
    }
}

@Composable
private fun EquipmentColumn(
    slots: List<EquipmentSlot>,
    equipment: EquipmentState,
    onSlotSelected: (EquipmentSlot) -> Unit,
    onUnequipRequested: (EquipmentSlot) -> Unit,
    modifier: Modifier = Modifier
) {
    Column(modifier = modifier, verticalArrangement = Arrangement.spacedBy(8.dp)) {
        slots.forEach { slot ->
            EquipmentSlotButton(
                slot = slot,
                item = equipment.itemAt(slot),
                onClick = { onSlotSelected(slot) },
                onUnequip = { onUnequipRequested(slot) },
                modifier = Modifier.fillMaxWidth().weight(1f)
            )
        }
    }
}

@Composable
private fun EquipmentSlotButton(
    slot: EquipmentSlot,
    item: InventoryItem?,
    onClick: () -> Unit,
    onUnequip: () -> Unit,
    modifier: Modifier = Modifier
) {
    val description = item?.let { "${slot.label}, equipped ${it.name}" } ?: "${slot.label}, empty"
    Surface(
        modifier = modifier
            .border(1.dp, item?.rarity?.accent?.copy(alpha = 0.75f) ?: AethelgardMuted.copy(alpha = 0.28f), RoundedCornerShape(8.dp))
            .combinedClickable(onClick = onClick, onLongClick = onUnequip)
            .semantics { contentDescription = description },
        color = AethelgardPanelRaised,
        shape = RoundedCornerShape(8.dp)
    ) {
        Column(modifier = Modifier.fillMaxSize().padding(8.dp), verticalArrangement = Arrangement.Center) {
            Text(slot.label, color = AethelgardMuted, fontSize = 9.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.height(5.dp))
            if (item == null) {
                Text("EMPTY", color = AethelgardMuted.copy(alpha = 0.6f), fontSize = 10.sp)
            } else {
                Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    Text(item.iconGlyph, color = AethelgardIvory, fontSize = 18.sp, fontWeight = FontWeight.Bold)
                    Column {
                        Text(item.name, color = AethelgardIvory, fontSize = 10.sp, fontWeight = FontWeight.Bold, maxLines = 1)
                        item.power?.let { Text("POWER $it", color = AethelgardCyan, fontSize = 9.sp) }
                    }
                }
            }
        }
    }
}

@Composable
private fun HeroSilhouette(modifier: Modifier = Modifier) {
    Box(modifier = modifier, contentAlignment = Alignment.Center) {
        Column(horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Box(Modifier.size(54.dp).background(Color(0xFF422B43), RoundedCornerShape(50)))
            Box(Modifier.width(78.dp).height(110.dp).background(Color(0xFF5D2946), RoundedCornerShape(34.dp, 34.dp, 16.dp, 16.dp)))
            Box(Modifier.width(50.dp).height(72.dp).background(Color(0xFF3A2B45), RoundedCornerShape(18.dp)))
            Text("HERO", color = AethelgardGold, fontSize = 10.sp, fontWeight = FontWeight.Bold)
        }
    }
}

@Composable
@Composable
private fun CraftPanel(modifier: Modifier = Modifier) {
    Surface(modifier = modifier, color = AethelgardPanel, shape = RoundedCornerShape(12.dp)) {
        Column(modifier = Modifier.fillMaxSize().padding(24.dp), verticalArrangement = Arrangement.Center, horizontalAlignment = Alignment.CenterHorizontally) {
            Text("CRAFTING BENCH", color = AethelgardGold, fontSize = 16.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.height(8.dp))
            Text("Select a recipe from the field HUD to craft with your gathered materials.", color = AethelgardMuted, fontSize = 12.sp, textAlign = TextAlign.Center)
        }
    }
}

@Composable
private fun InventoryFooter(playerLevel: Int, inventory: List<InventoryItem?>) {
    Row(
        modifier = Modifier.fillMaxWidth().height(30.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text("HP 100  |  STA 82  |  LV $playerLevel", color = AethelgardIvory, fontSize = 11.sp)
        HorizontalDivider(modifier = Modifier.width(24.dp), color = Color.Transparent)
        Text("Carry slots  ${inventory.count { it != null }} / 24", color = AethelgardMuted, fontSize = 11.sp)
        Text("THE FIRST EMBER", color = AethelgardGold, fontSize = 11.sp, fontWeight = FontWeight.Bold)
    }
}

private fun sampleItems(): List<InventoryItem?> = listOf(
    InventoryItem("saber-01", "weapon.ironbloom_saber", "Ironbloom Saber", InventoryCategory.WEAPONS, ItemRarity.RARE, iconGlyph = "S", description = "A light saber that carries the warmth of the first ember.", equipSlot = EquipmentSlot.MAIN_HAND, power = 18, effectSummary = "Ember Edge: basic attacks emit a warm-light hit flash."),
    InventoryItem("tonic-01", "consumable.moonleaf_tonic", "Moonleaf Tonic", InventoryCategory.CONSUMABLES, ItemRarity.RARE, quantity = 3, iconGlyph = "T", description = "Restores 25 health.", effectSummary = "Restore 25 HP."),
    InventoryItem("wood-01", "material.wood", "Wood", InventoryCategory.MATERIALS, ItemRarity.COMMON, quantity = 12, iconGlyph = "W", description = "Dry timber gathered from the forest edge."),
    InventoryItem("fiber-01", "material.fiber", "Fiber", InventoryCategory.MATERIALS, ItemRarity.COMMON, quantity = 8, iconGlyph = "F", description = "Strong forest fiber used for bindings.", isFavorite = true),
    InventoryItem("chest-01", "armor.wanderer_chest", "Wanderer Chest", InventoryCategory.ARMOR, ItemRarity.UNCOMMON, iconGlyph = "C", description = "A practical chest piece for long expeditions.", equipSlot = EquipmentSlot.CHEST, power = 8),
    InventoryItem("ember-01", "quest.ember_kit", "Ember Kit", InventoryCategory.QUEST, ItemRarity.EPIC, iconGlyph = "E", description = "The key to awakening the campfire shrine.", isLocked = true)
) + List(18) { null }

@Preview(widthDp = 1100, heightDp = 650, showBackground = true)
@Composable
private fun AethelgardInventoryPreview() {
    val items = remember { sampleItems() }
    var selectedId by remember { mutableStateOf(items.firstOrNull()?.instanceId) }
    var activeCategory by remember { mutableStateOf(InventoryCategory.ALL) }
    MaterialTheme {
        AethelgardInventoryScreen(
            inventory = items,
            equipment = EquipmentState(mapOf(EquipmentSlot.MAIN_HAND to items[0], EquipmentSlot.CHEST to items[4])),
            playerLevel = 2,
            selectedItemId = selectedId,
            activeCategory = activeCategory,
            activePanel = InventoryPanel.INVENTORY,
            onPanelSelected = {},
            onCategorySelected = { activeCategory = it },
            onItemSelected = { selectedId = it.instanceId },
            onItemLongPressed = { selectedId = it.instanceId },
            onEquipRequested = { selectedId = it.instanceId },
            onUnequipRequested = {},
            onClose = {}
        )
    }
}

@Preview(widthDp = 520, heightDp = 650, showBackground = true)
@Composable
private fun AethelgardEquipmentPreview() {
    val items = remember { sampleItems() }
    MaterialTheme {
        EquipmentPaperDoll(
            equipment = EquipmentState(mapOf(EquipmentSlot.MAIN_HAND to items[0], EquipmentSlot.CHEST to items[4])),
            onSlotSelected = {},
            onUnequipRequested = {}
        )
    }
}
