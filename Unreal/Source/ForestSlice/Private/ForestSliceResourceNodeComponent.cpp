#include "ForestSliceResourceNodeComponent.h"

#include "ForestSliceInventoryComponent.h"
#include "ForestSliceProgressionComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UForestSliceResourceNodeComponent::UForestSliceResourceNodeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UForestSliceResourceNodeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceResourceNodeComponent, bDepleted);
}

FForestSliceInteractableCandidate UForestSliceResourceNodeComponent::GetInteractionCandidate() const
{
    FForestSliceInteractableCandidate Candidate;
    Candidate.Actor = GetOwner();
    Candidate.Kind = EForestSliceInteractionKind::ResourceNode;
    Candidate.ActionId = TEXT("Gather");
    Candidate.Prompt = FText::FromString(FString::Printf(TEXT("Gather %s"), *ItemId.ToString()));
    Candidate.Priority = 50;
    Candidate.MaxDistance = InteractionDistance;
    return Candidate;
}

bool UForestSliceResourceNodeComponent::TryCollect(AActor* Collector)
{
    if (bDepleted || !Collector || !GetOwner() || !GetOwner()->HasAuthority() ||
        StableResourceId.IsNone() || ItemId.IsNone() || Quantity <= 0) {
        return false;
    }

    if (FVector::DistSquared(Collector->GetActorLocation(), GetOwner()->GetActorLocation()) > FMath::Square(InteractionDistance)) {
        return false;
    }

    UForestSliceInventoryComponent* Inventory = Collector->FindComponentByClass<UForestSliceInventoryComponent>();
    if (!Inventory || !Inventory->AddItem(ItemId, Quantity, StackLimit)) return false;

    bDepleted = true;
    if (UForestSliceProgressionComponent* Progression = Collector->FindComponentByClass<UForestSliceProgressionComponent>()) {
        Progression->AwardGrindingXP(GatheringXP);
    }
    BroadcastDepletedPresentation();
    return true;
}

void UForestSliceResourceNodeComponent::OnRep_Depleted()
{
    BroadcastDepletedPresentation();
}

void UForestSliceResourceNodeComponent::BroadcastDepletedPresentation()
{
    if (!GetOwner()) return;
    GetOwner()->SetActorEnableCollision(!bDepleted);
    GetOwner()->SetActorHiddenInGame(bDepleted);
}
