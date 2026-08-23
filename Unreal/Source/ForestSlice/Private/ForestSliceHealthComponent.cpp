#include "ForestSliceHealthComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UForestSliceHealthComponent::UForestSliceHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UForestSliceHealthComponent::BeginPlay()
{
    Super::BeginPlay();
    State.Health = FMath::Clamp(State.Health, 0.0f, State.MaxHealth);
    State.Poise = FMath::Clamp(State.Poise, 0.0f, State.MaxPoise);
    State.bDowned = State.Health <= DownedHealthThreshold && State.Health > 0.0f;
    State.bDead = State.Health <= 0.0f;
}

bool UForestSliceHealthComponent::ApplyDamage(float Damage, float PoiseDamage, FVector Impulse, FName DamageType)
{
    if (!GetOwner()->HasAuthority() || State.bDead || Damage < 0.0f || PoiseDamage < 0.0f) return false;

    const float PreviousHealth = State.Health;
    State.Health = FMath::Clamp(State.Health - Damage, 0.0f, State.MaxHealth);
    State.Poise = FMath::Max(0.0f, State.Poise - PoiseDamage);
    State.bDowned = State.Health <= DownedHealthThreshold && State.Health > 0.0f;
    State.bDead = State.Health <= 0.0f;

    if (!Impulse.IsNearlyZero()) GetOwner()->AddActorWorldOffset(Impulse, true);
    BroadcastDamage(DamageType, PreviousHealth - State.Health, State.bDead);
    return true;
}

void UForestSliceHealthComponent::RestoreFullHealth()
{
    if (!GetOwner()->HasAuthority()) return;
    State.Health = State.MaxHealth;
    State.Poise = State.MaxPoise;
    State.bDowned = false;
    State.bDead = false;
    OnRep_State();
}

void UForestSliceHealthComponent::OnRep_State()
{
    // Blueprint listeners can react to replicated health, poise, downed, and death state.
}

void UForestSliceHealthComponent::BroadcastDamage(FName DamageType, float Damage, bool bKilled)
{
    DamageTaken.Broadcast(DamageType, Damage, State.Health, bKilled);
}

void UForestSliceHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceHealthComponent, State);
}
