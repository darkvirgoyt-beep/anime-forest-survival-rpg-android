#include "ForestSliceServerDirectorySubsystem.h"

void UForestSliceServerDirectorySubsystem::SetDirectory(const TArray<FForestSliceServerEntry>& InServers)
{
    Servers = InServers;
    if (!SelectedServer.ServerId.IsNone()) {
        for (const FForestSliceServerEntry& Server : Servers) {
            if (Server.ServerId == SelectedServer.ServerId) {
                SelectedServer = Server;
                return;
            }
        }
    }
    if (!Servers.IsEmpty()) {
        SelectedServer = Servers[0];
        ServerSelected.Broadcast(SelectedServer);
    }
}

bool UForestSliceServerDirectorySubsystem::SelectServer(FName ServerId)
{
    for (const FForestSliceServerEntry& Server : Servers) {
        if (Server.ServerId != ServerId || Server.Status == EForestSliceServerStatus::Offline || Server.Status == EForestSliceServerStatus::Maintenance) continue;
        SelectedServer = Server;
        ServerSelected.Broadcast(SelectedServer);
        return true;
    }
    return false;
}

void UForestSliceServerDirectorySubsystem::SetMeasuredPing(FName ServerId, int32 PingMilliseconds)
{
    const int32 ClampedPing = FMath::Max(-1, PingMilliseconds);
    for (FForestSliceServerEntry& Server : Servers) {
        if (Server.ServerId != ServerId) continue;
        Server.PingMilliseconds = ClampedPing;
        if (SelectedServer.ServerId == ServerId) {
            SelectedServer = Server;
            ServerSelected.Broadcast(SelectedServer);
        }
        return;
    }
}
