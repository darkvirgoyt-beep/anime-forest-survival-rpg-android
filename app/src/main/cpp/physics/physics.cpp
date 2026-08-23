#include "physics.h"

#include <algorithm>
#include <cmath>

namespace forest::physics {

namespace {
constexpr float kSkinWidth = 0.005f;
constexpr float kEpsilon = 0.0001f;

float clampMagnitude(float value, float limit) {
    return std::clamp(value, -limit, limit);
}

float criticallyDampedSpring(float& value, float& velocity, float target,
                             float frequency, float deltaSeconds) {
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.05f);
    const float omega = frequency * 2.0f * 3.14159265359f;
    const float acceleration = (target - value) * omega * omega - velocity * 2.0f * omega;
    velocity += acceleration * dt;
    value += velocity * dt;
    return value;
}
}

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
        body.position.x += dx < 0.0f ? px + kSkinWidth : -px - kSkinWidth;
        body.velocity.x = 0.0f;
    } else {
        body.position.y += dy < 0.0f ? py + kSkinWidth : -py - kSkinWidth;
        // Obstacles are walls in the walkable X/Z plane; they do not provide
        // vertical support for a character that is airborne.
        body.velocity.y = 0.0f;
    }
}

WaterState sampleWater(const CharacterBody& body, const WaterVolume* volumes, int volumeCount) {
    WaterState result{};
    if (volumes == nullptr || volumeCount <= 0) return result;

    const float bottom = body.position.y - body.halfExtents.y;
    const float top = body.position.y + body.halfExtents.y;
    for (int i = 0; i < volumeCount; ++i) {
        const WaterVolume& volume = volumes[i];
        const Aabb bodyBox{body.position, body.halfExtents};
        if (!bodyBox.overlaps(volume.bounds)) continue;

        const float depth = std::clamp((volume.surfaceY - bottom) / (body.halfExtents.y * 2.0f), 0.0f, 1.0f);
        if (!result.overlapping || depth > result.depth) {
            result.overlapping = true;
            result.depth = depth;
            result.submerged = top <= volume.surfaceY;
            result.surfaceY = volume.surfaceY;
            result.current = volume.current;
            result.buoyancy = volume.buoyancy;
            result.drag = volume.drag;
        }
    }
    return result;
}

void CharacterBody::applyImpulse(const Vec2& impulse, float maxImpulseSpeed) {
    velocity += impulse;
    const float magnitude = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (magnitude <= maxImpulseSpeed || magnitude < kEpsilon) return;
    const float scale = maxImpulseSpeed / magnitude;
    velocity = velocity * scale;
}

void CharacterBody::step(const Vec2& input, float deltaSeconds,
                         const StaticObstacle* obstacles, int obstacleCount,
                         const WaterVolume* waterVolumes, int waterCount) {
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.05f);
    water = sampleWater(*this, waterVolumes, waterCount);

    const float inputLength = std::sqrt(input.x * input.x + input.y * input.y);
    const Vec2 direction = inputLength > 1.0f ? input * (1.0f / inputLength) : input;
    const float locomotionMultiplier = water.overlapping ? waterSpeedMultiplier : 1.0f;
    const float targetX = direction.x * maxSpeed * locomotionMultiplier;
    const float targetY = direction.y * maxSpeed * locomotionMultiplier;
    const float controlScale = grounded ? 1.0f : 0.42f;
    const float response = 1.0f - std::exp(-acceleration * controlScale * (water.overlapping ? 0.72f : 1.0f) * dt);

    // Exponential response reaches the requested speed smoothly and remains
    // stable when a device drops a frame, unlike a frame-sized linear step.
    velocity.x += (targetX - velocity.x) * response;
    velocity.y += (targetY - velocity.y) * response;
    if (inputLength < 0.05f) {
        const float damping = std::exp(-friction * (grounded ? 1.0f : 0.38f) * dt);
        velocity.x = water.current.x + (velocity.x - water.current.x) * damping;
        velocity.y = water.current.y + (velocity.y - water.current.y) * damping;
    }

    // Vertical motion is independent from the X/Z walkable plane. This keeps
    // forward/back input responsive while a jump is in progress.
    verticalVelocity += gravity * dt;
    if (water.overlapping) {
        const float depth = water.depth;
        const float buoyancyForce = -gravity * water.buoyancy * depth;
        verticalVelocity += buoyancyForce * dt;
        const float dragFactor = std::exp(-water.drag * (0.72f + depth * 0.88f) * dt);
        velocity.x = velocity.x * dragFactor + water.current.x * dt;
        velocity.y = velocity.y * dragFactor + water.current.y * dt;
        if (depth > 0.82f && verticalVelocity < 0.0f) verticalVelocity *= 0.35f;
    }

    verticalPosition += verticalVelocity * dt;
    grounded = false;
    if (verticalPosition <= 0.0f) {
        verticalPosition = 0.0f;
        verticalVelocity = 0.0f;
        grounded = true;
    }

    position += velocity * dt;
    for (int i = 0; i < obstacleCount; ++i) resolveCollision(*this, obstacles[i]);

    if (position.x <= -0.90f || position.x >= 0.90f) velocity.x = 0.0f;
    if (position.y <= -0.50f || position.y >= 0.52f) velocity.y = 0.0f;
    position.x = std::clamp(position.x, -0.90f, 0.90f);
    position.y = std::clamp(position.y, -0.50f, 0.52f);
    water = sampleWater(*this, waterVolumes, waterCount);
}

void CharacterBody::jump(bool allowGrace) {
    if (!grounded && !allowGrace) return;
    verticalVelocity = jumpVelocity * (water.overlapping ? waterJumpMultiplier : 1.0f);
    verticalPosition = 0.001f;
    grounded = false;
}

void SecondaryMotion::step(const Vec2& bodyVelocity, const Vec2& wind,
                           float deltaSeconds, bool inWater) {
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.05f);
    const float wetTarget = inWater ? 1.0f : 0.0f;
    wetness = approach(wetness, wetTarget, dt * (inWater ? 3.4f : 0.55f));

    const Vec2 drag = bodyVelocity * -0.035f + wind;
    const float hairTargetX = clampMagnitude(drag.x * (1.0f - wetness * 0.28f), 0.08f);
    const float hairTargetY = clampMagnitude(drag.y * 0.45f + std::sin(bodyVelocity.x * 3.0f) * 0.008f, 0.05f);
    const float clothTargetX = clampMagnitude(drag.x * 0.72f, 0.11f);
    const float clothTargetY = clampMagnitude(drag.y * 0.95f - std::abs(bodyVelocity.x) * 0.012f, 0.08f);

    criticallyDampedSpring(hairOffset.x, hairVelocity.x, hairTargetX, 3.6f - wetness * 1.0f, dt);
    criticallyDampedSpring(hairOffset.y, hairVelocity.y, hairTargetY, 3.3f - wetness * 0.8f, dt);
    criticallyDampedSpring(clothOffset.x, clothVelocity.x, clothTargetX, 2.4f, dt);
    criticallyDampedSpring(clothOffset.y, clothVelocity.y, clothTargetY, 2.1f, dt);
}

} // namespace forest::physics
