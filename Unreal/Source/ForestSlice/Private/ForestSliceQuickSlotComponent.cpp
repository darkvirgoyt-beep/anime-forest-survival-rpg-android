#include "ForestSliceQuickSlotComponent.h"

#include "Net/UnrealNetwork.h"

UForestSliceQuickSlotComponent::UForestSliceQuickSlotComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    Slots.SetNum(9);
    for (int32 Index = 0; Index < Slots.Num(); ++Index) Slots[Index].SlotIndex = Index;
}

bool UForestSliceQuickSlotComponent::SetSlot(int32 SlotIndex, FName ItemId, int32 Quantity, bool bUsableInCombat)
{
    if (!GetOwner()->HasAuthority() || !IsValidSlotIndex(SlotIndex) || ItemId.IsNone() || Quantity < 0) return false;
    Slots[SlotIndex].SlotIndex = SlotIndex;
    Slots[SlotIndex].ItemId = ItemId;
    Slots[SlotIndex].Quantity = Quantity;
    Slots[SlotIndex].bUsableInCombat = bUsableInCombat;
    OnRep_Slots();
    return true;
}

bool UForestSliceQuickSlotComponent::SelectSlot(int32 SlotIndex)
{
    if (!GetOwner()->HasAuthority() || !IsValidSlotIndex(SlotIndex) || Slots[SlotIndex].ItemId.IsNone()) return false;
    ActiveSlot = SlotIndex;
    BroadcastActiveSlot();
    return true;
}

bool UForestSliceQuickSlotComponent::ConsumeActive(int32 Amount)
{
    if (!GetOwner()->HasAuthority() || Amount <= 0 || !IsValidSlotIndex(ActiveSlot)) return false;
    FForestSliceQuickSlotEntry& Entry = Slots[ActiveSlot];
    if (Entry.ItemId.IsNone() || Entry.Quantity < Amount) return false;
    Entry.Quantity -= Amount;
    if (Entry.Quantity == 0) Entry.ItemId = NAME_None;
    OnRep_Slots();
    return true;
}

void UForestSliceQuickSlotComponent::OnRep_Slots()
{
    BroadcastActiveSlot();
}

void UForestSliceQuickSlotComponent::OnRep_ActiveSlot()
{
    BroadcastActiveSlot();
}

void UForestSliceQuickSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceQuickSlotComponent, Slots);
    DOREPLIFETIME(UForestSliceQuickSlotComponent, ActiveSlot);
}

bool UForestSliceQuickSlotComponent::IsValidSlotIndex(int32 SlotIndex) const
{
    return Slots.IsValidIndex(SlotIndex) && SlotIndex >= 0 && SlotIndex < 9;
}

void UForestSliceQuickSlotComponent::BroadcastActiveSlot()
{
    if (!Slots.IsValidIndex(ActiveSlot)) return;
    QuickSlotChanged.Broadcast(ActiveSlot, Slots[ActiveSlot].ItemId);
}
