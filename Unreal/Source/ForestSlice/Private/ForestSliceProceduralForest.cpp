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
    const float XKm = WorldLocation.X / 1000.0f;
    if (XKm < 0.0f || XKm > MapSizeKilometers) return FName(TEXT("Unknown"));
    if (XKm < MapSizeKilometers * 0.34f) return FName(TEXT("Forest"));
    if (XKm < MapSizeKilometers * 0.68f) return FName(TEXT("Sand"));
    return FName(TEXT("Snow"));
}

void AForestSliceProceduralForest::InitializeMapLayout()
{
    if (MapLandmarks.Num() > 0) return;
    const auto AddLandmark = [this](const TCHAR* Id, const TCHAR* Biome, float XKm, float YKm) {
        FForestSliceMapLandmark Landmark;
        Landmark.Id = FName(Id);
        Landmark.Biome = FName(Biome);
        Landmark.CoordinateKm = FVector2D(XKm, YKm);
        MapLandmarks.Add(Landmark);
    };
    AddLandmark(TEXT("ForestCamp"), TEXT("Forest"), 10.0f, 16.0f);
    AddLandmark(TEXT("FarmingVillage"), TEXT("Forest"), 14.0f, 40.0f);
    AddLandmark(TEXT("MossCave"), TEXT("Forest"), 26.0f, 78.0f);
    AddLandmark(TEXT("SandGate"), TEXT("Sand"), 40.0f, 48.0f);
    AddLandmark(TEXT("SunKiln"), TEXT("Sand"), 47.0f, 28.0f);
    AddLandmark(TEXT("Oasis"), TEXT("Sand"), 55.0f, 69.0f);
    AddLandmark(TEXT("FrostGate"), TEXT("Snow"), 74.0f, 57.0f);
    AddLandmark(TEXT("PredatorBasin"), TEXT("Snow"), 84.0f, 59.0f);
    AddLandmark(TEXT("FrostclawArena"), TEXT("Snow"), 88.0f, 83.0f);
}

void AForestSliceProceduralForest::RegenerateWorld(int32 NewSeed)
{
    WorldSeed = NewSeed;
    ChunkPresentationTime = 0.0f;
    LoadedChunks.Empty();
    RemoveAllInstances();
    GenerateAroundLocation(GetActorLocation());
}
