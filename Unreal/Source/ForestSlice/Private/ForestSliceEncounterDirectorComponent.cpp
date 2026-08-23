#include "ForestSliceEncounterDirectorComponent.h"

UForestSliceEncounterDirectorComponent::UForestSliceEncounterDirectorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UForestSliceEncounterDirectorComponent::RebuildBudget(EForestSliceEncounterBiome Biome, float WorldTimeHours, float WeatherIntensity, int32 GraphicsQuality)
{
    const int32 Quality = FMath::Clamp(GraphicsQuality, 0, 4);
    const float Hour = FMath::Fmod(FMath::Max(0.0f, WorldTimeHours), 24.0f);
    const bool bNight = Hour >= 19.0f || Hour < 6.0f;
    const int32 BiomeBonus = Biome == EForestSliceEncounterBiome::Ruins ? 2 : Biome == EForestSliceEncounterBiome::Snow ? 1 : 0;
    const int32 NightBonus = bNight ? 2 : 0;
    const int32 WeatherBonus = WeatherIntensity > 0.65f ? 1 : 0;

    Budget.ActiveCreatureLimit = FMath::Clamp(5 + BiomeBonus + NightBonus + WeatherBonus + Quality, 4, 14);
    Budget.EliteSlots = FMath::Clamp((bNight ? 1 : 0) + (Biome == EForestSliceEncounterBiome::Ruins ? 1 : 0) + Quality / 3, 0, 3);
    Budget.RespawnSeconds = FMath::Max(12.0f, 38.0f - static_cast<float>(Quality) * 4.0f - (bNight ? 5.0f : 0.0f));
    Budget.ThreatMultiplier = 1.0f + (bNight ? 0.15f : 0.0f) + FMath::Min(0.25f, FMath::Max(0.0f, WeatherIntensity) * 0.25f);
    ++Budget.Revision;
    BudgetChanged.Broadcast(Budget);
}

bool UForestSliceEncounterDirectorComponent::CanSpawn(int32 ActiveCreatures, bool bElite) const
{
    if (ActiveCreatures < 0 || ActiveCreatures >= Budget.ActiveCreatureLimit) return false;
    return !bElite || Budget.EliteSlots > 0;
}
