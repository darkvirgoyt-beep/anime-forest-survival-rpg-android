#include "progression.h"

#include <algorithm>

namespace forest::rpg {

int Progression::experienceRequirementForLevel(int targetLevel) {
    const int safeLevel = std::clamp(targetLevel, 0, kMaxLevel);
    if (safeLevel >= kMaxLevel) return 0;
    const int nextLevel = safeLevel + 1;
    // Exact integer curve. The final transition, 99 -> 100, is exactly 100000 XP.
    return 10 * nextLevel * nextLevel;
}

void Progression::awardExperience(int amount) {
    if (amount <= 0) return;
    totalExperience = std::max(0, totalExperience) + amount;
    if (level >= kMaxLevel) {
        level = kMaxLevel;
        experience = 0;
        experienceToNext = 0;
        return;
    }

    experience = std::max(0, experience) + amount;
    while (level < kMaxLevel) {
        experienceToNext = experienceRequirementForLevel(level);
        if (experience < experienceToNext) break;
        experience -= experienceToNext;
        ++level;
    }

    if (level >= kMaxLevel) {
        level = kMaxLevel;
        experience = 0;
        experienceToNext = 0;
    } else {
        experienceToNext = experienceRequirementForLevel(level);
        experience = std::clamp(experience, 0, experienceToNext - 1);
    }
}

void Progression::restoreState(int savedLevel, int savedExperience, int savedExperienceToNext, int savedTotalExperience) {
    level = std::clamp(savedLevel, 0, kMaxLevel);
    experience = std::max(0, savedExperience);
    totalExperience = std::max(0, savedTotalExperience);
    experienceToNext = experienceRequirementForLevel(level);

    if (level >= kMaxLevel) {
        level = kMaxLevel;
        experience = 0;
        experienceToNext = 0;
        return;
    }

    // Ignore stale/rounded thresholds from old clients and use the canonical curve.
    (void)savedExperienceToNext;
    experience = std::clamp(experience, 0, experienceToNext - 1);
}

void Progression::restoreLegacyExperience(int legacyExperience) {
    level = 0;
    experience = 0;
    experienceToNext = experienceRequirementForLevel(0);
    totalExperience = 0;
    awardExperience(std::max(0, legacyExperience));
}

void Progression::recordGather() {
    gatheringActions = std::min(gatheringActions + 1, 3);
    awardGrindingXP(12);
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
            return "THE FIRST EMBER  -  Aurora arrives - gather 3 caches for the camp";
        case QuestStage::CraftEmberKit:
            return "THE FIRST EMBER  -  At camp, craft the ember kit";
        case QuestStage::DefeatWarden:
            return "THE FIRST EMBER  -  Face the Forest Warden beneath the roots";
        case QuestStage::Complete:
            return "THE FIRST EMBER  -  Heartfire restored - the wilds remember you";
    }
    return "THE FIRST EMBER";
}

} // namespace forest::rpg
