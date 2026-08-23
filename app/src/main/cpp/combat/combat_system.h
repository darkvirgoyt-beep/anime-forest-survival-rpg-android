#pragma once

#include "../physics/physics.h"

namespace forest::combat {

enum class AttackPhase {
    None,
    Startup,
    Active,
    Recovery
};

struct Hitbox {
    physics::Vec2 offset{};
    physics::Vec2 halfExtents{0.12f, 0.08f};
    float damage = 0.10f;
    float knockback = 0.20f;
};

struct AttackDefinition {
    float startup = 0.10f;
    float active = 0.10f;
    float recovery = 0.26f;
    float comboWindow = 0.20f;
    Hitbox hitbox{};
};

struct CombatEvent {
    bool attackStarted = false;
    bool heavyAttack = false;
    bool hitConfirmed = false;
    bool attackFinished = false;
    int comboIndex = 0;
    float hitStopSeconds = 0.0f;
};

class CombatSystem {
public:
    CombatSystem();
    void tick(float deltaSeconds);
    bool requestAttack();
    bool requestHeavyAttack();
    bool requestDodge();
    void confirmHit();
    CombatEvent consumeEvent();
    Hitbox currentHitbox(const physics::Vec2& facing) const;
    AttackPhase phase() const { return currentPhase; }
    int comboIndex() const { return currentCombo; }
    bool isHitActive() const { return currentPhase == AttackPhase::Active; }
    bool isHeavyAttack() const { return currentHeavy; }
    float hitStopSeconds() const { return hitStop; }

private:
    static constexpr int kAttackCount = 3;
    AttackDefinition attacks[kAttackCount]{};
    AttackDefinition heavyAttack{};
    AttackPhase currentPhase = AttackPhase::None;
    int currentCombo = 0;
    float phaseTimer = 0.0f;
    float comboTimer = 0.0f;
    float hitStop = 0.0f;
    bool queuedAttack = false;
    bool queuedHeavyAttack = false;
    bool currentHeavy = false;
    bool activeEventSent = false;
    CombatEvent pendingEvent{};
};

bool intersects(const physics::Aabb& a, const physics::Aabb& b);

} // namespace forest::combat
