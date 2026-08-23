#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ForestSliceMobileHUD.generated.h"

class AForestSliceCharacter;
enum class EForestSliceTool : uint8;

UCLASS(Abstract, BlueprintType)
class FORESTSLICE_API UForestSliceMobileHUD : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Mobile|Binding")
    void SetControlledCharacter(AForestSliceCharacter* InCharacter);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void JoystickChanged(FVector2D Value);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void LookPadChanged(FVector2D Value);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void SprintPressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void SprintReleased();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void AttackPressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void HeavyAttackPressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void JumpPressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void DodgePressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    void SelectShovel();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    void SelectPickaxe();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    void PlanGroundPressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    void CreateFarmContourPressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    void DigPressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    void PlantSeedPressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Gathering")
    void GatherPressed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void WeaponSwitchPressed(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Aim")
    void SetGyroToggle(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Aim")
    void SetGyroSensorSupport(bool bSupported);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Aim")
    void PushGyroSample(float RotationX, float RotationY, float Sensitivity);

    UFUNCTION(BlueprintPure, Category = "Mobile|Aim")
    bool IsGyroSupported() const { return bGyroSupported; }

    UFUNCTION(BlueprintPure, Category = "Mobile|Aim")
    bool IsGyroEnabled() const { return bGyroEnabled; }

    UFUNCTION(BlueprintImplementableEvent, Category = "Mobile|Aim")
    void RefreshGyroVisualState();

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Mobile|Binding")
    TWeakObjectPtr<AForestSliceCharacter> ControlledCharacter;

    UPROPERTY(BlueprintReadOnly, Category = "Mobile|Aim")
    bool bGyroSupported = false;

    UPROPERTY(BlueprintReadOnly, Category = "Mobile|Aim")
    bool bGyroEnabled = false;
};
