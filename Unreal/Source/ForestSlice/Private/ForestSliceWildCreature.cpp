#include "ForestSliceWildCreature.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

AForestSliceWildCreature::AForestSliceWildCreature()
{
    bReplicates = true;
    SetReplicateMovement(true);
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    NetCullDistanceSquared = FMath::Square(18000.0f);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    JumpMaxCount = 1;
    JumpMaxHoldTime = 0.0f;
}

void AForestSliceWildCreature::BeginPlay()
{
    Super::BeginPlay();
    ApplyLocomotionSettings();
}

void AForestSliceWildCreature::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AForestSliceWildCreature, CreatureState);
    DOREPLIFETIME(AForestSliceWildCreature, LocomotionMode);
}

void AForestSliceWildCreature::InitializeFromSpawn(
    FName InSpeciesId,
    EForestSliceCreatureDisposition InDisposition,
    EForestSliceCreatureRole InRole,
    bool bInCaptureCandidate,
    bool bInElite,
    int32 InSpawnRevision)
{
    if (!HasAuthority())
    {
        return;
    }

    CreatureState.SpeciesId = InSpeciesId;
    CreatureState.Disposition = InDisposition;
    CreatureState.Role = InRole;
    CreatureState.bCaptureCandidate = bInCaptureCandidate;
    CreatureState.bElite = bInElite;
    CreatureState.SpawnRevision = InSpawnRevision;
    OnSpawnStateReady(CreatureState);
    ForceNetUpdate();
}

void AForestSliceWildCreature::OnRep_CreatureState()
{
    OnSpawnStateReady(CreatureState);
}

void AForestSliceWildCreature::SetWildCreatureLocomotionMode(EForestSliceWildLocomotionMode NewMode)
{
    if (!HasAuthority())
    {
        return;
    }

    LocomotionMode = NewMode;
    ApplyLocomotionSettings();
    ForceNetUpdate();
}

float AForestSliceWildCreature::GetExpectedJumpDistanceMeters(float HorizontalSpeedCmPerSecond) const
{
    const float Gravity = FMath::Max(FMath::Abs(GetCharacterMovement()->GetGravityZ()), 1.0f);
    const float AirTimeSeconds = 2.0f * Locomotion.JumpZVelocityCmPerSecond / Gravity;
    return FMath::Max(0.0f, HorizontalSpeedCmPerSecond) * AirTimeSeconds / 100.0f;
}

float AForestSliceWildCreature::GetExpectedJumpApexMeters() const
{
    const float Gravity = FMath::Max(FMath::Abs(GetCharacterMovement()->GetGravityZ()), 1.0f);
    return FMath::Square(Locomotion.JumpZVelocityCmPerSecond) / (2.0f * Gravity * 100.0f);
}

void AForestSliceWildCreature::OnRep_LocomotionMode()
{
    ApplyLocomotionSettings();
}

void AForestSliceWildCreature::ApplyLocomotionSettings()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (!Movement)
    {
        return;
    }

    switch (LocomotionMode)
    {
    case EForestSliceWildLocomotionMode::Alert:
        Movement->MaxWalkSpeed = Locomotion.AlertSpeedCmPerSecond;
        break;
    case EForestSliceWildLocomotionMode::Sprint:
        Movement->MaxWalkSpeed = Locomotion.SprintSpeedCmPerSecond;
        break;
    default:
        Movement->MaxWalkSpeed = Locomotion.RoamSpeedCmPerSecond;
        break;
    }

    Movement->MaxAcceleration = Locomotion.MaxAccelerationCmPerSecondSquared;
    Movement->BrakingDecelerationWalking = Locomotion.BrakingDecelerationCmPerSecondSquared;
    Movement->RotationRate = FRotator(0.0f, Locomotion.RotationRateYawDegreesPerSecond, 0.0f);
    Movement->AirControl = Locomotion.AirControl;
    Movement->JumpZVelocity = Locomotion.JumpZVelocityCmPerSecond;
    Movement->GravityScale = Locomotion.GravityScale;
}
