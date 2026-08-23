#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceToolLoadoutComponent.generated.h"

UENUM(BlueprintType)
enum class EForestSliceToolTier : uint8
{
    Primitive,
    Copper,
    Iron,
    Froststeel,
    Aethelite
};

USTRUCT(BlueprintType)
struct FForestSliceToolDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ToolId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EForestSliceToolTier Tier = EForestSliceToolTier::Primitive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HarvestPower = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxDurability = 100;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Durability = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> SupportedResourceTags;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForestSliceToolChanged, FName, ToolId, int32, Durability);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceToolLoadoutComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceToolLoadoutComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Tools")
    bool EquipTool(FName ToolId);

    UFUNCTION(BlueprintCallable, Category = "Tools")
    bool UseEquippedTool(int32 DurabilityCost = 1);

    UFUNCTION(BlueprintCallable, Category = "Tools")
    bool RepairTool(FName ToolId, int32 DurabilityAmount);

    UFUNCTION(BlueprintPure, Category = "Tools")
    const FForestSliceToolDefinition& GetEquippedTool() const { return EquippedTool; }

    UFUNCTION(BlueprintPure, Category = "Tools")
    const TArray<FForestSliceToolDefinition>& GetTools() const { return Tools; }

    UFUNCTION(BlueprintPure, Category = "Tools")
    bool IsToolUsable() const { return EquippedTool.Durability > 0; }

    UPROPERTY(BlueprintAssignable, Category = "Tools")
    FForestSliceToolChanged ToolChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_ToolState, EditAnywhere, BlueprintReadOnly, Category = "Tools")
    TArray<FForestSliceToolDefinition> Tools;

    UPROPERTY(ReplicatedUsing = OnRep_ToolState, EditAnywhere, BlueprintReadOnly, Category = "Tools")
    FForestSliceToolDefinition EquippedTool;

    UFUNCTION()
    void OnRep_ToolState();

private:
    void InitializeStarterTools();
    FForestSliceToolDefinition* FindTool(FName ToolId);
    void BroadcastChanged();
};
