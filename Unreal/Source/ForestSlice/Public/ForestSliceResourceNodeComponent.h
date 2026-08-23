#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceInteractionComponent.h"
#include "ForestSliceResourceNodeComponent.generated.h"

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceResourceNodeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceResourceNodeComponent();

    UFUNCTION(BlueprintPure, Category = "Gathering")
    FForestSliceInteractableCandidate GetInteractionCandidate() const;

    UFUNCTION(BlueprintCallable, Category = "Gathering")
    bool TryCollect(AActor* Collector);

    UFUNCTION(BlueprintPure, Category = "Gathering")
    bool IsDepleted() const { return bDepleted; }

    UFUNCTION(BlueprintPure, Category = "Gathering")
    FName GetStableResourceId() const { return StableResourceId; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering")
    FName StableResourceId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering")
    FName ItemId = TEXT("ForestFiber");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering", meta = (ClampMin = "1"))
    int32 Quantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering", meta = (ClampMin = "1"))
    int32 StackLimit = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering", meta = (ClampMin = "0"))
    int32 GatheringXP = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gathering", meta = (ClampMin = "1.0"))
    float InteractionDistance = 260.0f;

    UPROPERTY(ReplicatedUsing = OnRep_Depleted, EditAnywhere, BlueprintReadOnly, Category = "Gathering")
    bool bDepleted = false;

    UFUNCTION()
    void OnRep_Depleted();

private:
    void BroadcastDepletedPresentation();
};
