#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceCombatComponent.generated.h"

UENUM(BlueprintType)
enum class EForestSliceCombatPhase : uint8
{
    None,
    Startup,
    Active,
    Recovery,
    Stagger,
    Downed,
    Dead
};

USTRUCT(BlueprintType)
struct FForestSliceAttackDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName AttackId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float StartupSeconds = 0.10f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float ActiveSeconds = 0.10f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float RecoverySeconds = 0.28f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float ComboBufferSeconds = 0.20f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float StaminaCost = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Range = 180.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Damage = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float PoiseDamage = 8.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Knockback = 120.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName MontageSection = NAME_None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FForestSliceCombatEvent, FName, EventId, int32, ComboIndex, float, Damage, int32, WeaponIndex);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceCombatComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void RequestLightAttack();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void RequestHeavyAttack();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void SwitchWeapon(int32 NewWeaponIndex);

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsAttackActive() const { return CombatPhase == EForestSliceCombatPhase::Active; }

    UFUNCTION(BlueprintPure, Category = "Combat")
    EForestSliceCombatPhase GetCombatPhase() const { return CombatPhase; }

    UFUNCTION(BlueprintPure, Category = "Combat")
    int32 GetEquippedWeaponIndex() const { return EquippedWeaponIndex; }

    UPROPERTY(BlueprintAssignable, Category = "Combat")
    FForestSliceCombatEvent CombatEvent;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TArray<FForestSliceAttackDefinition> LightCombo;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    FForestSliceAttackDefinition HeavyAttack;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    TArray<FName> WeaponIds;

    UPROPERTY(ReplicatedUsing = OnRep_CombatPhase, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
    EForestSliceCombatPhase CombatPhase = EForestSliceCombatPhase::None;

    UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
    int32 ComboIndex = 0;

    UPROPERTY(ReplicatedUsing = OnRep_AttackPresentation, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
    FName CurrentAttackId = NAME_None;

    UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
    int32 EquippedWeaponIndex = 0;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
    bool bHeavyAttack = false;

    UFUNCTION()
    void OnRep_CombatPhase();

    UFUNCTION()
    void OnRep_AttackPresentation();

    UFUNCTION()
    void OnRep_EquippedWeapon();

    UFUNCTION(Server, Reliable)
    void ServerRequestAttack(bool bHeavy);

    UFUNCTION(Server, Reliable)
    void ServerSwitchWeapon(int32 NewWeaponIndex);

    void BeginAttack(bool bHeavy);
    void ResolveActiveHit();
    void FinishAttack();
    const FForestSliceAttackDefinition* GetCurrentAttack() const;

private:
    float PhaseTimer = 0.0f;
    float ComboBufferTimer = 0.0f;
    bool bQueuedAttack = false;
    bool bHitResolved = false;
};
