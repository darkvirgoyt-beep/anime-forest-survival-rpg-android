#pragma once

namespace forest::rpg {

enum class QuestStage {
    GatherMaterials,
    CraftEmberKit,
    DefeatWarden,
    Complete
};

struct Progression {
    int level = 1;
    int experience = 0;
    int experienceToNext = 100;
    int gatheringActions = 0;
    QuestStage questStage = QuestStage::GatherMaterials;
    bool emberKitCrafted = false;
    bool wardenDefeated = false;

    void awardExperience(int amount);
    void recordGather();
    void recordCraft();
    void recordWardenDefeat();
    const char* questObjective() const;
};

} // namespace forest::rpg
