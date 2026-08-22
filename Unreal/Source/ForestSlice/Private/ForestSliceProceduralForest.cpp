#include "ForestSliceProceduralForest.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

AForestSliceProceduralForest::AForestSliceProceduralForest()
{
    PrimaryActorTick.bCanEverTick = true;
    TreeInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreeInstances"));
    TreeInstances->SetupAttachment(RootComponent);
    RockInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RockInstances"));
    RockInstances->SetupAttachment(RootComponent);
}

void AForestSliceProceduralForest::BeginPlay()
{
    Super::BeginPlay();
    if (TreeMesh) TreeInstances->SetStaticMesh(TreeMesh);
    if (RockMesh) RockInstances->SetStaticMesh(RockMesh);
    GenerateAroundLocation(GetActorLocation());
}

void AForestSliceProceduralForest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
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

    const FVector Origin(Coordinate.X * ChunkSize, Coordinate.Y * ChunkSize, GetActorLocation().Z);
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
    if (TreeInstances) TreeInstances->ClearInstances();
    if (RockInstances) RockInstances->ClearInstances();
}

void AForestSliceProceduralForest::RebuildInstances()
{
    RemoveAllInstances();
    for (const TPair<FIntPoint, FForestSliceChunkRecord>& Pair : LoadedChunks) {
        if (TreeInstances && TreeMesh) TreeInstances->AddInstances(Pair.Value.TreeTransforms, false, true, true);
        if (RockInstances && RockMesh) RockInstances->AddInstances(Pair.Value.RockTransforms, false, true, true);
    }
}

void AForestSliceProceduralForest::RegenerateWorld(int32 NewSeed)
{
    WorldSeed = NewSeed;
    LoadedChunks.Empty();
    RemoveAllInstances();
    GenerateAroundLocation(GetActorLocation());
}
