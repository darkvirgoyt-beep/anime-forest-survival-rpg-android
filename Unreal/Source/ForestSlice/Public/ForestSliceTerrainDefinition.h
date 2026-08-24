#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "ForestSliceTerrainDefinition.generated.h"

/** The origin of a terrain heightmap. Google Earth is intentionally planning-only. */
UENUM(BlueprintType)
enum class EForestSliceTerrainSourceKind : uint8
{
    PlanningBoundaryOnly,
    USGS3DEP,
    NASASRTM,
    CopernicusDEM,
    LicensedCustomElevation
};

USTRUCT(BlueprintType)
struct FForestSliceTerrainSourceRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain")
    EForestSliceTerrainSourceKind SourceKind = EForestSliceTerrainSourceKind::PlanningBoundaryOnly;

    /** Human-readable data source, version, provider, and acquisition date. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain")
    FString Citation;

    /** URL or internal record for the applicable data license or public-domain notice. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain")
    FString LicenseRecord;

    /** Must remain false: Google Earth imagery, tiles, meshes, and elevation output are not game inputs. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain")
    bool bDerivedFromGoogleEarthContent = false;
};

/**
 * A data asset authored after a user exports a KML/KMZ boundary and an independently licensed
 * DEM has been cropped and converted to a 16-bit heightmap. It does not download map data.
 */
UCLASS(BlueprintType)
class FORESTSLICE_API UForestSliceTerrainDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain")
    FForestSliceTerrainSourceRecord SourceRecord;

    /** The original editable KML/KMZ boundary is stored outside packaged content and is not Google imagery. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain")
    FString PlanningBoundaryId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain")
    TSoftObjectPtr<UTexture2D> Heightmap16Bit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain", meta = (ClampMin = "256.0", ClampMax = "16000.0"))
    float PlayableWidthMeters = 8000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain", meta = (ClampMin = "256.0", ClampMax = "16000.0"))
    float PlayableHeightMeters = 8000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain", meta = (ClampMin = "0.01", ClampMax = "10.0"))
    float VerticalScale = 1.0f;

    /** Original biome masks generated from the independently licensed DEM and hand-authored design. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aethelgrad|Terrain")
    TArray<TSoftObjectPtr<UTexture2D>> OriginalBiomeMasks;

    UFUNCTION(BlueprintCallable, Category = "Aethelgrad|Terrain")
    bool ValidateForLandscapeImport(FText& OutFailure) const;
};
