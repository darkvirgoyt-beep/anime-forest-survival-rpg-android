#include "ForestSliceWeaponComponent.h"

#include "Net/UnrealNetwork.h"

UForestSliceWeaponComponent::UForestSliceWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    WeaponSlots = {
        {TEXT("Blade"), TEXT("hand_r_socket"), 1.0f, 1.35f, false, false},
        {TEXT("Greatblade"), TEXT("hand_r_socket"), 1.20f, 1.70f, false, false},
        {TEXT("Bow"), TEXT("hand_r_socket"), 0.85f, 1.10f, true, false},
        {TEXT("GatheringTool"), TEXT("hand_r_socket"), 0.50f, 0.70f, false, true}
    };
}

void UForestSliceWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    if (WeaponSlots.IsValidIndex(EquippedSlot)) EquippedWeaponId = WeaponSlots[EquippedSlot].WeaponId;
}

void UForestSliceWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    SwitchCooldown = FMath::Max(0.0f, SwitchCooldown - DeltaTime);
}

void UForestSliceWeaponComponent::RequestSwitchToSlot(int32 SlotIndex)
{
    if (SwitchCooldown > 0.0f || !WeaponSlots.IsValidIndex(SlotIndex) || SlotIndex == EquippedSlot) return;
    if (!GetOwner()->HasAuthority()) {
        ServerSwitchToSlot(SlotIndex);
        return;
    }
    ApplySwitch(SlotIndex);
}

void UForestSliceWeaponComponent::ServerSwitchToSlot_Implementation(int32 SlotIndex)
{
    RequestSwitchToSlot(SlotIndex);
}

void UForestSliceWeaponComponent::ApplySwitch(int32 SlotIndex)
{
    if (!WeaponSlots.IsValidIndex(SlotIndex)) return;
    const FName PreviousWeapon = EquippedWeaponId;
    EquippedSlot = SlotIndex;
    EquippedWeaponId = WeaponSlots[SlotIndex].WeaponId;
    SwitchCooldown = SwitchDuration;
    WeaponChanged.Broadcast(PreviousWeapon, EquippedWeaponId);
}

FForestSliceWeaponDefinition UForestSliceWeaponComponent::GetEquippedDefinition() const
{
    return WeaponSlots.IsValidIndex(EquippedSlot) ? WeaponSlots[EquippedSlot] : FForestSliceWeaponDefinition{};
}

void UForestSliceWeaponComponent::OnRep_EquippedSlot(int32 PreviousSlot)
{
    const FName PreviousWeapon = WeaponSlots.IsValidIndex(PreviousSlot) ? WeaponSlots[PreviousSlot].WeaponId : NAME_None;
    WeaponChanged.Broadcast(PreviousWeapon, EquippedWeaponId);
}

void UForestSliceWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceWeaponComponent, EquippedSlot);
    DOREPLIFETIME(UForestSliceWeaponComponent, EquippedWeaponId);
}
