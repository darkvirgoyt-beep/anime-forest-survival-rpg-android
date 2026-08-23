#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceQuickSlotComponent.generated.h"

USTRUCT(BlueprintType)
struct FForestSliceQuickSlotEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SlotIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUsableInCombat = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForestSliceQuickSlotChanged, int32, ActiveSlot, FName, ItemId);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceQuickSlotComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceQuickSlotComponent();

    UFUNCTION(BlueprintCallable, Category = "QuickSlot")
    bool SetSlot(int32 SlotIndex, FName ItemId, int32 Quantity, bool bUsableInCombat = true);

    UFUNCTION(BlueprintCallable, Category = "QuickSlot")
    bool SelectSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "QuickSlot")
    bool ConsumeActive(int32 Amount = 1);

    UFUNCTION(BlueprintPure, Category = "QuickSlot")
    const TArray<FForestSliceQuickSlotEntry>& GetSlots() const { return Slots; }

    UFUNCTION(BlueprintPure, Category = "QuickSlot")
    int32 GetActiveSlot() const { return ActiveSlot; }

    UPROPERTY(BlueprintAssignable, Category = "QuickSlot")
    FForestSliceQuickSlotChanged QuickSlotChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_Slots, EditAnywhere, BlueprintReadOnly, Category = "QuickSlot")
    TArray<FForestSliceQuickSlotEntry> Slots;

    UPROPERTY(ReplicatedUsing = OnRep_ActiveSlot, EditAnywhere, BlueprintReadOnly, Category = "QuickSlot")
    int32 ActiveSlot = 0;

    UFUNCTION()
    void OnRep_Slots();

    UFUNCTION()
    void OnRep_ActiveSlot();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    bool IsValidSlotIndex(int32 SlotIndex) const;
    void BroadcastActiveSlot();
};
