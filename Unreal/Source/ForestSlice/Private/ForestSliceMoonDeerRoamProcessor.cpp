#include "ForestSliceMoonDeerRoamProcessor.h"

#include "ForestSliceMassAvoidanceSettings.h"
#include "ForestSliceMoonDeerAvoidanceTrait.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ForestSliceMoonDeerRoamProcessor)

UForestSliceMoonDeerRoamProcessor::UForestSliceMoonDeerRoamProcessor()
{
    ExecutionFlags = (int32)EProcessorExecutionFlags::All;
    ExecutionOrder.ExecuteBefore.Add(FName(TEXT("Avoidance")));
}

void UForestSliceMoonDeerRoamProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
    EntityQuery.Initialize(EntityManager);
    EntityQuery.AddTagRequirement<FForestSliceMoonDeerTag>(EMassFragmentPresence::All);
    EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddRequirement<FForestSliceMoonDeerRoamFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.RegisterWithProcessor(*this);
}

void UForestSliceMoonDeerRoamProcessor::Execute(FMassEntityManager&, FMassExecutionContext& Context)
{
    const UForestSliceMassAvoidanceSettings* Settings = GetDefault<UForestSliceMassAvoidanceSettings>();
    if (!Settings || !Settings->IsMoonDeerMassAvoidanceEnabled())
    {
        return;
    }

    EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& ChunkContext)
    {
        const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
        const TArrayView<FMassMoveTargetFragment> MoveTargets = ChunkContext.GetMutableFragmentView<FMassMoveTargetFragment>();
        const TArrayView<FForestSliceMoonDeerRoamFragment> RoamStates = ChunkContext.GetMutableFragmentView<FForestSliceMoonDeerRoamFragment>();
        const float DeltaSeconds = ChunkContext.GetDeltaTimeSeconds();

        for (int32 Index = 0; Index < ChunkContext.GetNumEntities(); ++Index)
        {
            const FVector CurrentLocation = Transforms[Index].GetTransform().GetLocation();
            FForestSliceMoonDeerRoamFragment& Roam = RoamStates[Index];
            FMassMoveTargetFragment& MoveTarget = MoveTargets[Index];
            Roam.TimeUntilRetarget = FMath::Max(0.0f, Roam.TimeUntilRetarget - DeltaSeconds);

            const float DistanceToTarget = FVector::Dist2D(CurrentLocation, Roam.Target);
            if (Roam.TimeUntilRetarget <= 0.0f || DistanceToTarget < 100.0f || Roam.Target.IsNearlyZero())
            {
                const float Phase = CurrentLocation.X * 0.0017f + CurrentLocation.Y * 0.0023f;
                const float Angle = FMath::Fmod(FMath::Abs(FMath::Sin(Phase) * 1000.0f), 2.0f * PI);
                const float Radius = 260.0f + FMath::Abs(FMath::Cos(Phase * 0.73f)) * 180.0f;
                Roam.Target = CurrentLocation + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
                Roam.TimeUntilRetarget = 2.5f;
            }

            const FVector DesiredDirection = (Roam.Target - CurrentLocation).GetSafeNormal2D();
            MoveTarget.Center = Roam.Target;
            MoveTarget.Forward = DesiredDirection;
            MoveTarget.DesiredSpeed = FMassInt16Real(190.0f);
            MoveTarget.SlackRadius = 90.0f;
            MoveTarget.DistanceToGoal = FVector::Dist2D(CurrentLocation, Roam.Target);
            MoveTarget.EntityDistanceToGoal = MoveTarget.DistanceToGoal;
        }
    });
}
