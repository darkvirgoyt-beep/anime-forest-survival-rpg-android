#include "ForestSliceToolLoadoutComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UForestSliceToolLoadoutComponent::UForestSliceToolLoadoutComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UForestSliceToolLoadoutComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeStarterTools();
}

void UForestSliceToolLoadoutComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceToolLoadoutComponent, Tools);
    DOREPLIFETIME(UForestSliceToolLoadoutComponent, EquippedTool);
}

FForestSliceToolDefinition* UForestSliceToolLoadoutComponent::FindTool(FName ToolId)
{
    return Tools.FindByPredicate([ToolId](const FForestSliceToolDefinition& Tool) {
        return Tool.ToolId == ToolId;
    });
}

bool UForestSliceToolLoadoutComponent::EquipTool(FName ToolId)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    FForestSliceToolDefinition* Tool = FindTool(ToolId);
    if (!Tool) return false;
    EquippedTool = *Tool;
    BroadcastChanged();
    return true;
}

bool UForestSliceToolLoadoutComponent::UseEquippedTool(int32 DurabilityCost)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || DurabilityCost <= 0 || EquippedTool.ToolId.IsNone() || EquippedTool.Durability < DurabilityCost) return false;
    EquippedTool.Durability = FMath::Max(0, EquippedTool.Durability - DurabilityCost);
    if (FForestSliceToolDefinition* Tool = FindTool(EquippedTool.ToolId)) Tool->Durability = EquippedTool.Durability;
    BroadcastChanged();
    return true;
}

bool UForestSliceToolLoadoutComponent::RepairTool(FName ToolId, int32 DurabilityAmount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || DurabilityAmount <= 0) return false;
    FForestSliceToolDefinition* Tool = FindTool(ToolId);
    if (!Tool) return false;
    Tool->Durability = FMath::Clamp(Tool->Durability + DurabilityAmount, 0, Tool->MaxDurability);
    if (EquippedTool.ToolId == ToolId) EquippedTool = *Tool;
    BroadcastChanged();
    return true;
}

void UForestSliceToolLoadoutComponent::OnRep_ToolState()
{
    BroadcastChanged();
}

void UForestSliceToolLoadoutComponent::BroadcastChanged()
{
    ToolChanged.Broadcast(EquippedTool.ToolId, EquippedTool.Durability);
}

void UForestSliceToolLoadoutComponent::InitializeStarterTools()
{
    if (Tools.Num() > 0) return;

    const auto AddTool = [this](const TCHAR* Id, EForestSliceToolTier Tier, float Power, int32 Durability, TArray<FName> Tags) {
        FForestSliceToolDefinition Tool;
        Tool.ToolId = FName(Id);
        Tool.Tier = Tier;
        Tool.HarvestPower = Power;
        Tool.MaxDurability = Durability;
        Tool.Durability = Durability;
        Tool.SupportedResourceTags = MoveTemp(Tags);
        Tools.Add(MoveTemp(Tool));
    };

    AddTool(TEXT("StoneAxe"), EForestSliceToolTier::Primitive, 1.0f, 80, TArray<FName>{FName(TEXT("Wood")), FName(TEXT("Fiber"))});
    AddTool(TEXT("StonePickaxe"), EForestSliceToolTier::Primitive, 1.0f, 80, TArray<FName>{FName(TEXT("Stone")), FName(TEXT("Ore"))});
    AddTool(TEXT("BoneShovel"), EForestSliceToolTier::Primitive, 0.9f, 70, TArray<FName>{FName(TEXT("Soil")), FName(TEXT("Clay"))});
    AddTool(TEXT("CopperAxe"), EForestSliceToolTier::Copper, 1.45f, 140, TArray<FName>{FName(TEXT("Wood")), FName(TEXT("Fiber"))});
    AddTool(TEXT("IronPickaxe"), EForestSliceToolTier::Iron, 1.85f, 220, TArray<FName>{FName(TEXT("Stone")), FName(TEXT("Ore")), FName(TEXT("Crystal"))});
    AddTool(TEXT("FroststeelHatchet"), EForestSliceToolTier::Froststeel, 2.3f, 300, TArray<FName>{FName(TEXT("Wood")), FName(TEXT("Frostwood"))});

    if (Tools.Num() > 0) EquippedTool = Tools[0];
}
