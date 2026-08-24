#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceHealthComponent.generated.h"

USTRUCT(BlueprintType)
struct FForestSliceHealthState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Health = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Poise = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxPoise = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDowned = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDead = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FForestSliceDamageEvent, FName, DamageType, float, Damage, float, RemainingHealth, bool, bKilled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForestSliceHealthChanged, float, Health, float, MaxHealth);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceHealthComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Health")
    bool ApplyDamage(float Damage, float PoiseDamage, FVector Impulse, FName DamageType = NAME_None);

    UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Health")
    void RestoreFullHealth();

    UFUNCTION(BlueprintPure, Category = "Health")
    const FForestSliceHealthState& GetState() const { return State; }

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsAlive() const { return !State.bDead; }

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsDamageImmune() const { return DamageImmunityRemaining > 0.0f; }

    UPROPERTY(BlueprintAssignable, Category = "Health")
    FForestSliceDamageEvent DamageTaken;

    UPROPERTY(BlueprintAssignable, Category = "Health")
    FForestSliceHealthChanged HealthChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_State, EditAnywhere, BlueprintReadOnly, Category = "Health")
    FForestSliceHealthState State;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.0"))
    float DownedHealthThreshold = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Regeneration", meta = (ClampMin = "0.0", ForceUnits = "HP/s"))
    float HealthRegenPerSecond = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Regeneration", meta = (ClampMin = "0.0", ForceUnits = "s"))
    float HealthRegenDelaySeconds = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Regeneration", meta = (ClampMin = "0.0", ForceUnits = "poise/s"))
    float PoiseRegenPerSecond = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Damage", meta = (ClampMin = "0.0", ForceUnits = "s"))
    float DamageImmunitySeconds = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health|Damage", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
    float MaxKnockbackImpulse = 700.0f;

    UFUNCTION()
    void OnRep_State();

private:
    float HealthRegenDelayRemaining = 0.0f;
    float DamageImmunityRemaining = 0.0f;

    void BroadcastDamage(FName DamageType, float Damage, bool bKilled);
    void BroadcastHealthChanged();
    void NormalizeState();
};
