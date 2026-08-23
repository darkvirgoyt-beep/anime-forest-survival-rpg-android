#include "progression.h"

#include <algorithm>

namespace forest::rpg {

void Progression::awardExperience(int amount) {
    if (amount <= 0 || (questStage == QuestStage::Complete && wardenDefeated)) return;
    experience += amount;
    while (experience >= experienceToNext) {
        experience -= experienceToNext;
        ++level;
        experienceToNext = 100 + (level - 1) * 35;
    }
}

void Progression::recordGather() {
    gatheringActions = std::min(gatheringActions + 1, 3);
    awardExperience(12);
    if (questStage == QuestStage::GatherMaterials && gatheringActions >= 3) {
        questStage = QuestStage::CraftEmberKit;
    }
}

void Progression::recordCraft() {
    if (questStage != QuestStage::CraftEmberKit) return;
    emberKitCrafted = true;
    questStage = QuestStage::DefeatWarden;
    awardExperience(35);
}

void Progression::recordWardenDefeat() {
    if (wardenDefeated) return;
    awardExperience(120);
    wardenDefeated = true;
    questStage = QuestStage::Complete;
}

const char* Progression::questObjective() const {
    switch (questStage) {
        case QuestStage::GatherMaterials:
            return "THE FIRST EMBER  •  Gather 3 resource caches";
        case QuestStage::CraftEmberKit:
            return "THE FIRST EMBER  •  Craft the ember kit";
        case QuestStage::DefeatWarden:
            return "THE FIRST EMBER  •  Defeat the forest warden";
        case QuestStage::Complete:
            return "THE FIRST EMBER  •  Quest complete — the wilds remember you";
    }
    return "THE FIRST EMBER";
}

} // namespace forest::rpg
