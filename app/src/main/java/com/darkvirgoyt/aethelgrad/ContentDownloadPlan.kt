package com.darkvirgoyt.aethelgrad

/**
 * Packaging plan for future cooked 3D Aethelgard content. Its size budgets are never
 * player-facing download facts; the client displays only bytes from a signed archive or Play.
 */
object ContentDownloadPlan {
    data class QualityEnvelope(
        val id: String,
        val textureLabel: String,
        val foliageDensity: Int,
        val effectScalePercent: Int,
        val shadowQuality: String,
        val waterQuality: String,
        val requiresDownloadedContent: Boolean
    )

    private val highQualityEnvelope = QualityEnvelope(
        id = "cinematic-high",
        textureLabel = "high-resolution PBR",
        foliageDensity = 100,
        effectScalePercent = 140,
        shadowQuality = "full mobile dynamic shadows",
        waterQuality = "layered river, foam, and reflection accents",
        requiresDownloadedContent = true
    )

    fun qualityEnvelopeFor(tier: ResourceTier): QualityEnvelope = highQualityEnvelope

    enum class ResourceTier(
        val graphicsTierIndex: Int,
        val description: String
    ) {
        HIGH(
            graphicsTierIndex = 4,
            description = "High graphics are available only after a measured, signed cooked archive or Play Asset Delivery release is published for this APK."
        )
    }

    enum class WorldSector(val bit: Int, val label: String) {
        SAND(1 shl 1, "SAND FRONTIER"),
        SNOW(1 shl 2, "SNOW FRONTIER"),
        DUNGEON(1 shl 3, "ROOT DUNGEON")
    }

    data class Pack(
        val playPackName: String,
        val targetMiB: Int,
        val contents: String,
        val requiredBeforeStart: Boolean = true,
        val sector: WorldSector? = null
    )

    val packs = listOf(
        Pack("assetpack_graphics_base", 450, "compiled materials, base shaders, shared meshes, mobile render resources"),
        Pack("assetpack_forest", 350, "forest launch region, village, foliage, water, collision, navigation"),
        Pack("assetpack_characters", 500, "heroes, NPCs, animals, enemies, rigs, animation bindings"),
        Pack("assetpack_shaders_gles", 250, "compiled OpenGL ES shader libraries and pipeline state resources"),
        Pack("assetpack_world_streaming", 400, "world partition descriptors, streamed sublevels, nav data for all world sectors"),
        Pack("assetpack_terrain_lod", 425, "terrain heightfields, landscape LODs, virtual shadow maps"),
        Pack("assetpack_animation_sets", 425, "locomotion, combat, traversal, emotes, montage sections"),
        Pack("assetpack_sand", 400, "sand biome terrain, settlements, rocks, foliage, weather", sector = WorldSector.SAND),
        Pack("assetpack_snow", 400, "snow biome terrain, caves, ice materials, weather", sector = WorldSector.SNOW),
        Pack("assetpack_dungeons", 400, "dungeon cells, props, traps, encounter data, lighting data", sector = WorldSector.DUNGEON),
        Pack("assetpack_hd_textures", 500, "high-resolution PBR textures, virtual-texture pages, decals", sector = WorldSector.SAND),
        Pack("assetpack_foliage_lods", 400, "foliage clusters, impostors, mobile LODs", sector = WorldSector.SAND),
        Pack("assetpack_audio_hd", 450, "music, ambience, combat sounds, wildlife and high-quality mixes", sector = WorldSector.SNOW),
        Pack("assetpack_vfx", 300, "Niagara systems, weather effects, impact effects", sector = WorldSector.DUNGEON),
        Pack("assetpack_cinematics", 450, "story scenes, sequences and camera animation data", sector = WorldSector.DUNGEON),
        Pack("assetpack_voice", 250, "dialogue, localization voice banks, subtitles metadata", sector = WorldSector.DUNGEON),
        Pack("assetpack_shaders_vulkan", 300, "compiled Vulkan shader libraries and pipeline state resources", sector = WorldSector.DUNGEON),
        Pack("assetpack_pipeline_cache", 100, "device-safe pipeline cache seeds and shader warm-up data", sector = WorldSector.DUNGEON)
    )

    /** Planning-only pack group; the runtime never reports these target sizes as installed bytes. */
    fun packsFor(tier: ResourceTier): List<Pack> = packs

    fun packNamesFor(tier: ResourceTier): List<String> = packsFor(tier).map { it.playPackName }

    fun startupPacksFor(tier: ResourceTier): List<Pack> = packsFor(tier).filter { it.requiredBeforeStart }

    fun startupPackNamesFor(tier: ResourceTier): List<String> = startupPacksFor(tier).map { it.playPackName }

    fun packsForSector(tier: ResourceTier, sector: WorldSector): List<Pack> = packsFor(tier)
        .filter { it.sector == sector }

    fun packNamesForSector(tier: ResourceTier, sector: WorldSector): List<String> = packsForSector(tier, sector).map { it.playPackName }

    fun totalMiBFor(tier: ResourceTier): Int = packsFor(tier).sumOf { it.targetMiB }

    fun startupMiBFor(tier: ResourceTier): Int = startupPacksFor(tier).sumOf { it.targetMiB }

    fun sectorMiBFor(tier: ResourceTier, sector: WorldSector): Int = packsForSector(tier, sector).sumOf { it.targetMiB }

    fun totalGiBLabelFor(tier: ResourceTier): String = "%.1f GB".format(totalMiBFor(tier) / 1024.0)

    val totalMiB: Int = packs.sumOf { it.targetMiB }
    val requiredMiB: Int = packs.filter { it.requiredBeforeStart }.sumOf { it.targetMiB }
    // Reserve headroom for the required high-end archive and safe updates.
    val minimumFreeSpaceMiB: Int = totalMiB + 128
    val totalGiBLabel: String = "%.1f GB".format(totalMiB / 1024.0)
    val summary: String = packs.joinToString("  •  ") { "${it.playPackName}: ${it.targetMiB} MB" }
    val requiredPackNames: List<String> = startupPackNamesFor(ResourceTier.HIGH)
}
