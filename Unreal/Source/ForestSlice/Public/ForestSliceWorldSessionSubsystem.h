#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ForestSliceWorldSessionSubsystem.generated.h"

UENUM(BlueprintType)
enum class EForestSliceWorldPrivacy : uint8
{
    InviteOnly,
    FriendsOnly,
    Public
};

USTRUCT(BlueprintType)
struct FForestSliceCloudSaveHeader
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SchemaVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName WorldId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PlayerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 Revision = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ContentHash;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString UpdatedAtUtc;
};

USTRUCT(BlueprintType)
struct FForestSliceMissionState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName MissionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ObjectiveId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Progress = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Required = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCompleted = false;
};

USTRUCT(BlueprintType)
struct FForestSliceWorldMarker
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName MarkerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDiscovered = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCaveEntrance = false;
};

USTRUCT(BlueprintType)
struct FForestSliceCoopMember
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PlayerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PingMilliseconds = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bReady = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceWorldSessionChanged, FName, EventId);

UCLASS(BlueprintType)
class FORESTSLICE_API UForestSliceWorldSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "World")
    bool CreateWorld(FName WorldId, int32 WorldSeed, EForestSliceWorldPrivacy Privacy);

    UFUNCTION(BlueprintCallable, Category = "World")
    bool JoinWorld(FName WorldId, const FString& InviteCode);

    UFUNCTION(BlueprintCallable, Category = "World")
    bool SetCloudRevision(int64 Revision, const FString& ContentHash);

    UFUNCTION(BlueprintCallable, Category = "World")
    bool DiscoverMarker(FName MarkerId);

    UFUNCTION(BlueprintCallable, Category = "Mission")
    bool SetMissionProgress(FName MissionId, int32 Progress);

    UFUNCTION(BlueprintCallable, Category = "Coop")
    bool SetMemberReady(const FString& PlayerId, bool bReady);

    UFUNCTION(BlueprintPure, Category = "World")
    FForestSliceCloudSaveHeader GetSaveHeader() const { return SaveHeader; }

    UFUNCTION(BlueprintPure, Category = "World")
    FName GetWorldId() const { return WorldId; }

    UFUNCTION(BlueprintPure, Category = "World")
    int32 GetWorldSeed() const { return WorldSeed; }

    UFUNCTION(BlueprintPure, Category = "World")
    EForestSliceWorldPrivacy GetPrivacy() const { return Privacy; }

    UFUNCTION(BlueprintPure, Category = "Mission")
    const TArray<FForestSliceMissionState>& GetMissions() const { return Missions; }

    UFUNCTION(BlueprintPure, Category = "World")
    const TArray<FForestSliceWorldMarker>& GetMarkers() const { return Markers; }

    UFUNCTION(BlueprintPure, Category = "Coop")
    const TArray<FForestSliceCoopMember>& GetMembers() const { return Members; }

    UPROPERTY(BlueprintAssignable, Category = "World")
    FForestSliceWorldSessionChanged SessionChanged;

protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World")
    FName WorldId = NAME_None;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World")
    int32 WorldSeed = 0;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World")
    EForestSliceWorldPrivacy Privacy = EForestSliceWorldPrivacy::InviteOnly;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Save")
    FForestSliceCloudSaveHeader SaveHeader;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
    TArray<FForestSliceMissionState> Missions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World")
    TArray<FForestSliceWorldMarker> Markers;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coop")
    TArray<FForestSliceCoopMember> Members;
};
