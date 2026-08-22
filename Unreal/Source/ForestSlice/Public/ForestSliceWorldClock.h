#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestSliceWorldClock.generated.h"

UCLASS(BlueprintType)
class FORESTSLICE_API AForestSliceWorldClock : public AActor
{
    GENERATED_BODY()

public:
    AForestSliceWorldClock();

    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category = "World|Time")
    float GetWorldTimeHours() const { return WorldTimeHours; }

    UFUNCTION(BlueprintPure, Category = "World|Time")
    float GetDayAlpha() const;

    UFUNCTION(BlueprintPure, Category = "World|Time")
    bool IsNight() const;

    UFUNCTION(BlueprintCallable, Category = "World|Time")
    bool RequestSleep(AActor* Bed, APawn* RequestingPawn, float SleepUntilHour = 6.0f);

    UFUNCTION(BlueprintCallable, Category = "World|Time")
    void AdvanceTime(float Hours);

protected:
    UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "World|Time")
    float WorldTimeHours = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Time")
    float RealSecondsPerGameHour = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Time")
    float NightStartHour = 19.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Time")
    float NightEndHour = 6.0f;

private:
    bool IsValidSleepRequest(AActor* Bed, APawn* RequestingPawn) const;
};
