#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace forest::rpg {

enum class BiomeId : uint8_t {
    Forest,
    Sand,
    Snow,
    Ruins
};

struct EncounterBudget {
    int activeCreatures = 0;
    int eliteSlots = 0;
    float respawnSeconds = 30.0f;
    float threatMultiplier = 1.0f;
};

inline EncounterBudget encounterBudgetFor(BiomeId biome,
                                          float worldTimeHours,
                                          float weatherIntensity,
                                          int graphicsQuality) {
    const int quality = std::clamp(graphicsQuality, 0, 4);
    const float hour = std::fmod(std::max(0.0f, worldTimeHours), 24.0f);
    const bool night = hour >= 19.0f || hour < 6.0f;
    const int biomeBonus = biome == BiomeId::Ruins ? 2 : biome == BiomeId::Snow ? 1 : 0;
    const int nightBonus = night ? 2 : 0;
    const int weatherBonus = weatherIntensity > 0.65f ? 1 : 0;

    EncounterBudget budget;
    budget.activeCreatures = std::clamp(5 + biomeBonus + nightBonus + weatherBonus + quality, 4, 14);
    budget.eliteSlots = std::clamp((night ? 1 : 0) + (biome == BiomeId::Ruins ? 1 : 0) + quality / 3, 0, 3);
    budget.respawnSeconds = std::max(12.0f, 38.0f - static_cast<float>(quality) * 4.0f - (night ? 5.0f : 0.0f));
    budget.threatMultiplier = 1.0f + (night ? 0.15f : 0.0f) + std::min(0.25f, std::max(0.0f, weatherIntensity) * 0.25f);
    return budget;
}

inline bool canSpawnEncounter(const EncounterBudget& budget, int activeCreatures, bool elite) {
    if (activeCreatures < 0) return false;
    if (activeCreatures >= budget.activeCreatures) return false;
    if (elite && budget.eliteSlots <= 0) return false;
    return true;
}

}  // namespace forest::rpg
