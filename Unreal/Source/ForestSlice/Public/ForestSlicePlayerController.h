#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ForestSlicePlayerController.generated.h"

class UForestSliceMobileHUD;

UCLASS()
class FORESTSLICE_API AForestSlicePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AForestSlicePlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mobile|HUD")
    TSubclassOf<UForestSliceMobileHUD> MobileHUDClass;

private:
    UPROPERTY(Transient)
    TObjectPtr<UForestSliceMobileHUD> MobileHUD;

    void BindHUDToPawn();
};
