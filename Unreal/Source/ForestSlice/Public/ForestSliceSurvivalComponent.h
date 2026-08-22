#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceSurvivalComponent.generated.h"

USTRUCT(BlueprintType)
struct FForestSliceSurvivalState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Health = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Hunger = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Thirst = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Stamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Temperature = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Injury = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSheltered = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceSurvivalChanged, const FForestSliceSurvivalState&, State);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceSurvivalComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceSurvivalComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Survival")
    bool ConsumeStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Survival")
    void RestoreStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Survival")
    void SetSheltered(bool bInSheltered);

    UFUNCTION(BlueprintCallable, Category = "Survival")
    void ApplyDamage(float Amount, float InjuryAmount);

    UFUNCTION(BlueprintCallable, Category = "Survival")
    void RestoreFromSleep(float HealthAmount = 35.0f, float HungerAmount = 20.0f, float ThirstAmount = 25.0f);

    UFUNCTION(BlueprintPure, Category = "Survival")
    const FForestSliceSurvivalState& GetState() const { return State; }

    UPROPERTY(BlueprintAssignable, Category = "Survival")
    FForestSliceSurvivalChanged SurvivalChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_State, VisibleInstanceOnly, BlueprintReadOnly, Category = "Survival")
    FForestSliceSurvivalState State;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival")
    float HungerDrainPerSecond = 0.015f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival")
    float ThirstDrainPerSecond = 0.025f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival")
    float StaminaRecoveryPerSecond = 16.0f;

    UFUNCTION()
    void OnRep_State();

private:
    void BroadcastState();
};
