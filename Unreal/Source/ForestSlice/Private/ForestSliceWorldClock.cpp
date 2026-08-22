#include "ForestSliceWorldClock.h"

#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

AForestSliceWorldClock::AForestSliceWorldClock()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
}

void AForestSliceWorldClock::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || RealSecondsPerGameHour <= 0.0f) return;
    AdvanceTime(DeltaSeconds / RealSecondsPerGameHour);
}

void AForestSliceWorldClock::AdvanceTime(float Hours)
{
    if (!HasAuthority()) return;
    WorldTimeHours = FMath::Fmod(WorldTimeHours + FMath::Max(0.0f, Hours), 24.0f);
    if (WorldTimeHours < 0.0f) WorldTimeHours += 24.0f;
}

float AForestSliceWorldClock::GetDayAlpha() const
{
    return WorldTimeHours / 24.0f;
}

bool AForestSliceWorldClock::IsNight() const
{
    return WorldTimeHours >= NightStartHour || WorldTimeHours < NightEndHour;
}

bool AForestSliceWorldClock::IsValidSleepRequest(AActor* Bed, APawn* RequestingPawn) const
{
    if (!Bed || !RequestingPawn || !IsNight()) return false;
    if (RequestingPawn->GetVelocity().SizeSquared() > 25.0f) return false;
    if (FVector::DistSquared(Bed->GetActorLocation(), RequestingPawn->GetActorLocation()) > FMath::Square(250.0f)) return false;
    return true;
}

bool AForestSliceWorldClock::RequestSleep(AActor* Bed, APawn* RequestingPawn, float SleepUntilHour)
{
    if (!HasAuthority() || !IsValidSleepRequest(Bed, RequestingPawn)) return false;
    const float TargetHour = FMath::Clamp(SleepUntilHour, 0.0f, 23.99f);
    const float HoursToAdvance = WorldTimeHours < TargetHour
        ? TargetHour - WorldTimeHours
        : (24.0f - WorldTimeHours) + TargetHour;
    AdvanceTime(HoursToAdvance);
    return true;
}

void AForestSliceWorldClock::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AForestSliceWorldClock, WorldTimeHours);
}
