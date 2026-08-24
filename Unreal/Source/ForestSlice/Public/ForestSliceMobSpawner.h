#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestSliceMobSpawner.generated.h"

class AForestSliceMob;

UCLASS(BlueprintType, Blueprintable)
class FORESTSLICE_API AForestSliceMobSpawner : public AActor
{
    GENERATED_BODY()

public:
    AForestSliceMobSpawner();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Mob Spawner")
    void RespawnAll();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Spawner")
    TSubclassOf<AForestSliceMob> MobClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Spawner")
    int32 SpawnCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Spawner")
    float SpawnRadius = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Spawner")
    bool bSpawnOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Spawner")
    bool bAuthorityOnly = true;

private:
    UPROPERTY()
    TArray<TObjectPtr<AForestSliceMob>> SpawnedMobs;

    void ClearSpawnedMobs();
};
