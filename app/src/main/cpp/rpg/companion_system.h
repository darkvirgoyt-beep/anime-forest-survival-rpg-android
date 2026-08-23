#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace forest::rpg {

enum class CompanionCommand : uint8_t {
    Follow,
    Stay
};

struct CompanionCaptureRules {
    float maximumDistance = 0.38f;
    float maximumHealthFraction = 0.38f;
    int fiberCost = 2;
};

struct CompanionState {
    int creatureIndex = -1;
    float healthFraction = 0.0f;
    int bond = 0;
    CompanionCommand command = CompanionCommand::Follow;
    uint32_t revision = 0;
    bool captured = false;
};

struct CaptureResult {
    bool accepted = false;
    const char* reason = "invalid_capture";
    int remainingFiber = 0;
};

inline CaptureResult evaluateCapture(const CompanionCaptureRules& rules,
                                     const CompanionState& state,
                                     int creatureIndex,
                                     float distance,
                                     float healthFraction,
                                     int fiber) {
    if (state.captured) return {false, "companion_already_active", fiber};
    if (creatureIndex < 0) return {false, "creature_not_found", fiber};
    if (!std::isfinite(distance) || distance > rules.maximumDistance) return {false, "capture_out_of_range", fiber};
    if (!std::isfinite(healthFraction) || healthFraction > rules.maximumHealthFraction) return {false, "creature_too_healthy", fiber};
    if (fiber < rules.fiberCost) return {false, "insufficient_fiber", fiber};
    return {true, "captured", fiber - rules.fiberCost};
}

inline CaptureResult captureCompanion(const CompanionCaptureRules& rules,
                                      CompanionState& state,
                                      int creatureIndex,
                                      float distance,
                                      float healthFraction,
                                      int fiber) {
    const CaptureResult result = evaluateCapture(rules, state, creatureIndex, distance, healthFraction, fiber);
    if (!result.accepted) return result;
    state.creatureIndex = creatureIndex;
    state.healthFraction = 0.75f;
    state.bond = std::max(1, state.bond);
    state.command = CompanionCommand::Follow;
    state.revision += 1;
    state.captured = true;
    return result;
}

inline bool toggleCompanionCommand(CompanionState& state) {
    if (!state.captured) return false;
    state.command = state.command == CompanionCommand::Follow ? CompanionCommand::Stay : CompanionCommand::Follow;
    state.revision += 1;
    return true;
}

inline float companionAssistDamage(const CompanionState& state, bool heavyAttack, bool authoritativeOnline) {
    if (!state.captured || state.command != CompanionCommand::Follow || authoritativeOnline) return 0.0f;
    const float baseDamage = heavyAttack ? 10.0f : 5.0f;
    const float bondMultiplier = 1.0f + std::min(0.25f, static_cast<float>(state.bond) * 0.01f);
    return baseDamage * bondMultiplier;
}

}  // namespace forest::rpg
