#include "ForestSliceCreatureComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UForestSliceCreatureComponent::UForestSliceCreatureComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UForestSliceCreatureComponent::BeginPlay()
{
    Super::BeginPlay();
    BondProgress = FMath::Clamp(BondProgress, 0.0f, Profile.BondThreshold);
    bBonded = Profile.bCanBond && BondProgress >= Profile.BondThreshold;
}

bool UForestSliceCreatureComponent::AddBondProgress(float Amount)
{
    if (!GetOwner()->HasAuthority() || !Profile.bCanBond || Amount <= 0.0f || bBonded) return false;

    BondProgress = FMath::Clamp(BondProgress + Amount, 0.0f, Profile.BondThreshold);
    bBonded = BondProgress >= Profile.BondThreshold;
    if (bBonded) Command = EForestSliceCreatureCommand::Follow;
    BroadcastBondState();
    return true;
}

bool UForestSliceCreatureComponent::SetCommand(EForestSliceCreatureCommand NewCommand)
{
    if (!GetOwner()->HasAuthority() || !bBonded) return false;
    Command = NewCommand;
    BroadcastBondState();
    return true;
}

void UForestSliceCreatureComponent::ClearBond()
{
    if (!GetOwner()->HasAuthority()) return;
    BondProgress = 0.0f;
    bBonded = false;
    Command = EForestSliceCreatureCommand::Roam;
    BroadcastBondState();
}

void UForestSliceCreatureComponent::OnRep_BondState()
{
    BroadcastBondState();
}

void UForestSliceCreatureComponent::BroadcastBondState()
{
    const bool bMountEligible = bBonded && Profile.bCanMount;
    BondChanged.Broadcast(BondProgress, bBonded, bMountEligible, Command);
}

void UForestSliceCreatureComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceCreatureComponent, BondProgress);
    DOREPLIFETIME(UForestSliceCreatureComponent, bBonded);
    DOREPLIFETIME(UForestSliceCreatureComponent, Command);
}
