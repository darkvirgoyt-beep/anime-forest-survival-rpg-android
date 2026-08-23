pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "anime-forest-survival-rpg"
include(":app")
include(":assetpack_core")
include(":assetpack_forest")
include(":assetpack_sand")
include(":assetpack_snow")
include(":assetpack_characters")
include(":assetpack_audio_hd")
include(":assetpack_cinematics")
include(":assetpack_hd_textures")
include(":assetpack_dungeons")
include(":assetpack_vfx")
include(":assetpack_voice")
include(":assetpack_graphics_base")
include(":assetpack_shaders_vulkan")
include(":assetpack_shaders_gles")
include(":assetpack_pipeline_cache")
include(":assetpack_world_streaming")
include(":assetpack_foliage_lods")
include(":assetpack_terrain_lod")
include(":assetpack_animation_sets")

