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

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceHealthComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Health")
    bool ApplyDamage(float Damage, float PoiseDamage, FVector Impulse, FName DamageType = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void RestoreFullHealth();

    UFUNCTION(BlueprintPure, Category = "Health")
    const FForestSliceHealthState& GetState() const { return State; }

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsAlive() const { return !State.bDead; }

    UPROPERTY(BlueprintAssignable, Category = "Health")
    FForestSliceDamageEvent DamageTaken;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_State, EditAnywhere, BlueprintReadOnly, Category = "Health")
    FForestSliceHealthState State;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
    float DownedHealthThreshold = 0.0f;

    UFUNCTION()
    void OnRep_State();

private:
    void BroadcastDamage(FName DamageType, float Damage, bool bKilled);
};
