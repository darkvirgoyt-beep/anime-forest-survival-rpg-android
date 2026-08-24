#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ForestSliceFirstPlayableSubsystem.generated.h"

/**
 * The smallest player-visible progression for the original Aethelgard Unreal production path.
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|FirstPlayable")
    EForestSliceFirstPlayablePhase Phase = EForestSliceFirstPlayablePhase::Boot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|FirstPlayable")
    FString AccountId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|FirstPlayable")
    FString WorldId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|FirstPlayable")
    int32 WorldSeed = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|FirstPlayable")
    bool bAccountVerified = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|FirstPlayable")
    bool bWorldRecovered = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|FirstPlayable")
    bool bCharacterConfirmed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|FirstPlayable")
    bool bCampBuilt = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|FirstPlayable")
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
    UPROPERTY(BlueprintAssignable, Category = "Aethelgard|FirstPlayable")
    FForestSliceFirstPlayablePhaseChanged OnPhaseChanged;

    UFUNCTION(BlueprintPure, Category = "Aethelgard|FirstPlayable")
    FForestSliceFirstPlayableState GetState() const { return State; }

    /** Called only after the platform bridge and backend exchange report an authenticated account. */
    UFUNCTION(BlueprintCallable, Category = "Aethelgard|FirstPlayable")
    void BeginAuthenticatedSession(const FString& VerifiedAccountId);

    /** Called after an owned world manifest is authorized and recovered by the online service. */
    UFUNCTION(BlueprintCallable, Category = "Aethelgard|FirstPlayable")
    void ResolveOwnedWorld(const FString& AuthorizedWorldId, int32 AuthorizedWorldSeed);

    UFUNCTION(BlueprintCallable, Category = "Aethelgard|FirstPlayable")
    void ConfirmCharacterSetup();

    UFUNCTION(BlueprintCallable, Category = "Aethelgard|FirstPlayable")
    void ConfirmCampBuilt();

    UFUNCTION(BlueprintCallable, Category = "Aethelgard|FirstPlayable")
    void ConfirmBedPlaced();

    UFUNCTION(BlueprintCallable, Category = "Aethelgard|FirstPlayable")
    void ReportBlockingError();

    UFUNCTION(BlueprintCallable, Category = "Aethelgard|FirstPlayable")
    void ResetFirstPlayable();

private:
    void TransitionTo(EForestSliceFirstPlayablePhase NextPhase);

    FForestSliceFirstPlayableState State;
};
