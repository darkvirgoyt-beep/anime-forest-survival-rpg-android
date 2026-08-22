#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ForestSliceCharacter.generated.h"

class UCameraComponent;
class UForestSliceCombatComponent;
class UForestSliceSurvivalComponent;
class UForestSliceWeaponComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

UCLASS(config = Game)
class FORESTSLICE_API AForestSliceCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AForestSliceCharacter();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Mobile|Aim")
    void SetGyroEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Aim")
    void ApplyGyroInput(float RotationX, float RotationY, float Sensitivity);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Aim")
    void SetDeviceGyroscopeSupport(bool bSupported);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void SetVirtualMove(FVector2D MoveVector);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void SetVirtualLook(FVector2D LookVector);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void SetVirtualSprintHeld(bool bHeld);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void TriggerVirtualSlide();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void TriggerVirtualDodge();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void TriggerVirtualLightAttack();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Input")
    void TriggerVirtualJump();

    UFUNCTION(BlueprintPure, Category = "Survival")
    float GetStaminaNormalized() const;

    UFUNCTION(BlueprintPure, Category = "Survival")
    bool HasGyroscopeSupport() const { return bDeviceHasGyroscope; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceCombatComponent> CombatComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceWeaponComponent> WeaponComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceSurvivalComponent> SurvivalComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> SlideAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> DodgeAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LightAttackAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
    float MaxStamina = 100.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Survival")
    float Stamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 560.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SlideImpulse = 850.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float DodgeImpulse = 1050.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mobile|Aim")
    bool bDeviceHasGyroscope = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mobile|Aim")
    bool bGyroEnabled = false;

    UFUNCTION()
    void Move(const FInputActionValue& Value);

    UFUNCTION()
    void Look(const FInputActionValue& Value);

    UFUNCTION()
    void StartSprint(const FInputActionValue& Value);

    UFUNCTION()
    void StopSprint(const FInputActionValue& Value);

    UFUNCTION()
    void StartSlide(const FInputActionValue& Value);

    UFUNCTION()
    void StartDodge(const FInputActionValue& Value);

    UFUNCTION()
    void StartLightAttack(const FInputActionValue& Value);

    UFUNCTION()
    void StartJump(const FInputActionValue& Value);

private:
    bool bSprintHeld = false;
    float SlideCooldown = 0.0f;
    float DodgeCooldown = 0.0f;

    void ApplyMoveVector(FVector2D MoveVector);
    void ApplyLookVector(FVector2D LookVector);
    bool DetectGyroscopeSupport() const;

    virtual void Tick(float DeltaSeconds) override;
};
