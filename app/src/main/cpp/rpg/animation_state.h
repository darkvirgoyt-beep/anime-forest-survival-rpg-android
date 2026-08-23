#pragma once

#include <algorithm>
#include <cmath>

namespace forest::rpg {

enum class AnimationMotionState {
    Idle,
    Walk,
    Sprint,
    Jump,
    Fall,
    Swim,
    Dodge,
    Slide,
    Hitstun,
    Dead
};

enum class AnimationUpperBodyState {
    None,
    LightAttack1,
    LightAttack2,
    LightAttack3,
    HeavyAttack,
    CompanionCommand,
    Interact
};

struct AnimationStateInput {
    AnimationMotionState motion = AnimationMotionState::Idle;
    float speedNormalized = 0.0f;
    bool grounded = true;
    bool attackActive = false;
    bool heavyAttack = false;
    int comboIndex = 0;
    bool companionCaptured = false;
    bool companionStay = false;
};

struct AnimationIntent {
    AnimationMotionState motion = AnimationMotionState::Idle;
    AnimationUpperBodyState upperBody = AnimationUpperBodyState::None;
    float locomotionBlend = 0.0f;
    float playRate = 1.0f;
    float bodyLean = 0.0f;
    float verticalOffset = 0.0f;
    bool useRootMotion = false;
    bool additiveSecondaryMotion = true;
};

inline AnimationIntent deriveAnimationIntent(const AnimationStateInput& input) {
    AnimationIntent intent{};
    intent.motion = input.motion;
    intent.locomotionBlend = std::clamp(input.speedNormalized, 0.0f, 1.0f);
    intent.playRate = 1.0f + intent.locomotionBlend * 0.45f;
    intent.bodyLean = intent.locomotionBlend * (input.motion == AnimationMotionState::Sprint ? 0.18f : 0.08f);
    intent.verticalOffset = input.grounded ? 0.0f : (input.motion == AnimationMotionState::Jump ? 0.035f : -0.015f);
    intent.useRootMotion = input.motion == AnimationMotionState::Dodge || input.motion == AnimationMotionState::Slide;
    intent.additiveSecondaryMotion = input.motion != AnimationMotionState::Dead;

    if (input.motion == AnimationMotionState::Dead) {
        intent.playRate = 0.85f;
        intent.bodyLean = 0.0f;
        intent.additiveSecondaryMotion = false;
        return intent;
    }
    if (input.motion == AnimationMotionState::Hitstun) {
        intent.playRate = 1.15f;
        intent.bodyLean = -0.12f;
        return intent;
    }
    if (input.motion == AnimationMotionState::Dodge) {
        intent.playRate = 1.35f;
        intent.bodyLean = 0.22f;
        return intent;
    }
    if (input.motion == AnimationMotionState::Slide) {
        intent.playRate = 1.20f;
        intent.bodyLean = 0.28f;
        intent.verticalOffset = -0.22f;
        return intent;
    }
    if (input.motion == AnimationMotionState::Swim) {
        intent.playRate = 0.78f + intent.locomotionBlend * 0.20f;
        intent.bodyLean = 0.04f;
    }

    if (input.attackActive) {
        if (input.heavyAttack) {
            intent.upperBody = AnimationUpperBodyState::HeavyAttack;
        } else {
            const int combo = std::clamp(input.comboIndex, 0, 2);
            intent.upperBody = static_cast<AnimationUpperBodyState>(static_cast<int>(AnimationUpperBodyState::LightAttack1) + combo);
        }
        intent.additiveSecondaryMotion = true;
    }
    return intent;
}

inline const char* animationMotionName(AnimationMotionState state) {
    switch (state) {
    case AnimationMotionState::Idle: return "idle";
    case AnimationMotionState::Walk: return "walk";
    case AnimationMotionState::Sprint: return "sprint";
    case AnimationMotionState::Jump: return "jump";
    case AnimationMotionState::Fall: return "fall";
    case AnimationMotionState::Swim: return "swim";
    case AnimationMotionState::Dodge: return "dodge";
    case AnimationMotionState::Slide: return "slide";
    case AnimationMotionState::Hitstun: return "hitstun";
    case AnimationMotionState::Dead: return "dead";
    }
    return "idle";
}

} // namespace forest::rpg
