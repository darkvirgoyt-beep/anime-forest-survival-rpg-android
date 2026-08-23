#include "../app/src/main/cpp/rpg/progression.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    forest::rpg::Progression progression;
    assert(progression.level == 1);
    assert(progression.questStage == forest::rpg::QuestStage::GatherMaterials);

    progression.recordGather();
    progression.recordGather();
    assert(progression.gatheringActions == 2);
    assert(progression.questStage == forest::rpg::QuestStage::GatherMaterials);
    progression.recordGather();
    assert(progression.questStage == forest::rpg::QuestStage::CraftEmberKit);
    assert(progression.experience == 36);

    progression.recordCraft();
    assert(progression.emberKitCrafted);
    assert(progression.questStage == forest::rpg::QuestStage::DefeatWarden);
    assert(progression.experience == 71);

    progression.recordWardenDefeat();
    assert(progression.wardenDefeated);
    assert(progression.questStage == forest::rpg::QuestStage::Complete);
    assert(progression.level == 2);
    assert(progression.experience == 91);
    assert(std::string(progression.questObjective()).find("Quest complete") != std::string::npos);

    std::cout << "progression_test: PASS\n";
    return 0;
}
