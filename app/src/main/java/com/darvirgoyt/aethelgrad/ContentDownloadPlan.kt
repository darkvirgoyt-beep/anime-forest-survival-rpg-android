package com.darvirgoyt.aethelgrad

/**
 * Production download envelope for the real cooked 3D Aethelgard experience.
 *
 * These are physical Play Asset Delivery pack targets, not padding. The
 * checked-in prototype contains tiny development bundles; a production build
 * must replace them with real cooked Unreal .pak content.
 */
object ContentDownloadPlan {
    enum class ResourceTier(
        val storageLabel: String,
        val graphicsTierIndex: Int,
        val description: String
    ) {
        LOW(
            storageLabel = "4.2 GB",
            graphicsTierIndex = 0,
            description = "Complete world sectors, gameplay characters, low-resolution textures, GLES shaders, terrain LODs, animation, and core effects."
        ),
        HIGH(
            storageLabel = "6.6 GB",
            graphicsTierIndex = 4,
            description = "Complete world sectors, high-resolution characters and photos, HD textures, dense foliage, Vulkan/GLES shaders, pipeline cache, VFX, audio, cinematics, and animation."
        )
    }

    data class Pack(
        val playPackName: String,
        val targetMiB: Int,
        val contents: String,
        val requiredBeforeStart: Boolean = true
    )

    val packs = listOf(
        Pack("assetpack_graphics_base", 450, "compiled materials, base shaders, shared meshes, mobile render resources"),
        Pack("assetpack_forest", 350, "forest launch region, village, foliage, water, collision, navigation"),
        Pack("assetpack_sand", 400, "sand biome terrain, settlements, rocks, foliage, weather"),
        Pack("assetpack_snow", 400, "snow biome terrain, caves, ice materials, weather"),
        Pack("assetpack_characters", 500, "heroes, NPCs, animals, enemies, rigs, animation bindings, character photos"),
        Pack("assetpack_audio_hd", 450, "music, ambience, combat sounds, wildlife and high-quality mixes"),
        Pack("assetpack_cinematics", 450, "story scenes, pre-rendered sequences, camera animation data"),
        Pack("assetpack_hd_textures", 500, "high-resolution PBR textures, virtual-texture pages, decals"),
        Pack("assetpack_dungeons", 400, "dungeon cells, props, traps, encounter data, lighting data"),
        Pack("assetpack_vfx", 300, "Niagara systems, GPU particles, weather effects, impact effects"),
        Pack("assetpack_voice", 250, "dialogue, localization voice banks, subtitles metadata"),
        Pack("assetpack_shaders_vulkan", 300, "compiled Vulkan shader libraries and pipeline state resources"),
        Pack("assetpack_shaders_gles", 250, "compiled OpenGL ES shader libraries and pipeline state resources"),
        Pack("assetpack_pipeline_cache", 100, "device-safe pipeline cache seeds and shader warm-up data"),
        Pack("assetpack_world_streaming", 400, "world partition descriptors, streamed sublevels, nav data for all world sectors"),
        Pack("assetpack_foliage_lods", 400, "foliage clusters, impostors, Nanite-disabled mobile LODs"),
        Pack("assetpack_terrain_lod", 425, "terrain heightfields, landscape LODs, virtual shadow maps"),
        Pack("assetpack_animation_sets", 425, "locomotion, combat, traversal, emotes, montage sections")
    )

    private val lowResourcePackNames = setOf(
        "assetpack_graphics_base",
        "assetpack_forest",
        "assetpack_sand",
        "assetpack_snow",
        "assetpack_characters",
        "assetpack_dungeons",
        "assetpack_shaders_gles",
        "assetpack_world_streaming",
        "assetpack_terrain_lod",
        "assetpack_animation_sets"
    )

    fun packsFor(tier: ResourceTier): List<Pack> = when (tier) {
        ResourceTier.LOW -> packs.filter { it.playPackName in lowResourcePackNames }
        ResourceTier.HIGH -> packs
    }

    fun packNamesFor(tier: ResourceTier): List<String> = packsFor(tier)
        .filter { it.requiredBeforeStart }
        .map { it.playPackName }

    fun totalMiBFor(tier: ResourceTier): Int = packsFor(tier).sumOf { it.targetMiB }

    fun totalGiBLabelFor(tier: ResourceTier): String = "%.1f GB".format(totalMiBFor(tier) / 1024.0)

    val totalMiB: Int = packs.sumOf { it.targetMiB }
    val requiredMiB: Int = packs.filter { it.requiredBeforeStart }.sumOf { it.targetMiB }
    // Reserve headroom for Play staging, filesystem metadata, and safe pack updates.
    val minimumFreeSpaceMiB: Int = requiredMiB + 512
    val totalGiBLabel: String = "%.1f GB".format(requiredMiB / 1024.0)
    val summary: String = packs.joinToString("  •  ") { "${it.playPackName}: ${it.targetMiB} MB" }
    val requiredPackNames: List<String> = packNamesFor(ResourceTier.HIGH)
}
