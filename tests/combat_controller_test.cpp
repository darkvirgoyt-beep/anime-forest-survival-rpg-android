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
    controller.body.position = {0.0f, -0.50f};
    controller.body.grounded = true;
    assert(controller.jump());
    assert(controller.state == forest::controller::LocomotionState::Jump);
    assert(!controller.jump());

    std::cout << "combat_controller_test: PASS\n";
    return 0;
}
