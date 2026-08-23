#include "ForestSliceMobileHUD.h"

#include "ForestSliceCharacter.h"
#include "ForestSliceCombatComponent.h"
#include "ForestSliceWeaponComponent.h"
#include "ForestSliceMobPresentationComponent.h"

void UForestSliceMobileHUD::SetControlledCharacter(AForestSliceCharacter* InCharacter)
{
    ControlledCharacter = InCharacter;
    if (InCharacter) {
        bGyroSupported = InCharacter->HasGyroscopeSupport();
        if (!bGyroSupported) bGyroEnabled = false;
    }
    RefreshGyroVisualState();
}

void UForestSliceMobileHUD::JoystickChanged(FVector2D Value)
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetVirtualMove(Value.GetClampedToMaxSize(1.0f));
}

void UForestSliceMobileHUD::LookPadChanged(FVector2D Value)
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetVirtualLook(Value);
}

void UForestSliceMobileHUD::SprintPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetVirtualSprintHeld(true);
}

void UForestSliceMobileHUD::SprintReleased()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetVirtualSprintHeld(false);
}

void UForestSliceMobileHUD::AttackPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualLightAttack();
}

void UForestSliceMobileHUD::HeavyAttackPressed()
{
    // Bind this to the character's combat component in the Blueprint HUD.
    if (ControlledCharacter.IsValid()) {
        if (UForestSliceCombatComponent* Combat = ControlledCharacter->FindComponentByClass<UForestSliceCombatComponent>()) {
            Combat->RequestHeavyAttack();
        }
    }
}

void UForestSliceMobileHUD::JumpPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualJump();
}

void UForestSliceMobileHUD::DodgePressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualDodge();
}

void UForestSliceMobileHUD::SelectShovel()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetActiveGroundTool(EForestSliceTool::Shovel);
}

void UForestSliceMobileHUD::SelectPickaxe()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetActiveGroundTool(EForestSliceTool::Pickaxe);
}

void UForestSliceMobileHUD::PlanGroundPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualPlanGround();
}

void UForestSliceMobileHUD::CreateFarmContourPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualCreateFarmContour();
}

void UForestSliceMobileHUD::DigPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualDig();
}

void UForestSliceMobileHUD::PlantSeedPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualPlantSeed();
}

void UForestSliceMobileHUD::GatherPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualCollect();
}

void UForestSliceMobileHUD::WeaponSwitchPressed(int32 SlotIndex)
{
    if (ControlledCharacter.IsValid()) {
        if (UForestSliceWeaponComponent* Weapons = ControlledCharacter->FindComponentByClass<UForestSliceWeaponComponent>()) {
            Weapons->RequestSwitchToSlot(SlotIndex);
        }
    }
}

void UForestSliceMobileHUD::SetGyroToggle(bool bEnabled)
{
    bGyroEnabled = bGyroSupported && bEnabled;
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetGyroEnabled(bGyroEnabled);
    RefreshGyroVisualState();
}

void UForestSliceMobileHUD::SetGyroSensorSupport(bool bSupported)
{
    bGyroSupported = bSupported;
    if (!bGyroSupported) bGyroEnabled = false;
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetDeviceGyroscopeSupport(bSupported);
    RefreshGyroVisualState();
}

void UForestSliceMobileHUD::PushGyroSample(float RotationX, float RotationY, float Sensitivity)
{
    if (!bGyroSupported || !bGyroEnabled || !ControlledCharacter.IsValid()) return;
    ControlledCharacter->ApplyGyroInput(RotationX, RotationY, Sensitivity);
}

void UForestSliceMobileHUD::SetFocusedMob(UForestSliceMobPresentationComponent* InMob)
{
    FocusedMob = InMob;
}

void UForestSliceMobileHUD::ClearFocusedMob()
{
    FocusedMob.Reset();
}

float UForestSliceMobileHUD::GetFocusedMobHealthRatio() const
{
    return FocusedMob.IsValid() ? FocusedMob->GetHealthRatio() : 0.0f;
}

FName UForestSliceMobileHUD::GetFocusedMobDisplayName() const
{
    return FocusedMob.IsValid() ? FocusedMob->GetDisplayName() : NAME_None;
}

int32 UForestSliceMobileHUD::GetFocusedMobLevel() const
{
    return FocusedMob.IsValid() ? FocusedMob->GetLevel() : 0;
}

bool UForestSliceMobileHUD::IsFocusedMobBoss() const
{
    return FocusedMob.IsValid() && FocusedMob->IsBoss();
}

bool UForestSliceMobileHUD::IsFocusedMobBaseAffiliated() const
{
    return FocusedMob.IsValid() && FocusedMob->IsBaseAffiliated();
}

FName UForestSliceMobileHUD::GetFocusedMobBaseId() const
{
    return FocusedMob.IsValid() ? FocusedMob->GetBaseId() : NAME_None;
}
