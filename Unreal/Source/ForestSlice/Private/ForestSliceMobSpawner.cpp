#include "ForestSliceMobSpawner.h"

#include "Engine/World.h"
#include "ForestSliceMob.h"

AForestSliceMobSpawner::AForestSliceMobSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
}

void AForestSliceMobSpawner::BeginPlay()
{
    Super::BeginPlay();
    if (bSpawnOnBeginPlay) RespawnAll();
}

void AForestSliceMobSpawner::RespawnAll()
{
    if (!GetWorld() || !MobClass) return;
    if (bAuthorityOnly && !HasAuthority()) return;

    ClearSpawnedMobs();
    const int32 Count = FMath::Clamp(SpawnCount, 0, 32);
    for (int32 Index = 0; Index < Count; ++Index) {
        const FVector Offset = FVector(
            FMath::FRandRange(-SpawnRadius, SpawnRadius),
            FMath::FRandRange(-SpawnRadius, SpawnRadius),
            0.0f
        );
        const FTransform SpawnTransform(GetActorRotation(), GetActorLocation() + Offset);
        FActorSpawnParameters Parameters;
        Parameters.Owner = this;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        if (AForestSliceMob* Mob = GetWorld()->SpawnActor<AForestSliceMob>(MobClass, SpawnTransform, Parameters)) {
            SpawnedMobs.Add(Mob);
        }
    }
}

void AForestSliceMobSpawner::ClearSpawnedMobs()
{
    for (AForestSliceMob* Mob : SpawnedMobs) {
        if (IsValid(Mob)) Mob->Destroy();
    }
    SpawnedMobs.Empty();
}
