#pragma once

#include <algorithm>
#include <cstdio>
#include <string>

namespace forest::rpg {

struct CloudState {
    int schemaVersion = 4;
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
};

inline std::string serializeCloudState(const CloudState& state) {
    char buffer[1024]{};
    std::snprintf(buffer, sizeof(buffer),
        "{\"schemaVersion\":4,\"playerX\":%.6f,\"playerY\":%.6f,\"health\":%.6f,\"stamina\":%.6f,\"hunger\":%.6f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"level\":%d,\"experienceToNext\":%d,\"totalExperience\":%d,\"day\":%d,\"worldTime\":%.6f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d,\"emberlingTrust\":%d,\"emberlingBonded\":%d,\"emberlingStay\":%d}",
        state.playerX, state.playerY, state.health, state.stamina, state.hunger, state.wood, state.fiber, state.stone,
        state.experience, state.level, state.experienceToNext, state.totalExperience, state.day, state.worldTime,
        state.gatheringActions, state.questStage, state.emberKitCrafted ? 1 : 0, state.wardenDefeated ? 1 : 0,
        state.emberlingTrust, state.emberlingBonded ? 1 : 0, state.emberlingStay ? 1 : 0);
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
    const int v4Fields = std::sscanf(payload,
        "{\"schemaVersion\":%d,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"level\":%d,\"experienceToNext\":%d,\"totalExperience\":%d,\"day\":%d,\"worldTime\":%f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d,\"emberlingTrust\":%d,\"emberlingBonded\":%d,\"emberlingStay\":%d}",
        &schemaVersion, &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone,
        &parsed.experience, &parsed.level, &parsed.experienceToNext, &parsed.totalExperience, &parsed.day, &parsed.worldTime,
        &parsed.gatheringActions, &parsed.questStage, &emberKitCrafted, &wardenDefeated, &parsed.emberlingTrust, &emberlingBonded, &emberlingStay);
    if (v4Fields == 22 && schemaVersion == 4) {
        parsed.schemaVersion = 4;
        parsed.emberKitCrafted = emberKitCrafted != 0;
        parsed.wardenDefeated = wardenDefeated != 0;
        parsed.emberlingBonded = emberlingBonded != 0;
        parsed.emberlingStay = parsed.emberlingBonded && emberlingStay != 0;
    } else {
    const int v3Fields = std::sscanf(payload,
        "{\"schemaVersion\":%d,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"level\":%d,\"experienceToNext\":%d,\"totalExperience\":%d,\"day\":%d,\"worldTime\":%f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d}",
        &schemaVersion, &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone,
        &parsed.experience, &parsed.level, &parsed.experienceToNext, &parsed.totalExperience, &parsed.day, &parsed.worldTime,
        &parsed.gatheringActions, &parsed.questStage, &emberKitCrafted, &wardenDefeated);
    if (v3Fields == 19 && schemaVersion == 3) {
        parsed.schemaVersion = 3;
        parsed.emberKitCrafted = emberKitCrafted != 0;
        parsed.wardenDefeated = wardenDefeated != 0;
    } else {
        const int v2Fields = std::sscanf(payload,
            "{\"schemaVersion\":%d,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"day\":%d,\"worldTime\":%f,\"gatheringActions\":%d,\"questStage\":%d,\"emberKitCrafted\":%d,\"wardenDefeated\":%d}",
            &schemaVersion, &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone,
            &parsed.experience, &parsed.day, &parsed.worldTime, &parsed.gatheringActions, &parsed.questStage, &emberKitCrafted, &wardenDefeated);
        if (v2Fields == 16 && schemaVersion == 2) {
            parsed.schemaVersion = 2;
            parsed.emberKitCrafted = emberKitCrafted != 0;
            parsed.wardenDefeated = wardenDefeated != 0;
        } else {
            const int v1Fields = std::sscanf(payload,
                "{\"schemaVersion\":1,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"day\":%d,\"worldTime\":%f}",
                &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone, &parsed.experience, &parsed.day, &parsed.worldTime);
            if (v1Fields != 11) return false;
            parsed.schemaVersion = 1;
        }
    }
    }
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
    parsed.emberlingTrust = std::clamp(parsed.emberlingTrust, 0, 3);
    if (parsed.emberlingTrust < 3) {
        parsed.emberlingBonded = false;
        parsed.emberlingStay = false;
    }
    result = parsed;
    return true;
}

} // namespace forest::rpg
