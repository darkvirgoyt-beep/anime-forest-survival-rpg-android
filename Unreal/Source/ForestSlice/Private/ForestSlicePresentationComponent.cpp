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
    LocomotionSpeedNormalized = FMath::Clamp(SpeedNormalized, 0.0f, 1.0f);
    bSprinting = bInSprinting;
    bFalling = bInFalling;
}

void UForestSlicePresentationComponent::PlayMovementAnimation(EForestSliceMovementAnimation Animation)
{
    if (!bPresentationEnabled) return;

    switch (Animation) {
    case EForestSliceMovementAnimation::Dodge:
        PlayMontage(ResolveMontage(CueSet.DodgeMontage));
        PlaySound(CueSet.DodgeWhoosh);
        break;
    case EForestSliceMovementAnimation::Slide:
        PlayMontage(ResolveMontage(CueSet.SlideMontage));
        PlaySound(CueSet.DodgeWhoosh);
        break;
    case EForestSliceMovementAnimation::Jump:
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
