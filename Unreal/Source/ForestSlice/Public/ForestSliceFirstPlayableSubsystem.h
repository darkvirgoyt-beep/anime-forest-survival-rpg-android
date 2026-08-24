#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ForestSliceFirstPlayableSubsystem.generated.h"

/**
 * The smallest player-visible progression for the original Aethelgrad Unreal production path.
 * It coordinates only local presentation and hand-off points; account verification, cloud-save
 * ownership, combat outcomes, and persistence remain server-authoritative responsibilities.
 */
UENUM(BlueprintType)
enum class EForestSliceFirstPlayablePhase : uint8
{
    Boot,
    AccountGate,
    WorldRecovery,
    CharacterSetup,
    ForestArrival,
    CampTutorial,
    Complete,
    Error
};

USTRUCT(BlueprintType)
struct FForestSliceFirstPlayableState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgrad|FirstPlayable")
    EForestSliceFirstPlayablePhase Phase = EForestSliceFirstPlayablePhase::Boot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgrad|FirstPlayable")
    FString AccountId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgrad|FirstPlayable")
    FString WorldId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgrad|FirstPlayable")
    int32 WorldSeed = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgrad|FirstPlayable")
    bool bAccountVerified = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgrad|FirstPlayable")
    bool bWorldRecovered = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgrad|FirstPlayable")
    bool bCharacterConfirmed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgrad|FirstPlayable")
    bool bCampBuilt = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgrad|FirstPlayable")
    bool bBedPlaced = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FForestSliceFirstPlayablePhaseChanged,
    EForestSliceFirstPlayablePhase, Previous,
    EForestSliceFirstPlayablePhase, Current);

UCLASS(BlueprintType)
class FORESTSLICE_API UForestSliceFirstPlayableSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Aethelgrad|FirstPlayable")
    FForestSliceFirstPlayablePhaseChanged OnPhaseChanged;

    UFUNCTION(BlueprintPure, Category = "Aethelgrad|FirstPlayable")
    FForestSliceFirstPlayableState GetState() const { return State; }

    /** Called only after the platform bridge and backend exchange report an authenticated account. */
    UFUNCTION(BlueprintCallable, Category = "Aethelgrad|FirstPlayable")
    void BeginAuthenticatedSession(const FString& VerifiedAccountId);

    /** Called after an owned world manifest is authorized and recovered by the online service. */
    UFUNCTION(BlueprintCallable, Category = "Aethelgrad|FirstPlayable")
    void ResolveOwnedWorld(const FString& AuthorizedWorldId, int32 AuthorizedWorldSeed);

    UFUNCTION(BlueprintCallable, Category = "Aethelgrad|FirstPlayable")
    void ConfirmCharacterSetup();

    UFUNCTION(BlueprintCallable, Category = "Aethelgrad|FirstPlayable")
    void ConfirmCampBuilt();

    UFUNCTION(BlueprintCallable, Category = "Aethelgrad|FirstPlayable")
    void ConfirmBedPlaced();

    UFUNCTION(BlueprintCallable, Category = "Aethelgrad|FirstPlayable")
    void ReportBlockingError();

    UFUNCTION(BlueprintCallable, Category = "Aethelgrad|FirstPlayable")
    void ResetFirstPlayable();

private:
    void TransitionTo(EForestSliceFirstPlayablePhase NextPhase);

    FForestSliceFirstPlayableState State;
};
