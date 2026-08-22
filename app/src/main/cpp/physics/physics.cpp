#include "physics.h"

#include <algorithm>
#include <cmath>

namespace forest::physics {

bool Aabb::overlaps(const Aabb& other) const {
    return std::abs(center.x - other.center.x) <= (halfExtents.x + other.halfExtents.x)
        && std::abs(center.y - other.center.y) <= (halfExtents.y + other.halfExtents.y);
}

float approach(float current, float target, float maxDelta) {
    if (current < target) return std::min(current + maxDelta, target);
    if (current > target) return std::max(current - maxDelta, target);
    return target;
}

void resolveCollision(CharacterBody& body, const StaticObstacle& obstacle) {
    const Aabb bodyBox{body.position, body.halfExtents};
    if (!bodyBox.overlaps(obstacle.bounds)) return;

    const float dx = obstacle.bounds.center.x - body.position.x;
    const float px = (obstacle.bounds.halfExtents.x + body.halfExtents.x) - std::abs(dx);
    const float dy = obstacle.bounds.center.y - body.position.y;
    const float py = (obstacle.bounds.halfExtents.y + body.halfExtents.y) - std::abs(dy);

    if (px < py) {
        body.position.x += dx < 0.0f ? px : -px;
        body.velocity.x = 0.0f;
    } else {
        body.position.y += dy < 0.0f ? py : -py;
        if (dy > 0.0f) body.grounded = true;
        body.velocity.y = 0.0f;
    }
}

void CharacterBody::step(const Vec2& input, float deltaSeconds, const StaticObstacle* obstacles, int obstacleCount) {
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.05f);
    const float inputLength = std::sqrt(input.x * input.x + input.y * input.y);
    const Vec2 direction = inputLength > 1.0f ? input * (1.0f / inputLength) : input;
    const float targetX = direction.x * maxSpeed;
    const float targetY = direction.y * maxSpeed;
    const float blend = acceleration * dt;

    velocity.x = approach(velocity.x, targetX, blend);
    velocity.y = approach(velocity.y, targetY, blend);
    if (inputLength < 0.05f) {
        velocity.x = approach(velocity.x, 0.0f, friction * dt);
        velocity.y = approach(velocity.y, 0.0f, friction * dt);
    }

    grounded = false;
    velocity.y += gravity * dt;
    position += velocity * dt;
    for (int i = 0; i < obstacleCount; ++i) resolveCollision(*this, obstacles[i]);

    if (position.y <= -0.50f) {
        position.y = -0.50f;
        velocity.y = 0.0f;
        grounded = true;
    }
    if (position.x <= -0.90f || position.x >= 0.90f) velocity.x = 0.0f;
    if (position.y <= -0.50f || position.y >= 0.52f) velocity.y = 0.0f;
    position.x = std::clamp(position.x, -0.90f, 0.90f);
    position.y = std::clamp(position.y, -0.50f, 0.52f);
}

void CharacterBody::jump() {
    if (!grounded) return;
    velocity.y = jumpVelocity;
    grounded = false;
}

} // namespace forest::physics
