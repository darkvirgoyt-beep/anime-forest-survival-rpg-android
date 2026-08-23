plugins {
    id("com.android.application")
    kotlin("android")
}

android {
    namespace = "com.darvirgoyt.aethelgrad"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.darvirgoyt.aethelgrad"
        minSdk = 26
        targetSdk = 35
        versionCode = 2
        versionName = "0.2.1"

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17 -Wall -Wextra -Wpedantic"
            }
        }

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            // Test-distribution signing only. Replace with a protected release keystore for Play Store publishing.
            signingConfig = signingConfigs.getByName("debug")
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

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
}
