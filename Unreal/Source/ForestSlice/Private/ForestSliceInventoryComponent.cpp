#include "ForestSliceInventoryComponent.h"

#include "Net/UnrealNetwork.h"

UForestSliceInventoryComponent::UForestSliceInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UForestSliceInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceInventoryComponent, Stacks);
}

bool UForestSliceInventoryComponent::HasCapacityFor(FName ItemId, int32 Quantity, int32 StackLimit) const
{
    if (ItemId.IsNone() || Quantity <= 0) return false;

    int32 Remaining = Quantity;
    for (const FForestSliceInventoryStack& Stack : Stacks) {
        if (Stack.ItemId == ItemId) {
            Remaining -= FMath::Max(0, FMath::Min(Stack.StackLimit, StackLimit) - Stack.Quantity);
            if (Remaining <= 0) return true;
        }
    }

    const int32 SlotsNeeded = FMath::DivideAndRoundUp(Remaining, FMath::Max(1, StackLimit));
    return Stacks.Num() + SlotsNeeded <= MaxSlots;
}

bool UForestSliceInventoryComponent::AddItem(FName ItemId, int32 Quantity, int32 StackLimit)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || ItemId.IsNone() || Quantity <= 0) return false;
    StackLimit = FMath::Clamp(StackLimit, 1, 9999);
    if (!HasCapacityFor(ItemId, Quantity, StackLimit)) return false;

    int32 Remaining = Quantity;
    for (FForestSliceInventoryStack& Stack : Stacks) {
        if (Stack.ItemId != ItemId || Stack.Quantity >= Stack.StackLimit) continue;
        const int32 Space = FMath::Max(0, Stack.StackLimit - Stack.Quantity);
        const int32 Added = FMath::Min(Space, Remaining);
        Stack.Quantity += Added;
        Remaining -= Added;
        if (Remaining <= 0) break;
    }

    while (Remaining > 0) {
        FForestSliceInventoryStack& NewStack = Stacks.AddDefaulted_GetRef();
        NewStack.ItemId = ItemId;
        NewStack.StackLimit = StackLimit;
        NewStack.Quantity = FMath::Min(StackLimit, Remaining);
        Remaining -= NewStack.Quantity;
    }

    BroadcastChanged();
    return true;
}

bool UForestSliceInventoryComponent::RemoveItem(FName ItemId, int32 Quantity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || ItemId.IsNone() || Quantity <= 0 || GetQuantity(ItemId) < Quantity) return false;

    int32 Remaining = Quantity;
    for (int32 Index = Stacks.Num() - 1; Index >= 0 && Remaining > 0; --Index) {
        FForestSliceInventoryStack& Stack = Stacks[Index];
        if (Stack.ItemId != ItemId) continue;
        const int32 Removed = FMath::Min(Stack.Quantity, Remaining);
        Stack.Quantity -= Removed;
        Remaining -= Removed;
        if (Stack.Quantity <= 0) Stacks.RemoveAt(Index);
    }

    BroadcastChanged();
    return true;
}

int32 UForestSliceInventoryComponent::GetQuantity(FName ItemId) const
{
    int32 Total = 0;
    for (const FForestSliceInventoryStack& Stack : Stacks) {
        if (Stack.ItemId == ItemId) Total += FMath::Max(0, Stack.Quantity);
    }
    return Total;
}

void UForestSliceInventoryComponent::OnRep_Stacks()
{
    BroadcastChanged();
}

void UForestSliceInventoryComponent::BroadcastChanged()
{
    InventoryChanged.Broadcast();
}
