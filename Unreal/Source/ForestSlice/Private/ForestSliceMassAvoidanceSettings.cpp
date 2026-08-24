#include "ForestSliceMassAvoidanceSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ForestSliceMassAvoidanceSettings)

bool UForestSliceMassAvoidanceSettings::IsMoonDeerMassAvoidanceEnabled() const
{
#if PLATFORM_ANDROID
    return bEnableMoonDeerMassAvoidance && bAllowMoonDeerMassAvoidanceOnAndroid;
#else
    return bEnableMoonDeerMassAvoidance;
#endif
}
