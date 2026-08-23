#include "ForestSliceWorldSessionSubsystem.h"

namespace
{
    bool IsValidInviteCode(const FString& InviteCode)
    {
        const FString CleanCode = InviteCode.TrimStartAndEnd();
        if (CleanCode.Len() != 6) return false;
        for (const TCHAR Character : CleanCode)
        {
            const bool bUppercaseLetter = Character >= TCHAR('A') && Character <= TCHAR('Z');
            const bool bDigit = Character >= TCHAR('0') && Character <= TCHAR('9');
            if (!bUppercaseLetter && !bDigit) return false;
        }
        return true;
    }
}

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
    if (InWorldId.IsNone() || !IsValidInviteCode(InviteCode) || WorldId != InWorldId) return false;
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

bool UForestSliceWorldSessionSubsystem::AddOrReconnectMember(const FForestSliceCoopMember& InMember)
{
    const FString CleanPlayerId = InMember.PlayerId.TrimStartAndEnd();
    if (CleanPlayerId.IsEmpty()) return false;

    for (FForestSliceCoopMember& Member : Members)
    {
        if (Member.PlayerId != CleanPlayerId) continue;
        Member.DisplayName = InMember.DisplayName;
        Member.PingMilliseconds = FMath::Max(-1, InMember.PingMilliseconds);
        Member.bReady = InMember.bReady;
        SessionChanged.Broadcast(TEXT("PartyMemberReconnected"));
        return true;
    }

    if (Members.Num() >= MaxCoopMembers) return false;
    FForestSliceCoopMember NewMember = InMember;
    NewMember.PlayerId = CleanPlayerId;
    NewMember.PingMilliseconds = FMath::Max(-1, InMember.PingMilliseconds);
    Members.Add(MoveTemp(NewMember));
    SessionChanged.Broadcast(TEXT("PartyMemberJoined"));
    return true;
}

bool UForestSliceWorldSessionSubsystem::RemoveMember(const FString& PlayerId)
{
    const FString CleanPlayerId = PlayerId.TrimStartAndEnd();
    if (CleanPlayerId.IsEmpty()) return false;
    const int32 Removed = Members.RemoveAll([&CleanPlayerId](const FForestSliceCoopMember& Member)
    {
        return Member.PlayerId == CleanPlayerId;
    });
    if (Removed == 0) return false;
    SessionChanged.Broadcast(TEXT("PartyMemberLeft"));
    return true;
}

bool UForestSliceWorldSessionSubsystem::CanStartCoopSession() const
{
    if (Members.Num() == 0) return false;
    for (const FForestSliceCoopMember& Member : Members)
    {
        if (!Member.bReady) return false;
    }
    return true;
}
