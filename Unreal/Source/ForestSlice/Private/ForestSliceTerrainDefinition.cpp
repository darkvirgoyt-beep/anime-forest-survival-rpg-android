#include "ForestSliceTerrainDefinition.h"

bool UForestSliceTerrainDefinition::ValidateForLandscapeImport(FText& OutFailure) const
{
    if (SourceRecord.SourceKind == EForestSliceTerrainSourceKind::PlanningBoundaryOnly)
    {
        OutFailure = FText::FromString(TEXT("Choose an independently licensed DEM before importing a landscape."));
        return false;
    }

    if (SourceRecord.bDerivedFromGoogleEarthContent)
    {
        OutFailure = FText::FromString(TEXT("Google Earth imagery, terrain meshes, tiles, and elevation output are not valid game terrain inputs."));
        return false;
    }

    if (PlanningBoundaryId.IsEmpty() || SourceRecord.Citation.IsEmpty() || SourceRecord.LicenseRecord.IsEmpty())
    {
        OutFailure = FText::FromString(TEXT("Record the planning boundary, source citation, and license before importing terrain."));
        return false;
    }

    if (Heightmap16Bit.IsNull())
    {
        OutFailure = FText::FromString(TEXT("Assign a cropped, independently sourced 16-bit heightmap."));
        return false;
    }

    if (PlayableWidthMeters > 16000.0f || PlayableHeightMeters > 16000.0f)
    {
        OutFailure = FText::FromString(TEXT("Start with a maximum 16 km by 16 km first-playable region; stream later expansions separately."));
        return false;
    }

    OutFailure = FText::GetEmpty();
    return true;
}
