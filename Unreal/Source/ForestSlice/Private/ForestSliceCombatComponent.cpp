#include "ForestSliceCombatComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "Kismet/GameplayStatics.h"
#include "ForestSliceWeaponComponent.h"

UForestSliceCombatComponent::UForestSliceCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);

    LightCombo = {
        {TEXT("Light_01"), 0.08f, 0.10f, 0.26f, 0.18f, 10.0f, 180.0f, 10.0f, 8.0f, 120.0f, 45.0f, 36.0f, TEXT("Light_01")},
        {TEXT("Light_02"), 0.10f, 0.11f, 0.28f, 0.20f, 12.0f, 190.0f, 13.0f, 10.0f, 145.0f, 55.0f, 40.0f, TEXT("Light_02")},
        {TEXT("Light_03"), 0.14f, 0.14f, 0.38f, 0.24f, 16.0f, 220.0f, 20.0f, 16.0f, 220.0f, 70.0f, 48.0f, TEXT("Light_03")}
    };
    HeavyAttack = {TEXT("Heavy_01"), 0.32f, 0.18f, 0.58f, 0.0f, 28.0f, 240.0f, 38.0f, 28.0f, 320.0f, 90.0f, 54.0f, TEXT("Heavy_01")};
    WeaponIds = {TEXT("Blade"), TEXT("Greatblade"), TEXT("Bow"), TEXT("GatheringTool")};
}

void UForestSliceCombatComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UForestSliceCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ComboBufferTimer = FMath::Max(0.0f, ComboBufferTimer - DeltaTime);
    if (CombatPhase == EForestSliceCombatPhase::None) {
        if (bQueuedAttack) {
            const bool bContinueCombo = ComboBufferTimer > 0.0f && !bHeavyAttack && !LightCombo.IsEmpty();
            ComboIndex = bContinueCombo ? (ComboIndex + 1) % LightCombo.Num() : 0;
            BeginAttack(bHeavyAttack);
            bQueuedAttack = false;
        }
        return;
    }

    PhaseTimer -= DeltaTime;
    if (CombatPhase == EForestSliceCombatPhase::Startup && PhaseTimer <= 0.0f) {
        CombatPhase = EForestSliceCombatPhase::Active;
        PhaseTimer = GetCurrentAttack() ? GetCurrentAttack()->ActiveSeconds : 0.0f;
        bHitResolved = false;
        ResolveActiveHit();
    } else if (CombatPhase == EForestSliceCombatPhase::Active && PhaseTimer <= 0.0f) {
        CombatPhase = EForestSliceCombatPhase::Recovery;
        PhaseTimer = GetCurrentAttack() ? GetCurrentAttack()->RecoverySeconds : 0.0f;
        ComboBufferTimer = GetCurrentAttack() ? GetCurrentAttack()->ComboBufferSeconds : 0.0f;
    } else if (CombatPhase == EForestSliceCombatPhase::Recovery && PhaseTimer <= 0.0f) {
        FinishAttack();
    }
}

void UForestSliceCombatComponent::RequestLightAttack()
{
    if (CombatPhase == EForestSliceCombatPhase::None ||
        (CombatPhase == EForestSliceCombatPhase::Recovery && ComboBufferTimer > 0.0f)) {
        if (GetOwner()->HasAuthority()) {
            bHeavyAttack = false;
            bQueuedAttack = true;
        } else {
            ServerRequestAttack(false);
        }
    }
}

void UForestSliceCombatComponent::RequestHeavyAttack()
{
    if (CombatPhase == EForestSliceCombatPhase::None) {
        if (GetOwner()->HasAuthority()) {
            bHeavyAttack = true;
            bQueuedAttack = true;
        } else {
            ServerRequestAttack(true);
        }
    }
}

void UForestSliceCombatComponent::SwitchWeapon(int32 NewWeaponIndex)
{
    if (!WeaponIds.IsValidIndex(NewWeaponIndex) || NewWeaponIndex == EquippedWeaponIndex) return;
    if (!GetOwner()->HasAuthority()) {
        ServerSwitchWeapon(NewWeaponIndex);
        return;
    }

    bQueuedAttack = false;
    CombatPhase = EForestSliceCombatPhase::None;
    ComboBufferTimer = 0.0f;
    ComboIndex = 0;
    EquippedWeaponIndex = NewWeaponIndex;
    CombatEvent.Broadcast(TEXT("WeaponSwitched"), ComboIndex, 0.0f, EquippedWeaponIndex);
}

void UForestSliceCombatComponent::ServerRequestAttack_Implementation(bool bHeavy)
{
    if (bHeavy) RequestHeavyAttack();
    else RequestLightAttack();
}

void UForestSliceCombatComponent::ServerSwitchWeapon_Implementation(int32 NewWeaponIndex)
{
    SwitchWeapon(NewWeaponIndex);
}

void UForestSliceCombatComponent::BeginAttack(bool bHeavy)
{
    const FForestSliceAttackDefinition* Attack = bHeavy ? &HeavyAttack : GetCurrentAttack();
    if (!Attack) return;
    CombatPhase = EForestSliceCombatPhase::Startup;
    PhaseTimer = Attack->StartupSeconds;

    if (ACharacter* Character = Cast<ACharacter>(GetOwner())) {
        const FVector Lunge = Character->GetActorForwardVector() * Attack->LungeDistance;
        if (Attack->LungeDistance > 0.0f && Character->GetCharacterMovement()->IsMovingOnGround()) {
            Character->LaunchCharacter(Lunge, true, false);
        }
    }
    CombatEvent.Broadcast(Attack->AttackId, ComboIndex, 0.0f, EquippedWeaponIndex);
}

void UForestSliceCombatComponent::ResolveActiveHit()
{
    const FForestSliceAttackDefinition* Attack = GetCurrentAttack();
    if (bHeavyAttack) Attack = &HeavyAttack;
    if (!Attack || bHitResolved || !GetOwner()->HasAuthority()) return;

    bHitResolved = true;
    AActor* OwnerActor = GetOwner();
    ACharacter* Character = Cast<ACharacter>(OwnerActor);
    if (!Character || !GetWorld()) return;

    const FVector Forward = Character->GetActorForwardVector();
    const FVector Start = OwnerActor->GetActorLocation() + Forward * FMath::Min(Attack->Range * 0.35f, 80.0f);
    const FVector End = OwnerActor->GetActorLocation() + Forward * Attack->Range;
    const FCollisionShape Shape = FCollisionShape::MakeSphere(Attack->HitRadius);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ForestSliceAttack), false, OwnerActor);
    QueryParams.AddIgnoredActor(OwnerActor);

    TArray<FHitResult> Hits;
    const bool bHitAnything = GetWorld()->SweepMultiByChannel(
        Hits,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        Shape,
        QueryParams
    );

    const UForestSliceWeaponComponent* WeaponComponent = OwnerActor->FindComponentByClass<UForestSliceWeaponComponent>();
    const FForestSliceWeaponDefinition Weapon = WeaponComponent
        ? WeaponComponent->GetEquippedDefinition()
        : FForestSliceWeaponDefinition{};
    const float DamageMultiplier = bHeavyAttack ? Weapon.HeavyDamageMultiplier : Weapon.LightDamageMultiplier;
    const float Damage = Attack->Damage * FMath::Max(0.0f, DamageMultiplier);

    if (bHitAnything) {
        TSet<AActor*> UniqueActors;
        for (const FHitResult& Hit : Hits) {
            AActor* HitActor = Hit.GetActor();
            if (!IsValid(HitActor) || UniqueActors.Contains(HitActor)) continue;
            UniqueActors.Add(HitActor);
            UGameplayStatics::ApplyDamage(HitActor, Damage, Character->GetController(), OwnerActor, nullptr);
            CombatEvent.Broadcast(TEXT("HitConfirmed"), ComboIndex, Damage, EquippedWeaponIndex);
        }
    } else {
        CombatEvent.Broadcast(TEXT("AttackWhiff"), ComboIndex, 0.0f, EquippedWeaponIndex);
    }
}

float UForestSliceCombatComponent::GetMovementSpeedScale() const
{
    switch (CombatPhase) {
        case EForestSliceCombatPhase::Startup:
            return bHeavyAttack ? 0.25f : 0.55f;
        case EForestSliceCombatPhase::Active:
            return 0.10f;
        case EForestSliceCombatPhase::Recovery:
            return 0.45f;
        case EForestSliceCombatPhase::Stagger:
        case EForestSliceCombatPhase::Downed:
        case EForestSliceCombatPhase::Dead:
            return 0.0f;
        case EForestSliceCombatPhase::None:
        default:
            return 1.0f;
    }
}

void UForestSliceCombatComponent::FinishAttack()
{
    CombatPhase = EForestSliceCombatPhase::None;
    if (!bQueuedAttack && ComboBufferTimer <= 0.0f) ComboIndex = 0;
    CombatEvent.Broadcast(TEXT("AttackFinished"), ComboIndex, 0.0f, EquippedWeaponIndex);
}

const FForestSliceAttackDefinition* UForestSliceCombatComponent::GetCurrentAttack() const
{
    return LightCombo.IsValidIndex(ComboIndex) ? &LightCombo[ComboIndex] : nullptr;
}

void UForestSliceCombatComponent::OnRep_CombatPhase()
{
    CombatEvent.Broadcast(TEXT("CombatPhaseChanged"), ComboIndex, 0.0f, EquippedWeaponIndex);
}

void UForestSliceCombatComponent::OnRep_EquippedWeapon()
{
    CombatEvent.Broadcast(TEXT("WeaponChanged"), ComboIndex, 0.0f, EquippedWeaponIndex);
}

void UForestSliceCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceCombatComponent, CombatPhase);
    DOREPLIFETIME(UForestSliceCombatComponent, ComboIndex);
    DOREPLIFETIME(UForestSliceCombatComponent, EquippedWeaponIndex);
}
