#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ForestSliceGameMode.generated.h"

class AForestSlicePlayerController;

UCLASS()
class FORESTSLICE_API AForestSliceGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AForestSliceGameMode();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
    TSubclassOf<AForestSlicePlayerController> ProductionPlayerControllerClass;
};

