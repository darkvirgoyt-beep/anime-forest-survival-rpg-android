#include "ForestSliceOriginalWorldGenerator.h"

namespace ForestSliceOriginalWorld
{
    float Hash01(int32 X, int32 Y, int32 Seed)
    {
        uint32 Value = static_cast<uint32>(X) * 374761393u;
        Value += static_cast<uint32>(Y) * 668265263u;
        Value += static_cast<uint32>(Seed) * 1442695041u;
        Value = (Value ^ (Value >> 13u)) * 1274126177u;
        return static_cast<float>(Value ^ (Value >> 16u)) / static_cast<float>(MAX_uint32);
    }

    float Smooth(float Value)
    {
        return Value * Value * (3.0f - 2.0f * Value);
    }

    float ValueNoise(float X, float Y, int32 Seed)
    {
        const int32 CellX = FMath::FloorToInt(X);
        const int32 CellY = FMath::FloorToInt(Y);
        const float LocalX = Smooth(X - static_cast<float>(CellX));
        const float LocalY = Smooth(Y - static_cast<float>(CellY));
        const float A = FMath::Lerp(Hash01(CellX, CellY, Seed), Hash01(CellX + 1, CellY, Seed), LocalX);
        const float B = FMath::Lerp(Hash01(CellX, CellY + 1, Seed), Hash01(CellX + 1, CellY + 1, Seed), LocalX);
        return FMath::Lerp(A, B, LocalY) * 2.0f - 1.0f;
    }

    float FractalNoise(float X, float Y, int32 Seed)
    {
        float Frequency = 1.0f;
        float Amplitude = 1.0f;
        float Total = 0.0f;
        float Weight = 0.0f;
        for (int32 Octave = 0; Octave < 5; ++Octave)
        {
            Total += ValueNoise(X * Frequency, Y * Frequency, Seed + Octave * 911) * Amplitude;
            Weight += Amplitude;
            Frequency *= 2.05f;
            Amplitude *= 0.52f;
        }
        return Total / Weight;
    }
}

TArray<FForestSliceOriginalBiomeSpec> UForestSliceOriginalWorldGenerator::BuildDefaultBiomeAtlas()
{
    return {
        { EForestSliceOriginalBiome::FrostwakeCrown, FVector2D(0.48f, 0.16f), 0.23f, 1650.0f, 0.42f, -0.92f },
        { EForestSliceOriginalBiome::IronrootHighlands, FVector2D(0.18f, 0.34f), 0.25f, 980.0f, 0.38f, -0.18f },
        { EForestSliceOriginalBiome::VerdantVeil, FVector2D(0.51f, 0.51f), 0.28f, 510.0f, 0.83f, 0.18f },
        { EForestSliceOriginalBiome::ShardwaterCoast, FVector2D(0.84f, 0.48f), 0.23f, 80.0f, 0.72f, 0.32f },
        { EForestSliceOriginalBiome::SunkenCanopy, FVector2D(0.28f, 0.79f), 0.24f, 230.0f, 0.95f, 0.46f },
        { EForestSliceOriginalBiome::EmberfallHollow, FVector2D(0.70f, 0.77f), 0.21f, 690.0f, 0.26f, 0.66f }
    };
}

float UForestSliceOriginalWorldGenerator::SampleOriginalElevationMeters(int32 WorldSeed, float NormalizedX, float NormalizedY)
{
    const float X = FMath::Clamp(NormalizedX, 0.0f, 1.0f);
    const float Y = FMath::Clamp(NormalizedY, 0.0f, 1.0f);
    const float Continental = ForestSliceOriginalWorld::FractalNoise(X * 2.1f, Y * 2.1f, WorldSeed);
    const float Ridge = 1.0f - FMath::Abs(ForestSliceOriginalWorld::FractalNoise(X * 5.0f, Y * 5.0f, WorldSeed + 73));
    const float Valley = ForestSliceOriginalWorld::FractalNoise(X * 10.0f, Y * 10.0f, WorldSeed + 307);
    return FMath::Max(0.0f, 520.0f + Continental * 490.0f + Ridge * 740.0f + Valley * 105.0f);
}

EForestSliceOriginalBiome UForestSliceOriginalWorldGenerator::SelectOriginalBiome(float NormalizedX, float NormalizedY)
{
    const FVector2D Point(FMath::Clamp(NormalizedX, 0.0f, 1.0f), FMath::Clamp(NormalizedY, 0.0f, 1.0f));
    const TArray<FForestSliceOriginalBiomeSpec> Biomes = BuildDefaultBiomeAtlas();
    const FForestSliceOriginalBiomeSpec* Nearest = &Biomes[0];
    float BestScore = TNumericLimits<float>::Max();
    for (const FForestSliceOriginalBiomeSpec& Candidate : Biomes)
    {
        const float Score = FVector2D::Distance(Point, Candidate.AtlasAnchor) / Candidate.InfluenceRadius;
        if (Score < BestScore)
        {
            BestScore = Score;
            Nearest = &Candidate;
        }
    }
    return Nearest->Biome;
}

bool UForestSliceOriginalWorldGenerator::ValidateOriginalWorldDefinition(const TArray<FForestSliceOriginalBiomeSpec>& Biomes, FText& OutFailure)
{
    if (Biomes.Num() != 6)
    {
        OutFailure = FText::FromString(TEXT("The original atlas requires exactly six fictional biome specifications."));
        return false;
    }

    TSet<EForestSliceOriginalBiome> UniqueBiomes;
    for (const FForestSliceOriginalBiomeSpec& Biome : Biomes)
    {
        if (Biome.AtlasAnchor.X < 0.0f || Biome.AtlasAnchor.Y < 0.0f || Biome.AtlasAnchor.X > 1.0f || Biome.AtlasAnchor.Y > 1.0f)
        {
            OutFailure = FText::FromString(TEXT("Biome anchors must use fictional normalized atlas coordinates between zero and one."));
            return false;
        }
        UniqueBiomes.Add(Biome.Biome);
    }

    if (UniqueBiomes.Num() != Biomes.Num())
    {
        OutFailure = FText::FromString(TEXT("Each fictional biome must be represented once."));
        return false;
    }

    OutFailure = FText::GetEmpty();
    return true;
}
