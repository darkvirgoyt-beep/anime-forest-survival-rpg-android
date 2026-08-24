#include "ForestSliceMoonDeerAvoidanceTrait.h"

#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "Avoidance/MassAvoidanceFragments.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ForestSliceMoonDeerAvoidanceTrait)

void UForestSliceMoonDeerAvoidanceTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
    BuildContext.AddTag<FForestSliceMoonDeerTag>();
    BuildContext.RequireFragment<FTransformFragment>();
    BuildContext.RequireFragment<FMassVelocityFragment>();
    BuildContext.RequireFragment<FMassMoveTargetFragment>();

    BuildContext.AddFragment_GetRef<FMassForceFragment>().Value = FVector::ZeroVector;
    BuildContext.AddFragment_GetRef<FAgentRadiusFragment>().Radius = AgentRadiusCm;
    BuildContext.AddFragment<FForestSliceMoonDeerRoamFragment>();
    BuildContext.AddFragment_GetRef<FMassAvoidanceColliderFragment>() =
        FMassAvoidanceColliderFragment(FMassCircleCollider(AgentRadiusCm));
}
