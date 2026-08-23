#include "ForestSliceProgressionComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UForestSliceProgressionComponent::UForestSliceProgressionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UForestSliceProgressionComponent::BeginPlay()
{
    Super::BeginPlay();
    NormalizeState();
}

void UForestSliceProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceProgressionComponent, State);
}

int32 UForestSliceProgressionComponent::GetXPRequirementForLevel(int32 Level) const
{
    const int32 SafeLevel = FMath::Clamp(Level, 0, MaxLevel);
    if (SafeLevel >= MaxLevel) return 0;
    if (SafeLevel == MaxLevel - 1) return 100000;

    // Deterministic integer hash jitter makes the thresholds difficult to guess
    // while the 1000 XP level band guarantees strict ascending order.
    uint32 Seed = static_cast<uint32>(SafeLevel + 1) * 0x9E3779B9u + 0x7F4A7C15u;
    Seed ^= Seed >> 16;
    Seed *= 0x85EBCA6Bu;
    Seed ^= Seed >> 13;
    Seed *= 0xC2B2AE35u;
    Seed ^= Seed >> 16;
    const int32 Jitter = static_cast<int32>(Seed % 701u) - 350;
    const int32 FineOffset = static_cast<int32>((Seed >> 10) % 89u);
    return (SafeLevel + 1) * 1000 + Jitter + FineOffset;
}

int32 UForestSliceProgressionComponent::AwardExperience(int32 Amount)
{
    if (Amount <= 0 || !GetOwner() || !GetOwner()->HasAuthority() || IsMaxLevel()) return 0;

    const int32 PreviousLevel = State.Level;
    State.Experience += Amount;
    State.TotalExperience += Amount;

    while (State.Level < MaxLevel) {
        State.ExperienceToNext = GetXPRequirementForLevel(State.Level);
        if (State.Experience < State.ExperienceToNext) break;
        State.Experience -= State.ExperienceToNext;
        ++State.Level;
    }

    NormalizeState();
    if (State.Level != PreviousLevel) BroadcastLevelChanged();
    return Amount;
}

int32 UForestSliceProgressionComponent::AwardGrindingXP(int32 Amount)
{
    return AwardExperience(Amount);
}

float UForestSliceProgressionComponent::GetLevelProgressNormalized() const
{
    if (State.Level >= MaxLevel || State.ExperienceToNext <= 0) return 1.0f;
    return FMath::Clamp(static_cast<float>(State.Experience) / static_cast<float>(State.ExperienceToNext), 0.0f, 1.0f);
}

void UForestSliceProgressionComponent::NormalizeState()
{
    State.Level = FMath::Clamp(State.Level, 0, MaxLevel);
    State.Experience = FMath::Max(0, State.Experience);
    State.TotalExperience = FMath::Max(0, State.TotalExperience);
    State.ExperienceToNext = GetXPRequirementForLevel(State.Level);

    if (State.Level >= MaxLevel) {
        State.Experience = 0;
        State.ExperienceToNext = 0;
    } else {
        State.Experience = FMath::Min(State.Experience, FMath::Max(0, State.ExperienceToNext - 1));
    }
}

void UForestSliceProgressionComponent::BroadcastLevelChanged()
{
    LevelChanged.Broadcast(State.Level, State.Experience, State.ExperienceToNext);
}

void UForestSliceProgressionComponent::OnRep_State()
{
    NormalizeState();
    BroadcastLevelChanged();
}
