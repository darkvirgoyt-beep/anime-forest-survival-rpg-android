#include "ForestSliceCreatureCompanionComponent.h"

#include "ForestSliceHealthComponent.h"
#include "ForestSliceInventoryComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UForestSliceCreatureCompanionComponent::UForestSliceCreatureCompanionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UForestSliceCreatureCompanionComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner() && GetOwner()->HasAuthority() && State.Revision == 0) {
        State.CompanionId = FGuid::NewGuid();
    }
}

void UForestSliceCreatureCompanionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceCreatureCompanionComponent, State);
}

bool UForestSliceCreatureCompanionComponent::TryCaptureCreature(AActor* TargetCreature)
{
    if (!GetOwner() || GetOwner()->HasAuthority()) {
        FString FailureReason;
        if (!ValidateCaptureTarget(TargetCreature, FailureReason)) return false;
        ApplyCapture(TargetCreature);
        return true;
    }

    ServerCaptureCreature(TargetCreature);
    return true;
}

bool UForestSliceCreatureCompanionComponent::ToggleCommand()
{
    if (!State.bCaptured) return false;
    return SetCommand(State.Command == EForestSliceCompanionCommand::Follow
        ? EForestSliceCompanionCommand::Stay
        : EForestSliceCompanionCommand::Follow);
}

bool UForestSliceCreatureCompanionComponent::SetCommand(EForestSliceCompanionCommand NewCommand)
{
    if (!State.bCaptured) return false;
    if (!GetOwner() || GetOwner()->HasAuthority()) {
        if (State.Command == NewCommand) return true;
        State.Command = NewCommand;
        ++State.Revision;
        BroadcastState();
        return true;
    }

    ServerSetCommand(NewCommand);
    return true;
}

void UForestSliceCreatureCompanionComponent::ServerCaptureCreature_Implementation(AActor* TargetCreature)
{
    FString FailureReason;
    if (ValidateCaptureTarget(TargetCreature, FailureReason)) {
        ApplyCapture(TargetCreature);
    }
}

void UForestSliceCreatureCompanionComponent::ServerSetCommand_Implementation(EForestSliceCompanionCommand NewCommand)
{
    SetCommand(NewCommand);
}

void UForestSliceCreatureCompanionComponent::OnRep_State()
{
    BroadcastState();
}

bool UForestSliceCreatureCompanionComponent::ValidateCaptureTarget(AActor* TargetCreature, FString& FailureReason) const
{
    if (State.bCaptured) {
        FailureReason = TEXT("companion_already_active");
        return false;
    }
    if (!GetOwner() || !TargetCreature || TargetCreature == GetOwner()) {
        FailureReason = TEXT("creature_not_found");
        return false;
    }
    if (!TargetCreature->ActorHasTag(TEXT("Creature")) && !TargetCreature->ActorHasTag(TEXT("WildCreature"))) {
        FailureReason = TEXT("target_is_not_a_wild_creature");
        return false;
    }
    const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetCreature->GetActorLocation());
    if (!FMath::IsFinite(Distance) || Distance > CaptureRange) {
        FailureReason = TEXT("capture_out_of_range");
        return false;
    }

    const UForestSliceHealthComponent* Health = TargetCreature->FindComponentByClass<UForestSliceHealthComponent>();
    if (!Health || !Health->IsAlive() || Health->GetState().MaxHealth <= 0.0f) {
        FailureReason = TEXT("creature_health_unavailable");
        return false;
    }
    const float HealthFraction = Health->GetState().Health / Health->GetState().MaxHealth;
    if (HealthFraction > CaptureHealthFraction) {
        FailureReason = TEXT("creature_too_healthy");
        return false;
    }

    const UForestSliceInventoryComponent* Inventory = GetOwner()->FindComponentByClass<UForestSliceInventoryComponent>();
    if (!Inventory || Inventory->GetQuantity(TEXT("fiber")) < FiberCost) {
        FailureReason = TEXT("insufficient_fiber");
        return false;
    }
    return true;
}

void UForestSliceCreatureCompanionComponent::ApplyCapture(AActor* TargetCreature)
{
    if (!TargetCreature) return;
    UForestSliceInventoryComponent* Inventory = GetOwner()->FindComponentByClass<UForestSliceInventoryComponent>();
    if (!Inventory || !Inventory->RemoveItem(TEXT("fiber"), FiberCost)) return;

    State.CompanionId = FGuid::NewGuid();
    State.CreatureId = TargetCreature->GetFName();
    State.DisplayName = FText::FromName(TargetCreature->GetFName());
    State.Level = 1;
    State.Bond = 1;
    State.HealthFraction = 0.75f;
    State.Command = EForestSliceCompanionCommand::Follow;
    State.bCaptured = true;
    ++State.Revision;
    TargetCreature->Tags.AddUnique(TEXT("CapturedCreature"));
    TargetCreature->SetOwner(GetOwner());
    BroadcastState();
}

void UForestSliceCreatureCompanionComponent::BroadcastState()
{
    CompanionChanged.Broadcast(State);
}
