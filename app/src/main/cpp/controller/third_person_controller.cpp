#include "third_person_controller.h"

#include <algorithm>
#include <cmath>

namespace forest::controller {

namespace {
constexpr float PI = 3.14159265359f;
constexpr float kSprintSpeed = 0.78f;
constexpr float kSprintStaminaPerSecond = 0.16f;
constexpr float kStaminaRecoveryPerSecond = 0.13f;
constexpr float kDodgeCost = 0.25f;
constexpr float kDodgeDuration = 0.30f;
constexpr float kSlideCost = 0.18f;
constexpr float kSlideDuration = 0.38f;
constexpr float kInvulnerabilityDuration = 0.42f;
constexpr float kHitstunDuration = 0.18f;
}

void CameraState::orbit(float deltaYaw, float deltaPitch) {
    yaw += deltaYaw;
    // Allow almost 180 degrees of vertical travel while avoiding the exact
    // pole singularity. Combined with the full 360 degree yaw loop, this
    // gives the intended 540 degree-class third-person orbit range.
    pitch = std::clamp(pitch + deltaPitch, -1.52f, 1.52f);
    while (yaw > PI) yaw -= PI * 2.0f;
    while (yaw < -PI) yaw += PI * 2.0f;
}

void CameraState::resolveObstruction(float measuredDistance, float deltaSeconds) {
    targetDistance = std::clamp(measuredDistance, minDistance, maxDistance);
    const float blend = 1.0f - std::exp(-18.0f * std::clamp(deltaSeconds, 0.0f, 0.05f));
    distance += (targetDistance - distance) * blend;
}

void ThirdPersonController::tick(const InputFrame& input, float deltaSeconds,
                                 const physics::StaticObstacle* obstacles, int obstacleCount,
                                 const physics::WaterVolume* waterVolumes, int waterCount) {
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.05f);
    motionTime += dt;
    invulnerabilitySeconds = std::max(0.0f, invulnerabilitySeconds - dt);
    hitstunSeconds = std::max(0.0f, hitstunSeconds - dt);

    if (!isAlive()) {
        state = LocomotionState::Dead;
        secondaryMotion.step(body.velocity, {0.0f, 0.0f}, dt, body.water.overlapping);
        return;
    }
    if (hitstunSeconds > 0.0f) {
        state = LocomotionState::Hitstun;
        const physics::Vec2 reducedInput{input.moveX * 0.15f, input.moveY * 0.15f};
        body.step(reducedInput, dt, obstacles, obstacleCount, waterVolumes, waterCount);
        secondaryMotion.step(body.velocity, {0.015f * std::sin(motionTime * 0.7f), 0.0f}, dt, body.water.overlapping);
        return;
    }

    if (dodgeSeconds > 0.0f) {
        dodgeSeconds = std::max(0.0f, dodgeSeconds - dt);
        body.velocity = dodgeVelocity;
        body.step({0.0f, 0.0f}, dt, obstacles, obstacleCount, waterVolumes, waterCount);
        secondaryMotion.step(body.velocity, {0.015f * std::sin(motionTime * 0.7f), 0.0f}, dt, body.water.overlapping);
        state = dodgeSeconds > 0.0f ? LocomotionState::Dodge : LocomotionState::Idle;
        return;
    }
    if (slideSeconds > 0.0f) {
        slideSeconds = std::max(0.0f, slideSeconds - dt);
        body.velocity = dodgeVelocity;
        body.step({0.0f, 0.0f}, dt, obstacles, obstacleCount, waterVolumes, waterCount);
        secondaryMotion.step(body.velocity, {0.015f * std::sin(motionTime * 0.7f), 0.0f}, dt, body.water.overlapping);
        state = slideSeconds > 0.0f ? LocomotionState::Slide : LocomotionState::Idle;
        return;
    }

    const float inputMagnitude = std::sqrt(input.moveX * input.moveX + input.moveY * input.moveY);
    const bool moving = inputMagnitude > 0.08f;
    const bool sprinting = moving && input.sprintHeld && stamina > 0.01f && !body.water.submerged;
    body.maxSpeed = sprinting ? kSprintSpeed : 0.55f;
    if (sprinting) stamina = std::max(0.0f, stamina - kSprintStaminaPerSecond * dt);
    else stamina = std::min(maxStamina, stamina + kStaminaRecoveryPerSecond * dt);

    const float cosYaw = std::cos(camera.yaw);
    const float sinYaw = std::sin(camera.yaw);
    physics::Vec2 cameraRelative{
        input.moveX * cosYaw - input.moveY * sinYaw,
        input.moveX * sinYaw + input.moveY * cosYaw
    };
    if (moving) {
        const float directionLength = std::sqrt(cameraRelative.x * cameraRelative.x + cameraRelative.y * cameraRelative.y);
        if (directionLength > 0.001f) {
            lastMoveDirection = cameraRelative * (1.0f / directionLength);
            facingRadians = std::atan2(cameraRelative.y, cameraRelative.x);
        }
    }
    body.step(cameraRelative, dt, obstacles, obstacleCount, waterVolumes, waterCount);
    secondaryMotion.step(body.velocity, {0.018f * std::sin(motionTime * 0.7f), 0.004f * std::cos(motionTime * 0.43f)}, dt, body.water.overlapping);

    if (body.water.submerged) state = LocomotionState::Swim;
    else if (!body.grounded) state = body.velocity.y > 0.0f ? LocomotionState::Jump : LocomotionState::Fall;
    else if (sprinting) state = LocomotionState::Sprint;
    else if (moving) state = LocomotionState::Walk;
    else state = LocomotionState::Idle;
}

bool ThirdPersonController::jump() {
    if (!isAlive() || dodgeSeconds > 0.0f || stamina < 0.12f) return false;
    if (!body.grounded && !body.water.submerged) return false;
    stamina -= 0.12f;
    body.jump();
    state = LocomotionState::Jump;
    return true;
}

bool ThirdPersonController::dodge() {
    if (!isAlive() || dodgeSeconds > 0.0f || stamina < kDodgeCost) return false;
    stamina -= kDodgeCost;
    dodgeSeconds = kDodgeDuration;
    invulnerabilitySeconds = kInvulnerabilityDuration;
    const float speed = body.water.overlapping ? 0.58f : 0.90f;
    dodgeVelocity = lastMoveDirection * speed;
    body.velocity = dodgeVelocity;
    state = LocomotionState::Dodge;
    return true;
}

bool ThirdPersonController::slide() {
    if (!isAlive() || !body.grounded || body.water.overlapping || dodgeSeconds > 0.0f || slideSeconds > 0.0f || stamina < kSlideCost) return false;
    stamina -= kSlideCost;
    slideSeconds = kSlideDuration;
    invulnerabilitySeconds = 0.12f;
    dodgeVelocity = lastMoveDirection * 0.72f;
    body.velocity = dodgeVelocity;
    state = LocomotionState::Slide;
    return true;
}

bool ThirdPersonController::takeDamage(float amount, const physics::Vec2& knockback) {
    if (!isAlive() || isInvulnerable()) return false;
    health = std::max(0.0f, health - std::max(0.0f, amount));
    body.applyImpulse(knockback);
    hitstunSeconds = kHitstunDuration;
    state = health > 0.0f ? LocomotionState::Hitstun : LocomotionState::Dead;
    return true;
}

} // namespace forest::controller
