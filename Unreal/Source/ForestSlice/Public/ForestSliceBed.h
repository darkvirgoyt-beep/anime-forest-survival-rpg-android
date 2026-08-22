#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestSliceBed.generated.h"

class AForestSliceWorldClock;

UCLASS(BlueprintType)
class FORESTSLICE_API AForestSliceBed : public AActor
{
    GENERATED_BODY()

public:
    AForestSliceBed();

    UFUNCTION(BlueprintCallable, Category = "Survival|Sleep")
    bool RequestSleep(APawn* RequestingPawn);

    UFUNCTION(BlueprintPure, Category = "Survival|Sleep")
    bool IsOccupied() const { return Occupant.IsValid(); }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival|Sleep")
    TObjectPtr<AForestSliceWorldClock> WorldClock;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Survival|Sleep")
    TWeakObjectPtr<APawn> Occupant;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival|Sleep")
    float SleepDurationHours = 8.0f;
};
