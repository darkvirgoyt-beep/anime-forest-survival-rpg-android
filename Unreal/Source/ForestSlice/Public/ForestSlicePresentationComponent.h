#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSlicePresentationComponent.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EForestSliceMovementAnimation : uint8
{
    Dodge,
    Slide,
    Jump
};

USTRUCT(BlueprintType)
struct FForestSlicePresentationCueSet
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> LightAttackMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> HeavyAttackMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> DodgeMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> SlideMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> JumpMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
    TSoftObjectPtr<UNiagaraSystem> SwordTrail;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
    TSoftObjectPtr<UNiagaraSystem> SwordHitBurst;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
    TSoftObjectPtr<UNiagaraSystem> WeaponSwitchBurst;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> SwordWhoosh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> SwordHit;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> MagicPulse;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> Footstep;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> DodgeWhoosh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    FName WeaponFxSocket = TEXT("hand_r_socket");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.05", ClampMax = "2.0"))
    float TrailLifetime = 0.42f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float EffectsVolume = 1.0f;
};

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSlicePresentationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSlicePresentationComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Presentation")
    void PlayMovementAnimation(EForestSliceMovementAnimation Animation);

    UFUNCTION(BlueprintCallable, Category = "Presentation")
    void SetPresentationEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Presentation")
    bool IsPresentationEnabled() const { return bPresentationEnabled; }

    UFUNCTION(BlueprintPure, Category = "Presentation")
    float GetLocomotionSpeedNormalized() const { return LocomotionSpeedNormalized; }

    UFUNCTION(BlueprintPure, Category = "Presentation")
    bool IsSprinting() const { return bSprinting; }

    UFUNCTION(BlueprintPure, Category = "Presentation")
    bool IsFalling() const { return bFalling; }

    UFUNCTION(BlueprintCallable, Category = "Presentation")
    void UpdateLocomotionState(float SpeedNormalized, bool bInSprinting, bool bInFalling);

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    FForestSlicePresentationCueSet CueSet;

    UFUNCTION()
    void HandleCombatEvent(FName EventId, int32 InComboIndex, float Damage, int32 WeaponIndex);

    UFUNCTION()
    void HandleWeaponChanged(FName PreviousWeapon, FName NewWeapon);

private:
    UPROPERTY(Transient)
    TObjectPtr<class UNiagaraComponent> ActiveSwordTrail;

    FTimerHandle TrailStopTimer;
    bool bPresentationEnabled = true;
    bool bSprinting = false;
    bool bFalling = false;
    float LocomotionSpeedNormalized = 0.0f;

    void PlayAttackMontage(FName EventId, bool bHeavy);
    void PlayMontage(UAnimMontage* Montage, FName SectionName = NAME_None);
    void SpawnSwordTrail();
    void StopSwordTrail();
    void SpawnHitBurst() const;
    void PlaySound(const TSoftObjectPtr<USoundBase>& Sound) const;
    UNiagaraSystem* ResolveNiagara(const TSoftObjectPtr<UNiagaraSystem>& Asset) const;
    UAnimMontage* ResolveMontage(const TSoftObjectPtr<UAnimMontage>& Asset) const;
};
