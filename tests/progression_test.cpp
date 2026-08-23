#include <cassert>
#include <iostream>
#include <string>

#include "rpg/progression.h"

int main() {
    using forest::rpg::Progression;

    Progression progression;
    assert(progression.level == 0);
    assert(progression.experience == 0);
    assert(progression.experienceToNext == 10);
    assert(progression.totalExperience == 0);
    assert(progression.questStage == forest::rpg::QuestStage::GatherMaterials);

    assert(Progression::experienceRequirementForLevel(0) == 10);
    assert(Progression::experienceRequirementForLevel(1) == 40);
    assert(Progression::experienceRequirementForLevel(9) == 1000);
    assert(Progression::experienceRequirementForLevel(99) == 100000);
    assert(Progression::experienceRequirementForLevel(100) == 0);

    progression.recordGather();
    progression.recordGather();
    assert(progression.gatheringActions == 2);
    assert(progression.questStage == forest::rpg::QuestStage::GatherMaterials);
    progression.recordGather();
    assert(progression.questStage == forest::rpg::QuestStage::CraftEmberKit);
    assert(progression.level == 1);
    assert(progression.experience == 26);
    assert(progression.totalExperience == 36);

    progression.recordCraft();
    assert(progression.emberKitCrafted);
    assert(progression.questStage == forest::rpg::QuestStage::DefeatWarden);
    assert(progression.level == 2);
    assert(progression.experience == 21);
    assert(progression.totalExperience == 71);

    progression.recordWardenDefeat();
    assert(progression.wardenDefeated);
    assert(progression.questStage == forest::rpg::QuestStage::Complete);
    assert(progression.level == 3);
    assert(progression.experience == 51);
    assert(progression.totalExperience == 191);
    assert(std::string(progression.questObjective()).find("Heartfire restored") != std::string::npos);

    // Grinding remains available after the quest is complete.
    progression.awardGrindingXP(9);
    assert(progression.experience == 60);
    assert(progression.totalExperience == 200);

    Progression maxed;
    maxed.restoreState(99, 99999, 1, 0);
    maxed.awardGrindingXP(1);
    assert(maxed.level == 100);
    assert(maxed.experience == 0);
    assert(maxed.experienceToNext == 0);

    std::cout << "progression_test: PASS\n";
    return 0;
}
