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

    // Exact integer curve: level L -> L+1 costs 10 * (L+1)^2.
    const int32 NextLevel = SafeLevel + 1;
    return 10 * NextLevel * NextLevel;
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
