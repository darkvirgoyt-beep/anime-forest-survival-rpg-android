#include "../app/src/main/cpp/combat/combat_system.h"
#include "../app/src/main/cpp/controller/third_person_controller.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    forest::combat::CombatSystem combat;
    assert(combat.requestAttack());
    combat.tick(0.05f);
    combat.tick(0.05f);
    combat.tick(0.05f);
    assert(combat.phase() == forest::combat::AttackPhase::Active);
    const auto hitbox = combat.currentHitbox({1.0f, 0.0f});
    assert(hitbox.damage > 0.0f);
    combat.confirmHit();
    const auto hitEvent = combat.consumeEvent();
    assert(hitEvent.hitConfirmed);
    assert(combat.hitStopSeconds() > 0.0f);

    forest::combat::CombatSystem heavyCombat;
    assert(heavyCombat.requestHeavyAttack());
    heavyCombat.tick(0.30f);
    const auto heavyStart = heavyCombat.consumeEvent();
    assert(heavyStart.attackStarted && heavyStart.heavyAttack);
    assert(heavyCombat.isHeavyAttack());
    assert(heavyCombat.currentHitbox({1.0f, 0.0f}).damage > hitbox.damage);

    forest::controller::ThirdPersonController controller;
    controller.body.position = {0.0f, -0.50f};
    controller.body.grounded = true;
    assert(controller.dodge());
    assert(controller.state == forest::controller::LocomotionState::Dodge);
    assert(controller.isInvulnerable());
    const float healthBefore = controller.health;
    assert(!controller.takeDamage(0.25f, {0.5f, 0.0f}));
    assert(std::abs(controller.health - healthBefore) < 0.0001f);

    controller = {};
    controller.body.grounded = true;
    assert(controller.takeDamage(0.25f, {0.0f, 0.0f}));
    const float damagedHealth = controller.health;
    for (int i = 0; i < 60; ++i) {
        controller.tick({}, 1.0f / 60.0f, nullptr, 0, nullptr, 0);
    }
    assert(std::abs(controller.health - damagedHealth) < 0.0001f);
    for (int i = 0; i < 150; ++i) {
        controller.tick({}, 1.0f / 60.0f, nullptr, 0, nullptr, 0);
    }
    assert(controller.health > damagedHealth);
    assert(controller.health <= controller.maxHealth);

    controller = {};
    controller.body.position = {0.0f, -0.50f};
    controller.body.grounded = true;
    assert(controller.jump());
    assert(controller.state == forest::controller::LocomotionState::Jump);
    assert(!controller.jump());

    controller = {};
    controller.body.position = {0.0f, -0.50f};
    controller.body.grounded = true;
    const forest::controller::InputFrame walkInput{1.0f, 0.0f, 0.0f, false};
    controller.tick(walkInput, 1.0f / 60.0f, nullptr, 0, nullptr, 0);
    assert(controller.body.velocity.x > 0.0f);
    assert(controller.state == forest::controller::LocomotionState::Walk);

    const float staminaBeforeSprint = controller.stamina;
    const forest::controller::InputFrame sprintInput{1.0f, 0.0f, 0.0f, true};
    controller.tick(sprintInput, 1.0f / 60.0f, nullptr, 0, nullptr, 0);
    assert(controller.state == forest::controller::LocomotionState::Sprint);
    assert(controller.stamina < staminaBeforeSprint);

    const forest::physics::WaterVolume stream{
        {{0.0f, -0.25f}, {0.30f, 0.30f}}, -0.05f, {0.02f, 0.0f}, 0.80f, 3.0f
    };
    controller.body.position = {0.0f, -0.20f};
    controller.tick(walkInput, 1.0f / 60.0f, nullptr, 0, &stream, 1);
    assert(controller.body.water.overlapping);
    assert(controller.state == forest::controller::LocomotionState::Swim || controller.state == forest::controller::LocomotionState::Walk);

    std::cout << "combat_controller_test: PASS\n";
    return 0;
}
