#pragma once

#include <algorithm>
#include <cstdio>
#include <string>

namespace forest::rpg {

struct CloudState {
    int schemaVersion = 5;
    int sourceSchemaVersion = 5; // parser metadata; never serialized
    float playerX = -0.55f;
    float playerY = -0.08f;
    float health = 1.0f;
    float stamina = 1.0f;
    float hunger = 0.82f;
    int wood = 12;
    int fiber = 8;
    int stone = 4;
    int experience = 0;
    int level = 0;
    int experienceToNext = 991;
    int totalExperience = 0;
    int day = 1;
    float worldTime = 0.0f;
    int gatheringActions = 0;
    int questStage = 0;
    bool emberKitCrafted = false;
    bool wardenDefeated = false;
    int emberlingTrust = 0;
    bool emberlingBonded = false;
    bool emberlingStay = false;
    int discoveredSectors = 1; // forest launch sector; bits 1, 2, 3 are sand, snow, dungeon
    int capturedMobIndex = -1;
    bool capturedCompanionStay = false;
    bool campBuilt = false;
    float campX = -0.64f;
    float campY = 0.52f;
    float campZ = 0.0f;
    float campYaw = 0.0f;
    float campScale = 1.0f;
    int companionRevision = 0;
    int campRevision = 0;
};

inline constexpr int kForestSectorMask = 1;
inline constexpr int kKnownBiomeSectorMask = 15;

inline int normalizeDiscoveredSectors(int sectors) {
    // A save may contain stale future bits, a malformed negative value, or a
    // sparse unlock (for example Sand without Forest). Keep only shipped biome
    // bits and always retain Forest as the safe launch and recovery sector.
    const int knownSectors = sectors > 0 ? (sectors & kKnownBiomeSectorMask) : 0;
    return knownSectors | kForestSectorMask;
}

inline std::string serializeCloudState(const CloudState& state) {
    char buffer[1024]{};
    std::snprintf(buffer, sizeof(buffer),
        "{\"schemaVersion\":5,\"playerX\":%.6f,\"playerY\":%.6f,\"health\":%.6f,\"stamina\":%.6f,\"hunger\":%.6f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"level\":%d,\"experienceToNext\":%d,\"totalExperience\":%d,\"day\":%d,\"worldTime\":%.6f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d,\"emberlingTrust\":%d,\"emberlingBonded\":%d,\"emberlingStay\":%d,\"discoveredSectors\":%d,\"capturedMobIndex\":%d,\"capturedCompanionStay\":%d,\"campBuilt\":%d,\"campX\":%.6f,\"campY\":%.6f,\"campZ\":%.6f,\"campYaw\":%.6f,\"campScale\":%.6f,\"companionRevision\":%d,\"campRevision\":%d}",
        state.playerX, state.playerY, state.health, state.stamina, state.hunger, state.wood, state.fiber, state.stone,
        state.experience, state.level, state.experienceToNext, state.totalExperience, state.day, state.worldTime,
        state.gatheringActions, state.questStage, state.emberKitCrafted ? 1 : 0, state.wardenDefeated ? 1 : 0,
        state.emberlingTrust, state.emberlingBonded ? 1 : 0, state.emberlingStay ? 1 : 0,
        normalizeDiscoveredSectors(state.discoveredSectors), state.capturedMobIndex, state.capturedCompanionStay ? 1 : 0,
        state.campBuilt ? 1 : 0, state.campX, state.campY, state.campZ, state.campYaw, state.campScale,
        state.companionRevision, state.campRevision);
    return buffer;
}

inline bool parseCloudState(const char* payload, CloudState& result) {
    if (payload == nullptr) return false;
    CloudState parsed{};
    int schemaVersion = 0;
    int emberKitCrafted = 0;
    int wardenDefeated = 0;
    int emberlingBonded = 0;
    int emberlingStay = 0;
    int discoveredSectors = 1;
    int capturedCompanionStay = 0;
    int campBuilt = 0;

    // Dispatch by the fixed version prefix first. Reading one bounded digit
    // avoids making legacy payloads repeatedly attempt every newer format and
    // avoids a second formatted scan before the actual parse.
    constexpr char kSchemaPrefix[] = "{\"schemaVersion\":";
    constexpr std::size_t kSchemaPrefixLength = sizeof(kSchemaPrefix) - 1;
    for (std::size_t prefixIndex = 0; prefixIndex < kSchemaPrefixLength; ++prefixIndex) {
        if (payload[prefixIndex] != kSchemaPrefix[prefixIndex]) return false;
    }
    const char versionDigit = payload[kSchemaPrefixLength];
    if (versionDigit < '1' || versionDigit > '5') return false;
    schemaVersion = versionDigit - '0';

    bool parsedKnownLayout = false;
    switch (schemaVersion) {
        case 5: {
            const int v5Fields = std::sscanf(payload,
                "{\"schemaVersion\":%d,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"level\":%d,\"experienceToNext\":%d,\"totalExperience\":%d,\"day\":%d,\"worldTime\":%f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d,\"emberlingTrust\":%d,\"emberlingBonded\":%d,\"emberlingStay\":%d,\"discoveredSectors\":%d,\"capturedMobIndex\":%d,\"capturedCompanionStay\":%d,\"campBuilt\":%d,\"campX\":%f,\"campY\":%f,\"campZ\":%f,\"campYaw\":%f,\"campScale\":%f,\"companionRevision\":%d,\"campRevision\":%d}",
                &schemaVersion, &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone,
                &parsed.experience, &parsed.level, &parsed.experienceToNext, &parsed.totalExperience, &parsed.day, &parsed.worldTime,
                &parsed.gatheringActions, &parsed.questStage, &emberKitCrafted, &wardenDefeated, &parsed.emberlingTrust, &emberlingBonded, &emberlingStay,
                &discoveredSectors, &parsed.capturedMobIndex, &capturedCompanionStay, &campBuilt, &parsed.campX, &parsed.campY, &parsed.campZ,
                &parsed.campYaw, &parsed.campScale, &parsed.companionRevision, &parsed.campRevision);
            if (v5Fields == 33) {
                parsedKnownLayout = true;
                break;
            }

            // Accept the authority draft’s schema-5 order before this field was
            // combined with map discovery.
            const int v5AuthorityFields = std::sscanf(payload,
                "{\"schemaVersion\":%d,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"level\":%d,\"experienceToNext\":%d,\"totalExperience\":%d,\"day\":%d,\"worldTime\":%f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d,\"emberlingTrust\":%d,\"emberlingBonded\":%d,\"emberlingStay\":%d,\"capturedMobIndex\":%d,\"capturedCompanionStay\":%d,\"campBuilt\":%d,\"campX\":%f,\"campY\":%f,\"campZ\":%f,\"campYaw\":%f,\"campScale\":%f,\"companionRevision\":%d,\"campRevision\":%d}",
                &schemaVersion, &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone,
                &parsed.experience, &parsed.level, &parsed.experienceToNext, &parsed.totalExperience, &parsed.day, &parsed.worldTime,
                &parsed.gatheringActions, &parsed.questStage, &emberKitCrafted, &wardenDefeated, &parsed.emberlingTrust, &emberlingBonded, &emberlingStay,
                &parsed.capturedMobIndex, &capturedCompanionStay, &campBuilt, &parsed.campX, &parsed.campY, &parsed.campZ, &parsed.campYaw,
                &parsed.campScale, &parsed.companionRevision, &parsed.campRevision);
            if (v5AuthorityFields == 32) {
                parsedKnownLayout = true;
                break;
            }

            const int v5MapFields = std::sscanf(payload,
                "{\"schemaVersion\":%d,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"level\":%d,\"experienceToNext\":%d,\"totalExperience\":%d,\"day\":%d,\"worldTime\":%f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d,\"emberlingTrust\":%d,\"emberlingBonded\":%d,\"emberlingStay\":%d,\"discoveredSectors\":%d}",
                &schemaVersion, &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone,
                &parsed.experience, &parsed.level, &parsed.experienceToNext, &parsed.totalExperience, &parsed.day, &parsed.worldTime,
                &parsed.gatheringActions, &parsed.questStage, &emberKitCrafted, &wardenDefeated, &parsed.emberlingTrust, &emberlingBonded, &emberlingStay,
                &discoveredSectors);
            parsedKnownLayout = v5MapFields == 23;
            break;
        }
        case 4: {
            const int v4Fields = std::sscanf(payload,
                "{\"schemaVersion\":%d,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"level\":%d,\"experienceToNext\":%d,\"totalExperience\":%d,\"day\":%d,\"worldTime\":%f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d,\"emberlingTrust\":%d,\"emberlingBonded\":%d,\"emberlingStay\":%d}",
                &schemaVersion, &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone,
                &parsed.experience, &parsed.level, &parsed.experienceToNext, &parsed.totalExperience, &parsed.day, &parsed.worldTime,
                &parsed.gatheringActions, &parsed.questStage, &emberKitCrafted, &wardenDefeated, &parsed.emberlingTrust, &emberlingBonded, &emberlingStay);
            parsedKnownLayout = v4Fields == 22;
            break;
        }
        case 3: {
            const int v3Fields = std::sscanf(payload,
                "{\"schemaVersion\":%d,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"level\":%d,\"experienceToNext\":%d,\"totalExperience\":%d,\"day\":%d,\"worldTime\":%f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d}",
                &schemaVersion, &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone,
                &parsed.experience, &parsed.level, &parsed.experienceToNext, &parsed.totalExperience, &parsed.day, &parsed.worldTime,
                &parsed.gatheringActions, &parsed.questStage, &emberKitCrafted, &wardenDefeated);
            parsedKnownLayout = v3Fields == 19;
            break;
        }
        case 2: {
            const int v2Fields = std::sscanf(payload,
                "{\"schemaVersion\":%d,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"day\":%d,\"worldTime\":%f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d}",
                &schemaVersion, &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone,
                &parsed.experience, &parsed.day, &parsed.worldTime, &parsed.gatheringActions, &parsed.questStage, &emberKitCrafted, &wardenDefeated);
            parsedKnownLayout = v2Fields == 16;
            break;
        }
        case 1: {
            const int v1Fields = std::sscanf(payload,
                "{\"schemaVersion\":1,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"day\":%d,\"worldTime\":%f}",
                &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone, &parsed.experience, &parsed.day, &parsed.worldTime);
            parsedKnownLayout = v1Fields == 11;
            break;
        }
        default:
            return false;
    }

    if (!parsedKnownLayout) return false;
    const int sourceSchemaVersion = schemaVersion;
    parsed.schemaVersion = 5;
    parsed.sourceSchemaVersion = sourceSchemaVersion;
    parsed.emberKitCrafted = emberKitCrafted != 0;
    parsed.wardenDefeated = wardenDefeated != 0;
    parsed.emberlingBonded = parsed.emberlingBonded || emberlingBonded != 0;
    parsed.emberlingStay = parsed.emberlingBonded && (parsed.emberlingStay || emberlingStay != 0);
    parsed.capturedCompanionStay = parsed.capturedMobIndex >= 0 && (parsed.capturedCompanionStay || capturedCompanionStay != 0);
    parsed.campBuilt = parsed.campBuilt || campBuilt != 0;
    parsed.discoveredSectors = normalizeDiscoveredSectors(discoveredSectors);
    parsed.health = std::clamp(parsed.health, 0.0f, 1.0f);
    parsed.stamina = std::clamp(parsed.stamina, 0.0f, 1.0f);
    parsed.hunger = std::clamp(parsed.hunger, 0.0f, 1.0f);
    parsed.wood = std::max(0, parsed.wood);
    parsed.fiber = std::max(0, parsed.fiber);
    parsed.stone = std::max(0, parsed.stone);
    parsed.experience = std::max(0, parsed.experience);
    parsed.level = std::clamp(parsed.level, 0, 100);
    parsed.experienceToNext = std::max(0, parsed.experienceToNext);
    parsed.totalExperience = std::max(0, parsed.totalExperience);
    parsed.day = std::max(1, parsed.day);
    parsed.worldTime = std::max(0.0f, parsed.worldTime);
    parsed.gatheringActions = std::clamp(parsed.gatheringActions, 0, 3);
    parsed.questStage = std::clamp(parsed.questStage, 0, 3);
    parsed.capturedMobIndex = std::clamp(parsed.capturedMobIndex, -1, 31);
    parsed.campX = std::clamp(parsed.campX, -128.0f, 128.0f);
    parsed.campY = std::clamp(parsed.campY, -128.0f, 128.0f);
    parsed.campZ = std::clamp(parsed.campZ, -8.0f, 32.0f);
    parsed.campYaw = std::clamp(parsed.campYaw, -3.14159265f, 3.14159265f);
    parsed.campScale = std::clamp(parsed.campScale, 0.75f, 1.25f);
    parsed.companionRevision = std::max(0, parsed.companionRevision);
    parsed.campRevision = std::max(0, parsed.campRevision);
    parsed.emberlingTrust = std::clamp(parsed.emberlingTrust, 0, 3);
    if (parsed.emberlingTrust < 3) {
        parsed.emberlingBonded = false;
        parsed.emberlingStay = false;
    }
    result = parsed;
    return true;
}

} // namespace forest::rpg
