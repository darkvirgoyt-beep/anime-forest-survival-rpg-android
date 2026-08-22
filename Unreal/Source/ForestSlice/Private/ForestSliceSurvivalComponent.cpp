#include "ForestSliceSurvivalComponent.h"

#include "Net/UnrealNetwork.h"

UForestSliceSurvivalComponent::UForestSliceSurvivalComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UForestSliceSurvivalComponent::BeginPlay()
{
    Super::BeginPlay();
    State = {};
}

void UForestSliceSurvivalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!GetOwner()->HasAuthority()) return;

    const float Dt = FMath::Clamp(DeltaTime, 0.0f, 0.1f);
    State.Hunger = FMath::Max(0.0f, State.Hunger - HungerDrainPerSecond * Dt);
    State.Thirst = FMath::Max(0.0f, State.Thirst - ThirstDrainPerSecond * Dt);
    State.Stamina = FMath::Min(100.0f, State.Stamina + StaminaRecoveryPerSecond * Dt);

    if (State.Hunger <= 0.0f || State.Thirst <= 0.0f) {
        State.Health = FMath::Max(0.0f, State.Health - 1.5f * Dt);
        State.Injury = FMath::Min(100.0f, State.Injury + 0.4f * Dt);
    }
    if (!State.bSheltered) {
        State.Temperature = FMath::Clamp(State.Temperature + 0.002f * Dt, 0.0f, 1.0f);
    }
    BroadcastState();
}

bool UForestSliceSurvivalComponent::ConsumeStamina(float Amount)
{
    if (!GetOwner()->HasAuthority() || Amount < 0.0f || State.Stamina < Amount) return false;
    State.Stamina -= Amount;
    BroadcastState();
    return true;
}

void UForestSliceSurvivalComponent::RestoreStamina(float Amount)
{
    if (!GetOwner()->HasAuthority()) return;
    State.Stamina = FMath::Clamp(State.Stamina + FMath::Max(0.0f, Amount), 0.0f, 100.0f);
    BroadcastState();
}

void UForestSliceSurvivalComponent::SetSheltered(bool bInSheltered)
{
    if (!GetOwner()->HasAuthority()) return;
    State.bSheltered = bInSheltered;
    BroadcastState();
}

void UForestSliceSurvivalComponent::ApplyDamage(float Amount, float InjuryAmount)
{
    if (!GetOwner()->HasAuthority()) return;
    State.Health = FMath::Max(0.0f, State.Health - FMath::Max(0.0f, Amount));
    State.Injury = FMath::Clamp(State.Injury + FMath::Max(0.0f, InjuryAmount), 0.0f, 100.0f);
    BroadcastState();
}

void UForestSliceSurvivalComponent::RestoreFromSleep(float HealthAmount, float HungerAmount, float ThirstAmount)
{
    if (!GetOwner()->HasAuthority()) return;
    State.Health = FMath::Min(100.0f, State.Health + FMath::Max(0.0f, HealthAmount));
    State.Hunger = FMath::Min(100.0f, State.Hunger + FMath::Max(0.0f, HungerAmount));
    State.Thirst = FMath::Min(100.0f, State.Thirst + FMath::Max(0.0f, ThirstAmount));
    State.Stamina = 100.0f;
    State.Injury = FMath::Max(0.0f, State.Injury - 15.0f);
    BroadcastState();
}

void UForestSliceSurvivalComponent::OnRep_State()
{
    BroadcastState();
}

void UForestSliceSurvivalComponent::BroadcastState()
{
    SurvivalChanged.Broadcast(State);
}

void UForestSliceSurvivalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceSurvivalComponent, State);
}
