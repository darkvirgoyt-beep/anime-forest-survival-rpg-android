#include <cassert>
#include <iostream>
#include <string>

#include "rpg/cloud_state.h"

namespace {

std::string payloadWithBiomeMask(int biomeMask) {
    forest::rpg::CloudState state{};
    const std::string marker = "\"discoveredSectors\":";
    std::string payload = forest::rpg::serializeCloudState(state);
    const std::size_t start = payload.find(marker);
    assert(start != std::string::npos);
    const std::size_t valueStart = start + marker.size();
    const std::size_t valueEnd = payload.find('}', valueStart);
    assert(valueEnd != std::string::npos);
    payload.replace(valueStart, valueEnd - valueStart, std::to_string(biomeMask));
    return payload;
}

void expectNormalizedBiomeMask(int inputMask, int expectedMask) {
    forest::rpg::CloudState restored{};
    assert(forest::rpg::parseCloudState(payloadWithBiomeMask(inputMask).c_str(), restored));
    assert(restored.schemaVersion == 5);
    assert(restored.discoveredSectors == expectedMask);
}

} // namespace

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

    // Zero, negative, stale future bits, and sparse sector unlocks must never
    // remove Forest, grant unknown biomes, or leave recovery without a spawn.
    expectNormalizedBiomeMask(0, kForestSector);
    expectNormalizedBiomeMask(-7, kForestSector);
    expectNormalizedBiomeMask(16, kForestSector);
    expectNormalizedBiomeMask(kSandSector, kForestSector | kSandSector);
    expectNormalizedBiomeMask(kSnowSector, kForestSector | kSnowSector);
    expectNormalizedBiomeMask(kDungeonSector, kForestSector | kDungeonSector);
    expectNormalizedBiomeMask(18, kForestSector | kSandSector);
    expectNormalizedBiomeMask(47, 15);

    // Serialization applies the same rule, so a sparse runtime state cannot
    // write a save that launches outside the Forest recovery sector.
    explorer.discoveredSectors = kDungeonSector;
    assert(forest::rpg::parseCloudState(forest::rpg::serializeCloudState(explorer).c_str(), restored));
    assert(restored.discoveredSectors == (kForestSector | kDungeonSector));

    assert(forest::rpg::parseCloudState(
        "{\"schemaVersion\":4,\"playerX\":0,\"playerY\":0,\"health\":1,\"stamina\":1,\"hunger\":1,\"wood\":0,\"fiber\":0,\"stone\":0,\"experience\":0,\"level\":0,\"experienceToNext\":991,\"totalExperience\":0,\"day\":1,\"worldTime\":0,\"gatheringActions\":0,\"questStage\":0,\"emberKitCrafted\":0,\"wardenDefeated\":0,\"emberlingTrust\":0,\"emberlingBonded\":0,\"emberlingStay\":0}",
        restored));
    assert(restored.schemaVersion == 5 && restored.sourceSchemaVersion == 4);
    assert(restored.discoveredSectors == kForestSector);

    std::cout << "exploration_progression_test: PASS\n";
}
