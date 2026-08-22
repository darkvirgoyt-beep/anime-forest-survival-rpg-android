#include "ForestSliceBed.h"

#include "ForestSliceSurvivalComponent.h"
#include "ForestSliceWorldClock.h"
#include "GameFramework/Pawn.h"

AForestSliceBed::AForestSliceBed()
{
    bReplicates = true;
    SetReplicateMovement(false);
}

bool AForestSliceBed::RequestSleep(APawn* RequestingPawn)
{
    if (!HasAuthority() || Occupant.IsValid() || !WorldClock || !RequestingPawn) return false;
    Occupant = RequestingPawn;
    const bool bSlept = WorldClock->RequestSleep(this, RequestingPawn, 6.0f);
    if (bSlept) {
        if (UForestSliceSurvivalComponent* Survival = RequestingPawn->FindComponentByClass<UForestSliceSurvivalComponent>()) {
            Survival->RestoreFromSleep();
        }
    }
    Occupant = nullptr;
    return bSlept;
}
