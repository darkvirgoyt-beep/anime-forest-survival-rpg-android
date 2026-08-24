#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "ForestSliceMoonDeerAvoidanceTrait.generated.h"

/** Tag used to identify passive Moon Deer Mass entities for presentation and diagnostics. */
USTRUCT()
struct FForestSliceMoonDeerTag : public FMassTag
{
    GENERATED_BODY()
};

/** Deterministic, lightweight roaming state; gameplay authority remains outside Mass Avoidance. */
USTRUCT()
struct FForestSliceMoonDeerRoamFragment : public FMassFragment
{
    GENERATED_BODY()

    FVector Target = FVector::ZeroVector;
    float TimeUntilRetarget = 0.0f;
};

/**
 * Fragment contract for passive Moon Deer herds. The MassEntity config that uses this
 * trait must also provide a representation/visualization trait and a move-target source.
 */
UCLASS(BlueprintType, EditInlineNew)
class FORESTSLICE_API UForestSliceMoonDeerAvoidanceTrait : public UMassEntityTraitBase
{
    GENERATED_BODY()

public:
    virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Mass Avoidance", meta = (ClampMin = "1.0", ClampMax = "200.0", ForceUnits = "cm"))
    float AgentRadiusCm = 38.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Mass Avoidance", meta = (ClampMin = "0.0", ClampMax = "2000.0", ForceUnits = "cm/s"))
    float DesiredSpeedCmPerSecond = 190.0f;
};
