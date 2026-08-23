#include <cassert>
#include <iostream>

#include "rpg/emberling_companion.h"

int main() {
    forest::rpg::EmberlingState state{};
    int fiber = 3;
    assert(forest::rpg::interactWithEmberling(state, false, fiber, state.x, state.y) == forest::rpg::EmberlingInteraction::NeedsEmberKit);
    assert(fiber == 3);
    assert(forest::rpg::interactWithEmberling(state, true, fiber, state.x, state.y) == forest::rpg::EmberlingInteraction::BondAdvanced);
    assert(state.trust == 1 && fiber == 2 && !state.bonded);
    assert(forest::rpg::interactWithEmberling(state, true, fiber, state.x, state.y) == forest::rpg::EmberlingInteraction::BondAdvanced);
    assert(forest::rpg::interactWithEmberling(state, true, fiber, state.x, state.y) == forest::rpg::EmberlingInteraction::Bonded);
    assert(state.bonded && state.trust == 3 && fiber == 0);
    assert(forest::rpg::interactWithEmberling(state, true, fiber, state.x, state.y) == forest::rpg::EmberlingInteraction::CommandChanged);
    assert(state.stay);
    state.stay = false;
    forest::rpg::updateEmberling(state, 0.50f, -0.20f, 0.10f, false);
    assert(state.x > -0.46f);
    std::cout << "emberling_companion_test: PASS\n";
}
