#include "ForestSliceInteractionComponent.h"

#include "GameFramework/Actor.h"

UForestSliceInteractionComponent::UForestSliceInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UForestSliceInteractionComponent::RegisterCandidate(const FForestSliceInteractableCandidate& Candidate)
{
    if (!IsValid(Candidate.Actor) || Candidate.Kind == EForestSliceInteractionKind::None || Candidate.ActionId.IsNone()) return;

    const int32 ExistingIndex = Candidates.IndexOfByPredicate([&Candidate](const FForestSliceInteractableCandidate& Existing)
    {
        return Existing.Actor == Candidate.Actor;
    });
    if (ExistingIndex == INDEX_NONE) Candidates.Add(Candidate);
    else Candidates[ExistingIndex] = Candidate;
}

void UForestSliceInteractionComponent::UnregisterCandidate(AActor* Actor)
{
    Candidates.RemoveAll([Actor](const FForestSliceInteractableCandidate& Candidate)
    {
        return Candidate.Actor == Actor;
    });
    if (BestCandidate.Actor == Actor) {
        BestCandidate = {};
        CandidateChanged.Broadcast(BestCandidate);
    }
}

void UForestSliceInteractionComponent::ClearCandidates()
{
    Candidates.Reset();
    BestCandidate = {};
    CandidateChanged.Broadcast(BestCandidate);
}

bool UForestSliceInteractionComponent::RefreshBestCandidate(const FVector& ViewerLocation)
{
    FForestSliceInteractableCandidate NewBest;
    for (const FForestSliceInteractableCandidate& Candidate : Candidates) {
        if (!IsValid(Candidate.Actor)) continue;
        const float Distance = FVector::Dist(ViewerLocation, Candidate.Actor->GetActorLocation());
        if (Distance > Candidate.MaxDistance) continue;
        if (NewBest.Actor == nullptr || IsCandidateBetter(Candidate, NewBest, ViewerLocation)) NewBest = Candidate;
    }

    const bool bChanged = NewBest.Actor != BestCandidate.Actor || NewBest.ActionId != BestCandidate.ActionId;
    BestCandidate = NewBest;
    if (bChanged) CandidateChanged.Broadcast(BestCandidate);
    return BestCandidate.Actor != nullptr;
}

void UForestSliceInteractionComponent::ConfirmBestCandidate()
{
    if (IsValid(BestCandidate.Actor)) InteractionConfirmed.Broadcast(BestCandidate);
}

bool UForestSliceInteractionComponent::IsCandidateBetter(const FForestSliceInteractableCandidate& Left, const FForestSliceInteractableCandidate& Right, const FVector& ViewerLocation) const
{
    if (Left.Priority != Right.Priority) return Left.Priority > Right.Priority;
    return FVector::DistSquared(ViewerLocation, Left.Actor->GetActorLocation()) < FVector::DistSquared(ViewerLocation, Right.Actor->GetActorLocation());
}
