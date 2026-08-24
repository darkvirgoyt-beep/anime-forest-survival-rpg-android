#include "ForestSliceGameMode.h"

#include "ForestSliceCharacter.h"
#include "ForestSlicePlayerController.h"

AForestSliceGameMode::AForestSliceGameMode()
{
    DefaultPawnClass = AForestSliceCharacter::StaticClass();
    PlayerControllerClass = AForestSlicePlayerController::StaticClass();
}

