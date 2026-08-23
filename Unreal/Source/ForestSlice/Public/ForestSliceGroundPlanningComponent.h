#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceGroundPlanningComponent.generated.h"

UENUM(BlueprintType)
enum class EForestSliceTool : uint8
{
    None UMETA(DisplayName = "None"),
    Pickaxe UMETA(DisplayName = "Pickaxe"),
    Shovel UMETA(DisplayName = "Shovel")
};

USTRUCT(BlueprintType)
struct FForestSliceFarmContour
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farm Contour")
    FVector Center = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farm Contour")
    FVector2D Size = FVector2D(300.0f, 300.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Farm Contour")
    float Height = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farm Contour")
    bool bPlanned = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farm Contour")
    bool bTopsoilExposed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farm Contour")
    bool bSeeded = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farm Contour")
    float Moisture = 0.0f;
};

UCLASS(ClassGroup = (World), meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceGroundPlanningComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceGroundPlanningComponent();

    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation")
    float DigRadius = 85.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation")
    float UndergroundRevealDepth = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation")
    float ShovelDigPower = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Excavation")
    float PickaxeDigPower = 0.8f;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Excavation")
    TObjectPtr<AActor> UndergroundContentActor;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Excavation")
    float ExcavatedDepth = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Excavation")
    bool bUndergroundRevealed = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ground Planning")
    bool bGroundPlanned = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Farm Contour")
    FForestSliceFarmContour CurrentContour;

    UFUNCTION(BlueprintCallable, Category = "Excavation")
    bool DigAtLocation(const FVector& Location, EForestSliceTool Tool);

    UFUNCTION(BlueprintCallable, Category = "Ground Planning")
    bool PlanGround(const FVector& Center, FVector2D Size);

    UFUNCTION(BlueprintCallable, Category = "Farm Contour")
    bool CreateFarmContour(const FVector& Center, FVector2D Size, float Height);

    UFUNCTION(BlueprintCallable, Category = "Farm Contour")
    bool PlantSeed();

    UFUNCTION(BlueprintPure, Category = "Excavation")
    bool IsUndergroundRevealed() const { return bUndergroundRevealed; }

    UFUNCTION(BlueprintPure, Category = "Ground Planning")
    bool IsGroundPlanned() const { return bGroundPlanned; }

    UFUNCTION(BlueprintPure, Category = "Farm Contour")
    bool HasFarmContour() const { return CurrentContour.bPlanned; }

    UFUNCTION(BlueprintPure, Category = "Farm Contour")
    FForestSliceFarmContour GetCurrentContour() const { return CurrentContour; }

    UFUNCTION(BlueprintPure, Category = "Excavation")
    float GetExcavationProgress() const;

private:
    bool IsInsidePlanningArea(const FVector& Location) const;
    void UpdateUndergroundVisibility();
};
