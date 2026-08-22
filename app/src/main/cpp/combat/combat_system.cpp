#include "combat_system.h"

#include <algorithm>
#include <cmath>

namespace forest::combat {

namespace {
constexpr float kEpsilon = 0.0001f;
}

CombatSystem::CombatSystem()
    : attacks{
        AttackDefinition{0.08f, 0.10f, 0.26f, 0.18f, {{0.13f, 0.01f}, {0.13f, 0.08f}, 0.10f, 0.20f}},
        AttackDefinition{0.10f, 0.11f, 0.28f, 0.20f, {{0.15f, 0.01f}, {0.15f, 0.09f}, 0.13f, 0.25f}},
        AttackDefinition{0.14f, 0.14f, 0.38f, 0.24f, {{0.18f, 0.01f}, {0.18f, 0.11f}, 0.20f, 0.38f}}
    } {}

bool intersects(const physics::Aabb& a, const physics::Aabb& b) {
    return a.overlaps(b);
}

void CombatSystem::tick(float deltaSeconds) {
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.05f);
    if (hitStop > 0.0f) {
        hitStop = std::max(0.0f, hitStop - dt);
        return;
    }

    comboTimer = std::max(0.0f, comboTimer - dt);
    if (currentPhase == AttackPhase::None) {
        if (queuedAttack) {
            queuedAttack = false;
            currentCombo = comboTimer > 0.0f ? (currentCombo + 1) % kAttackCount : 0;
            phaseTimer = attacks[currentCombo].startup;
            currentPhase = AttackPhase::Startup;
            activeEventSent = false;
            pendingEvent.attackStarted = true;
            pendingEvent.comboIndex = currentCombo;
        }
        return;
    }

    phaseTimer -= dt;
    if (currentPhase == AttackPhase::Startup && phaseTimer <= kEpsilon) {
        currentPhase = AttackPhase::Active;
        phaseTimer = attacks[currentCombo].active;
        activeEventSent = false;
    } else if (currentPhase == AttackPhase::Active && phaseTimer <= kEpsilon) {
        currentPhase = AttackPhase::Recovery;
        phaseTimer = attacks[currentCombo].recovery;
        comboTimer = attacks[currentCombo].comboWindow;
    } else if (currentPhase == AttackPhase::Recovery && phaseTimer <= kEpsilon) {
        pendingEvent.attackFinished = true;
        currentPhase = AttackPhase::None;
        if (!queuedAttack) currentCombo = 0;
    }
}

bool CombatSystem::requestAttack() {
    if (currentPhase == AttackPhase::None || (currentPhase == AttackPhase::Recovery && comboTimer > 0.0f)) {
        queuedAttack = true;
        return true;
    }
    return false;
}

bool CombatSystem::requestDodge() {
    if (currentPhase == AttackPhase::None) return true;
    queuedAttack = false;
    currentPhase = AttackPhase::None;
    comboTimer = 0.0f;
    return true;
}

void CombatSystem::confirmHit() {
    if (!isHitActive()) return;
    pendingEvent.hitConfirmed = true;
    pendingEvent.comboIndex = currentCombo;
    hitStop = 0.06f;
}

CombatEvent CombatSystem::consumeEvent() {
    CombatEvent result = pendingEvent;
    pendingEvent = {};
    return result;
}

Hitbox CombatSystem::currentHitbox(const physics::Vec2& facing) const {
    const AttackDefinition& attack = attacks[currentCombo];
    Hitbox result = attack.hitbox;
    const float sign = facing.x < 0.0f ? -1.0f : 1.0f;
    result.offset.x *= sign;
    return result;
}

} // namespace forest::combat
