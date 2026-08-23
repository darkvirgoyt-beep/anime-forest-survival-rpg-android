#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceProgressionComponent.generated.h"

USTRUCT(BlueprintType)
struct FForestSliceProgressionState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Experience = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ExperienceToNext = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalExperience = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FForestSliceProgressionEvent, int32, NewLevel, int32, Experience, int32, ExperienceToNext);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceProgressionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceProgressionComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Progression")
    int32 AwardExperience(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Progression")
    int32 AwardGrindingXP(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Progression")
    const FForestSliceProgressionState& GetState() const { return State; }

    UFUNCTION(BlueprintPure, Category = "Progression")
    float GetLevelProgressNormalized() const;

    UFUNCTION(BlueprintPure, Category = "Progression")
    int32 GetXPRequirementForLevel(int32 Level) const;

    UFUNCTION(BlueprintPure, Category = "Progression")
    bool IsMaxLevel() const { return State.Level >= MaxLevel; }

    UPROPERTY(BlueprintAssignable, Category = "Progression")
    FForestSliceProgressionEvent LevelChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_State, EditAnywhere, BlueprintReadOnly, Category = "Progression")
    FForestSliceProgressionState State;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression")
    int32 MaxLevel = 100;

    UFUNCTION()
    void OnRep_State();

private:
    void NormalizeState();
    void BroadcastLevelChanged();
};
