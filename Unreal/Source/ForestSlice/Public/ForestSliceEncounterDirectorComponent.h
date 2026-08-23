#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceEncounterDirectorComponent.generated.h"

UENUM(BlueprintType)
enum class EForestSliceEncounterBiome : uint8
{
    Forest,
    Sand,
    Snow,
    Ruins
};

USTRUCT(BlueprintType)
struct FForestSliceEncounterBudget
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ActiveCreatureLimit = 5;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 EliteSlots = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float RespawnSeconds = 38.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float ThreatMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Revision = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceEncounterBudgetChanged, const FForestSliceEncounterBudget&, Budget);

UCLASS(ClassGroup = (World), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceEncounterDirectorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceEncounterDirectorComponent();

    UFUNCTION(BlueprintCallable, Category = "Encounters")
    void RebuildBudget(EForestSliceEncounterBiome Biome, float WorldTimeHours, float WeatherIntensity, int32 GraphicsQuality);

    UFUNCTION(BlueprintPure, Category = "Encounters")
    bool CanSpawn(int32 ActiveCreatures, bool bElite) const;

    UFUNCTION(BlueprintPure, Category = "Encounters")
    const FForestSliceEncounterBudget& GetBudget() const { return Budget; }

    UPROPERTY(BlueprintAssignable, Category = "Encounters")
    FForestSliceEncounterBudgetChanged BudgetChanged;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounters")
    FForestSliceEncounterBudget Budget;
};
