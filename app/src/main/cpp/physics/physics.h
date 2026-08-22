#pragma once

namespace forest::physics {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
};

struct Aabb {
    Vec2 center{};
    Vec2 halfExtents{0.04f, 0.04f};

    bool overlaps(const Aabb& other) const;
};

struct StaticObstacle {
    Aabb bounds{};
};

struct CharacterBody {
    Vec2 position{};
    Vec2 velocity{};
    Vec2 halfExtents{0.045f, 0.08f};
    float acceleration = 5.8f;
    float maxSpeed = 0.55f;
    float friction = 7.0f;
    float gravity = -1.5f;
    float jumpVelocity = 0.72f;
    bool grounded = true;

    void step(const Vec2& input, float deltaSeconds, const StaticObstacle* obstacles, int obstacleCount);
    void jump();
};

float approach(float current, float target, float maxDelta);
void resolveCollision(CharacterBody& body, const StaticObstacle& obstacle);

} // namespace forest::physics
