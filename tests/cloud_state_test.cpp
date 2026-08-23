#include <cassert>
#include <cmath>
#include <iostream>

#include "rpg/cloud_state.h"

int main() {
    forest::rpg::CloudState original{};
    original.playerX = 0.34f;
    original.playerY = -0.22f;
    original.health = 0.68f;
    original.stamina = 0.42f;
    original.hunger = 0.73f;
    original.wood = 19;
    original.fiber = 7;
    original.stone = 12;
    original.experience = 84;
    original.day = 4;
    original.worldTime = 512.5f;
    original.gatheringActions = 3;
    original.questStage = 3;
    original.emberKitCrafted = true;
    original.wardenDefeated = true;

    forest::rpg::CloudState restored{};
    assert(forest::rpg::parseCloudState(forest::rpg::serializeCloudState(original).c_str(), restored));
    assert(std::abs(restored.playerX - original.playerX) < 0.001f);
    assert(restored.wood == original.wood);
    assert(restored.day == original.day);
    assert(restored.gatheringActions == 3 && restored.questStage == 3);
    assert(restored.emberKitCrafted && restored.wardenDefeated);
    assert(!forest::rpg::parseCloudState("{\"schemaVersion\":2}", restored));
    assert(forest::rpg::parseCloudState("{\"schemaVersion\":1,\"playerX\":0,\"playerY\":0,\"health\":9,\"stamina\":-2,\"hunger\":2,\"wood\":-4,\"fiber\":-3,\"stone\":-2,\"experience\":-1,\"day\":0,\"worldTime\":-5}", restored));
    assert(restored.health == 1.0f && restored.stamina == 0.0f && restored.wood == 0 && restored.day == 1);
    assert(restored.questStage == 0 && !restored.emberKitCrafted && !restored.wardenDefeated);
    std::cout << "cloud_state_test: PASS\n";
}
