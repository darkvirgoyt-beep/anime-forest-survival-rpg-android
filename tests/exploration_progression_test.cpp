#include <cassert>
#include <iostream>

#include "rpg/cloud_state.h"

int main() {
    constexpr int kForestSector = 1;
    constexpr int kSandSector = 2;
    constexpr int kSnowSector = 4;
    constexpr int kDungeonSector = 8;

    forest::rpg::CloudState explorer{};
    assert(explorer.schemaVersion == 5);
    assert(explorer.discoveredSectors == kForestSector);

    // Exploration unlocks persist as a bit field so each discovered biome can
    // independently drive its content pack and landmark availability.
    explorer.discoveredSectors = kForestSector | kSandSector | kSnowSector | kDungeonSector;
    forest::rpg::CloudState restored{};
    assert(forest::rpg::parseCloudState(forest::rpg::serializeCloudState(explorer).c_str(), restored));
    assert(restored.schemaVersion == 5);
    assert(restored.discoveredSectors == 15);
    assert((restored.discoveredSectors & kSandSector) != 0);
    assert((restored.discoveredSectors & kSnowSector) != 0);
    assert((restored.discoveredSectors & kDungeonSector) != 0);

    // Corrupted or over-broad sector masks are bounded to the playable map.
    explorer.discoveredSectors = 47;
    assert(forest::rpg::parseCloudState(forest::rpg::serializeCloudState(explorer).c_str(), restored));
    assert(restored.discoveredSectors == 15);

    assert(forest::rpg::parseCloudState(
        "{\"schemaVersion\":4,\"playerX\":0,\"playerY\":0,\"health\":1,\"stamina\":1,\"hunger\":1,\"wood\":0,\"fiber\":0,\"stone\":0,\"experience\":0,\"level\":0,\"experienceToNext\":991,\"totalExperience\":0,\"day\":1,\"worldTime\":0,\"gatheringActions\":0,\"questStage\":0,\"emberKitCrafted\":0,\"wardenDefeated\":0,\"emberlingTrust\":0,\"emberlingBonded\":0,\"emberlingStay\":0}",
        restored));
    assert(restored.schemaVersion == 5 && restored.sourceSchemaVersion == 4);
    assert(restored.discoveredSectors == kForestSector);

    std::cout << "exploration_progression_test: PASS\n";
}
