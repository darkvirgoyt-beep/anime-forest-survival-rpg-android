#include <cassert>
#include <cmath>
#include <string>

#include "rpg/companion_system.h"
#include "rpg/encounter_director.h"
#include "rpg/quality_profile.h"

int main() {
    using namespace forest::rpg;

    const CompanionCaptureRules rules;
    CompanionState companion;
    CaptureResult tooHealthy = evaluateCapture(rules, companion, 2, 0.20f, 0.60f, 8);
    assert(!tooHealthy.accepted);
    assert(std::string(tooHealthy.reason) == "creature_too_healthy");

    CaptureResult captured = captureCompanion(rules, companion, 2, 0.20f, 0.30f, 8);
    assert(captured.accepted);
    assert(captured.remainingFiber == 6);
    assert(companion.captured);
    assert(companion.command == CompanionCommand::Follow);

    assert(std::abs(companionAssistDamage(companion, false, false) - 5.05f) < 0.001f);
    assert(companionAssistDamage(companion, true, true) == 0.0f);
    assert(toggleCompanionCommand(companion));
    assert(companion.command == CompanionCommand::Stay);
    assert(companionAssistDamage(companion, false, false) == 0.0f);
    assert(toggleCompanionCommand(companion));
    assert(companion.command == CompanionCommand::Follow);

    const EncounterBudget forestDay = encounterBudgetFor(BiomeId::Forest, 12.0f, 0.0f, 0);
    const EncounterBudget ruinsNight = encounterBudgetFor(BiomeId::Ruins, 22.0f, 0.8f, 4);
    assert(ruinsNight.activeCreatures > forestDay.activeCreatures);
    assert(ruinsNight.eliteSlots >= 2);
    assert(ruinsNight.respawnSeconds < forestDay.respawnSeconds);
    assert(canSpawnEncounter(forestDay, 0, false));
    assert(!canSpawnEncounter(forestDay, forestDay.activeCreatures, false));

    const QualityProfile low = qualityProfileFor(0, true);
    const QualityProfile waiting = qualityProfileFor(4, false);
    const QualityProfile high = qualityProfileFor(4, true);
    assert(low.id == QualityProfileId::Low);
    assert(waiting.id == QualityProfileId::Low);
    assert(high.id == QualityProfileId::High);
    assert(low.vegetationDetailCount < high.vegetationDetailCount);
    assert(low.weatherParticleBudget < high.weatherParticleBudget);
    assert(low.effectScalePercent < high.effectScalePercent);
    assert(!low.highResolutionTextures && high.highResolutionTextures);

    return 0;
}
