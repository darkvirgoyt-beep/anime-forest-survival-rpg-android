#include "ForestSliceGameMode.h"

#include "ForestSliceCharacter.h"

AForestSliceGameMode::AForestSliceGameMode()
{
    DefaultPawnClass = AForestSliceCharacter::StaticClass();
}

