#pragma once

#include <algorithm>
#include <cmath>

namespace forest::rpg {

enum class EmberlingInteraction {
    TooFar,
    NeedsEmberKit,
    NeedsFiber,
    BondAdvanced,
    Bonded,
    CommandChanged
};

struct EmberlingState {
    float x = -0.46f;
    float y = -0.16f;
    int trust = 0;
    bool bonded = false;
    bool stay = false;
    float pulse = 0.0f;
};

constexpr int kEmberlingTrustRequired = 3;
constexpr float kEmberlingInteractionRange = 0.26f;

inline float emberlingDistance(const EmberlingState& state, float playerX, float playerY) {
    const float dx = state.x - playerX;
    const float dy = state.y - playerY;
    return std::sqrt(dx * dx + dy * dy);
}

inline EmberlingInteraction interactWithEmberling(EmberlingState& state, bool emberKitCrafted, int& fiber, float playerX, float playerY) {
    if (emberlingDistance(state, playerX, playerY) > kEmberlingInteractionRange) return EmberlingInteraction::TooFar;
    if (!emberKitCrafted) return EmberlingInteraction::NeedsEmberKit;
    if (state.bonded) {
        state.stay = !state.stay;
        state.pulse = 1.0f;
        return EmberlingInteraction::CommandChanged;
    }
    if (fiber < 1) return EmberlingInteraction::NeedsFiber;
    --fiber;
    state.trust = std::min(kEmberlingTrustRequired, state.trust + 1);
    state.pulse = 1.0f;
    if (state.trust >= kEmberlingTrustRequired) {
        state.bonded = true;
        state.stay = false;
        return EmberlingInteraction::Bonded;
    }
    return EmberlingInteraction::BondAdvanced;
}

inline void updateEmberling(EmberlingState& state, float playerX, float playerY, float deltaSeconds, bool storming) {
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.10f);
    state.pulse = std::max(0.0f, state.pulse - dt * 1.5f);
    if (state.bonded && !state.stay) {
        const float followX = playerX - 0.14f;
        const float followY = playerY + 0.10f;
        const float catchUp = storming ? 4.6f : 3.4f;
        state.x += (followX - state.x) * std::min(1.0f, dt * catchUp);
        state.y += (followY - state.y) * std::min(1.0f, dt * catchUp);
    }
}

inline const char* emberlingStatus(const EmberlingState& state) {
    if (state.bonded) return state.stay ? "EMBERLING_STAY" : "EMBERLING_FOLLOW";
    if (state.trust > 0) return "EMBERLING_TRUST";
    return "EMBERLING_WILD";
}

} // namespace forest::rpg
