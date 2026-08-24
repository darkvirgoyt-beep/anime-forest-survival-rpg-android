#include "ForestSliceWildCreature.h"

#include "Net/UnrealNetwork.h"

AForestSliceWildCreature::AForestSliceWildCreature()
{
    bReplicates = true;
    SetReplicateMovement(true);
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    NetCullDistanceSquared = FMath::Square(18000.0f);
}

void AForestSliceWildCreature::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AForestSliceWildCreature, CreatureState);
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
