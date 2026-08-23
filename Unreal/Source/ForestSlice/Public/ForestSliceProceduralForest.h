#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestSliceProceduralForest.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EForestSliceBiome : uint8
{
    VerdantCrown,
    MistfenWetlands,
    EmberfallHighlands,
    SunscorchExpanse,
    MoonstoneCoast,
    FrostveilTundra,
    AethelPeaks
};

USTRUCT(BlueprintType)
struct FForestSliceBiomeProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EForestSliceBiome Biome = EForestSliceBiome::VerdantCrown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Id = FName(TEXT("VerdantCrown"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D CenterKm = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float InfluenceRadiusKm = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TemperatureCelsius = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Moisture = 0.65f;
};

USTRUCT(BlueprintType)
struct FForestSliceRiverSegment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Id = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D StartKm = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D EndKm = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WidthMeters = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DepthMeters = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FlowSpeedMetersPerSecond = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bNavigableBySwimming = false;
};

USTRUCT(BlueprintType)
struct FForestSliceMapLandmark
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Id = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Biome = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D CoordinateKm = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct FForestSliceChunkRecord
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FIntPoint Coordinate = FIntPoint::ZeroValue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bGenerated = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float PresentationAlpha = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FVector> ResourceLocations;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FTransform> GroundTransforms;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FTransform> TreeTransforms;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FTransform> RockTransforms;
};

UCLASS(BlueprintType)
class FORESTSLICE_API AForestSliceProceduralForest : public AActor
{
    GENERATED_BODY()

public:
    AForestSliceProceduralForest();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "World|Streaming")
    void GenerateAroundLocation(FVector WorldLocation);

    UFUNCTION(BlueprintCallable, Category = "World|Streaming")
    void RegenerateWorld(int32 NewSeed);

    UFUNCTION(BlueprintPure, Category = "World|Streaming")
    int32 GetLoadedChunkCount() const { return LoadedChunks.Num(); }

    UFUNCTION(BlueprintPure, Category = "World|Streaming")
    float GetChunkPresentationAlpha(FIntPoint Coordinate) const;

    UFUNCTION(BlueprintPure, Category = "World|Map")
    FName GetBiomeAtWorldLocation(FVector WorldLocation) const;

    UFUNCTION(BlueprintPure, Category = "World|Map")
    FForestSliceBiomeProfile GetBiomeProfileAtWorldLocation(FVector WorldLocation) const;

    UFUNCTION(BlueprintPure, Category = "World|Map")
    const TArray<FForestSliceBiomeProfile>& GetBiomeProfiles() const { return BiomeProfiles; }

    UFUNCTION(BlueprintPure, Category = "World|Water")
    const TArray<FForestSliceRiverSegment>& GetRiverSegments() const { return RiverSegments; }

    UFUNCTION(BlueprintPure, Category = "World|Water")
    float GetNearestRiverDistanceMeters(FVector WorldLocation) const;

    UFUNCTION(BlueprintPure, Category = "World|Map")
    float GetWorldSizeCentimeters() const { return MapSizeKilometers * 100000.0f; }

    UFUNCTION(BlueprintPure, Category = "World|Map")
    const TArray<FForestSliceMapLandmark>& GetMapLandmarks() const { return MapLandmarks; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundInstances;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeInstances;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RockInstances;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Generation")
    int32 WorldSeed = 42817;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Generation", meta = (ClampMin = "500"))
    float ChunkSize = 4000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Generation", meta = (ClampMin = "1", ClampMax = "8"))
    int32 ActiveRadius = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Generation")
    int32 TreesPerChunk = 48;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Generation")
    int32 RocksPerChunk = 14;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Assets")
    TObjectPtr<UStaticMesh> GroundMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Assets")
    TObjectPtr<UStaticMesh> TreeMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Assets")
    TObjectPtr<UStaticMesh> RockMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Map")
    TArray<FForestSliceMapLandmark> MapLandmarks;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Map")
    TArray<FForestSliceBiomeProfile> BiomeProfiles;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Water")
    TArray<FForestSliceRiverSegment> RiverSegments;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World|Map", meta = (ClampMin = "1.0", ClampMax = "200.0"))
    float MapSizeKilometers = 100.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World|Streaming")
    TMap<FIntPoint, FForestSliceChunkRecord> LoadedChunks;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World|Streaming")
    float ChunkPresentationTime = 0.0f;

private:
    FIntPoint LocationToChunk(FVector WorldLocation) const;
    void GenerateChunk(FIntPoint Coordinate);
    void UnloadChunk(FIntPoint Coordinate);
    void RemoveAllInstances();
    void RebuildInstances();
    void InitializeMapLayout();
    void InitializeBiomeAndRiverLayout();
    int32 MakeChunkSeed(FIntPoint Coordinate) const;
};
