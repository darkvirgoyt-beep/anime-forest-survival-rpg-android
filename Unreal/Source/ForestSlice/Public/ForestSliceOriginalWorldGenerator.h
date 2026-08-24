#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ForestSliceOriginalWorldGenerator.generated.h"

/** Fully fictional Aethelgard biome identifiers; these do not map to real-world locations. */
UENUM(BlueprintType)
enum class EForestSliceOriginalBiome : uint8
{
    VerdantVeil,
    FrostwakeCrown,
    ShardwaterCoast,
    IronrootHighlands,
    SunkenCanopy,
    EmberfallHollow
};

USTRUCT(BlueprintType)
struct FForestSliceOriginalBiomeSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Original World")
    EForestSliceOriginalBiome Biome = EForestSliceOriginalBiome::VerdantVeil;

    /** Position in an invented 0..1 atlas, never latitude or longitude. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Original World")
    FVector2D AtlasAnchor = FVector2D(0.5f, 0.5f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Original World", meta = (ClampMin = "0.05", ClampMax = "0.45"))
    float InfluenceRadius = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Original World")
    float BaselineElevationMeters = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Original World", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Moisture = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgard|Original World", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float ThermalBias = 0.0f;
};

/**
 * Deterministic terrain helpers for an original fictional landscape. The input is normalized
 * game-space only. It never reads KML, map services, imagery, DEM values, or geographic data.
 */
UCLASS()
class FORESTSLICE_API UForestSliceOriginalWorldGenerator : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Aethelgard|Original World")
    static TArray<FForestSliceOriginalBiomeSpec> BuildDefaultBiomeAtlas();

    UFUNCTION(BlueprintPure, Category = "Aethelgard|Original World")
    static float SampleOriginalElevationMeters(int32 WorldSeed, float NormalizedX, float NormalizedY);

    UFUNCTION(BlueprintPure, Category = "Aethelgard|Original World")
    static EForestSliceOriginalBiome SelectOriginalBiome(float NormalizedX, float NormalizedY);

    UFUNCTION(BlueprintPure, Category = "Aethelgard|Original World")
    static bool ValidateOriginalWorldDefinition(const TArray<FForestSliceOriginalBiomeSpec>& Biomes, FText& OutFailure);
};
