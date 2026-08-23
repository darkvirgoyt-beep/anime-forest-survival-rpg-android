#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceInventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FForestSliceInventoryStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StackLimit = 10;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FForestSliceInventoryChanged);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceInventoryComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool AddItem(FName ItemId, int32 Quantity = 1, int32 StackLimit = 10);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(FName ItemId, int32 Quantity = 1);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetQuantity(FName ItemId) const;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetUsedSlotCount() const { return Stacks.Num(); }

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetMaxSlots() const { return MaxSlots; }

    UFUNCTION(BlueprintPure, Category = "Inventory")
    const TArray<FForestSliceInventoryStack>& GetStacks() const { return Stacks; }

    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FForestSliceInventoryChanged InventoryChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_Stacks, EditAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FForestSliceInventoryStack> Stacks;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "64"))
    int32 MaxSlots = 20;

    UFUNCTION()
    void OnRep_Stacks();

private:
    bool HasCapacityFor(FName ItemId, int32 Quantity, int32 StackLimit) const;
    void BroadcastChanged();
};
