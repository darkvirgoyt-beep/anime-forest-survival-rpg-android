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
    original.experience = 840;
    original.level = 8;
    original.experienceToNext = 810;
    original.totalExperience = 5040;
    original.day = 4;
    original.worldTime = 512.5f;
    original.gatheringActions = 3;
    original.questStage = 3;
    original.emberKitCrafted = true;
    original.wardenDefeated = true;
    original.emberlingTrust = 3;
    original.emberlingBonded = true;
    original.emberlingStay = true;
    original.discoveredSectors = 15;
    original.capturedMobIndex = 2;
    original.capturedCompanionStay = true;
    original.campBuilt = true;
    original.campX = -0.44f;
    original.campY = 0.31f;
    original.campZ = 1.2f;
    original.campYaw = -0.7f;
    original.campScale = 1.15f;
    original.companionRevision = 3;
    original.campRevision = 2;

    forest::rpg::CloudState restored{};
    assert(forest::rpg::parseCloudState(forest::rpg::serializeCloudState(original).c_str(), restored));
    assert(restored.schemaVersion == 5);
    assert(std::abs(restored.playerX - original.playerX) < 0.001f);
    assert(restored.wood == original.wood);
    assert(restored.day == original.day);
    assert(restored.level == original.level);
    assert(restored.experience == original.experience);
    assert(restored.experienceToNext == original.experienceToNext);
    assert(restored.totalExperience == original.totalExperience);
    assert(restored.gatheringActions == 3 && restored.questStage == 3);
    assert(restored.emberKitCrafted && restored.wardenDefeated);
    assert(restored.emberlingTrust == 3 && restored.emberlingBonded && restored.emberlingStay);
    assert(restored.discoveredSectors == 15);
    assert(restored.capturedMobIndex == 2 && restored.capturedCompanionStay && restored.campBuilt);
    assert(std::abs(restored.campX - original.campX) < 0.001f && std::abs(restored.campY - original.campY) < 0.001f);
    assert(std::abs(restored.campZ - original.campZ) < 0.001f && std::abs(restored.campYaw - original.campYaw) < 0.001f && std::abs(restored.campScale - original.campScale) < 0.001f);
    assert(restored.companionRevision == 3 && restored.campRevision == 2);

    assert(!forest::rpg::parseCloudState("{\"schemaVersion\":2}", restored));
    assert(forest::rpg::parseCloudState("{\"schemaVersion\":2,\"playerX\":0,\"playerY\":0,\"health\":0.5,\"stamina\":0.5,\"hunger\":0.5,\"wood\":1,\"fiber\":2,\"stone\":3,\"experience\":84,\"day\":2,\"worldTime\":5,\"gatheringActions\":1,\"questStage\":1,\"emberKitCrafted\":0,\"wardenDefeated\":0}", restored));
    assert(restored.schemaVersion == 2 && restored.experience == 84);
    assert(restored.discoveredSectors == 1);
    assert(restored.level == 0 && restored.experienceToNext == 991 && restored.totalExperience == 0);
    assert(restored.emberlingTrust == 0 && !restored.emberlingBonded && !restored.emberlingStay);
    assert(restored.capturedMobIndex == -1 && !restored.capturedCompanionStay && !restored.campBuilt);
    assert(restored.companionRevision == 0 && restored.campRevision == 0);

    assert(forest::rpg::parseCloudState("{\"schemaVersion\":1,\"playerX\":0,\"playerY\":0,\"health\":9,\"stamina\":-2,\"hunger\":2,\"wood\":-4,\"fiber\":-3,\"stone\":-2,\"experience\":-1,\"day\":0,\"worldTime\":-5}", restored));
    assert(restored.schemaVersion == 1);
    assert(restored.health == 1.0f && restored.stamina == 0.0f && restored.wood == 0 && restored.day == 1);
    assert(restored.experience == 0 && restored.level == 0 && restored.experienceToNext == 991);
    assert(restored.questStage == 0 && !restored.emberKitCrafted && !restored.wardenDefeated);
    std::cout << "cloud_state_test: PASS\n";
}
