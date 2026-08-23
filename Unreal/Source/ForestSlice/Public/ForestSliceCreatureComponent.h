#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceCreatureComponent.generated.h"

UENUM(BlueprintType)
enum class EForestSliceCreatureRole : uint8
{
    Wildlife,
    Predator,
    Companion,
    Mount,
    Boss
};

UENUM(BlueprintType)
enum class EForestSliceCreatureCommand : uint8
{
    Roam,
    Follow,
    Stay,
    Defend,
    ReturnToCamp
};

USTRUCT(BlueprintType)
struct FForestSliceCreatureProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SpeciesId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EForestSliceCreatureRole Role = EForestSliceCreatureRole::Wildlife;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanBond = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanMount = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BondThreshold = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FForestSliceCreatureBondChanged, float, BondProgress, bool, bBonded, bool, bMountEligible, EForestSliceCreatureCommand, Command);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceCreatureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceCreatureComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Creature")
    bool AddBondProgress(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Creature")
    bool SetCommand(EForestSliceCreatureCommand NewCommand);

    UFUNCTION(BlueprintCallable, Category = "Creature")
    void ClearBond();

    UFUNCTION(BlueprintPure, Category = "Creature")
    const FForestSliceCreatureProfile& GetProfile() const { return Profile; }

    UFUNCTION(BlueprintPure, Category = "Creature")
    float GetBondProgress() const { return BondProgress; }

    UFUNCTION(BlueprintPure, Category = "Creature")
    bool IsBonded() const { return bBonded; }

    UFUNCTION(BlueprintPure, Category = "Creature")
    bool IsMountEligible() const { return bBonded && Profile.bCanMount; }

    UFUNCTION(BlueprintPure, Category = "Creature")
    EForestSliceCreatureCommand GetCommand() const { return Command; }

    UPROPERTY(BlueprintAssignable, Category = "Creature")
    FForestSliceCreatureBondChanged BondChanged;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    FForestSliceCreatureProfile Profile;

    UPROPERTY(ReplicatedUsing = OnRep_BondState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Creature")
    float BondProgress = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_BondState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Creature")
    bool bBonded = false;

    UPROPERTY(ReplicatedUsing = OnRep_BondState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Creature")
    EForestSliceCreatureCommand Command = EForestSliceCreatureCommand::Roam;

    UFUNCTION()
    void OnRep_BondState();

private:
    void BroadcastBondState();
};
