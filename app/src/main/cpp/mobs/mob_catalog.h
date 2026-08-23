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
    Druid,
    MoonDeer,
    MossbackBoar,
    RiverOtter,
    CanopyFox
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
    bool tameable;
    int tamingCost;
};

inline constexpr MobProfile kProfiles[] = {
    {MobType::ArcaneWizard, "arcane_wizard", "Arcane Wizard", 46, 0.10f, 0.78f, 5.0f, 2.2f, 1.00f, true, false, 0},
    {MobType::Barbarian, "barbarian", "Barbarian", 92, 0.20f, 0.28f, 10.0f, 1.35f, 1.16f, false, false, 0},
    {MobType::Cleric, "cleric", "Cleric", 64, 0.12f, 0.70f, 4.0f, 2.5f, 1.04f, true, false, 0},
    {MobType::Monk, "monk", "Monk", 58, 0.25f, 0.25f, 7.0f, 0.92f, 0.98f, false, false, 0},
    {MobType::Necromancer, "necromancer", "Necromancer", 52, 0.09f, 0.86f, 6.0f, 2.0f, 1.08f, true, false, 0},
    {MobType::Samurai, "samurai", "Samurai", 76, 0.18f, 0.30f, 9.0f, 1.15f, 1.08f, false, false, 0},
    {MobType::Artificer, "artificer", "Artificer", 62, 0.14f, 0.74f, 5.5f, 1.75f, 1.02f, true, false, 0},
    {MobType::Druid, "druid", "Druid", 70, 0.13f, 0.62f, 5.0f, 1.90f, 1.12f, true, false, 0},
    {MobType::MoonDeer, "moon_deer", "Moon Deer", 58, 0.23f, 0.22f, 2.0f, 2.60f, 1.05f, false, true, 2},
    {MobType::MossbackBoar, "mossback_boar", "Mossback Boar", 82, 0.18f, 0.24f, 4.0f, 1.80f, 1.18f, false, true, 3},
    {MobType::RiverOtter, "river_otter", "River Otter", 44, 0.28f, 0.18f, 1.5f, 2.10f, 0.72f, false, true, 2},
    {MobType::CanopyFox, "canopy_fox", "Canopy Fox", 50, 0.31f, 0.20f, 2.5f, 1.70f, 0.82f, false, true, 2}
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
