#pragma once

#include "../physics/physics.h"

namespace forest::controller {

enum class LocomotionState {
    Idle,
    Walk,
    Sprint,
    Jump,
    Fall,
    Dodge,
    Attack,
    Hitstun,
    Dead
};

struct InputFrame {
    float moveX = 0.0f;
    float moveY = 0.0f;
    float cameraYaw = 0.0f;
    bool sprintHeld = false;
};

struct CameraState {
    float yaw = 0.0f;
    float pitch = -0.24f;
    float distance = 3.8f;
    float targetDistance = 3.8f;
    float minDistance = 1.4f;
    float maxDistance = 5.5f;

    void orbit(float deltaYaw, float deltaPitch);
    void resolveObstruction(float measuredDistance, float deltaSeconds);
};

class ThirdPersonController {
public:
    physics::CharacterBody body{};
    CameraState camera{};
    LocomotionState state = LocomotionState::Idle;
    float facingRadians = 0.0f;
    float stamina = 1.0f;
    float maxStamina = 1.0f;
    float health = 1.0f;
    float maxHealth = 1.0f;
    float invulnerabilitySeconds = 0.0f;
    float hitstunSeconds = 0.0f;

    void tick(const InputFrame& input, float deltaSeconds,
              const physics::StaticObstacle* obstacles, int obstacleCount);
    bool jump();
    bool dodge();
    bool takeDamage(float amount, const physics::Vec2& knockback);
    bool isInvulnerable() const { return invulnerabilitySeconds > 0.0f; }
    bool isAlive() const { return health > 0.0f; }

private:
    float dodgeSeconds = 0.0f;
    physics::Vec2 dodgeVelocity{};
};

} // namespace forest::controller
