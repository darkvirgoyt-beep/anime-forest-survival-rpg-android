#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceBuildingComponent.generated.h"

class UForestSliceInventoryComponent;

UENUM(BlueprintType)
enum class EForestSliceBuildingCategory : uint8
{
    Foundation,
    Structure,
    Utility,
    Storage,
    Farming,
    Crafting,
    Defense,
    Traversal
};

USTRUCT(BlueprintType)
struct FForestSliceMaterialCost
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FForestSliceBuildingRecipe
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RecipeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EForestSliceBuildingCategory Category = EForestSliceBuildingCategory::Structure;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Tier = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Footprint = FVector2D(200.0f, 200.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequiresFoundation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanPlaceOnWater = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FForestSliceMaterialCost> Costs;
};

USTRUCT(BlueprintType)
struct FForestSlicePlacedBuilding
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FGuid InstanceId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FName RecipeId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FTransform Transform;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bCompleted = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceBuildingChanged, FForestSlicePlacedBuilding, Building);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceBuildingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceBuildingComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Building")
    bool CanPlaceBuilding(FName RecipeId, FVector Location, FRotator Rotation, FName& FailureReason) const;

    UFUNCTION(BlueprintCallable, Category = "Building")
    bool PlaceBuilding(FName RecipeId, FVector Location, FRotator Rotation);

    UFUNCTION(BlueprintPure, Category = "Building")
    const TArray<FForestSliceBuildingRecipe>& GetRecipes() const { return Recipes; }

    UFUNCTION(BlueprintPure, Category = "Building")
    const TArray<FForestSlicePlacedBuilding>& GetPlacedBuildings() const { return PlacedBuildings; }

    UPROPERTY(BlueprintAssignable, Category = "Building")
    FForestSliceBuildingChanged BuildingPlaced;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Recipes")
    TArray<FForestSliceBuildingRecipe> Recipes;

    UPROPERTY(ReplicatedUsing = OnRep_PlacedBuildings, VisibleInstanceOnly, BlueprintReadOnly, Category = "Building|World")
    TArray<FForestSlicePlacedBuilding> PlacedBuildings;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Validation", meta = (ClampMin = "100.0"))
    float PlacementDistance = 700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Validation", meta = (ClampMin = "0.0"))
    float MaxBuildSlopeDegrees = 18.0f;

    UFUNCTION()
    void OnRep_PlacedBuildings();

private:
    const FForestSliceBuildingRecipe* FindRecipe(FName RecipeId) const;
    void InitializeDefaultRecipes();
    bool HasMaterials(const FForestSliceBuildingRecipe& Recipe, const UForestSliceInventoryComponent* Inventory) const;
};
