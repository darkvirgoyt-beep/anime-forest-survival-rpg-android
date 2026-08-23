#include <cassert>
#include <cmath>
#include <set>
#include <string>

#include "mobs/mob_catalog.h"

int main() {
    assert(forest::mobs::kProfileCount == 12);
    std::set<std::string> ids;
    int rangedCount = 0;
    int meleeCount = 0;
    int tameableCount = 0;
    for (int i = 0; i < forest::mobs::kProfileCount; ++i) {
        const forest::mobs::MobProfile& mob = forest::mobs::kProfiles[i];
        assert(mob.id != nullptr);
        assert(mob.displayName != nullptr);
        assert(ids.insert(mob.id).second);
        assert(mob.maxHealth >= 40);
        assert(mob.moveSpeed > 0.0f);
        assert(mob.attackRange > 0.0f);
        assert(mob.attackDamage > 0.0f);
        assert(mob.attackCooldown > 0.0f);
        assert(mob.scale > 0.0f);
        if (mob.ranged) {
            ++rangedCount;
        } else {
            ++meleeCount;
        }
        if (mob.tameable) {
            ++tameableCount;
            assert(mob.tamingCost >= 2);
        } else {
            assert(mob.tamingCost == 0);
        }
    }
    assert(rangedCount == 5);
    assert(meleeCount == 7);
    assert(tameableCount == 4);
    assert(std::string(forest::mobs::profile(forest::mobs::MobType::ArcaneWizard).displayName) == "Arcane Wizard");
    assert(std::string(forest::mobs::profile(forest::mobs::MobType::Druid).displayName) == "Druid");
    assert(std::string(forest::mobs::profile(forest::mobs::MobType::MoonDeer).displayName) == "Moon Deer");
    assert(std::string(forest::mobs::profile(forest::mobs::MobType::MossbackBoar).displayName) == "Mossback Boar");
    assert(forest::mobs::profile(forest::mobs::MobType::RiverOtter).tameable);
    assert(forest::mobs::profile(forest::mobs::MobType::CanopyFox).tameable);
    assert(forest::mobs::profile(forest::mobs::MobType::Barbarian).maxHealth >
           forest::mobs::profile(forest::mobs::MobType::ArcaneWizard).maxHealth);
    assert(forest::mobs::profile(forest::mobs::MobType::Monk).moveSpeed >
           forest::mobs::profile(forest::mobs::MobType::Cleric).moveSpeed);
    return 0;
}
