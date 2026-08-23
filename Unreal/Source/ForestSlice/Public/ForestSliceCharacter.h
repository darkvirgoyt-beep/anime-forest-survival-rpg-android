#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ForestSliceGroundPlanningComponent.h"
#include "ForestSliceCharacter.generated.h"

class UCameraComponent;
class UForestSliceCombatComponent;
class UForestSliceCreatureCompanionComponent;
class UForestSliceCharacterProfileComponent;
class UForestSliceHealthComponent;
class UForestSliceGroundPlanningComponent;
class UForestSliceInteractionComponent;
class UForestSliceInventoryComponent;
class UForestSliceQuickSlotComponent;
class UForestSliceResourceNodeComponent;
class UForestSlicePresentationComponent;
class UForestSliceProgressionComponent;
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

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    void SetActiveGroundTool(EForestSliceTool Tool);

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    bool TriggerVirtualDig();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    bool TriggerVirtualPlanGround();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    bool TriggerVirtualCreateFarmContour();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Ground")
    bool TriggerVirtualPlantSeed();

    UFUNCTION(BlueprintCallable, Category = "Mobile|Gathering")
    bool TriggerVirtualCollect();

    UFUNCTION(Server, Reliable)
    void ServerTriggerVirtualCollect();

    UFUNCTION(BlueprintPure, Category = "Ground")
    UForestSliceGroundPlanningComponent* GetGroundPlanningComponent() const { return GroundPlanningComponent; }

    UFUNCTION(BlueprintPure, Category = "Ground")
    EForestSliceTool GetActiveGroundTool() const { return ActiveGroundTool; }

    UFUNCTION(BlueprintPure, Category = "Survival")
    float GetStaminaNormalized() const;

    UFUNCTION(BlueprintPure, Category = "Systems")
    UForestSliceInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

    UFUNCTION(BlueprintPure, Category = "Systems")
    UForestSliceQuickSlotComponent* GetQuickSlotComponent() const { return QuickSlotComponent; }

    UFUNCTION(BlueprintPure, Category = "Systems")
    UForestSliceInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

    UFUNCTION(BlueprintPure, Category = "Companion")
    UForestSliceCreatureCompanionComponent* GetCreatureCompanionComponent() const { return CreatureCompanionComponent; }

    UFUNCTION(BlueprintPure, Category = "Systems")
    UForestSliceHealthComponent* GetHealthComponent() const { return HealthComponent; }

    UFUNCTION(BlueprintPure, Category = "Systems")
    UForestSliceProgressionComponent* GetProgressionComponent() const { return ProgressionComponent; }

    UFUNCTION(BlueprintCallable, Category = "Progression")
    int32 AwardGrindingXP(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Progression")
    int32 GetCharacterLevel() const;

    UFUNCTION(BlueprintPure, Category = "Progression")
    int32 GetLiveExperience() const;

    UFUNCTION(BlueprintPure, Category = "Systems")
    UForestSliceCharacterProfileComponent* GetCharacterProfileComponent() const { return CharacterProfileComponent; }

    UFUNCTION(BlueprintPure, Category = "Presentation")
    UForestSlicePresentationComponent* GetPresentationComponent() const { return PresentationComponent; }

    UFUNCTION(BlueprintPure, Category = "Survival")
    bool HasGyroscopeSupport() const { return bDeviceHasGyroscope; }

    UFUNCTION(BlueprintPure, Category = "Movement")
    bool IsInWater() const { return bInWater; }

    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetWetnessAlpha() const { return WetnessAlpha; }

    UFUNCTION(BlueprintPure, Category = "Movement")
    FVector GetHairMotionOffset() const { return HairMotionOffset; }

    UFUNCTION(BlueprintPure, Category = "Movement")
    FVector GetClothMotionOffset() const { return ClothMotionOffset; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceCombatComponent> CombatComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceWeaponComponent> WeaponComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceSurvivalComponent> SurvivalComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceGroundPlanningComponent> GroundPlanningComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceInteractionComponent> InteractionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceQuickSlotComponent> QuickSlotComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceInventoryComponent> InventoryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion")
    TObjectPtr<UForestSliceCreatureCompanionComponent> CreatureCompanionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceProgressionComponent> ProgressionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Systems")
    TObjectPtr<UForestSliceCharacterProfileComponent> CharacterProfileComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UForestSlicePresentationComponent> PresentationComponent;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 560.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Fidelity")
    float GroundAcceleration = 2600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Fidelity")
    float GroundBraking = 2100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Fidelity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AirControl = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Fidelity", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float WalkableSlopeDegrees = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SlideImpulse = 850.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float DodgeImpulse = 1050.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Water")
    float SwimSpeed = 260.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Water")
    float WaterSpeedMultiplier = 0.58f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Water")
    float WaterDrag = 2.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Water|Fidelity")
    float WaterAcceleration = 1400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MaxCameraOrbitDegrees = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Fidelity")
    float CameraLagSpeed = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Fidelity")
    float CameraRotationLagSpeed = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MinCameraPitchDegrees = -18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MaxCameraPitchDegrees = 58.0f;

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
    bool TryCollectFromView();
    bool bSprintHeld = false;
    EForestSliceTool ActiveGroundTool = EForestSliceTool::Shovel;
    float CameraOrbitDegrees = 0.0f;
    bool bInWater = false;
    float WetnessAlpha = 0.0f;
    float SlideCooldown = 0.0f;
    float DodgeCooldown = 0.0f;
    FVector HairMotionOffset = FVector::ZeroVector;
    FVector HairMotionVelocity = FVector::ZeroVector;
    FVector ClothMotionOffset = FVector::ZeroVector;
    FVector ClothMotionVelocity = FVector::ZeroVector;

    void ApplyMoveVector(FVector2D MoveVector);
    void ApplyLookVector(FVector2D LookVector);
    bool DetectGyroscopeSupport() const;

    virtual void Tick(float DeltaSeconds) override;
};
