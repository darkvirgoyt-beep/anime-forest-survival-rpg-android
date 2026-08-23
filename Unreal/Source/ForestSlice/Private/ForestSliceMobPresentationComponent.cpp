#include "ForestSliceMobPresentationComponent.h"

#include "ForestSliceHealthComponent.h"
#include "GameFramework/Actor.h"

UForestSliceMobPresentationComponent::UForestSliceMobPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UForestSliceMobPresentationComponent::BeginPlay()
{
    Super::BeginPlay();

    HealthComponent = GetOwner() ? GetOwner()->FindComponentByClass<UForestSliceHealthComponent>() : nullptr;
    if (HealthComponent) {
        HealthComponent->DamageTaken.AddDynamic(this, &UForestSliceMobPresentationComponent::OnHealthChanged);
    }
    BroadcastPresentation();
}

void UForestSliceMobPresentationComponent::MarkTargeted(bool bTargeted)
{
    bIsTargeted = bTargeted;
    BroadcastPresentation();
}

void UForestSliceMobPresentationComponent::SetBaseAffiliation(FName NewBaseId)
{
    BaseId = NewBaseId;
    BroadcastPresentation();
}

float UForestSliceMobPresentationComponent::GetHealthRatio() const
{
    if (!HealthComponent) return 0.0f;
    const FForestSliceHealthState& State = HealthComponent->GetState();
    return State.MaxHealth > SMALL_NUMBER ? FMath::Clamp(State.Health / State.MaxHealth, 0.0f, 1.0f) : 0.0f;
}

bool UForestSliceMobPresentationComponent::ShouldShowHealthBar(float DistanceMeters, bool bTargetedByPlayer) const
{
    if (bBoss) return DistanceMeters <= MaxHealthBarDistanceMeters * 2.5f;
    if (bTargetedByPlayer || bIsTargeted) return DistanceMeters <= MaxHealthBarDistanceMeters;
    const UWorld* World = GetWorld();
    const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
    return DistanceMeters <= MaxHealthBarDistanceMeters && CurrentTime - LastCombatTime <= HealthBarGraceSeconds;
}

bool UForestSliceMobPresentationComponent::ShouldShowBaseMarker(float DistanceMeters) const
{
    return !BaseId.IsNone() && DistanceMeters <= (bBoss ? 1500.0f : 750.0f);
}

FLinearColor UForestSliceMobPresentationComponent::GetHealthBarColor() const
{
    if (bBoss) return FLinearColor(0.85f, 0.10f, 0.12f, 1.0f);
    if (bElite) return FLinearColor(0.72f, 0.24f, 0.92f, 1.0f);
    return FLinearColor(0.92f, 0.18f, 0.12f, 1.0f);
}

void UForestSliceMobPresentationComponent::OnHealthChanged(FName DamageType, float Damage, float RemainingHealth, bool bKilled)
{
    LastCombatTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    BroadcastPresentation();
}

void UForestSliceMobPresentationComponent::BroadcastPresentation()
{
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (HealthComponent) {
        const FForestSliceHealthState& State = HealthComponent->GetState();
        Health = State.Health;
        MaxHealth = State.MaxHealth;
    }
    PresentationChanged.Broadcast(DisplayName, Level, Health, MaxHealth, bElite || bBoss, !BaseId.IsNone());
}
