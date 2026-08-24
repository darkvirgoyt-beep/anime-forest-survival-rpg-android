#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ForestSliceMassAvoidanceSettings.generated.h"

/** Runtime gate for the experimental ambient Moon Deer Mass Avoidance path. */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Aethelgard Mass Avoidance"))
class FORESTSLICE_API UForestSliceMassAvoidanceSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(config, EditAnywhere, Category = "Moon Deer", meta = (DisplayName = "Enable experimental Moon Deer Mass Avoidance"))
    bool bEnableMoonDeerMassAvoidance = false;

    UPROPERTY(config, EditAnywhere, Category = "Moon Deer", meta = (DisplayName = "Allow on Android"))
    bool bAllowMoonDeerMassAvoidanceOnAndroid = false;

    UPROPERTY(config, EditAnywhere, Category = "Moon Deer", meta = (ClampMin = "1", ClampMax = "32"))
    int32 MaxAmbientMoonDeer = 24;

    UPROPERTY(config, EditAnywhere, Category = "Moon Deer", meta = (ClampMin = "1.0", ClampMax = "200.0", ForceUnits = "cm"))
    float MoonDeerAgentRadiusCm = 38.0f;

    UFUNCTION(BlueprintPure, Category = "Aethelgard|Mass Avoidance")
    bool IsMoonDeerMassAvoidanceEnabled() const;
};
