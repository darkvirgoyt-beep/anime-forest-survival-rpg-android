#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceInteractionComponent.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EForestSliceInteractionKind : uint8
{
    None,
    ResourceNode,
    BuildFrame,
    Bed,
    Mount,
    CraftingStation,
    Chest,
    QuestObject,
    NPC
};

USTRUCT(BlueprintType)
struct FForestSliceInteractableCandidate
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<AActor> Actor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EForestSliceInteractionKind Kind = EForestSliceInteractionKind::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ActionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Prompt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxDistance = 260.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceInteractionChanged, const FForestSliceInteractableCandidate&, Candidate);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceInteractionComponent();

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void RegisterCandidate(const FForestSliceInteractableCandidate& Candidate);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void UnregisterCandidate(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void ClearCandidates();

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool RefreshBestCandidate(const FVector& ViewerLocation);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void ConfirmBestCandidate();

    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool HasValidCandidate() const { return BestCandidate.Actor != nullptr; }

    UFUNCTION(BlueprintPure, Category = "Interaction")
    FForestSliceInteractableCandidate GetBestCandidate() const { return BestCandidate; }

    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FForestSliceInteractionChanged CandidateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FForestSliceInteractionChanged InteractionConfirmed;

protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction")
    FForestSliceInteractableCandidate BestCandidate;

private:
    UPROPERTY()
    TArray<FForestSliceInteractableCandidate> Candidates;

    bool IsCandidateBetter(const FForestSliceInteractableCandidate& Left, const FForestSliceInteractableCandidate& Right, const FVector& ViewerLocation) const;
};
