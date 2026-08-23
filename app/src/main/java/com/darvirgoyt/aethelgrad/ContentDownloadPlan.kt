package com.darvirgoyt.aethelgrad

/**
 * Production download envelope for the real cooked 3D Aethelgard experience.
 *
 * The selected Low or High tier is downloaded and mounted completely before
 * world entry. This prevents missing meshes, shaders, world sectors, audio, or
 * VFX from appearing after the player starts.
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

    private val lowQualityEnvelope = QualityEnvelope(
        id = "mobile-balanced",
        textureLabel = "balanced PBR",
        foliageDensity = 55,
        effectScalePercent = 70,
        shadowQuality = "dynamic contact shadows",
        waterQuality = "animated river surface",
        requiresDownloadedContent = true
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

    fun qualityEnvelopeFor(tier: ResourceTier): QualityEnvelope = when (tier) {
        ResourceTier.LOW -> lowQualityEnvelope
        ResourceTier.HIGH -> highQualityEnvelope
    }

    enum class ResourceTier(
        val storageLabel: String,
        val graphicsTierIndex: Int,
        val description: String
    ) {
        LOW(
            storageLabel = "4.2 GB",
            graphicsTierIndex = 0,
            description = "Launch forest first, then download biome sectors, low-resolution textures, GLES shaders, terrain LODs, animation, and core effects as they are discovered."
        ),
        HIGH(
            storageLabel = "6.6 GB",
            graphicsTierIndex = 4,
            description = "Launch forest first, then stream high-resolution biomes, characters, HD textures, foliage, VFX, audio, cinematics, and animation as the world opens."
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
        Pack("assetpack_characters", 500, "heroes, NPCs, animals, enemies, rigs, animation bindings, character photos"),
        Pack("assetpack_shaders_gles", 250, "compiled OpenGL ES shader libraries and pipeline state resources"),
        Pack("assetpack_world_streaming", 400, "world partition descriptors, streamed sublevels, nav data for all world sectors"),
        Pack("assetpack_terrain_lod", 425, "terrain heightfields, landscape LODs, virtual shadow maps"),
        Pack("assetpack_animation_sets", 425, "locomotion, combat, traversal, emotes, montage sections"),
        Pack("assetpack_sand", 400, "sand biome terrain, settlements, rocks, foliage, weather", sector = WorldSector.SAND),
        Pack("assetpack_snow", 400, "snow biome terrain, caves, ice materials, weather", sector = WorldSector.SNOW),
        Pack("assetpack_dungeons", 400, "dungeon cells, props, traps, encounter data, lighting data", sector = WorldSector.DUNGEON),
        Pack("assetpack_hd_textures", 500, "high-resolution PBR textures, virtual-texture pages, decals", sector = WorldSector.SAND),
        Pack("assetpack_foliage_lods", 400, "foliage clusters, impostors, Nanite-disabled mobile LODs", sector = WorldSector.SAND),
        Pack("assetpack_audio_hd", 450, "music, ambience, combat sounds, wildlife and high-quality mixes", sector = WorldSector.SNOW),
        Pack("assetpack_vfx", 300, "Niagara systems, weather effects, impact effects", sector = WorldSector.DUNGEON),
        Pack("assetpack_cinematics", 450, "story scenes, pre-rendered sequences, camera animation data", sector = WorldSector.DUNGEON),
        Pack("assetpack_voice", 250, "dialogue, localization voice banks, subtitles metadata", sector = WorldSector.DUNGEON),
        Pack("assetpack_shaders_vulkan", 300, "compiled Vulkan shader libraries and pipeline state resources", sector = WorldSector.DUNGEON),
        Pack("assetpack_pipeline_cache", 100, "device-safe pipeline cache seeds and shader warm-up data", sector = WorldSector.DUNGEON)
    )

    private val lowResourcePackNames = setOf(
        "assetpack_graphics_base",
        "assetpack_forest",
        "assetpack_characters",
        "assetpack_shaders_gles",
        "assetpack_world_streaming",
        "assetpack_terrain_lod",
        "assetpack_animation_sets",
        "assetpack_sand",
        "assetpack_snow",
        "assetpack_dungeons"
    )

    fun packsFor(tier: ResourceTier): List<Pack> = when (tier) {
        ResourceTier.LOW -> packs.filter { it.playPackName in lowResourcePackNames }
        ResourceTier.HIGH -> packs
    }

    /** All packs in a tier, used for the full installed-footprint estimate. */
    fun packNamesFor(tier: ResourceTier): List<String> = packsFor(tier).map { it.playPackName }

    /** Every pack in the selected tier is required before the first playable world entry. */
    fun startupPacksFor(tier: ResourceTier): List<Pack> = packsFor(tier)

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
    // Reserve headroom for full-tier Play staging, filesystem metadata, and safe pack updates.
    val minimumFreeSpaceMiB: Int = requiredMiB + 512
    val totalGiBLabel: String = "%.1f GB".format(requiredMiB / 1024.0)
    val summary: String = packs.joinToString("  •  ") { "${it.playPackName}: ${it.targetMiB} MB" }
    val requiredPackNames: List<String> = startupPackNamesFor(ResourceTier.HIGH)
}
