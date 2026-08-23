#include "ForestSliceProceduralForest.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

AForestSliceProceduralForest::AForestSliceProceduralForest()
{
    PrimaryActorTick.bCanEverTick = true;
    GroundInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GroundInstances"));
    GroundInstances->SetupAttachment(RootComponent);
    TreeInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreeInstances"));
    TreeInstances->SetupAttachment(RootComponent);
    RockInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RockInstances"));
    RockInstances->SetupAttachment(RootComponent);
}

void AForestSliceProceduralForest::BeginPlay()
{
    Super::BeginPlay();
    if (GroundMesh) GroundInstances->SetStaticMesh(GroundMesh);
    if (TreeMesh) TreeInstances->SetStaticMesh(TreeMesh);
    if (RockMesh) RockInstances->SetStaticMesh(RockMesh);
    InitializeMapLayout();
    GenerateAroundLocation(GetActorLocation());
}

void AForestSliceProceduralForest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ChunkPresentationTime += FMath::Max(0.0f, DeltaSeconds);

    bool bPresentationChanged = false;
    for (TPair<FIntPoint, FForestSliceChunkRecord>& Pair : LoadedChunks) {
        const float PreviousAlpha = Pair.Value.PresentationAlpha;
        Pair.Value.PresentationAlpha = FMath::Min(1.0f, PreviousAlpha + FMath::Max(0.0f, DeltaSeconds) * 2.8f);
        bPresentationChanged |= !FMath::IsNearlyEqual(PreviousAlpha, Pair.Value.PresentationAlpha);
    }
    if (bPresentationChanged) RebuildInstances();

    if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0)) {
        GenerateAroundLocation(PlayerPawn->GetActorLocation());
    }
}

FIntPoint AForestSliceProceduralForest::LocationToChunk(FVector WorldLocation) const
{
    return FIntPoint(
        FMath::FloorToInt(WorldLocation.X / ChunkSize),
        FMath::FloorToInt(WorldLocation.Y / ChunkSize)
    );
}

int32 AForestSliceProceduralForest::MakeChunkSeed(FIntPoint Coordinate) const
{
    uint32 Hash = GetTypeHash(WorldSeed);
    Hash = HashCombine(Hash, GetTypeHash(Coordinate.X));
    Hash = HashCombine(Hash, GetTypeHash(Coordinate.Y));
    return static_cast<int32>(Hash & 0x7fffffff);
}

void AForestSliceProceduralForest::GenerateAroundLocation(FVector WorldLocation)
{
    const FIntPoint Center = LocationToChunk(WorldLocation);
    TSet<FIntPoint> Desired;
    for (int32 Y = -ActiveRadius; Y <= ActiveRadius; ++Y) {
        for (int32 X = -ActiveRadius; X <= ActiveRadius; ++X) {
            const FIntPoint Coordinate = Center + FIntPoint(X, Y);
            Desired.Add(Coordinate);
            if (!LoadedChunks.Contains(Coordinate)) GenerateChunk(Coordinate);
        }
    }

    TArray<FIntPoint> ToUnload;
    for (const TPair<FIntPoint, FForestSliceChunkRecord>& Pair : LoadedChunks) {
        if (!Desired.Contains(Pair.Key)) ToUnload.Add(Pair.Key);
    }
    for (const FIntPoint& Coordinate : ToUnload) UnloadChunk(Coordinate);
}

void AForestSliceProceduralForest::GenerateChunk(FIntPoint Coordinate)
{
    FRandomStream Random(MakeChunkSeed(Coordinate));
    FForestSliceChunkRecord Record;
    Record.Coordinate = Coordinate;
    Record.bGenerated = true;
    Record.PresentationAlpha = 0.0f;

    const FVector Origin(Coordinate.X * ChunkSize, Coordinate.Y * ChunkSize, GetActorLocation().Z);
    float GroundScale = 1.0f;
    if (GroundMesh) {
        const FVector GroundExtent = GroundMesh->GetBounds().BoxExtent;
        const float GroundWidth = 2.0f * FMath::Max(GroundExtent.X, GroundExtent.Y);
        GroundScale = ChunkSize / FMath::Max(1.0f, GroundWidth);
    }
    Record.GroundTransforms.Add(FTransform(FRotator::ZeroRotator,
        Origin + FVector(ChunkSize * 0.5f, ChunkSize * 0.5f, 0.0f), FVector(GroundScale)));
    for (int32 Index = 0; Index < TreesPerChunk; ++Index) {
        const FVector Location = Origin + FVector(
            Random.FRandRange(-ChunkSize * 0.48f, ChunkSize * 0.48f),
            Random.FRandRange(-ChunkSize * 0.48f, ChunkSize * 0.48f),
            0.0f
        );
        const float Scale = Random.FRandRange(0.80f, 1.35f);
        const FTransform Transform(FRotator(0.0f, Random.FRandRange(0.0f, 360.0f), 0.0f), Location, FVector(Scale));
        Record.TreeTransforms.Add(Transform);
    }

    for (int32 Index = 0; Index < RocksPerChunk; ++Index) {
        const FVector Location = Origin + FVector(
            Random.FRandRange(-ChunkSize * 0.48f, ChunkSize * 0.48f),
            Random.FRandRange(-ChunkSize * 0.48f, ChunkSize * 0.48f),
            0.0f
        );
        const float Scale = Random.FRandRange(0.65f, 1.40f);
        const FTransform Transform(FRotator(0.0f, Random.FRandRange(0.0f, 360.0f), 0.0f), Location, FVector(Scale));
        Record.RockTransforms.Add(Transform);
        Record.ResourceLocations.Add(Location);
    }

    LoadedChunks.Add(Coordinate, MoveTemp(Record));
    RebuildInstances();
}

void AForestSliceProceduralForest::UnloadChunk(FIntPoint Coordinate)
{
    LoadedChunks.Remove(Coordinate);
    RebuildInstances();
}

void AForestSliceProceduralForest::RemoveAllInstances()
{
    if (GroundInstances) GroundInstances->ClearInstances();
    if (TreeInstances) TreeInstances->ClearInstances();
    if (RockInstances) RockInstances->ClearInstances();
}

void AForestSliceProceduralForest::RebuildInstances()
{
    RemoveAllInstances();
    for (const TPair<FIntPoint, FForestSliceChunkRecord>& Pair : LoadedChunks) {
        const float RevealScale = FMath::Lerp(0.05f, 1.0f, Pair.Value.PresentationAlpha);
        TArray<FTransform> AnimatedGround;
        AnimatedGround.Reserve(Pair.Value.GroundTransforms.Num());
        for (const FTransform& SourceTransform : Pair.Value.GroundTransforms) {
            FTransform AnimatedTransform = SourceTransform;
            AnimatedTransform.SetScale3D(SourceTransform.GetScale3D() * RevealScale);
            AnimatedGround.Add(AnimatedTransform);
        }
        if (GroundInstances && GroundMesh) GroundInstances->AddInstances(AnimatedGround, false, true, true);

        TArray<FTransform> AnimatedTrees;
        AnimatedTrees.Reserve(Pair.Value.TreeTransforms.Num());
        for (const FTransform& SourceTransform : Pair.Value.TreeTransforms) {
            FTransform AnimatedTransform = SourceTransform;
            AnimatedTransform.SetScale3D(SourceTransform.GetScale3D() * RevealScale);
            AnimatedTrees.Add(AnimatedTransform);
        }
        TArray<FTransform> AnimatedRocks;
        AnimatedRocks.Reserve(Pair.Value.RockTransforms.Num());
        for (const FTransform& SourceTransform : Pair.Value.RockTransforms) {
            FTransform AnimatedTransform = SourceTransform;
            AnimatedTransform.SetScale3D(SourceTransform.GetScale3D() * RevealScale);
            AnimatedRocks.Add(AnimatedTransform);
        }
        if (TreeInstances && TreeMesh) TreeInstances->AddInstances(AnimatedTrees, false, true, true);
        if (RockInstances && RockMesh) RockInstances->AddInstances(AnimatedRocks, false, true, true);
    }
}

float AForestSliceProceduralForest::GetChunkPresentationAlpha(FIntPoint Coordinate) const
{
    if (const FForestSliceChunkRecord* Record = LoadedChunks.Find(Coordinate)) {
        return Record->PresentationAlpha;
    }
    return 0.0f;
}

FName AForestSliceProceduralForest::GetBiomeAtWorldLocation(FVector WorldLocation) const
{
    return GetBiomeProfileAtWorldLocation(WorldLocation).Id;
}

void AForestSliceProceduralForest::InitializeMapLayout()
{
    if (MapLandmarks.Num() > 0 && BiomeProfiles.Num() > 0 && RiverSegments.Num() > 0) return;
    const auto AddLandmark = [this](const TCHAR* Id, const TCHAR* Biome, float XKm, float YKm) {
        FForestSliceMapLandmark Landmark;
        Landmark.Id = FName(Id);
        Landmark.Biome = FName(Biome);
        Landmark.CoordinateKm = FVector2D(XKm, YKm);
        MapLandmarks.Add(Landmark);
    };
    AddLandmark(TEXT("ForestCamp"), TEXT("VerdantCrown"), 10.0f, 16.0f);
    AddLandmark(TEXT("FarmingVillage"), TEXT("VerdantCrown"), 14.0f, 40.0f);
    AddLandmark(TEXT("MossCave"), TEXT("MistfenWetlands"), 26.0f, 78.0f);
    AddLandmark(TEXT("SandGate"), TEXT("SunscorchExpanse"), 40.0f, 48.0f);
    AddLandmark(TEXT("SunKiln"), TEXT("SunscorchExpanse"), 47.0f, 28.0f);
    AddLandmark(TEXT("Oasis"), TEXT("SunscorchExpanse"), 55.0f, 69.0f);
    AddLandmark(TEXT("FrostGate"), TEXT("FrostveilTundra"), 74.0f, 57.0f);
    AddLandmark(TEXT("PredatorBasin"), TEXT("FrostveilTundra"), 84.0f, 59.0f);
    AddLandmark(TEXT("FrostclawArena"), TEXT("AethelPeaks"), 88.0f, 83.0f);
    InitializeBiomeAndRiverLayout();
}

void AForestSliceProceduralForest::InitializeBiomeAndRiverLayout()
{
    if (BiomeProfiles.Num() == 0) {
        const auto AddBiome = [this](EForestSliceBiome Biome, const TCHAR* Id, FVector2D CenterKm, float RadiusKm, float Temperature, float Moisture) {
            FForestSliceBiomeProfile Profile;
            Profile.Biome = Biome;
            Profile.Id = FName(Id);
            Profile.CenterKm = CenterKm;
            Profile.InfluenceRadiusKm = RadiusKm;
            Profile.TemperatureCelsius = Temperature;
            Profile.Moisture = Moisture;
            BiomeProfiles.Add(Profile);
        };
        AddBiome(EForestSliceBiome::VerdantCrown, TEXT("VerdantCrown"), FVector2D(14.0f, 30.0f), 31.0f, 18.0f, 0.78f);
        AddBiome(EForestSliceBiome::MistfenWetlands, TEXT("MistfenWetlands"), FVector2D(18.0f, 78.0f), 24.0f, 12.0f, 0.96f);
        AddBiome(EForestSliceBiome::EmberfallHighlands, TEXT("EmberfallHighlands"), FVector2D(38.0f, 76.0f), 25.0f, 24.0f, 0.32f);
        AddBiome(EForestSliceBiome::SunscorchExpanse, TEXT("SunscorchExpanse"), FVector2D(54.0f, 34.0f), 27.0f, 31.0f, 0.14f);
        AddBiome(EForestSliceBiome::MoonstoneCoast, TEXT("MoonstoneCoast"), FVector2D(78.0f, 22.0f), 25.0f, 16.0f, 0.72f);
        AddBiome(EForestSliceBiome::FrostveilTundra, TEXT("FrostveilTundra"), FVector2D(78.0f, 61.0f), 24.0f, -8.0f, 0.42f);
        AddBiome(EForestSliceBiome::AethelPeaks, TEXT("AethelPeaks"), FVector2D(90.0f, 88.0f), 22.0f, -2.0f, 0.58f);
    }

    if (RiverSegments.Num() == 0) {
        const auto AddRiver = [this](const TCHAR* Id, FVector2D StartKm, FVector2D EndKm, float Width, float Depth, float Flow, bool bSwimmable) {
            FForestSliceRiverSegment River;
            River.Id = FName(Id);
            River.StartKm = StartKm;
            River.EndKm = EndKm;
            River.WidthMeters = Width;
            River.DepthMeters = Depth;
            River.FlowSpeedMetersPerSecond = Flow;
            River.bNavigableBySwimming = bSwimmable;
            RiverSegments.Add(River);
        };
        AddRiver(TEXT("ForestRiver"), FVector2D(8.0f, 5.0f), FVector2D(31.0f, 50.0f), 24.0f, 1.4f, 1.1f, false);
        AddRiver(TEXT("FenBranch"), FVector2D(31.0f, 50.0f), FVector2D(18.0f, 94.0f), 18.0f, 1.0f, 0.7f, false);
        AddRiver(TEXT("OasisRun"), FVector2D(31.0f, 50.0f), FVector2D(62.0f, 43.0f), 12.0f, 0.8f, 0.45f, false);
        AddRiver(TEXT("SnowmeltFall"), FVector2D(62.0f, 43.0f), FVector2D(92.0f, 86.0f), 30.0f, 2.2f, 1.6f, true);
    }
}

FForestSliceBiomeProfile AForestSliceProceduralForest::GetBiomeProfileAtWorldLocation(FVector WorldLocation) const
{
    FForestSliceBiomeProfile Unknown;
    Unknown.Id = FName(TEXT("Unknown"));
    if (WorldLocation.X < 0.0f || WorldLocation.Y < 0.0f || WorldLocation.X > MapSizeKilometers * 1000.0f || WorldLocation.Y > MapSizeKilometers * 1000.0f || BiomeProfiles.Num() == 0) {
        return Unknown;
    }

    const FVector2D LocationKm(WorldLocation.X / 1000.0f, WorldLocation.Y / 1000.0f);
    const FForestSliceBiomeProfile* BestProfile = nullptr;
    float BestDistanceSquared = TNumericLimits<float>::Max();
    for (const FForestSliceBiomeProfile& Profile : BiomeProfiles) {
        const float DistanceSquared = FVector2D::DistSquared(LocationKm, Profile.CenterKm);
        if (DistanceSquared < BestDistanceSquared) {
            BestDistanceSquared = DistanceSquared;
            BestProfile = &Profile;
        }
    }
    return BestProfile ? *BestProfile : Unknown;
}

float AForestSliceProceduralForest::GetNearestRiverDistanceMeters(FVector WorldLocation) const
{
    if (RiverSegments.Num() == 0) return TNumericLimits<float>::Max();
    const FVector2D PointKm(WorldLocation.X / 1000.0f, WorldLocation.Y / 1000.0f);
    float BestDistanceSquared = TNumericLimits<float>::Max();
    for (const FForestSliceRiverSegment& River : RiverSegments) {
        const FVector2D Segment = River.EndKm - River.StartKm;
        const float SegmentLengthSquared = Segment.SizeSquared();
        const float T = SegmentLengthSquared > SMALL_NUMBER
            ? FMath::Clamp(FVector2D::DotProduct(PointKm - River.StartKm, Segment) / SegmentLengthSquared, 0.0f, 1.0f)
            : 0.0f;
        const FVector2D ClosestPoint = River.StartKm + Segment * T;
        BestDistanceSquared = FMath::Min(BestDistanceSquared, FVector2D::DistSquared(PointKm, ClosestPoint));
    }
    return FMath::Sqrt(BestDistanceSquared) * 1000.0f;
}

void AForestSliceProceduralForest::RegenerateWorld(int32 NewSeed)
{
    WorldSeed = NewSeed;
    ChunkPresentationTime = 0.0f;
    LoadedChunks.Empty();
    RemoveAllInstances();
    GenerateAroundLocation(GetActorLocation());
}
