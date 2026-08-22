#include "../app/src/main/cpp/physics/physics.h"

#include <cassert>
#include <cmath>
#include <iostream>

using forest::physics::Aabb;
using forest::physics::CharacterBody;
using forest::physics::StaticObstacle;
using forest::physics::Vec2;

int main() {
    CharacterBody body;
    body.position = {0.0f, -0.50f};
    body.grounded = true;
    body.step({1.0f, 0.0f}, 1.0f / 60.0f, nullptr, 0);
    assert(body.position.x > 0.0f);
    assert(std::abs(body.position.y - (-0.50f)) < 0.01f);

    body.jump();
    assert(body.velocity.y > 0.0f);
    body.jump();
    assert(body.velocity.y > 0.0f);

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
    std::cout << "physics_test: PASS\n";
    return 0;
}
