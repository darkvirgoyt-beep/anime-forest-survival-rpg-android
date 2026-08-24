#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestSliceOriginalWorldGenerator.h"
#include "ForestSliceWildCreature.h"
#include "ForestSliceBiomeSpawnDirector.generated.h"

class UForestSliceEncounterDirectorComponent;

USTRUCT(BlueprintType)
struct FForestSliceBiomeSpawnProfile
{
    GENERATED_BODY()

    /** Unique original species key, for example Emberling or DuskmawProwler. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    FName SpeciesId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    TSubclassOf<AForestSliceWildCreature> CreatureClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    TArray<EForestSliceOriginalBiome> AllowedBiomes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    EForestSliceCreatureDisposition Disposition = EForestSliceCreatureDisposition::Passive;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    EForestSliceCreatureRole Role = EForestSliceCreatureRole::Wildlife;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    bool bCaptureCandidate = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    bool bElite = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning", meta = (ClampMin = "1", ClampMax = "8"))
    int32 GroupSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning", meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float Weight = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForestSliceCreatureSpawned, AForestSliceWildCreature*, Creature, FName, SpeciesId);

/**
 * Place one instance in an authored level. It runs only on the Unreal authority, checks the
 * encounter budget, and asks Blueprint creature classes to provide original presentation/AI.
 */
UCLASS(Blueprintable)
class FORESTSLICE_API AForestSliceBiomeSpawnDirector : public AActor
{
    GENERATED_BODY()

public:
    AForestSliceBiomeSpawnDirector();

    virtual void BeginPlay() override;

    /** Server-owned player positions supplied by the authoritative session/game mode. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Aethelgard|Spawning")
    void SetAuthoritativePlayerAnchors(const TArray<FVector>& InAnchors);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Aethelgard|Spawning")
    void RefreshSpawnBudget(float WorldTimeHours, float WeatherIntensity, int32 GraphicsQuality);

    UFUNCTION(BlueprintPure, Category = "Aethelgard|Spawning")
    int32 GetActiveCreatureCount() const;

    UPROPERTY(BlueprintAssignable, Category = "Aethelgard|Spawning")
    FForestSliceCreatureSpawned CreatureSpawned;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    TObjectPtr<UForestSliceEncounterDirectorComponent> EncounterDirector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    TArray<FForestSliceBiomeSpawnProfile> SpawnProfiles;

    /** Invented world seed, unrelated to KML or geographic coordinates. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning")
    int32 OriginalWorldSeed = 190721;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning", meta = (ClampMin = "1000.0"))
    float MinimumSpawnDistance = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning", meta = (ClampMin = "2000.0"))
    float MaximumSpawnDistance = 6500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Spawning", meta = (ClampMin = "0.5"))
    float SpawnCheckSeconds = 4.0f;

private:
    void TickSpawning();
    void PruneInactiveCreatures();
    bool TrySpawnOne();
    int32 GetActiveEliteCount() const;
    const FForestSliceBiomeSpawnProfile* SelectProfile(EForestSliceOriginalBiome Biome, FRandomStream& Stream) const;
    bool FindSafeSpawnPoint(FRandomStream& Stream, FVector& OutLocation) const;
    EForestSliceOriginalBiome GetBiomeForLocation(const FVector& Location) const;

    TArray<FVector> PlayerAnchors;
    TArray<TWeakObjectPtr<AForestSliceWildCreature>> ActiveCreatures;
    FTimerHandle SpawnTimer;
    int32 SpawnRevision = 0;
};
