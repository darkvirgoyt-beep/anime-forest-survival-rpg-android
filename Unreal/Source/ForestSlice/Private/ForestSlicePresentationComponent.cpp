#include "ForestSlicePresentationComponent.h"

#include "ForestSliceCharacter.h"
#include "ForestSliceCombatComponent.h"
#include "ForestSliceWeaponComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

UForestSlicePresentationComponent::UForestSlicePresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UForestSlicePresentationComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AForestSliceCharacter* Character = Cast<AForestSliceCharacter>(GetOwner())) {
        if (UForestSliceCombatComponent* Combat = Character->FindComponentByClass<UForestSliceCombatComponent>()) {
            Combat->CombatEvent.AddDynamic(this, &UForestSlicePresentationComponent::HandleCombatEvent);
        }
        if (UForestSliceWeaponComponent* Weapon = Character->FindComponentByClass<UForestSliceWeaponComponent>()) {
            Weapon->WeaponChanged.AddDynamic(this, &UForestSlicePresentationComponent::HandleWeaponChanged);
        }
    }
}

void UForestSlicePresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AForestSliceCharacter* Character = Cast<AForestSliceCharacter>(GetOwner())) {
        if (UForestSliceCombatComponent* Combat = Character->FindComponentByClass<UForestSliceCombatComponent>()) {
            Combat->CombatEvent.RemoveDynamic(this, &UForestSlicePresentationComponent::HandleCombatEvent);
        }
        if (UForestSliceWeaponComponent* Weapon = Character->FindComponentByClass<UForestSliceWeaponComponent>()) {
            Weapon->WeaponChanged.RemoveDynamic(this, &UForestSlicePresentationComponent::HandleWeaponChanged);
        }
    }

    StopSwordTrail();
    Super::EndPlay(EndPlayReason);
}

void UForestSlicePresentationComponent::SetPresentationEnabled(bool bEnabled)
{
    bPresentationEnabled = bEnabled;
    if (!bPresentationEnabled) {
        StopSwordTrail();
    }
}

void UForestSlicePresentationComponent::UpdateLocomotionState(float SpeedNormalized, bool bInSprinting, bool bInFalling)
{
    UpdateAnimationIntent(SpeedNormalized, bInSprinting, bInFalling, false, false, 0, false, false);
}

void UForestSlicePresentationComponent::UpdateAnimationIntent(float SpeedNormalized, bool bInSprinting, bool bInFalling, bool bInSwimming, bool bInHitstun, int32 InComboIndex, bool bInAttackActive, bool bInHeavyAttack)
{
    LocomotionSpeedNormalized = FMath::Clamp(SpeedNormalized, 0.0f, 1.0f);
    bSprinting = bInSprinting;
    bFalling = bInFalling;
    AnimationIntent = FForestSliceAnimationIntent{};
    AnimationIntent.LocomotionBlend = LocomotionSpeedNormalized;
    AnimationIntent.PlayRate = 1.0f + LocomotionSpeedNormalized * 0.45f;
    AnimationIntent.BodyLean = LocomotionSpeedNormalized * (bInSprinting ? 0.18f : 0.08f);
    AnimationIntent.VerticalOffset = bInFalling ? -0.015f : 0.0f;
    AnimationIntent.bAdditiveSecondaryMotion = true;

    if (bInHitstun) {
        AnimationIntent.Motion = EForestSliceAnimationMotion::Hitstun;
        AnimationIntent.PlayRate = 1.15f;
        AnimationIntent.BodyLean = -0.12f;
    } else if (bInSwimming) {
        AnimationIntent.Motion = EForestSliceAnimationMotion::Swim;
        AnimationIntent.PlayRate = 0.78f + LocomotionSpeedNormalized * 0.20f;
        AnimationIntent.BodyLean = 0.04f;
    } else if (bInFalling) {
        AnimationIntent.Motion = EForestSliceAnimationMotion::Fall;
    } else if (bInSprinting) {
        AnimationIntent.Motion = EForestSliceAnimationMotion::Sprint;
    } else if (LocomotionSpeedNormalized > 0.08f) {
        AnimationIntent.Motion = EForestSliceAnimationMotion::Walk;
    } else {
        AnimationIntent.Motion = EForestSliceAnimationMotion::Idle;
    }

    if (bInAttackActive) {
        if (bInHeavyAttack) {
            AnimationIntent.UpperBody = EForestSliceAnimationUpperBody::HeavyAttack;
        } else {
            const int32 Combo = FMath::Clamp(InComboIndex, 0, 2);
            AnimationIntent.UpperBody = static_cast<EForestSliceAnimationUpperBody>(static_cast<uint8>(EForestSliceAnimationUpperBody::LightAttack1) + Combo);
        }
    }
}

void UForestSlicePresentationComponent::PlayMovementAnimation(EForestSliceMovementAnimation Animation)
{
    if (!bPresentationEnabled) return;

    switch (Animation) {
    case EForestSliceMovementAnimation::Dodge:
        AnimationIntent.Motion = EForestSliceAnimationMotion::Dodge;
        AnimationIntent.PlayRate = 1.35f;
        AnimationIntent.BodyLean = 0.22f;
        AnimationIntent.bUseRootMotion = true;
        PlayMontage(ResolveMontage(CueSet.DodgeMontage));
        PlaySound(CueSet.DodgeWhoosh);
        break;
    case EForestSliceMovementAnimation::Slide:
        AnimationIntent.Motion = EForestSliceAnimationMotion::Slide;
        AnimationIntent.PlayRate = 1.20f;
        AnimationIntent.BodyLean = 0.28f;
        AnimationIntent.VerticalOffset = -0.22f;
        AnimationIntent.bUseRootMotion = true;
        PlayMontage(ResolveMontage(CueSet.SlideMontage));
        PlaySound(CueSet.DodgeWhoosh);
        break;
    case EForestSliceMovementAnimation::Jump:
        AnimationIntent.Motion = EForestSliceAnimationMotion::Jump;
        AnimationIntent.VerticalOffset = 0.035f;
        PlayMontage(ResolveMontage(CueSet.JumpMontage));
        break;
    default:
        break;
    }
}

void UForestSlicePresentationComponent::HandleCombatEvent(FName EventId, int32 InComboIndex, float Damage, int32 WeaponIndex)
{
    if (!bPresentationEnabled) return;

    if (EventId == TEXT("HitConfirmed")) {
        SpawnHitBurst();
        PlaySound(CueSet.SwordHit);
        return;
    }

    if (EventId == TEXT("AttackFinished")) {
        StopSwordTrail();
        return;
    }

    if (EventId == TEXT("WeaponSwitched") || EventId == TEXT("WeaponChanged")) {
        StopSwordTrail();
        if (UNiagaraSystem* Burst = ResolveNiagara(CueSet.WeaponSwitchBurst)) {
            if (GetWorld()) {
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Burst, GetOwner()->GetActorLocation());
            }
        }
        PlaySound(CueSet.MagicPulse);
        return;
    }

    const bool bHeavy = EventId.ToString().StartsWith(TEXT("Heavy_"));
    const bool bLight = EventId.ToString().StartsWith(TEXT("Light_"));
    if (bHeavy || bLight) {
        AnimationIntent.UpperBody = bHeavy ? EForestSliceAnimationUpperBody::HeavyAttack : static_cast<EForestSliceAnimationUpperBody>(static_cast<uint8>(EForestSliceAnimationUpperBody::LightAttack1) + FMath::Clamp(InComboIndex, 0, 2));
        PlayAttackMontage(EventId, bHeavy);
        SpawnSwordTrail();
        PlaySound(CueSet.SwordWhoosh);
    }
}

void UForestSlicePresentationComponent::HandleWeaponChanged(FName PreviousWeapon, FName NewWeapon)
{
    if (!bPresentationEnabled || PreviousWeapon == NewWeapon) return;
    StopSwordTrail();
    if (UNiagaraSystem* Burst = ResolveNiagara(CueSet.WeaponSwitchBurst)) {
        if (GetWorld()) {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Burst, GetOwner()->GetActorLocation());
        }
    }
    PlaySound(CueSet.MagicPulse);
}

void UForestSlicePresentationComponent::PlayAttackMontage(FName EventId, bool bHeavy)
{
    UAnimMontage* Montage = ResolveMontage(bHeavy ? CueSet.HeavyAttackMontage : CueSet.LightAttackMontage);
    PlayMontage(Montage, bHeavy ? TEXT("Heavy_01") : EventId);
}

void UForestSlicePresentationComponent::PlayMontage(UAnimMontage* Montage, FName SectionName)
{
    if (!Montage) return;
    if (AForestSliceCharacter* Character = Cast<AForestSliceCharacter>(GetOwner())) {
        if (USkeletalMeshComponent* Mesh = Character->GetMesh()) {
            if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance()) {
                AnimInstance->Montage_Play(Montage, 1.0f);
                if (SectionName != NAME_None && Montage->IsValidSectionName(SectionName)) {
                    AnimInstance->Montage_JumpToSection(SectionName, Montage);
                }
            }
        }
    }
}

void UForestSlicePresentationComponent::SpawnSwordTrail()
{
    StopSwordTrail();
    UNiagaraSystem* Trail = ResolveNiagara(CueSet.SwordTrail);
    AForestSliceCharacter* Character = Cast<AForestSliceCharacter>(GetOwner());
    if (!Trail || !Character || !Character->GetMesh()) return;

    ActiveSwordTrail = UNiagaraFunctionLibrary::SpawnSystemAttached(
        Trail,
        Character->GetMesh(),
        CueSet.WeaponFxSocket,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::SnapToTarget,
        false,
        true,
        ENCPoolMethod::AutoRelease,
        true
    );

    if (GetWorld()) {
        GetWorld()->GetTimerManager().SetTimer(
            TrailStopTimer,
            this,
            &UForestSlicePresentationComponent::StopSwordTrail,
            CueSet.TrailLifetime,
            false
        );
    }
}

void UForestSlicePresentationComponent::StopSwordTrail()
{
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(TrailStopTimer);
    }
    if (ActiveSwordTrail) {
        ActiveSwordTrail->DeactivateImmediate();
        ActiveSwordTrail->DestroyComponent();
        ActiveSwordTrail = nullptr;
    }
}

void UForestSlicePresentationComponent::SpawnHitBurst() const
{
    UNiagaraSystem* Burst = ResolveNiagara(CueSet.SwordHitBurst);
    if (!Burst || !GetWorld() || !GetOwner()) return;

    FVector SpawnLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 130.0f;
    SpawnLocation.Z += 55.0f;
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Burst, SpawnLocation, GetOwner()->GetActorRotation());
}

void UForestSlicePresentationComponent::PlaySound(const TSoftObjectPtr<USoundBase>& Sound) const
{
    if (!GetWorld() || !GetOwner()) return;
    USoundBase* SoundAsset = Sound.IsValid() ? Sound.Get() : Sound.LoadSynchronous();
    if (SoundAsset) {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundAsset, GetOwner()->GetActorLocation(), CueSet.EffectsVolume);
    }
}

UNiagaraSystem* UForestSlicePresentationComponent::ResolveNiagara(const TSoftObjectPtr<UNiagaraSystem>& Asset) const
{
    return Asset.IsValid() ? Asset.Get() : Asset.LoadSynchronous();
}

UAnimMontage* UForestSlicePresentationComponent::ResolveMontage(const TSoftObjectPtr<UAnimMontage>& Asset) const
{
    return Asset.IsValid() ? Asset.Get() : Asset.LoadSynchronous();
}
