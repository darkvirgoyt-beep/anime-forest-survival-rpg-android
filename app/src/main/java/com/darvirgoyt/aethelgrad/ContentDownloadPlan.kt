package com.darvirgoyt.aethelgrad

/**
 * Production download envelope for the full 3D Aethelgard experience.
 *
 * The checked-in prototype uses tiny deterministic bundles so CI and offline
 * development stay practical. These values are the target CDN/Play Asset
 * Delivery budget for the real cooked content, not padding added to the APK.
 */
object ContentDownloadPlan {
    data class Pack(
        val name: String,
        val targetMiB: Int,
        val contents: String
    )

    val packs = listOf(
        Pack("Forest launch region", 1_700, "terrain, village, foliage, water, collision, navigation"),
        Pack("Characters and creatures", 1_250, "heroes, NPCs, animals, enemies, rigs, animations"),
        Pack("Materials and lighting", 1_150, "PBR textures, shaders, LODs, device-quality variants"),
        Pack("Combat, VFX, and world simulation", 900, "combat effects, weather, particles, AI and world data"),
        Pack("Audio and voices", 850, "music, ambience, combat sounds, wildlife and dialogue"),
        Pack("Cinematics and optional regions", 900, "story scenes, dungeons, sand/snow extensions and cinematics")
    )

    val totalMiB: Int = packs.sumOf { it.targetMiB }
    val totalGiBLabel: String = "%.1f GB".format(totalMiB / 1024.0)
    val summary: String = packs.joinToString("  •  ") { "${it.name}: ${it.targetMiB} MB" }
}
