#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ForestSliceServerDirectorySubsystem.generated.h"

UENUM(BlueprintType)
enum class EForestSliceServerStatus : uint8
{
    Unknown,
    Online,
    Busy,
    Maintenance,
    Offline
};

USTRUCT(BlueprintType)
struct FForestSliceServerEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ServerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Host;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Port = 7777;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PingMilliseconds = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentPlayers = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxPlayers = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EForestSliceServerStatus Status = EForestSliceServerStatus::Unknown;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceServerSelectionChanged, const FForestSliceServerEntry&, Server);

UCLASS(BlueprintType)
class FORESTSLICE_API UForestSliceServerDirectorySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Server")
    void SetDirectory(const TArray<FForestSliceServerEntry>& InServers);

    UFUNCTION(BlueprintCallable, Category = "Server")
    bool SelectServer(FName ServerId);

    UFUNCTION(BlueprintPure, Category = "Server")
    const TArray<FForestSliceServerEntry>& GetServers() const { return Servers; }

    UFUNCTION(BlueprintPure, Category = "Server")
    FForestSliceServerEntry GetSelectedServer() const { return SelectedServer; }

    UFUNCTION(BlueprintCallable, Category = "Server")
    void SetMeasuredPing(FName ServerId, int32 PingMilliseconds);

    UPROPERTY(BlueprintAssignable, Category = "Server")
    FForestSliceServerSelectionChanged ServerSelected;

private:
    UPROPERTY()
    TArray<FForestSliceServerEntry> Servers;

    UPROPERTY()
    FForestSliceServerEntry SelectedServer;
};
