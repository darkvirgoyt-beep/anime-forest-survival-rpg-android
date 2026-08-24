#include "ForestSliceMob.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AForestSliceMob::AForestSliceMob()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 72.0f);
    GetCharacterMovement()->MaxWalkSpeed = Definition.MoveSpeed;
    GetCharacterMovement()->MaxAcceleration = 1500.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1200.0f;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    bUseControllerRotationYaw = false;

    Definition = FForestSliceMobDefinition{};
}

void AForestSliceMob::BeginPlay()
{
    Super::BeginPlay();
    HomeLocation = GetActorLocation();
    CurrentHealth = Definition.MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = Definition.MoveSpeed;

    if (MobSkeletalMesh) {
        GetMesh()->SetSkeletalMesh(MobSkeletalMesh);
    }
    if (AnimationClass) {
        GetMesh()->SetAnimInstanceClass(AnimationClass);
    }

    if (HasAuthority()) {
        ChooseRoamTarget();
    }
}

void AForestSliceMob::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || MobState == EForestSliceMobState::Dead) return;
    TickAuthority(FMath::Min(DeltaSeconds, 0.10f));
}

void AForestSliceMob::TickAuthority(float DeltaSeconds)
{
    CooldownTimer = FMath::Max(0.0f, CooldownTimer - DeltaSeconds);
    AttackTimer = FMath::Max(0.0f, AttackTimer - DeltaSeconds);
    StaggerTimer = FMath::Max(0.0f, StaggerTimer - DeltaSeconds);
    RoamTimer = FMath::Max(0.0f, RoamTimer - DeltaSeconds);

    if (MobState == EForestSliceMobState::Stagger) {
        if (StaggerTimer <= 0.0f) MobState = EForestSliceMobState::Chase;
        return;
    }
    if (MobState == EForestSliceMobState::AttackWindup) {
        if (AttackTimer <= 0.0f) {
            ResolveAttack();
            MobState = EForestSliceMobState::AttackRecovery;
            AttackTimer = Definition.AttackRecovery;
        }
        return;
    }
    if (MobState == EForestSliceMobState::AttackRecovery) {
        if (AttackTimer <= 0.0f) {
            MobState = CombatTarget ? EForestSliceMobState::Chase : EForestSliceMobState::Roam;
            CooldownTimer = Definition.AttackCooldown;
        }
        return;
    }

    UpdateTarget();
    UpdateState(DeltaSeconds);
}

void AForestSliceMob::UpdateTarget()
{
    if (!IsValid(CombatTarget)) CombatTarget = nullptr;
    if (CombatTarget) {
        const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), CombatTarget->GetActorLocation());
        if (DistanceSquared > FMath::Square(Definition.AggroRange * 1.75f)) CombatTarget = nullptr;
    }
    if (CombatTarget || Definition.bPassiveUntilProvoked) return;

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (IsValid(PlayerPawn) && FVector::DistSquared2D(GetActorLocation(), PlayerPawn->GetActorLocation()) <= FMath::Square(Definition.AggroRange)) {
        CombatTarget = PlayerPawn;
    }
}

void AForestSliceMob::UpdateState(float DeltaSeconds)
{
    if (!IsAlive()) {
        MobState = EForestSliceMobState::Dead;
        return;
    }

    if (GetHealthNormalized() <= Definition.FleeHealthThreshold && Definition.Archetype != EForestSliceMobArchetype::EmberWarden) {
        MobState = EForestSliceMobState::Flee;
        if (CombatTarget) {
            const FVector Away = (GetActorLocation() - CombatTarget->GetActorLocation()).GetSafeNormal2D();
            MoveToward(GetActorLocation() + Away * Definition.AggroRange, 1.0f);
        }
        return;
    }

    if (CombatTarget) {
        const float Distance = FVector::Dist2D(GetActorLocation(), CombatTarget->GetActorLocation());
        if (Distance <= Definition.AttackRange && CooldownTimer <= 0.0f) {
            BeginAttack();
            return;
        }
        MobState = EForestSliceMobState::Chase;
        MoveToward(CombatTarget->GetActorLocation());
        return;
    }

    MobState = EForestSliceMobState::Roam;
    if (RoamTimer <= 0.0f || FVector::DistSquared2D(GetActorLocation(), RoamTarget) < FMath::Square(90.0f)) {
        ChooseRoamTarget();
    }
    MoveToward(RoamTarget, 0.65f);
}

void AForestSliceMob::MoveToward(const FVector& Destination, float Scale)
{
    FVector Direction = Destination - GetActorLocation();
    Direction.Z = 0.0f;
    Direction = Direction.GetSafeNormal();
    if (Direction.IsNearlyZero()) return;

    AddMovementInput(Direction, FMath::Clamp(Scale, 0.0f, 1.0f));
    const FRotator DesiredRotation = Direction.Rotation();
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, GetWorld()->GetDeltaSeconds(), 7.0f));
}

void AForestSliceMob::ChooseRoamTarget()
{
    const FVector Offset = FVector(FMath::FRandRange(-RoamRadius, RoamRadius), FMath::FRandRange(-RoamRadius, RoamRadius), 0.0f);
    RoamTarget = HomeLocation + Offset;
    RoamTimer = RoamRetargetSeconds;
}

void AForestSliceMob::BeginAttack()
{
    MobState = EForestSliceMobState::AttackWindup;
    AttackTimer = Definition.AttackWindup;
    BroadcastEvent(TEXT("AttackWindup"));
}

void AForestSliceMob::ResolveAttack()
{
    if (!IsValid(CombatTarget) || !GetWorld()) return;

    const FVector Forward = GetActorForwardVector();
    const FVector Start = GetActorLocation() + Forward * (Definition.AttackRange * 0.35f);
    const FVector End = GetActorLocation() + Forward * Definition.AttackRange;
    const FCollisionShape Shape = FCollisionShape::MakeSphere(48.0f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ForestSliceMobAttack), false, this);
    Params.AddIgnoredActor(this);

    TArray<FHitResult> Hits;
    GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, Shape, Params);
    for (const FHitResult& Hit : Hits) {
        if (Hit.GetActor() == CombatTarget) {
            UGameplayStatics::ApplyDamage(CombatTarget, Definition.AttackDamage, GetController(), this, nullptr);
            BroadcastEvent(TEXT("AttackHit"));
            return;
        }
    }
    BroadcastEvent(TEXT("AttackWhiff"));
}

float AForestSliceMob::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (!HasAuthority() || !IsAlive()) return 0.0f;
    const float AppliedDamage = FMath::Max(0.0f, DamageAmount);
    CurrentHealth = FMath::Max(0.0f, CurrentHealth - AppliedDamage);
    CombatTarget = IsValid(EventInstigator) ? EventInstigator->GetPawn() : DamageCauser;

    if (CurrentHealth <= 0.0f) {
        Die(EventInstigator);
    } else {
        MobState = EForestSliceMobState::Stagger;
        StaggerTimer = StaggerSeconds;
        BroadcastEvent(TEXT("Staggered"));
    }
    return AppliedDamage;
}

void AForestSliceMob::Provoke(AActor* InstigatorActor)
{
    if (!HasAuthority() || !IsAlive()) return;
    CombatTarget = InstigatorActor;
    bProvoked = true;
    MobState = EForestSliceMobState::Chase;
    BroadcastEvent(TEXT("Provoked"));
}

void AForestSliceMob::Die(AController* Killer)
{
    MobState = EForestSliceMobState::Dead;
    GetCharacterMovement()->DisableMovement();
    SetLifeSpan(8.0f);
    BroadcastEvent(TEXT("Death"));
    MobCombatEvent.Broadcast(TEXT("LootReady"), Definition.MobId, 0.0f, GetActorLocation(), Definition.ExperienceReward);
}

void AForestSliceMob::BroadcastEvent(FName EventId)
{
    MobCombatEvent.Broadcast(EventId, Definition.MobId, GetHealthNormalized(), GetActorLocation(), Definition.ExperienceReward);
}

float AForestSliceMob::GetHealthNormalized() const
{
    return Definition.MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / Definition.MaxHealth, 0.0f, 1.0f) : 0.0f;
}

void AForestSliceMob::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AForestSliceMob, CurrentHealth);
    DOREPLIFETIME(AForestSliceMob, MobState);
}
