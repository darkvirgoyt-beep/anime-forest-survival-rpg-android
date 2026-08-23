#pragma once

namespace forest::mobs {

enum class MobType {
    ArcaneWizard,
    Barbarian,
    Cleric,
    Monk,
    Necromancer,
    Samurai,
    Artificer,
    Druid
};

struct MobProfile {
    MobType type;
    const char* id;
    const char* displayName;
    int maxHealth;
    float moveSpeed;
    float attackRange;
    float attackDamage;
    float attackCooldown;
    float scale;
    bool ranged;
};

inline constexpr MobProfile kProfiles[] = {
    {MobType::ArcaneWizard, "arcane_wizard", "Arcane Wizard", 46, 0.10f, 0.78f, 5.0f, 2.2f, 1.00f, true},
    {MobType::Barbarian, "barbarian", "Barbarian", 92, 0.20f, 0.28f, 10.0f, 1.35f, 1.16f, false},
    {MobType::Cleric, "cleric", "Cleric", 64, 0.12f, 0.70f, 4.0f, 2.5f, 1.04f, true},
    {MobType::Monk, "monk", "Monk", 58, 0.25f, 0.25f, 7.0f, 0.92f, 0.98f, false},
    {MobType::Necromancer, "necromancer", "Necromancer", 52, 0.09f, 0.86f, 6.0f, 2.0f, 1.08f, true},
    {MobType::Samurai, "samurai", "Samurai", 76, 0.18f, 0.30f, 9.0f, 1.15f, 1.08f, false},
    {MobType::Artificer, "artificer", "Artificer", 62, 0.14f, 0.74f, 5.5f, 1.75f, 1.02f, true},
    {MobType::Druid, "druid", "Druid", 70, 0.13f, 0.62f, 5.0f, 1.90f, 1.12f, true}
};

inline constexpr int kProfileCount = static_cast<int>(sizeof(kProfiles) / sizeof(kProfiles[0]));

inline const MobProfile& profile(MobType type) {
    return kProfiles[static_cast<int>(type)];
}

}  // namespace forest::mobs

using forest::mobs::MobType;
using forest::mobs::MobProfile;
using forest::mobs::kProfiles;
using forest::mobs::kProfileCount;
using forest::mobs::profile;
