package com.darvirgoyt.aethelgrad

/**
 * Production download envelope for the real cooked 3D Aethelgard experience.
 *
 * These are physical Play Asset Delivery pack targets, not padding. The
 * checked-in prototype contains tiny development bundles; a production build
 * must replace them with real cooked Unreal .pak content.
 */
object ContentDownloadPlan {
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
        Pack("assetpack_characters", 500, "heroes, NPCs, animals, enemies, rigs, animation bindings"),
        Pack("assetpack_audio_hd", 450, "music, ambience, combat sounds, wildlife and high-quality mixes"),
        Pack("assetpack_cinematics", 450, "story scenes, pre-rendered sequences, camera animation data"),
        Pack("assetpack_hd_textures", 500, "high-resolution PBR textures, virtual-texture pages, decals"),
        Pack("assetpack_dungeons", 400, "dungeon cells, props, traps, encounter data, lighting data"),
        Pack("assetpack_vfx", 300, "Niagara systems, GPU particles, weather effects, impact effects"),
        Pack("assetpack_voice", 250, "dialogue, localization voice banks, subtitles metadata"),
        Pack("assetpack_shaders_vulkan", 300, "compiled Vulkan shader libraries and pipeline state resources"),
        Pack("assetpack_shaders_gles", 250, "compiled OpenGL ES shader libraries and pipeline state resources"),
        Pack("assetpack_pipeline_cache", 100, "device-safe pipeline cache seeds and shader warm-up data"),
        Pack("assetpack_world_streaming", 400, "world partition descriptors, streamed sublevels, nav data"),
        Pack("assetpack_foliage_lods", 400, "foliage clusters, impostors, Nanite-disabled mobile LODs"),
        Pack("assetpack_terrain_lod", 425, "terrain heightfields, landscape LODs, virtual shadow maps"),
        Pack("assetpack_animation_sets", 425, "locomotion, combat, traversal, emotes, montage sections")
    )

    val totalMiB: Int = packs.sumOf { it.targetMiB }
    val totalGiBLabel: String = "%.1f GB".format(totalMiB / 1024.0)
    val summary: String = packs.joinToString("  •  ") { "${it.playPackName}: ${it.targetMiB} MB" }
    val requiredPackNames: List<String> = packs.filter { it.requiredBeforeStart }.map { it.playPackName }
}
