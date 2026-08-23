#include <cassert>
#include <cmath>
#include <string>
#include <iostream>

#include "rpg/animation_state.h"

int main() {
    using namespace forest::rpg;

    const AnimationIntent idle = deriveAnimationIntent({AnimationMotionState::Idle, 0.0f, true, false, false, 0, false, false});
    assert(idle.motion == AnimationMotionState::Idle);
    assert(idle.upperBody == AnimationUpperBodyState::None);
    assert(idle.playRate == 1.0f);
    assert(!idle.useRootMotion);

    const AnimationIntent walk = deriveAnimationIntent({AnimationMotionState::Walk, 0.55f, true, false, false, 0, false, false});
    assert(walk.locomotionBlend > 0.54f && walk.locomotionBlend < 0.56f);
    assert(walk.playRate > 1.0f);
    assert(walk.bodyLean > 0.0f && walk.bodyLean < 0.10f);

    const AnimationIntent sprint = deriveAnimationIntent({AnimationMotionState::Sprint, 1.0f, true, false, false, 0, false, false});
    assert(sprint.playRate > walk.playRate);
    assert(std::abs(sprint.bodyLean - 0.18f) < 0.001f);

    const AnimationIntent dodge = deriveAnimationIntent({AnimationMotionState::Dodge, 0.9f, true, false, false, 0, false, false});
    assert(dodge.useRootMotion && dodge.playRate > 1.3f && dodge.bodyLean > sprint.bodyLean);

    const AnimationIntent slide = deriveAnimationIntent({AnimationMotionState::Slide, 0.8f, true, false, false, 0, false, false});
    assert(slide.useRootMotion && slide.verticalOffset < -0.2f);

    const AnimationIntent jump = deriveAnimationIntent({AnimationMotionState::Jump, 0.2f, false, false, false, 0, false, false});
    const AnimationIntent fall = deriveAnimationIntent({AnimationMotionState::Fall, 0.2f, false, false, false, 0, false, false});
    assert(jump.verticalOffset > 0.0f && fall.verticalOffset < 0.0f);

    const AnimationIntent swim = deriveAnimationIntent({AnimationMotionState::Swim, 0.7f, false, false, false, 0, false, false});
    assert(swim.playRate < 1.0f && swim.bodyLean > 0.0f);

    const AnimationIntent lightTwo = deriveAnimationIntent({AnimationMotionState::Walk, 0.4f, true, true, false, 1, false, false});
    assert(lightTwo.upperBody == AnimationUpperBodyState::LightAttack2);
    const AnimationIntent heavy = deriveAnimationIntent({AnimationMotionState::Walk, 0.4f, true, true, true, 2, false, false});
    assert(heavy.upperBody == AnimationUpperBodyState::HeavyAttack);

    const AnimationIntent dead = deriveAnimationIntent({AnimationMotionState::Dead, 0.0f, false, true, true, 2, true, true});
    assert(dead.playRate < 1.0f && !dead.additiveSecondaryMotion && dead.upperBody == AnimationUpperBodyState::None);

    assert(std::string(animationMotionName(AnimationMotionState::Sprint)) == "sprint");
    assert(std::string(animationMotionName(AnimationMotionState::Swim)) == "swim");
    std::cout << "animation_state_test: PASS\n";
    return 0;
}
