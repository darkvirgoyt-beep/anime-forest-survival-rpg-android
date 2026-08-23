#pragma once

namespace forest::rpg {

enum class QuestStage {
    GatherMaterials,
    CraftEmberKit,
    DefeatWarden,
    Complete
};

struct Progression {
    static constexpr int kMaxLevel = 100;

    int level = 0;
    int experience = 0;
    int experienceToNext = 991;
    int totalExperience = 0;
    int gatheringActions = 0;
    QuestStage questStage = QuestStage::GatherMaterials;
    bool emberKitCrafted = false;
    bool wardenDefeated = false;

    static int experienceRequirementForLevel(int level);

    void awardExperience(int amount);
    void awardGrindingXP(int amount) { awardExperience(amount); }
    void restoreState(int savedLevel, int savedExperience, int savedExperienceToNext, int savedTotalExperience);
    void restoreLegacyExperience(int legacyExperience);
    void recordGather();
    void recordCraft();
    void recordWardenDefeat();
    const char* questObjective() const;
};

} // namespace forest::rpg
