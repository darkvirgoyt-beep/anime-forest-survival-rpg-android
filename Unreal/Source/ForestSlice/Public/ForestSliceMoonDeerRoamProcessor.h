#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "ForestSliceMoonDeerRoamProcessor.generated.h"

/** Supplies low-frequency deterministic roaming targets for passive Moon Deer Mass entities. */
UCLASS()
class FORESTSLICE_API UForestSliceMoonDeerRoamProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UForestSliceMoonDeerRoamProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    FMassEntityQuery EntityQuery;
};
