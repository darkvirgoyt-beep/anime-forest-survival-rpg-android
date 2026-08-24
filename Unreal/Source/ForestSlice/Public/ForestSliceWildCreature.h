#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
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
class FORESTSLICE_API AForestSliceWildCreature : public AActor
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

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aethelgard|Creature")
    TObjectPtr<USceneComponent> CreatureRoot;

    UPROPERTY(ReplicatedUsing = OnRep_CreatureState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Aethelgard|Creature")
    FForestSliceWildCreatureState CreatureState;

    UFUNCTION()
    void OnRep_CreatureState();

    /** Blueprint subclasses bind this to start their original idle, herd, flee, or combat logic. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Aethelgard|Creature")
    void OnSpawnStateReady(const FForestSliceWildCreatureState& State);
};
