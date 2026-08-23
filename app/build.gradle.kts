plugins {
    id("com.android.application")
    kotlin("android")
}

val ciKeystorePath = providers.environmentVariable("AETHELGARD_RELEASE_KEYSTORE_PATH").orNull
val ciKeystorePassword = providers.environmentVariable("AETHELGARD_RELEASE_STORE_PASSWORD").orNull
val ciKeyAlias = providers.environmentVariable("AETHELGARD_RELEASE_KEY_ALIAS").orNull
val ciKeyPassword = providers.environmentVariable("AETHELGARD_RELEASE_KEY_PASSWORD").orNull
val hasCiReleaseSigning = listOf(ciKeystorePath, ciKeystorePassword, ciKeyAlias, ciKeyPassword).all { !it.isNullOrBlank() }

android {
    namespace = "com.darvirgoyt.aethelgrad"
    compileSdk = 35
    ndkVersion = "28.0.12433566"

    buildFeatures {
        buildConfig = true
    }

    defaultConfig {
        applicationId = "com.darvirgoyt.aethelgrad"
        minSdk = 26
        targetSdk = 35
        versionCode = 4
        versionName = "0.2.3"

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17 -Wall -Wextra -Wpedantic"
            }
        }

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }
    }

    signingConfigs {
        if (hasCiReleaseSigning) {
            create("ciRelease") {
                storeFile = file(ciKeystorePath!!)
                storePassword = ciKeystorePassword
                keyAlias = ciKeyAlias
                keyPassword = ciKeyPassword
            }
        }
    }

    buildTypes {
        debug {
            // Debug builds use the same production online guest session boundary.
        }
        release {
            isDebuggable = false
            isMinifyEnabled = false
            // GitHub CI uses protected secrets when configured; local builds remain debug-signed for test distribution.
            signingConfig = if (hasCiReleaseSigning) signingConfigs.getByName("ciRelease") else signingConfigs.getByName("debug")
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    assetPacks += listOf(
        ":assetpack_core",
        ":assetpack_forest",
        ":assetpack_sand",
        ":assetpack_snow",
        ":assetpack_characters",
        ":assetpack_audio_hd",
        ":assetpack_cinematics",
        ":assetpack_hd_textures",
        ":assetpack_dungeons",
        ":assetpack_vfx",
        ":assetpack_voice",
        ":assetpack_graphics_base",
        ":assetpack_shaders_vulkan",
        ":assetpack_shaders_gles",
        ":assetpack_pipeline_cache",
        ":assetpack_world_streaming",
        ":assetpack_foliage_lods",
        ":assetpack_terrain_lod",
        ":assetpack_animation_sets"
    )

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
}

dependencies {
    implementation("androidx.core:core-ktx:1.15.0")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("androidx.credentials:credentials:1.6.0")
    implementation("androidx.credentials:credentials-play-services-auth:1.6.0")
    implementation("com.google.android.libraries.identity.googleid:googleid:1.1.1")
    implementation("com.google.android.play:asset-delivery:2.3.0")
    implementation("com.google.android.play:asset-delivery-ktx:2.3.0")
}
