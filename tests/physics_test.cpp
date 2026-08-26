#include "../app/src/main/cpp/physics/physics.h"

#include <cassert>
#include <cmath>
#include <iostream>

using forest::physics::Aabb;
using forest::physics::CharacterBody;
using forest::physics::SecondaryMotion;
using forest::physics::StaticObstacle;
using forest::physics::Vec2;
using forest::physics::WaterVolume;

int main() {
    CharacterBody body;
    body.position = {0.0f, -0.50f};
    body.grounded = true;
    body.step({1.0f, 0.0f}, 1.0f / 60.0f, nullptr, 0);
    assert(body.position.x > 0.0f);
    assert(std::abs(body.position.y - (-0.50f)) < 0.01f);

    CharacterBody expandedWorldMover;
    expandedWorldMover.minWalkablePosition = {-1.55f, -1.00f};
    expandedWorldMover.maxWalkablePosition = {1.55f, 1.00f};
    expandedWorldMover.position = {1.48f, 0.90f};
    expandedWorldMover.velocity = {2.0f, 2.0f};
    expandedWorldMover.step({1.0f, 1.0f}, 1.0f / 30.0f, nullptr, 0);
    assert(expandedWorldMover.position.x > 0.90f && expandedWorldMover.position.y > 0.52f);
    assert(expandedWorldMover.position.x <= 1.55f && expandedWorldMover.position.y <= 1.00f);

    body.jump();
    assert(body.verticalVelocity > 0.0f);
    body.jump();
    assert(body.verticalVelocity > 0.0f);

    CharacterBody jumpMover;
    jumpMover.position = {0.0f, -0.20f};
    jumpMover.grounded = true;
    jumpMover.jump();
    const float forwardBeforeJumpStep = jumpMover.position.y;
    jumpMover.step({0.0f, 1.0f}, 1.0f / 60.0f, nullptr, 0);
    assert(jumpMover.position.y > forwardBeforeJumpStep);
    assert(jumpMover.verticalPosition > 0.0f);

    CharacterBody blocked;
    blocked.position = {-0.06f, -0.20f};
    blocked.velocity = {1.0f, 0.0f};
    const StaticObstacle wall{{{0.0f, -0.20f}, {0.04f, 0.30f}}};
    blocked.step({1.0f, 0.0f}, 1.0f / 60.0f, &wall, 1);
    assert(blocked.position.x <= -0.08f);
    assert(std::abs(blocked.velocity.x) < 0.001f);

    const Aabb a{{0.0f, 0.0f}, {0.1f, 0.1f}};
    const Aabb b{{0.15f, 0.0f}, {0.1f, 0.1f}};
    assert(a.overlaps(b));

    const WaterVolume stream{{{0.0f, -0.28f}, {0.40f, 0.28f}}, -0.20f, {0.05f, 0.0f}, 0.80f, 3.0f};
    CharacterBody swimmer;
    swimmer.position = {0.0f, -0.20f};
    swimmer.grounded = false;
    swimmer.velocity = {0.6f, -0.2f};
    swimmer.step({0.0f, 0.0f}, 1.0f / 60.0f, nullptr, 0, &stream, 1);
    assert(swimmer.water.overlapping);
    assert(swimmer.water.depth > 0.45f);
    assert(swimmer.velocity.x < 0.6f);
    assert(swimmer.velocity.y > -0.2f);

    swimmer.applyImpulse({2.0f, 0.0f}, 1.0f);
    assert(std::sqrt(swimmer.velocity.x * swimmer.velocity.x + swimmer.velocity.y * swimmer.velocity.y) <= 1.001f);

    SecondaryMotion motion;
    motion.step({0.8f, 0.0f}, {0.02f, 0.0f}, 1.0f / 60.0f, false);
    for (int i = 0; i < 120; ++i) motion.step({0.8f, 0.0f}, {0.02f, 0.0f}, 1.0f / 60.0f, false);
    assert(std::abs(motion.hairOffset.x) > 0.001f);
    assert(std::abs(motion.clothOffset.x) > 0.001f);
    assert(motion.wetness < 0.1f);
    motion.step({0.0f, 0.0f}, {0.0f, 0.0f}, 1.0f / 60.0f, true);
    assert(motion.wetness > 0.0f);

    std::cout << "physics_test: PASS\n";
    return 0;
}
