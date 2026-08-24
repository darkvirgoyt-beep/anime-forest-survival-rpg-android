#include "ForestSliceFirstPlayableSubsystem.h"

void UForestSliceFirstPlayableSubsystem::BeginAuthenticatedSession(const FString& VerifiedAccountId)
{
    if (VerifiedAccountId.IsEmpty())
    {
        ReportBlockingError();
        return;
    }

    State.AccountId = VerifiedAccountId;
    State.bAccountVerified = true;
    TransitionTo(EForestSliceFirstPlayablePhase::WorldRecovery);
}

void UForestSliceFirstPlayableSubsystem::ResolveOwnedWorld(const FString& AuthorizedWorldId, int32 AuthorizedWorldSeed)
{
    if (!State.bAccountVerified || AuthorizedWorldId.IsEmpty())
    {
        ReportBlockingError();
        return;
    }

    State.WorldId = AuthorizedWorldId;
    State.WorldSeed = AuthorizedWorldSeed;
    State.bWorldRecovered = true;
    TransitionTo(EForestSliceFirstPlayablePhase::CharacterSetup);
}

void UForestSliceFirstPlayableSubsystem::ConfirmCharacterSetup()
{
    if (!State.bWorldRecovered)
    {
        ReportBlockingError();
        return;
    }

    State.bCharacterConfirmed = true;
    TransitionTo(EForestSliceFirstPlayablePhase::ForestArrival);
}

void UForestSliceFirstPlayableSubsystem::ConfirmCampBuilt()
{
    if (State.Phase != EForestSliceFirstPlayablePhase::ForestArrival &&
        State.Phase != EForestSliceFirstPlayablePhase::CampTutorial)
    {
        ReportBlockingError();
        return;
    }

    State.bCampBuilt = true;
    TransitionTo(EForestSliceFirstPlayablePhase::CampTutorial);
}

void UForestSliceFirstPlayableSubsystem::ConfirmBedPlaced()
{
    if (!State.bCampBuilt)
    {
        ReportBlockingError();
        return;
    }

    State.bBedPlaced = true;
    TransitionTo(EForestSliceFirstPlayablePhase::Complete);
}

void UForestSliceFirstPlayableSubsystem::ReportBlockingError()
{
    TransitionTo(EForestSliceFirstPlayablePhase::Error);
}

void UForestSliceFirstPlayableSubsystem::ResetFirstPlayable()
{
    State = FForestSliceFirstPlayableState{};
    TransitionTo(EForestSliceFirstPlayablePhase::Boot);
}

void UForestSliceFirstPlayableSubsystem::TransitionTo(EForestSliceFirstPlayablePhase NextPhase)
{
    const EForestSliceFirstPlayablePhase Previous = State.Phase;
    if (Previous == NextPhase)
    {
        return;
    }

    State.Phase = NextPhase;
    OnPhaseChanged.Broadcast(Previous, NextPhase);
}
