#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceCreatureCompanionComponent.generated.h"

class UForestSliceHealthComponent;
class UForestSliceInventoryComponent;

UENUM(BlueprintType)
enum class EForestSliceCompanionCommand : uint8
{
    Follow,
    Stay
};

USTRUCT(BlueprintType)
struct FForestSliceCompanionState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FGuid CompanionId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FName CreatureId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Level = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Bond = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float HealthFraction = 0.75f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EForestSliceCompanionCommand Command = EForestSliceCompanionCommand::Follow;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Revision = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bCaptured = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceCompanionChanged, const FForestSliceCompanionState&, State);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceCreatureCompanionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceCreatureCompanionComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Companion|Capture")
    bool TryCaptureCreature(AActor* TargetCreature);

    UFUNCTION(BlueprintCallable, Category = "Companion|Command")
    bool ToggleCommand();

    UFUNCTION(BlueprintCallable, Category = "Companion|Command")
    bool SetCommand(EForestSliceCompanionCommand NewCommand);

    UFUNCTION(BlueprintPure, Category = "Companion")
    const FForestSliceCompanionState& GetCompanionState() const { return State; }

    UFUNCTION(BlueprintPure, Category = "Companion")
    bool HasCompanion() const { return State.bCaptured; }

    UPROPERTY(BlueprintAssignable, Category = "Companion")
    FForestSliceCompanionChanged CompanionChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_State, VisibleInstanceOnly, BlueprintReadOnly, Category = "Companion")
    FForestSliceCompanionState State;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Capture", meta = (ClampMin = "50.0"))
    float CaptureRange = 380.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Capture", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float CaptureHealthFraction = 0.38f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Capture", meta = (ClampMin = "1"))
    int32 FiberCost = 2;

    UFUNCTION(Server, Reliable)
    void ServerCaptureCreature(AActor* TargetCreature);

    UFUNCTION(Server, Reliable)
    void ServerSetCommand(EForestSliceCompanionCommand NewCommand);

    UFUNCTION()
    void OnRep_State();

private:
    bool ValidateCaptureTarget(AActor* TargetCreature, FString& FailureReason) const;
    void ApplyCapture(AActor* TargetCreature);
    void BroadcastState();
};
