#pragma once

#include <algorithm>
#include <cstdio>
#include <string>

namespace forest::rpg {

struct CloudState {
    float playerX = 0.0f;
    float playerY = -0.08f;
    float health = 1.0f;
    float stamina = 1.0f;
    float hunger = 0.82f;
    int wood = 12;
    int fiber = 8;
    int stone = 4;
    int experience = 0;
    int day = 1;
    float worldTime = 0.0f;
};

inline std::string serializeCloudState(const CloudState& state) {
    char buffer[512]{};
    std::snprintf(buffer, sizeof(buffer),
        "{\"schemaVersion\":1,\"playerX\":%.6f,\"playerY\":%.6f,\"health\":%.6f,\"stamina\":%.6f,\"hunger\":%.6f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"day\":%d,\"worldTime\":%.6f}",
        state.playerX, state.playerY, state.health, state.stamina, state.hunger, state.wood, state.fiber, state.stone, state.experience, state.day, state.worldTime);
    return buffer;
}

inline bool parseCloudState(const char* payload, CloudState& result) {
    if (payload == nullptr) return false;
    CloudState parsed{};
    const int fields = std::sscanf(payload,
        "{\"schemaVersion\":1,\"playerX\":%f,\"playerY\":%f,\"health\":%f,\"stamina\":%f,\"hunger\":%f,\"wood\":%d,\"fiber\":%d,\"stone\":%d,\"experience\":%d,\"day\":%d,\"worldTime\":%f}",
        &parsed.playerX, &parsed.playerY, &parsed.health, &parsed.stamina, &parsed.hunger, &parsed.wood, &parsed.fiber, &parsed.stone, &parsed.experience, &parsed.day, &parsed.worldTime);
    if (fields != 11) return false;
    parsed.health = std::clamp(parsed.health, 0.0f, 1.0f);
    parsed.stamina = std::clamp(parsed.stamina, 0.0f, 1.0f);
    parsed.hunger = std::clamp(parsed.hunger, 0.0f, 1.0f);
    parsed.wood = std::max(0, parsed.wood);
    parsed.fiber = std::max(0, parsed.fiber);
    parsed.stone = std::max(0, parsed.stone);
    parsed.experience = std::max(0, parsed.experience);
    parsed.day = std::max(1, parsed.day);
    parsed.worldTime = std::max(0.0f, parsed.worldTime);
    result = parsed;
    return true;
}

} // namespace forest::rpg
