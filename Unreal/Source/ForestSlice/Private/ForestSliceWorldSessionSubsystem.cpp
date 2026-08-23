#include "ForestSliceWorldSessionSubsystem.h"

bool UForestSliceWorldSessionSubsystem::CreateWorld(FName InWorldId, int32 InWorldSeed, EForestSliceWorldPrivacy InPrivacy)
{
    if (InWorldId.IsNone()) return false;
    WorldId = InWorldId;
    WorldSeed = InWorldSeed;
    Privacy = InPrivacy;
    SaveHeader = {};
    SaveHeader.WorldId = WorldId;
    SaveHeader.SchemaVersion = 1;
    SaveHeader.Revision = 0;
    Missions.Reset();
    Markers.Reset();
    Members.Reset();
    SessionChanged.Broadcast(TEXT("WorldCreated"));
    return true;
}

bool UForestSliceWorldSessionSubsystem::JoinWorld(FName InWorldId, const FString& InviteCode)
{
    if (InWorldId.IsNone() || InviteCode.TrimStartAndEnd().IsEmpty() || WorldId != InWorldId) return false;
    SessionChanged.Broadcast(TEXT("WorldJoined"));
    return true;
}

bool UForestSliceWorldSessionSubsystem::SetCloudRevision(int64 Revision, const FString& ContentHash)
{
    if (Revision < SaveHeader.Revision || ContentHash.TrimStartAndEnd().IsEmpty()) return false;
    SaveHeader.Revision = Revision;
    SaveHeader.ContentHash = ContentHash;
    SessionChanged.Broadcast(TEXT("CloudRevisionUpdated"));
    return true;
}

bool UForestSliceWorldSessionSubsystem::DiscoverMarker(FName MarkerId)
{
    for (FForestSliceWorldMarker& Marker : Markers) {
        if (Marker.MarkerId != MarkerId) continue;
        Marker.bDiscovered = true;
        SessionChanged.Broadcast(TEXT("MarkerDiscovered"));
        return true;
    }
    return false;
}

bool UForestSliceWorldSessionSubsystem::SetMissionProgress(FName MissionId, int32 Progress)
{
    for (FForestSliceMissionState& Mission : Missions) {
        if (Mission.MissionId != MissionId) continue;
        Mission.Progress = FMath::Clamp(Progress, 0, FMath::Max(1, Mission.Required));
        Mission.bCompleted = Mission.Progress >= Mission.Required;
        SessionChanged.Broadcast(Mission.bCompleted ? TEXT("MissionCompleted") : TEXT("MissionProgressed"));
        return true;
    }
    return false;
}

bool UForestSliceWorldSessionSubsystem::SetMemberReady(const FString& PlayerId, bool bReady)
{
    const FString CleanPlayerId = PlayerId.TrimStartAndEnd();
    if (CleanPlayerId.IsEmpty()) return false;
    for (FForestSliceCoopMember& Member : Members) {
        if (Member.PlayerId != CleanPlayerId) continue;
        Member.bReady = bReady;
        SessionChanged.Broadcast(TEXT("PartyReadyChanged"));
        return true;
    }
    return false;
}
