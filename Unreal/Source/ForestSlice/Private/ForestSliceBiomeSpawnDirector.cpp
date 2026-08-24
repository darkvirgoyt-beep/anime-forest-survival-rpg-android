#include "ForestSliceBiomeSpawnDirector.h"

#include "ForestSliceEncounterDirectorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace ForestSliceBiomeSpawning
{
    EForestSliceEncounterBiome ToEncounterBiome(EForestSliceOriginalBiome Biome)
    {
        switch (Biome)
        {
        case EForestSliceOriginalBiome::FrostwakeCrown: return EForestSliceEncounterBiome::Snow;
        case EForestSliceOriginalBiome::ShardwaterCoast: return EForestSliceEncounterBiome::Sand;
        case EForestSliceOriginalBiome::EmberfallHollow: return EForestSliceEncounterBiome::Ruins;
        default: return EForestSliceEncounterBiome::Forest;
        }
    }
}

AForestSliceBiomeSpawnDirector::AForestSliceBiomeSpawnDirector()
{
    bReplicates = true;
    bNetLoadOnClient = false;
    PrimaryActorTick.bCanEverTick = false;
    EncounterDirector = CreateDefaultSubobject<UForestSliceEncounterDirectorComponent>(TEXT("EncounterDirector"));
}

void AForestSliceBiomeSpawnDirector::BeginPlay()
{
    Super::BeginPlay();
    if (!HasAuthority())
    {
        return;
    }

    GetWorldTimerManager().SetTimer(SpawnTimer, this, &AForestSliceBiomeSpawnDirector::TickSpawning, SpawnCheckSeconds, true, 1.5f);
}

void AForestSliceBiomeSpawnDirector::SetAuthoritativePlayerAnchors(const TArray<FVector>& InAnchors)
{
    if (HasAuthority())
    {
        PlayerAnchors = InAnchors;
    }
}

void AForestSliceBiomeSpawnDirector::RefreshSpawnBudget(float WorldTimeHours, float WeatherIntensity, int32 GraphicsQuality)
{
    if (!HasAuthority())
    {
        return;
    }

    const EForestSliceOriginalBiome Biome = GetBiomeForLocation(GetActorLocation());
    EncounterDirector->RebuildBudget(ForestSliceBiomeSpawning::ToEncounterBiome(Biome), WorldTimeHours, WeatherIntensity, GraphicsQuality);
}

int32 AForestSliceBiomeSpawnDirector::GetActiveCreatureCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AForestSliceWildCreature>& Creature : ActiveCreatures)
    {
        Count += Creature.IsValid() ? 1 : 0;
    }
    return Count;
}

void AForestSliceBiomeSpawnDirector::TickSpawning()
{
    if (!HasAuthority() || PlayerAnchors.IsEmpty())
    {
        return;
    }

    PruneInactiveCreatures();
    TrySpawnOne();
}

void AForestSliceBiomeSpawnDirector::PruneInactiveCreatures()
{
    ActiveCreatures.RemoveAll([](const TWeakObjectPtr<AForestSliceWildCreature>& Creature) { return !Creature.IsValid(); });
}

bool AForestSliceBiomeSpawnDirector::TrySpawnOne()
{
    if (!EncounterDirector || !EncounterDirector->CanSpawn(ActiveCreatures.Num(), false))
    {
        return false;
    }

    FRandomStream Stream(OriginalWorldSeed + SpawnRevision * 4051);
    FVector SpawnLocation;
    if (!FindSafeSpawnPoint(Stream, SpawnLocation))
    {
        return false;
    }

    const EForestSliceOriginalBiome Biome = GetBiomeForLocation(SpawnLocation);
    const FForestSliceBiomeSpawnProfile* Profile = SelectProfile(Biome, Stream);
    if (!Profile || !Profile->CreatureClass)
    {
        return false;
    }
    if (Profile->bElite && GetActiveEliteCount() >= EncounterDirector->GetBudget().EliteSlots)
    {
        return false;
    }

    const int32 FreeSlots = EncounterDirector->GetBudget().ActiveCreatureLimit - ActiveCreatures.Num();
    const int32 RequestedGroupSize = Profile->bElite || Profile->Role == EForestSliceCreatureRole::Boss ? 1 : Profile->GroupSize;
    const int32 SpawnCount = FMath::Clamp(FMath::Min(RequestedGroupSize, FreeSlots), 0, 8);
    bool bSpawnedAny = false;

    for (int32 MemberIndex = 0; MemberIndex < SpawnCount; ++MemberIndex)
    {
        const float MemberAngle = Stream.FRandRange(0.0f, 2.0f * PI);
        const float MemberRadius = MemberIndex == 0 ? 0.0f : Stream.FRandRange(120.0f, 420.0f);
        const FVector MemberLocation = SpawnLocation + FVector(FMath::Cos(MemberAngle) * MemberRadius, FMath::Sin(MemberAngle) * MemberRadius, 0.0f);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = this;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AForestSliceWildCreature* Creature = GetWorld()->SpawnActor<AForestSliceWildCreature>(
            Profile->CreatureClass,
            MemberLocation,
            FRotator(0.0f, Stream.FRandRange(0.0f, 360.0f), 0.0f),
            SpawnParameters);
        if (!Creature)
        {
            continue;
        }

        ++SpawnRevision;
        Creature->InitializeFromSpawn(Profile->SpeciesId, Profile->Disposition, Profile->Role, Profile->bCaptureCandidate, Profile->bElite, SpawnRevision);
        ActiveCreatures.Add(Creature);
        CreatureSpawned.Broadcast(Creature, Profile->SpeciesId);
        bSpawnedAny = true;
    }
    return bSpawnedAny;
}

int32 AForestSliceBiomeSpawnDirector::GetActiveEliteCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AForestSliceWildCreature>& Creature : ActiveCreatures)
    {
        if (Creature.IsValid() && Creature->GetCreatureState().bElite)
        {
            ++Count;
        }
    }
    return Count;
}

const FForestSliceBiomeSpawnProfile* AForestSliceBiomeSpawnDirector::SelectProfile(EForestSliceOriginalBiome Biome, FRandomStream& Stream) const
{
    float TotalWeight = 0.0f;
    for (const FForestSliceBiomeSpawnProfile& Profile : SpawnProfiles)
    {
        if (Profile.CreatureClass && Profile.AllowedBiomes.Contains(Biome))
        {
            TotalWeight += Profile.Weight;
        }
    }
    if (TotalWeight <= 0.0f)
    {
        return nullptr;
    }

    float Selection = Stream.FRandRange(0.0f, TotalWeight);
    for (const FForestSliceBiomeSpawnProfile& Profile : SpawnProfiles)
    {
        if (!Profile.CreatureClass || !Profile.AllowedBiomes.Contains(Biome))
        {
            continue;
        }
        Selection -= Profile.Weight;
        if (Selection <= 0.0f)
        {
            return &Profile;
        }
    }
    return nullptr;
}

bool AForestSliceBiomeSpawnDirector::FindSafeSpawnPoint(FRandomStream& Stream, FVector& OutLocation) const
{
    const UWorld* World = GetWorld();
    if (!World || PlayerAnchors.IsEmpty())
    {
        return false;
    }

    for (int32 Attempt = 0; Attempt < 12; ++Attempt)
    {
        const FVector& Anchor = PlayerAnchors[Stream.RandRange(0, PlayerAnchors.Num() - 1)];
        const float AngleRadians = Stream.FRandRange(0.0f, 2.0f * PI);
        const float Distance = Stream.FRandRange(MinimumSpawnDistance, MaximumSpawnDistance);
        const FVector Horizontal = FVector(FMath::Cos(AngleRadians) * Distance, FMath::Sin(AngleRadians) * Distance, 0.0f);
        const FVector TraceStart = Anchor + Horizontal + FVector(0.0f, 0.0f, 9000.0f);
        const FVector TraceEnd = Anchor + Horizontal - FVector(0.0f, 0.0f, 9000.0f);
        FHitResult Hit;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AethelgardCreatureSpawn), false, this);
        if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams) &&
            FVector::Dist2D(Hit.ImpactPoint, Anchor) >= MinimumSpawnDistance)
        {
            OutLocation = Hit.ImpactPoint + FVector(0.0f, 0.0f, 35.0f);
            return true;
        }
    }
    return false;
}

EForestSliceOriginalBiome AForestSliceBiomeSpawnDirector::GetBiomeForLocation(const FVector& Location) const
{
    const FVector Origin = GetActorLocation();
    const float NormalizedX = FMath::Clamp((Location.X - Origin.X + 400000.0f) / 800000.0f, 0.0f, 1.0f);
    const float NormalizedY = FMath::Clamp((Location.Y - Origin.Y + 400000.0f) / 800000.0f, 0.0f, 1.0f);
    return UForestSliceOriginalWorldGenerator::SelectOriginalBiome(NormalizedX, NormalizedY);
}
