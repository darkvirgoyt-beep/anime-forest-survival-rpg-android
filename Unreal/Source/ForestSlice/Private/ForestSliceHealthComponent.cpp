#include "ForestSliceHealthComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UForestSliceHealthComponent::UForestSliceHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0f;
    SetIsReplicatedByDefault(true);
}

void UForestSliceHealthComponent::BeginPlay()
{
    Super::BeginPlay();
    NormalizeState();
    HealthRegenDelayRemaining = 0.0f;
    DamageImmunityRemaining = 0.0f;
    BroadcastHealthChanged();
}

void UForestSliceHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    const float SafeDelta = FMath::Clamp(DeltaTime, 0.0f, 0.10f);
    HealthRegenDelayRemaining = FMath::Max(0.0f, HealthRegenDelayRemaining - SafeDelta);
    DamageImmunityRemaining = FMath::Max(0.0f, DamageImmunityRemaining - SafeDelta);

    if (State.bDead || State.Health <= 0.0f)
    {
        return;
    }

    bool bChanged = false;
    if (State.Poise < State.MaxPoise)
    {
        State.Poise = FMath::Min(State.MaxPoise, State.Poise + PoiseRegenPerSecond * SafeDelta);
        bChanged = true;
    }
    if (HealthRegenDelayRemaining <= 0.0f && State.Health < State.MaxHealth)
    {
        State.Health = FMath::Min(State.MaxHealth, State.Health + HealthRegenPerSecond * SafeDelta);
        State.bDowned = State.Health <= DownedHealthThreshold && State.Health > 0.0f;
        bChanged = true;
    }

    if (bChanged)
    {
        NormalizeState();
        BroadcastHealthChanged();
        GetOwner()->ForceNetUpdate();
    }
}

bool UForestSliceHealthComponent::ApplyDamage(float Damage, float PoiseDamage, FVector Impulse, FName DamageType)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || State.bDead || IsDamageImmune() ||
        !FMath::IsFinite(Damage) || !FMath::IsFinite(PoiseDamage) ||
        Damage < 0.0f || PoiseDamage < 0.0f)
    {
        return false;
    }

    const float PreviousHealth = State.Health;
    const float PreviousPoise = State.Poise;
    State.Health = FMath::Clamp(State.Health - Damage, 0.0f, State.MaxHealth);
    State.Poise = FMath::Max(0.0f, State.Poise - PoiseDamage);
    State.bDowned = State.Health <= DownedHealthThreshold && State.Health > 0.0f;
    State.bDead = State.Health <= 0.0f;
    HealthRegenDelayRemaining = HealthRegenDelaySeconds;
    DamageImmunityRemaining = DamageImmunitySeconds;

    if (!Impulse.IsNearlyZero())
    {
        const FVector SafeImpulse = Impulse.GetClampedToMaxSize(MaxKnockbackImpulse);
        GetOwner()->AddActorWorldOffset(SafeImpulse, true);
    }

    const float AppliedDamage = PreviousHealth - State.Health;
    const float AppliedPoiseDamage = PreviousPoise - State.Poise;
    if (AppliedDamage <= 0.0f && AppliedPoiseDamage <= 0.0f)
    {
        return false;
    }

    BroadcastDamage(DamageType, AppliedDamage, State.bDead);
    BroadcastHealthChanged();
    GetOwner()->ForceNetUpdate();
    return true;
}

void UForestSliceHealthComponent::RestoreFullHealth()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    State.Health = State.MaxHealth;
    State.Poise = State.MaxPoise;
    State.bDowned = false;
    State.bDead = false;
    HealthRegenDelayRemaining = 0.0f;
    DamageImmunityRemaining = 0.0f;
    BroadcastHealthChanged();
    GetOwner()->ForceNetUpdate();
}

void UForestSliceHealthComponent::OnRep_State()
{
    NormalizeState();
    BroadcastHealthChanged();
}

void UForestSliceHealthComponent::BroadcastDamage(FName DamageType, float Damage, bool bKilled)
{
    DamageTaken.Broadcast(DamageType, Damage, State.Health, bKilled);
}

void UForestSliceHealthComponent::BroadcastHealthChanged()
{
    HealthChanged.Broadcast(State.Health, State.MaxHealth);
}

void UForestSliceHealthComponent::NormalizeState()
{
    State.MaxHealth = FMath::Max(1.0f, State.MaxHealth);
    State.MaxPoise = FMath::Max(0.0f, State.MaxPoise);
    State.Health = FMath::Clamp(State.Health, 0.0f, State.MaxHealth);
    State.Poise = FMath::Clamp(State.Poise, 0.0f, State.MaxPoise);
    State.bDead = State.bDead || State.Health <= 0.0f;
    State.bDowned = !State.bDead && State.Health <= DownedHealthThreshold;
}

void UForestSliceHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceHealthComponent, State);
}
