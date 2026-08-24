#include "ForestSlicePlayerController.h"

#include "ForestSliceCharacter.h"
#include "ForestSliceMobileHUD.h"

AForestSlicePlayerController::AForestSlicePlayerController()
{
    MobileHUDClass = UForestSliceMobileHUD::StaticClass();
    bShowMouseCursor = false;
}

void AForestSlicePlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController() && MobileHUDClass)
    {
        MobileHUD = CreateWidget<UForestSliceMobileHUD>(this, MobileHUDClass);
        if (MobileHUD)
        {
            MobileHUD->AddToViewport(100);
            BindHUDToPawn();
        }
    }
}

void AForestSlicePlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    BindHUDToPawn();
}

void AForestSlicePlayerController::OnUnPossess()
{
    if (MobileHUD)
    {
        MobileHUD->SetControlledCharacter(nullptr);
    }
    Super::OnUnPossess();
}

void AForestSlicePlayerController::BindHUDToPawn()
{
    if (MobileHUD)
    {
        MobileHUD->SetControlledCharacter(Cast<AForestSliceCharacter>(GetPawn()));
    }
}
