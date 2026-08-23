#include "ForestSliceBuildingComponent.h"

#include "ForestSliceInventoryComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UForestSliceBuildingComponent::UForestSliceBuildingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UForestSliceBuildingComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeDefaultRecipes();
}

void UForestSliceBuildingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceBuildingComponent, PlacedBuildings);
}

const FForestSliceBuildingRecipe* UForestSliceBuildingComponent::FindRecipe(FName RecipeId) const
{
    return Recipes.FindByPredicate([RecipeId](const FForestSliceBuildingRecipe& Recipe) {
        return Recipe.RecipeId == RecipeId;
    });
}

bool UForestSliceBuildingComponent::HasMaterials(const FForestSliceBuildingRecipe& Recipe, const UForestSliceInventoryComponent* Inventory) const
{
    if (!Inventory) return false;
    for (const FForestSliceMaterialCost& Cost : Recipe.Costs) {
        if (Inventory->GetQuantity(Cost.ItemId) < Cost.Quantity) return false;
    }
    return true;
}

bool UForestSliceBuildingComponent::CanPlaceBuilding(FName RecipeId, FVector Location, FRotator Rotation, FName& FailureReason) const
{
    FailureReason = NAME_None;
    const AActor* OwnerActor = GetOwner();
    const FForestSliceBuildingRecipe* Recipe = FindRecipe(RecipeId);
    if (!OwnerActor || !Recipe) {
        FailureReason = FName(TEXT("InvalidRecipe"));
        return false;
    }
    if (!OwnerActor->HasAuthority()) {
        FailureReason = FName(TEXT("AuthorityRequired"));
        return false;
    }
    if (FVector::DistSquared2D(OwnerActor->GetActorLocation(), Location) > FMath::Square(PlacementDistance)) {
        FailureReason = FName(TEXT("TooFarAway"));
        return false;
    }
    if (FMath::Abs(FMath::UnwindDegrees(Rotation.Roll)) > MaxBuildSlopeDegrees || FMath::Abs(FMath::UnwindDegrees(Rotation.Pitch)) > MaxBuildSlopeDegrees) {
        FailureReason = FName(TEXT("TooSteep"));
        return false;
    }

    for (const FForestSlicePlacedBuilding& Existing : PlacedBuildings) {
        const float MinimumClearance = FMath::Max(Recipe->Footprint.X, Recipe->Footprint.Y) * 0.45f;
        if (FVector::DistSquared2D(Existing.Transform.GetLocation(), Location) < FMath::Square(MinimumClearance)) {
            FailureReason = FName(TEXT("Occupied"));
            return false;
        }
    }

    if (Recipe->bRequiresFoundation) {
        const bool bHasNearbyFoundation = PlacedBuildings.ContainsByPredicate([Location](const FForestSlicePlacedBuilding& Existing) {
            return Existing.RecipeId == FName(TEXT("Foundation")) && FVector::DistSquared2D(Existing.Transform.GetLocation(), Location) <= FMath::Square(250.0f);
        });
        if (!bHasNearbyFoundation) {
            FailureReason = FName(TEXT("FoundationRequired"));
            return false;
        }
    }

    const UForestSliceInventoryComponent* Inventory = OwnerActor->FindComponentByClass<UForestSliceInventoryComponent>();
    if (!HasMaterials(*Recipe, Inventory)) {
        FailureReason = FName(TEXT("MissingMaterials"));
        return false;
    }
    return true;
}

bool UForestSliceBuildingComponent::PlaceBuilding(FName RecipeId, FVector Location, FRotator Rotation)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;

    FName FailureReason;
    if (!CanPlaceBuilding(RecipeId, Location, Rotation, FailureReason)) return false;

    const FForestSliceBuildingRecipe* Recipe = FindRecipe(RecipeId);
    UForestSliceInventoryComponent* Inventory = GetOwner()->FindComponentByClass<UForestSliceInventoryComponent>();
    if (!Recipe || !Inventory) return false;

    for (const FForestSliceMaterialCost& Cost : Recipe->Costs) {
        if (!Inventory->RemoveItem(Cost.ItemId, Cost.Quantity)) return false;
    }

    FForestSlicePlacedBuilding& Building = PlacedBuildings.AddDefaulted_GetRef();
    Building.InstanceId = FGuid::NewGuid();
    Building.RecipeId = RecipeId;
    Building.Transform = FTransform(Rotation, Location, FVector::OneVector);
    Building.bCompleted = true;
    BuildingPlaced.Broadcast(Building);
    return true;
}

void UForestSliceBuildingComponent::OnRep_PlacedBuildings()
{
    if (PlacedBuildings.Num() > 0) BuildingPlaced.Broadcast(PlacedBuildings.Last());
}

void UForestSliceBuildingComponent::InitializeDefaultRecipes()
{
    if (Recipes.Num() > 0) return;

    const auto Cost = [](const TCHAR* ItemId, int32 Quantity) {
        FForestSliceMaterialCost Result;
        Result.ItemId = FName(ItemId);
        Result.Quantity = Quantity;
        return Result;
    };
    const auto AddRecipe = [this](const TCHAR* Id, const TCHAR* Name, EForestSliceBuildingCategory Category, int32 Tier, FVector2D Footprint, bool bRequiresFoundation, TArray<FForestSliceMaterialCost> Costs) {
        FForestSliceBuildingRecipe Recipe;
        Recipe.RecipeId = FName(Id);
        Recipe.DisplayName = FText::FromString(Name);
        Recipe.Category = Category;
        Recipe.Tier = Tier;
        Recipe.Footprint = Footprint;
        Recipe.bRequiresFoundation = bRequiresFoundation;
        Recipe.Costs = MoveTemp(Costs);
        Recipes.Add(MoveTemp(Recipe));
    };

    AddRecipe(TEXT("Campfire"), TEXT("Campfire"), EForestSliceBuildingCategory::Utility, 1, FVector2D(180.0f, 180.0f), false, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 12), Cost(TEXT("Stone"), 6)});
    AddRecipe(TEXT("Foundation"), TEXT("Foundation"), EForestSliceBuildingCategory::Foundation, 1, FVector2D(240.0f, 240.0f), false, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 10), Cost(TEXT("Stone"), 4)});
    AddRecipe(TEXT("Wall"), TEXT("Wall"), EForestSliceBuildingCategory::Structure, 1, FVector2D(240.0f, 40.0f), true, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 8), Cost(TEXT("Fiber"), 4)});
    AddRecipe(TEXT("Roof"), TEXT("Roof"), EForestSliceBuildingCategory::Structure, 1, FVector2D(240.0f, 240.0f), true, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 12), Cost(TEXT("Fiber"), 8)});
    AddRecipe(TEXT("Chest"), TEXT("Storage Chest"), EForestSliceBuildingCategory::Storage, 1, FVector2D(100.0f, 100.0f), true, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 16), Cost(TEXT("Iron"), 2)});
    AddRecipe(TEXT("Bed"), TEXT("Bed"), EForestSliceBuildingCategory::Utility, 1, FVector2D(100.0f, 200.0f), true, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 12), Cost(TEXT("Fiber"), 12), Cost(TEXT("Hide"), 4)});
    AddRecipe(TEXT("Workbench"), TEXT("Workbench"), EForestSliceBuildingCategory::Crafting, 1, FVector2D(160.0f, 120.0f), true, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 20), Cost(TEXT("Stone"), 8)});
    AddRecipe(TEXT("FarmPlot"), TEXT("Farm Plot"), EForestSliceBuildingCategory::Farming, 1, FVector2D(300.0f, 300.0f), false, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 4), Cost(TEXT("Fiber"), 8)});
    AddRecipe(TEXT("Kiln"), TEXT("Kiln"), EForestSliceBuildingCategory::Crafting, 2, FVector2D(180.0f, 180.0f), true, TArray<FForestSliceMaterialCost>{Cost(TEXT("Stone"), 24), Cost(TEXT("Clay"), 12), Cost(TEXT("Coal"), 4)});
    AddRecipe(TEXT("Forge"), TEXT("Forge"), EForestSliceBuildingCategory::Crafting, 2, FVector2D(220.0f, 180.0f), true, TArray<FForestSliceMaterialCost>{Cost(TEXT("Stone"), 30), Cost(TEXT("Iron"), 12), Cost(TEXT("Coal"), 8)});
    AddRecipe(TEXT("Fence"), TEXT("Fence"), EForestSliceBuildingCategory::Defense, 1, FVector2D(220.0f, 40.0f), false, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 6)});
    AddRecipe(TEXT("Gate"), TEXT("Gate"), EForestSliceBuildingCategory::Defense, 1, FVector2D(220.0f, 60.0f), false, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 12), Cost(TEXT("Iron"), 4)});
    AddRecipe(TEXT("Lamp"), TEXT("Village Lamp"), EForestSliceBuildingCategory::Utility, 1, FVector2D(60.0f, 60.0f), false, TArray<FForestSliceMaterialCost>{Cost(TEXT("Wood"), 2), Cost(TEXT("Iron"), 2), Cost(TEXT("Oil"), 1)});
    AddRecipe(TEXT("Waystone"), TEXT("Waystone"), EForestSliceBuildingCategory::Traversal, 3, FVector2D(140.0f, 140.0f), true, TArray<FForestSliceMaterialCost>{Cost(TEXT("Stone"), 32), Cost(TEXT("Moonstone"), 6)});
}
