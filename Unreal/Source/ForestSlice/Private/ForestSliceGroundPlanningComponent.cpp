#include "ForestSliceGroundPlanningComponent.h"

UForestSliceGroundPlanningComponent::UForestSliceGroundPlanningComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UForestSliceGroundPlanningComponent::BeginPlay()
{
    Super::BeginPlay();
    UpdateUndergroundVisibility();
}

void UForestSliceGroundPlanningComponent::UpdateUndergroundVisibility()
{
    if (UndergroundContentActor) {
        UndergroundContentActor->SetActorHiddenInGame(!bUndergroundRevealed);
        UndergroundContentActor->SetActorEnableCollision(bUndergroundRevealed);
    }
}

bool UForestSliceGroundPlanningComponent::IsInsidePlanningArea(const FVector& Location) const
{
    if (!bGroundPlanned) return false;

    const FVector Delta = Location - CurrentContour.Center;
    return FMath::Abs(Delta.X) <= CurrentContour.Size.X * 0.5f
        && FMath::Abs(Delta.Y) <= CurrentContour.Size.Y * 0.5f;
}

bool UForestSliceGroundPlanningComponent::DigAtLocation(const FVector& Location, EForestSliceTool Tool)
{
    if (Tool == EForestSliceTool::None || !IsInsidePlanningArea(Location)) return false;

    const FVector Delta = Location - CurrentContour.Center;
    if (Delta.Size2D() > DigRadius + CurrentContour.Size.GetMax() * 0.5f) return false;

    const float ToolPower = Tool == EForestSliceTool::Shovel ? ShovelDigPower : PickaxeDigPower;
    ExcavatedDepth = FMath::Clamp(ExcavatedDepth + ToolPower * 12.0f, 0.0f, UndergroundRevealDepth);
    CurrentContour.bTopsoilExposed = ExcavatedDepth >= UndergroundRevealDepth * 0.20f;
    bUndergroundRevealed = ExcavatedDepth >= UndergroundRevealDepth;
    UpdateUndergroundVisibility();
    return true;
}

bool UForestSliceGroundPlanningComponent::PlanGround(const FVector& Center, FVector2D Size)
{
    const FVector2D SafeSize(
        FMath::Clamp(Size.X, 100.0f, 1200.0f),
        FMath::Clamp(Size.Y, 100.0f, 1200.0f)
    );

    CurrentContour.Center = Center;
    CurrentContour.Size = SafeSize;
    CurrentContour.Height = Center.Z;
    CurrentContour.bPlanned = true;
    CurrentContour.bTopsoilExposed = false;
    CurrentContour.bSeeded = false;
    CurrentContour.Moisture = 0.0f;
    bGroundPlanned = true;
    ExcavatedDepth = 0.0f;
    bUndergroundRevealed = false;
    UpdateUndergroundVisibility();
    return true;
}

bool UForestSliceGroundPlanningComponent::CreateFarmContour(const FVector& Center, FVector2D Size, float Height)
{
    if (!bGroundPlanned) return false;

    const FVector2D SafeSize(
        FMath::Clamp(Size.X, 100.0f, 1200.0f),
        FMath::Clamp(Size.Y, 100.0f, 1200.0f)
    );
    CurrentContour.Center = Center;
    CurrentContour.Size = SafeSize;
    CurrentContour.Height = Height;
    CurrentContour.bPlanned = true;
    return true;
}

bool UForestSliceGroundPlanningComponent::PlantSeed()
{
    if (!CurrentContour.bPlanned || !CurrentContour.bTopsoilExposed || !bUndergroundRevealed) return false;

    CurrentContour.bSeeded = true;
    CurrentContour.Moisture = 0.25f;
    return true;
}

float UForestSliceGroundPlanningComponent::GetExcavationProgress() const
{
    return FMath::Clamp(ExcavatedDepth / FMath::Max(UndergroundRevealDepth, 1.0f), 0.0f, 1.0f);
}
