package com.darkvirgoyt.aethelgrad

/**
 * Staged packaging plan for AETHELGRAD content. Size budgets are planning targets,
 * never claims about installed bytes; runtime progress comes only from measured payloads.
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

    private val stageOneEnvelope = QualityEnvelope(
        id = "mobile-stage-1",
        textureLabel = "authored forest mobile",
        foliageDensity = 60,
        effectScalePercent = 90,
        shadowQuality = "mobile dynamic shadows",
        waterQuality = "river surface and foam accents",
        requiresDownloadedContent = false
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

    fun qualityEnvelopeFor(tier: ResourceTier): QualityEnvelope =
        if (tier == ResourceTier.HIGH) highQualityEnvelope else stageOneEnvelope

    enum class ResourceTier(
        val graphicsTierIndex: Int,
        val description: String
    ) {
        STAGE_1(
            graphicsTierIndex = 2,
            description = "Stage 1 targets a 1 GiB authored forest release budget. Only measured launch-slice bytes are used; future content is added in later packs."
        ),
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

    private val stageOnePacks = listOf(
        Pack("assetpack_core", 96, "bootstrap UI, online authentication, launch scene and content catalog"),
        Pack("assetpack_graphics_base", 160, "mobile forest materials, base shaders, shared meshes and renderer resources"),
        Pack("assetpack_forest", 160, "forest launch region, water, collision and navigation slice"),
        Pack("assetpack_characters", 144, "Stage 1 Aurora character palette, runtime contract and animation bindings"),
        Pack("assetpack_audio_hd", 128, "Stage 1 forest ambience, UI, movement and combat audio", requiredBeforeStart = false),
        Pack("assetpack_shaders_gles", 96, "OpenGL ES mobile shader and material contract"),
        Pack("assetpack_world_streaming", 96, "Stage 1 forest sector streaming descriptor"),
        Pack("assetpack_terrain_lod", 64, "Stage 1 forest heightfield and terrain LOD data"),
        Pack("assetpack_animation_sets", 48, "Stage 1 locomotion and combat motion contract"),
        Pack("assetpack_foliage_lods", 32, "Stage 1 foliage cluster and mobile LOD data", requiredBeforeStart = false)
    )

    private val highEndPacks = listOf(
        Pack("assetpack_graphics_base", 450, "compiled materials, base shaders, shared meshes, mobile render resources"),
        Pack("assetpack_forest", 350, "forest launch region, village, foliage, water, collision, navigation"),
        Pack("assetpack_characters", 500, "heroes, NPCs, animals, enemies, rigs, animation bindings"),
        Pack("assetpack_shaders_gles", 250, "compiled OpenGL ES shader libraries and pipeline state resources"),
        Pack("assetpack_world_streaming", 400, "world partition descriptors, streamed sublevels, nav data for all world sectors"),
        Pack("assetpack_terrain_lod", 425, "terrain heightfields, landscape LODs, virtual shadow maps"),
        Pack("assetpack_animation_sets", 425, "locomotion, combat, traversal, emotes, montage sections"),
        Pack("assetpack_sand", 400, "sand biome terrain, settlements, rocks, foliage, weather", requiredBeforeStart = false, sector = WorldSector.SAND),
        Pack("assetpack_snow", 400, "snow biome terrain, caves, ice materials, weather", requiredBeforeStart = false, sector = WorldSector.SNOW),
        Pack("assetpack_dungeons", 400, "dungeon cells, props, traps, encounter data, lighting data", requiredBeforeStart = false, sector = WorldSector.DUNGEON),
        Pack("assetpack_hd_textures", 500, "high-resolution PBR textures, virtual-texture pages, decals", requiredBeforeStart = false, sector = WorldSector.SAND),
        Pack("assetpack_foliage_lods", 400, "foliage clusters, impostors, mobile LODs", requiredBeforeStart = false, sector = WorldSector.SAND),
        Pack("assetpack_audio_hd", 450, "music, ambience, combat sounds, wildlife and high-quality mixes", requiredBeforeStart = false, sector = WorldSector.SNOW),
        Pack("assetpack_vfx", 300, "Niagara systems, weather effects, impact effects", requiredBeforeStart = false, sector = WorldSector.DUNGEON),
        Pack("assetpack_cinematics", 450, "story scenes, sequences and camera animation data", requiredBeforeStart = false, sector = WorldSector.DUNGEON),
        Pack("assetpack_voice", 250, "dialogue, localization, voice banks, subtitles metadata", requiredBeforeStart = false, sector = WorldSector.DUNGEON),
        Pack("assetpack_shaders_vulkan", 300, "compiled Vulkan shader libraries and pipeline state resources", requiredBeforeStart = false, sector = WorldSector.DUNGEON),
        Pack("assetpack_pipeline_cache", 100, "device-safe pipeline cache seeds and shader warm-up data", requiredBeforeStart = false, sector = WorldSector.DUNGEON)
    )

    /** Full-cook compatibility view; the current phone release uses Stage 1 below. */
    val packs: List<Pack> = highEndPacks

    fun packsFor(tier: ResourceTier): List<Pack> =
        if (tier == ResourceTier.STAGE_1) stageOnePacks else highEndPacks

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

    val stageOneTotalMiB: Int = totalMiBFor(ResourceTier.STAGE_1)
    val fullContentTotalMiB: Int = totalMiBFor(ResourceTier.HIGH)
    val totalMiB: Int = stageOneTotalMiB
    val requiredMiB: Int = startupMiBFor(ResourceTier.STAGE_1)
    val minimumFreeSpaceMiB: Int = stageOneTotalMiB + 128
    val totalGiBLabel: String = totalGiBLabelFor(ResourceTier.STAGE_1)
    val summary: String = stageOnePacks.joinToString("  •  ") { "${it.playPackName}: ${it.targetMiB} MB" }
    val requiredPackNames: List<String> = startupPackNamesFor(ResourceTier.STAGE_1)
    val fullContentRequiredPackNames: List<String> = startupPackNamesFor(ResourceTier.HIGH)
}
