#pragma once

namespace forest::physics {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
};

struct Aabb {
    Vec2 center{};
    Vec2 halfExtents{0.04f, 0.04f};

    bool overlaps(const Aabb& other) const;
};

struct StaticObstacle {
    Aabb bounds{};
};

struct WaterVolume {
    Aabb bounds{};
    float surfaceY = 0.0f;
    Vec2 current{};
    float buoyancy = 0.72f;
    float drag = 2.8f;
};

struct WaterState {
    bool overlapping = false;
    bool submerged = false;
    float depth = 0.0f;
    float surfaceY = 0.0f;
    Vec2 current{};
    float buoyancy = 0.0f;
    float drag = 0.0f;
};

struct SecondaryMotion {
    Vec2 hairOffset{};
    Vec2 hairVelocity{};
    Vec2 clothOffset{};
    Vec2 clothVelocity{};
    float wetness = 0.0f;

    void step(const Vec2& bodyVelocity, const Vec2& wind, float deltaSeconds, bool inWater);
};

struct CharacterBody {
    // Position and velocity are the walkable X/Z plane. Vertical jump motion
    // is intentionally separate so forward/back movement never fights gravity.
    Vec2 position{};
    Vec2 velocity{};
    Vec2 halfExtents{0.045f, 0.08f};
    float verticalPosition = 0.0f;
    float verticalVelocity = 0.0f;
    float acceleration = 7.2f;
    float maxSpeed = 0.55f;
    float friction = 8.5f;
    float gravity = -2.35f;
    float jumpVelocity = 1.08f;
    float waterSpeedMultiplier = 0.52f;
    float waterJumpMultiplier = 0.68f;
    bool grounded = true;
    WaterState water{};

    void step(const Vec2& input, float deltaSeconds,
              const StaticObstacle* obstacles, int obstacleCount,
              const WaterVolume* waterVolumes = nullptr, int waterCount = 0);
    void jump(bool allowGrace = false);
    void applyImpulse(const Vec2& impulse, float maxSpeed = 1.75f);
};

float approach(float current, float target, float maxDelta);
void resolveCollision(CharacterBody& body, const StaticObstacle& obstacle);
WaterState sampleWater(const CharacterBody& body, const WaterVolume* volumes, int volumeCount);

} // namespace forest::physics
