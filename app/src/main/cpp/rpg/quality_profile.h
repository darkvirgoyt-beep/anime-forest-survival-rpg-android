#pragma once

#include <algorithm>
#include <cstdint>

namespace forest::rpg {

enum class QualityProfileId : uint8_t {
    Low,
    High
};

struct QualityProfile {
    QualityProfileId id;
    int vegetationDetailCount;
    int weatherParticleBudget;
    int effectScalePercent;
    int shadowSampleBudget;
    bool highResolutionTextures;
    bool premiumWaterAccents;
};

inline constexpr QualityProfile kLowQualityProfile{
    QualityProfileId::Low, 3, 20, 70, 1, false, false
};

inline constexpr QualityProfile kHighQualityProfile{
    QualityProfileId::High, 8, 52, 140, 4, true, true
};

inline const QualityProfile& qualityProfileFor(int graphicsQuality, bool contentReady) {
    const int quality = std::clamp(graphicsQuality, 0, 4);
    if (!contentReady || quality < 3) return kLowQualityProfile;
    return kHighQualityProfile;
}

}  // namespace forest::rpg
