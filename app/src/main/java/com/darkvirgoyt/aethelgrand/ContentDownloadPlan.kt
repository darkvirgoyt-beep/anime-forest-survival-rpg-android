package com.darkvirgoyt.aethelgrand

/**
 * Optional one-gibibyte runtime-content envelope for the cooked 3D Aethelgard
 * experience. The bundled renderer always provides a playable launch world;
 * this archive refines the game with licensed authored content after entry.
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
        requiresDownloadedContent = BuildConfig.FULL_CONTENT_BUILD
    )

    fun qualityEnvelopeFor(tier: ResourceTier): QualityEnvelope = highQualityEnvelope

    enum class ResourceTier(
        val storageLabel: String,
        val graphicsTierIndex: Int,
        val description: String
    ) {
        HIGH(
            storageLabel = "1.0 GB",
            graphicsTierIndex = 4,
            description = "Optional authored world sectors, characters, textures, foliage, shaders, VFX, audio, cinematics, and animation. The bundled world remains playable while this package is unavailable."
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
        val requiredBeforeStart: Boolean = false,
        val sector: WorldSector? = null
    )

    val packs = listOf(
        Pack("assetpack_graphics_base", 64, "compiled materials, base shaders, shared meshes, mobile render resources"),
        Pack("assetpack_forest", 90, "forest launch region, village, foliage, water, collision, navigation"),
        Pack("assetpack_characters", 140, "heroes, NPCs, animals, enemies, rigs, animation bindings"),
        Pack("assetpack_shaders_gles", 20, "compiled OpenGL ES shader libraries and pipeline state resources"),
        Pack("assetpack_world_streaming", 45, "world partition descriptors, streamed sublevels, nav data for all world sectors"),
        Pack("assetpack_terrain_lod", 55, "terrain heightfields, landscape LODs, virtual shadow maps"),
        Pack("assetpack_animation_sets", 60, "locomotion, combat, traversal, emotes, montage sections"),
        Pack("assetpack_sand", 60, "sand biome terrain, settlements, rocks, foliage, weather", sector = WorldSector.SAND),
        Pack("assetpack_snow", 60, "snow biome terrain, caves, ice materials, weather", sector = WorldSector.SNOW),
        Pack("assetpack_dungeons", 60, "dungeon cells, props, traps, encounter data, lighting data", sector = WorldSector.DUNGEON),
        Pack("assetpack_hd_textures", 110, "high-resolution PBR textures, virtual-texture pages, decals", sector = WorldSector.SAND),
        Pack("assetpack_foliage_lods", 35, "foliage clusters, impostors, mobile LODs", sector = WorldSector.SAND),
        Pack("assetpack_audio_hd", 80, "music, ambience, combat sounds, wildlife and high-quality mixes", sector = WorldSector.SNOW),
        Pack("assetpack_vfx", 40, "Niagara systems, weather effects, impact effects", sector = WorldSector.DUNGEON),
        Pack("assetpack_cinematics", 50, "story scenes, sequences and camera animation data", sector = WorldSector.DUNGEON),
        Pack("assetpack_voice", 20, "dialogue, localization voice banks, subtitles metadata", sector = WorldSector.DUNGEON),
        Pack("assetpack_shaders_vulkan", 25, "compiled Vulkan shader libraries and pipeline state resources", sector = WorldSector.DUNGEON),
        Pack("assetpack_pipeline_cache", 10, "device-safe pipeline cache seeds and shader warm-up data", sector = WorldSector.DUNGEON)
    )

    /** Returns the complete physical pack set for the selected resource tier. */
    fun packsFor(tier: ResourceTier): List<Pack> = packs

    fun packNamesFor(tier: ResourceTier): List<String> = packsFor(tier).map { it.playPackName }

    fun startupPacksFor(tier: ResourceTier): List<Pack> = if (BuildConfig.FULL_CONTENT_BUILD) {
        packsFor(tier)
    } else {
        packsFor(tier).filter { it.requiredBeforeStart }
    }

    fun startupPackNamesFor(tier: ResourceTier): List<String> = startupPacksFor(tier).map { it.playPackName }

    fun fullContentPackNamesFor(tier: ResourceTier): List<String> = packsFor(tier).map { it.playPackName }

    fun packsForSector(tier: ResourceTier, sector: WorldSector): List<Pack> = packsFor(tier)
        .filter { it.sector == sector }

    fun packNamesForSector(tier: ResourceTier, sector: WorldSector): List<String> = packsForSector(tier, sector).map { it.playPackName }

    fun totalMiBFor(tier: ResourceTier): Int = packsFor(tier).sumOf { it.targetMiB }

    fun startupMiBFor(tier: ResourceTier): Int = startupPacksFor(tier).sumOf { it.targetMiB }

    fun sectorMiBFor(tier: ResourceTier, sector: WorldSector): Int = packsForSector(tier, sector).sumOf { it.targetMiB }

    fun totalGiBLabelFor(tier: ResourceTier): String = "%.1f GB".format(totalMiBFor(tier) / 1024.0)

    val totalMiB: Int = packs.sumOf { it.targetMiB }
    val requiredMiB: Int = packs.filter { it.requiredBeforeStart }.sumOf { it.targetMiB }
    // Reserve modest headroom for the optional one-gibibyte archive and safe updates.
    val minimumFreeSpaceMiB: Int = totalMiB + 128
    val totalGiBLabel: String = "%.1f GB".format(totalMiB / 1024.0)
    val summary: String = packs.joinToString("  •  ") { "${it.playPackName}: ${it.targetMiB} MB" }
    val requiredPackNames: List<String> = startupPackNamesFor(ResourceTier.HIGH)
    val fullContentRequired: Boolean = BuildConfig.FULL_CONTENT_BUILD
}
