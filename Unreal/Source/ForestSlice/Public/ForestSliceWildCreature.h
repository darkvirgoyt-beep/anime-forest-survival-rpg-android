#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ForestSliceCreatureComponent.h"
#include "ForestSliceWildCreature.generated.h"

/** Original creature roles; meshes, animation blueprints, sounds, and AI remain authored asset work. */
UENUM(BlueprintType)
enum class EForestSliceCreatureDisposition : uint8
{
    Passive,
    Skittish,
    Hostile
};

/** Locomotion state selected by the authoritative AI controller or Behavior Tree service. */
UENUM(BlueprintType)
enum class EForestSliceWildLocomotionMode : uint8
{
    Roam,
    Alert,
    Sprint
};

/** Centimeter-based movement tuning inherited by each original creature Blueprint. */
USTRUCT(BlueprintType)
struct FForestSliceWildCreatureLocomotion
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Locomotion", meta = (ClampMin = "1.0"))
    float RoamSpeedCmPerSecond = 260.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Locomotion", meta = (ClampMin = "1.0"))
    float AlertSpeedCmPerSecond = 420.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Locomotion", meta = (ClampMin = "1.0"))
    float SprintSpeedCmPerSecond = 520.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Locomotion", meta = (ClampMin = "1.0"))
    float MaxAccelerationCmPerSecondSquared = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Locomotion", meta = (ClampMin = "1.0"))
    float BrakingDecelerationCmPerSecondSquared = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Locomotion", meta = (ClampMin = "0.0", ClampMax = "1440.0"))
    float RotationRateYawDegreesPerSecond = 520.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Locomotion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AirControl = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Locomotion", meta = (ClampMin = "0.0"))
    float JumpZVelocityCmPerSecond = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Locomotion", meta = (ClampMin = "0.1", ClampMax = "4.0"))
    float GravityScale = 1.15f;
};

USTRUCT(BlueprintType)
struct FForestSliceWildCreatureState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|Creature")
    FName SpeciesId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|Creature")
    EForestSliceCreatureDisposition Disposition = EForestSliceCreatureDisposition::Passive;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|Creature")
    EForestSliceCreatureRole Role = EForestSliceCreatureRole::Wildlife;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|Creature")
    bool bCaptureCandidate = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|Creature")
    bool bElite = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|Creature")
    int32 SpawnRevision = 0;
};

UCLASS(Blueprintable)
class FORESTSLICE_API AForestSliceWildCreature : public ACharacter
{
    GENERATED_BODY()

public:
    AForestSliceWildCreature();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Called by the authoritative spawn director after placement validation. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Aethelgard|Creature")
    void InitializeFromSpawn(FName InSpeciesId, EForestSliceCreatureDisposition InDisposition, EForestSliceCreatureRole InRole, bool bInCaptureCandidate, bool bInElite, int32 InSpawnRevision);

    UFUNCTION(BlueprintPure, Category = "Aethelgard|Creature")
    const FForestSliceWildCreatureState& GetCreatureState() const { return CreatureState; }

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Aethelgard|Locomotion")
    void SetWildCreatureLocomotionMode(EForestSliceWildLocomotionMode NewMode);

    UFUNCTION(BlueprintPure, Category = "Aethelgard|Locomotion")
    EForestSliceWildLocomotionMode GetWildCreatureLocomotionMode() const { return LocomotionMode; }

    UFUNCTION(BlueprintPure, Category = "Aethelgard|Locomotion")
    float GetExpectedJumpDistanceMeters(float HorizontalSpeedCmPerSecond) const;

    UFUNCTION(BlueprintPure, Category = "Aethelgard|Locomotion")
    float GetExpectedJumpApexMeters() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aethelgard|Locomotion")
    FForestSliceWildCreatureLocomotion Locomotion;

    UPROPERTY(ReplicatedUsing = OnRep_LocomotionMode, VisibleInstanceOnly, BlueprintReadOnly, Category = "Aethelgard|Locomotion")
    EForestSliceWildLocomotionMode LocomotionMode = EForestSliceWildLocomotionMode::Roam;

    UPROPERTY(ReplicatedUsing = OnRep_CreatureState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Aethelgard|Creature")
    FForestSliceWildCreatureState CreatureState;

    UFUNCTION()
    void OnRep_LocomotionMode();

    UFUNCTION()
    void OnRep_CreatureState();

    /** Blueprint subclasses bind this to start their original idle, herd, flee, or combat logic. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Aethelgard|Creature")
    void OnSpawnStateReady(const FForestSliceWildCreatureState& State);

private:
    void ApplyLocomotionSettings();
};
