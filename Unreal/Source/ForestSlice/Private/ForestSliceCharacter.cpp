#include "ForestSliceCharacter.h"
#include "ForestSliceCharacterProfileComponent.h"
#include "ForestSliceCombatComponent.h"
#include "ForestSlicePresentationComponent.h"
#include "ForestSliceProgressionComponent.h"
#include "ForestSliceHealthComponent.h"
#include "ForestSliceGroundPlanningComponent.h"
#include "ForestSliceInteractionComponent.h"
#include "ForestSliceInventoryComponent.h"
#include "ForestSliceQuickSlotComponent.h"
#include "ForestSliceResourceNodeComponent.h"
#include "ForestSliceSurvivalComponent.h"
#include "ForestSliceWeaponComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "PhysicsEngine/PhysicsVolume.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"

AForestSliceCharacter::AForestSliceCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    CombatComponent = CreateDefaultSubobject<UForestSliceCombatComponent>(TEXT("CombatComponent"));
    WeaponComponent = CreateDefaultSubobject<UForestSliceWeaponComponent>(TEXT("WeaponComponent"));
    SurvivalComponent = CreateDefaultSubobject<UForestSliceSurvivalComponent>(TEXT("SurvivalComponent"));
    GroundPlanningComponent = CreateDefaultSubobject<UForestSliceGroundPlanningComponent>(TEXT("GroundPlanningComponent"));
    InteractionComponent = CreateDefaultSubobject<UForestSliceInteractionComponent>(TEXT("InteractionComponent"));
    QuickSlotComponent = CreateDefaultSubobject<UForestSliceQuickSlotComponent>(TEXT("QuickSlotComponent"));
    InventoryComponent = CreateDefaultSubobject<UForestSliceInventoryComponent>(TEXT("InventoryComponent"));
    HealthComponent = CreateDefaultSubobject<UForestSliceHealthComponent>(TEXT("HealthComponent"));
    ProgressionComponent = CreateDefaultSubobject<UForestSliceProgressionComponent>(TEXT("ProgressionComponent"));
    CharacterProfileComponent = CreateDefaultSubobject<UForestSliceCharacterProfileComponent>(TEXT("CharacterProfileComponent"));
    PresentationComponent = CreateDefaultSubobject<UForestSlicePresentationComponent>(TEXT("PresentationComponent"));
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 620.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->MaxAcceleration = 2200.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1800.0f;
    GetCharacterMovement()->JumpZVelocity = 620.0f;
    GetCharacterMovement()->AirControl = 0.55f;
    GetCharacterMovement()->MaxStepHeight = 45.0f;
    GetCharacterMovement()->WalkableFloorAngle = 46.0f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 360.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 75.0f);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->ProbeSize = 14.0f;
    CameraBoom->ProbeChannel = ECC_Camera;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 14.0f;
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraRotationLagSpeed = 16.0f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

int32 AForestSliceCharacter::AwardGrindingXP(int32 Amount)
{
    return ProgressionComponent ? ProgressionComponent->AwardGrindingXP(Amount) : 0;
}

int32 AForestSliceCharacter::GetCharacterLevel() const
{
    return ProgressionComponent ? ProgressionComponent->GetState().Level : 0;
}

int32 AForestSliceCharacter::GetLiveExperience() const
{
    return ProgressionComponent ? ProgressionComponent->GetState().Experience : 0;
}

void AForestSliceCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (const APlayerController* PlayerController = Cast<APlayerController>(GetController())) {
        if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer()) {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
                if (DefaultMappingContext) Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }
}

void AForestSliceCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Dt = FMath::Clamp(DeltaSeconds, 0.0f, 0.10f);
    SlideCooldown = FMath::Max(0.0f, SlideCooldown - Dt);
    DodgeCooldown = FMath::Max(0.0f, DodgeCooldown - Dt);

    const APhysicsVolume* PhysicsVolume = GetWorld() ? GetWorld()->GetPhysicsVolume(GetActorLocation()) : nullptr;
    const bool WasInWater = bInWater;
    bInWater = PhysicsVolume && PhysicsVolume->bWaterVolume;
    if (bInWater != WasInWater) {
        if (bInWater) {
            GetCharacterMovement()->SetMovementMode(MOVE_Swimming);
            GetCharacterMovement()->MaxWalkSpeed = SwimSpeed;
        } else {
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            GetCharacterMovement()->MaxWalkSpeed = bSprintHeld ? SprintSpeed : WalkSpeed;
        }
    }
    if (bInWater) {
        GetCharacterMovement()->MaxWalkSpeed = bSprintHeld ? SwimSpeed * 1.10f : SwimSpeed;
        GetCharacterMovement()->BrakingDecelerationSwimming = WaterDrag * 100.0f;
    }

    if (bSprintHeld && !bInWater && GetVelocity().SizeSquared2D() > 100.0f && SurvivalComponent) {
        if (!SurvivalComponent->ConsumeStamina(18.0f * Dt)) {
            StopSprint(FInputActionValue());
        }
    }

    const FVector Velocity = GetVelocity();
    const FVector Wind(0.0f, 22.0f * FMath::Sin(GetWorld() ? GetWorld()->GetTimeSeconds() * 0.7f : 0.0f), 0.0f);
    const FVector HairTarget = FVector(-Velocity.X * 0.012f, -Velocity.Y * 0.010f, FMath::Abs(Velocity.Size2D()) * 0.003f) + Wind * 0.0015f;
    const FVector ClothTarget = FVector(-Velocity.X * 0.018f, -Velocity.Y * 0.014f, -FMath::Abs(Velocity.Size2D()) * 0.004f);
    const float HairFrequency = bInWater ? 5.0f : 8.0f;
    const float ClothFrequency = bInWater ? 3.2f : 5.0f;
    HairMotionVelocity += (HairTarget - HairMotionOffset) * HairFrequency * HairFrequency * Dt - HairMotionVelocity * 2.0f * HairFrequency * Dt;
    HairMotionOffset += HairMotionVelocity * Dt;
    ClothMotionVelocity += (ClothTarget - ClothMotionOffset) * ClothFrequency * ClothFrequency * Dt - ClothMotionVelocity * 2.0f * ClothFrequency * Dt;
    ClothMotionOffset += ClothMotionVelocity * Dt;
    WetnessAlpha = FMath::FInterpTo(WetnessAlpha, bInWater ? 1.0f : 0.0f, Dt, bInWater ? 3.5f : 0.45f);
    if (PresentationComponent) {
        const float SpeedNormalized = FMath::Clamp(GetVelocity().Size2D() / FMath::Max(SprintSpeed, 1.0f), 0.0f, 1.0f);
        PresentationComponent->UpdateLocomotionState(
            SpeedNormalized,
            bSprintHeld,
            GetCharacterMovement()->IsFalling()
        );
    }
}

void AForestSliceCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
        if (MoveAction) EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AForestSliceCharacter::Move);
        if (LookAction) EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AForestSliceCharacter::Look);
        if (SprintAction) {
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AForestSliceCharacter::StartSprint);
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AForestSliceCharacter::StopSprint);
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AForestSliceCharacter::StopSprint);
        }
        if (SlideAction) EnhancedInput->BindAction(SlideAction, ETriggerEvent::Started, this, &AForestSliceCharacter::StartSlide);
        if (DodgeAction) EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Started, this, &AForestSliceCharacter::StartDodge);
        if (LightAttackAction) EnhancedInput->BindAction(LightAttackAction, ETriggerEvent::Started, this, &AForestSliceCharacter::StartLightAttack);
        if (JumpAction) EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AForestSliceCharacter::StartJump);
    }
}

void AForestSliceCharacter::Move(const FInputActionValue& Value)
{
    ApplyMoveVector(Value.Get<FVector2D>());
}

void AForestSliceCharacter::ApplyMoveVector(FVector2D MoveVector)
{
    MoveVector = MoveVector.GetClampedToMaxSize(1.0f);
    if (!Controller || MoveVector.IsNearlyZero()) return;

    const FRotator ControlRotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MoveVector.Y);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MoveVector.X);
}

void AForestSliceCharacter::Look(const FInputActionValue& Value)
{
    ApplyLookVector(Value.Get<FVector2D>());
}

void AForestSliceCharacter::ApplyLookVector(FVector2D LookVector)
{
    if (!Controller) return;

    const float OrbitLimit = FMath::Clamp(MaxCameraOrbitDegrees, 180.0f, 540.0f);
    const float RequestedYaw = LookVector.X;
    const float AllowedYaw = FMath::Clamp(RequestedYaw, -OrbitLimit - CameraOrbitDegrees, OrbitLimit - CameraOrbitDegrees);
    CameraOrbitDegrees = FMath::Clamp(CameraOrbitDegrees + AllowedYaw, -OrbitLimit, OrbitLimit);
    AddControllerYawInput(AllowedYaw);

    const float CurrentPitch = FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch);
    const float TargetPitch = FMath::Clamp(CurrentPitch + LookVector.Y, MinCameraPitchDegrees, MaxCameraPitchDegrees);
    AddControllerPitchInput(TargetPitch - CurrentPitch);
}

void AForestSliceCharacter::SetVirtualMove(FVector2D MoveVector)
{
    ApplyMoveVector(MoveVector);
}

void AForestSliceCharacter::SetVirtualLook(FVector2D LookVector)
{
    ApplyLookVector(LookVector);
}

void AForestSliceCharacter::SetVirtualSprintHeld(bool bHeld)
{
    if (bHeld) {
        StartSprint(FInputActionValue());
    } else {
        StopSprint(FInputActionValue());
    }
}

void AForestSliceCharacter::TriggerVirtualSlide()
{
    StartSlide(FInputActionValue());
}

void AForestSliceCharacter::TriggerVirtualDodge()
{
    StartDodge(FInputActionValue());
}

void AForestSliceCharacter::TriggerVirtualLightAttack()
{
    StartLightAttack(FInputActionValue());
}

void AForestSliceCharacter::TriggerVirtualJump()
{
    StartJump(FInputActionValue());
}

void AForestSliceCharacter::SetActiveGroundTool(EForestSliceTool Tool)
{
    ActiveGroundTool = Tool;
}

bool AForestSliceCharacter::TriggerVirtualDig()
{
    if (!GroundPlanningComponent || ActiveGroundTool == EForestSliceTool::None) return false;

    const FVector DigLocation = GetActorLocation() + GetActorForwardVector() * 125.0f;
    return GroundPlanningComponent->DigAtLocation(DigLocation, ActiveGroundTool);
}

bool AForestSliceCharacter::TriggerVirtualPlanGround()
{
    if (!GroundPlanningComponent) return false;

    const FVector PlanLocation = GetActorLocation() + GetActorForwardVector() * 125.0f;
    return GroundPlanningComponent->PlanGround(PlanLocation, FVector2D(360.0f, 360.0f));
}

bool AForestSliceCharacter::TriggerVirtualCreateFarmContour()
{
    if (!GroundPlanningComponent) return false;

    const FVector ContourLocation = GetActorLocation() + GetActorForwardVector() * 125.0f;
    return GroundPlanningComponent->CreateFarmContour(ContourLocation, FVector2D(300.0f, 300.0f), ContourLocation.Z);
}

bool AForestSliceCharacter::TriggerVirtualPlantSeed()
{
    return GroundPlanningComponent && GroundPlanningComponent->PlantSeed();
}

bool AForestSliceCharacter::TriggerVirtualCollect()
{
    if (!HasAuthority()) {
        ServerTriggerVirtualCollect();
        return true;
    }
    return TryCollectFromView();
}

void AForestSliceCharacter::ServerTriggerVirtualCollect_Implementation()
{
    TryCollectFromView();
}

bool AForestSliceCharacter::TryCollectFromView()
{
    if (!GetWorld()) return false;

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
    const FVector End = Start + GetActorForwardVector() * 260.0f;
    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ForestSliceCollect), false, this);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams)) return false;

    if (UForestSliceResourceNodeComponent* ResourceNode = Hit.GetActor()->FindComponentByClass<UForestSliceResourceNodeComponent>()) {
        return ResourceNode->TryCollect(this);
    }
    return false;
}

void AForestSliceCharacter::StartSprint(const FInputActionValue& Value)
{
    bSprintHeld = !bInWater && SurvivalComponent && SurvivalComponent->GetState().Stamina > 0.0f;
    GetCharacterMovement()->MaxWalkSpeed = bInWater ? SwimSpeed : (bSprintHeld ? SprintSpeed : WalkSpeed);
}

void AForestSliceCharacter::StopSprint(const FInputActionValue& Value)
{
    const bool ShouldSlide = bSprintHeld && GetCharacterMovement()->IsMovingOnGround();
    bSprintHeld = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    if (ShouldSlide) StartSlide(Value);
}

void AForestSliceCharacter::StartSlide(const FInputActionValue& Value)
{
    if (bInWater || !GetCharacterMovement()->IsMovingOnGround() || SlideCooldown > 0.0f || !SurvivalComponent || !SurvivalComponent->ConsumeStamina(18.0f)) return;
    SlideCooldown = 0.65f;
    const FVector SlideDirection = GetLastMovementInputVector().IsNearlyZero() ? GetActorForwardVector() : GetLastMovementInputVector().GetSafeNormal2D();
    LaunchCharacter(SlideDirection * SlideImpulse + FVector(0.0f, 0.0f, 35.0f), true, false);
    if (PresentationComponent) PresentationComponent->PlayMovementAnimation(EForestSliceMovementAnimation::Slide);
}

void AForestSliceCharacter::StartDodge(const FInputActionValue& Value)
{
    if (DodgeCooldown > 0.0f || !SurvivalComponent || !SurvivalComponent->ConsumeStamina(25.0f)) return;
    DodgeCooldown = 0.85f;
    const FVector DodgeDirection = GetLastMovementInputVector().IsNearlyZero() ? GetActorForwardVector() : GetLastMovementInputVector().GetSafeNormal2D();
    LaunchCharacter(DodgeDirection * (bInWater ? DodgeImpulse * 0.60f : DodgeImpulse), true, false);
    if (PresentationComponent) PresentationComponent->PlayMovementAnimation(EForestSliceMovementAnimation::Dodge);
    // Gameplay Ability System will own the authoritative invulnerability tag in the next slice.
}

void AForestSliceCharacter::StartLightAttack(const FInputActionValue& Value)
{
    if (CombatComponent) CombatComponent->RequestLightAttack();
}

void AForestSliceCharacter::StartJump(const FInputActionValue& Value)
{
    Jump();
    if (PresentationComponent) PresentationComponent->PlayMovementAnimation(EForestSliceMovementAnimation::Jump);
}

bool AForestSliceCharacter::DetectGyroscopeSupport() const
{
#if PLATFORM_ANDROID
    // The Android platform bridge should set this from Sensor.TYPE_GYROSCOPE at startup.
    return bDeviceHasGyroscope;
#else
    return false;
#endif
}

void AForestSliceCharacter::SetDeviceGyroscopeSupport(bool bSupported)
{
    bDeviceHasGyroscope = bSupported;
    if (!bDeviceHasGyroscope) bGyroEnabled = false;
}

void AForestSliceCharacter::SetGyroEnabled(bool bEnabled)
{
    bGyroEnabled = DetectGyroscopeSupport() && bEnabled;
}

void AForestSliceCharacter::ApplyGyroInput(float RotationX, float RotationY, float Sensitivity)
{
    if (!bGyroEnabled || !bDeviceHasGyroscope) return;
    const float Scale = FMath::Clamp(Sensitivity, 0.05f, 4.0f);
    ApplyLookVector(FVector2D(RotationY * Scale, -RotationX * Scale));
}

float AForestSliceCharacter::GetStaminaNormalized() const
{
    return SurvivalComponent ? SurvivalComponent->GetStaminaNormalized() : 0.0f;
}
