#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceWeaponComponent.generated.h"

USTRUCT(BlueprintType)
struct FForestSliceWeaponDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName WeaponId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName SocketName = TEXT("hand_r_socket");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float LightDamageMultiplier = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float HeavyDamageMultiplier = 1.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bRanged = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bGatheringTool = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForestSliceWeaponChanged, FName, PreviousWeapon, FName, NewWeapon);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceWeaponComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Weapons")
    void RequestSwitchToSlot(int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category = "Weapons")
    FForestSliceWeaponDefinition GetEquippedDefinition() const;

    UFUNCTION(BlueprintPure, Category = "Weapons")
    int32 GetEquippedSlot() const { return EquippedSlot; }

    UFUNCTION(BlueprintPure, Category = "Weapons")
    bool IsSwitching() const { return SwitchCooldown > 0.0f; }

    UPROPERTY(BlueprintAssignable, Category = "Weapons")
    FForestSliceWeaponChanged WeaponChanged;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
    TArray<FForestSliceWeaponDefinition> WeaponSlots;

    UPROPERTY(ReplicatedUsing = OnRep_EquippedSlot, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapons")
    int32 EquippedSlot = 0;

    UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapons")
    FName EquippedWeaponId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
    float SwitchDuration = 0.35f;

    UFUNCTION()
    void OnRep_EquippedSlot(int32 PreviousSlot);

    UFUNCTION(Server, Reliable)
    void ServerSwitchToSlot(int32 SlotIndex);

private:
    float SwitchCooldown = 0.0f;

    void ApplySwitch(int32 SlotIndex);
};
