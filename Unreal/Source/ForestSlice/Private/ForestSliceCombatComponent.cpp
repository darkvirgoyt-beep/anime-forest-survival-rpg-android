#include "ForestSliceCombatComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "ForestSliceHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "ForestSliceSurvivalComponent.h"
#include "ForestSliceWeaponComponent.h"

UForestSliceCombatComponent::UForestSliceCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);

    LightCombo = {
        {TEXT("Light_01"), 0.08f, 0.10f, 0.26f, 0.18f, 10.0f, 180.0f, 10.0f, 8.0f, 120.0f, TEXT("Light_01")},
        {TEXT("Light_02"), 0.10f, 0.11f, 0.28f, 0.20f, 12.0f, 190.0f, 13.0f, 10.0f, 145.0f, TEXT("Light_02")},
        {TEXT("Light_03"), 0.14f, 0.14f, 0.38f, 0.24f, 16.0f, 220.0f, 20.0f, 16.0f, 220.0f, TEXT("Light_03")}
    };
    HeavyAttack = {TEXT("Heavy_01"), 0.32f, 0.18f, 0.58f, 0.0f, 28.0f, 240.0f, 38.0f, 28.0f, 320.0f, TEXT("Heavy_01")};
    WeaponIds = {TEXT("Blade"), TEXT("Greatblade"), TEXT("Bow"), TEXT("GatheringTool")};
}

void UForestSliceCombatComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UForestSliceCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (const UForestSliceHealthComponent* Health = GetOwner() ? GetOwner()->FindComponentByClass<UForestSliceHealthComponent>() : nullptr)
    {
        if (!Health->IsAlive())
        {
            bQueuedAttack = false;
            CombatPhase = EForestSliceCombatPhase::Dead;
            return;
        }
        if (Health->GetState().bDowned)
        {
            bQueuedAttack = false;
            CombatPhase = EForestSliceCombatPhase::Downed;
            return;
        }
        if (CombatPhase == EForestSliceCombatPhase::Downed || CombatPhase == EForestSliceCombatPhase::Dead)
        {
            CombatPhase = EForestSliceCombatPhase::None;
        }
    }

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
    CurrentAttackId = NAME_None;

    if (UForestSliceWeaponComponent* Weapon = GetOwner()->FindComponentByClass<UForestSliceWeaponComponent>())
    {
        Weapon->RequestSwitchToSlot(NewWeaponIndex);
        if (Weapon->GetEquippedSlot() != NewWeaponIndex)
        {
            return;
        }
    }

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
    if (const UForestSliceHealthComponent* Health = GetOwner() ? GetOwner()->FindComponentByClass<UForestSliceHealthComponent>() : nullptr)
    {
        if (!Health->IsAlive() || Health->GetState().bDowned)
        {
            return;
        }
    }

    const FForestSliceAttackDefinition* Attack = bHeavy ? &HeavyAttack : GetCurrentAttack();
    if (!Attack) return;
    if (GetOwner()->HasAuthority()) {
        if (UForestSliceSurvivalComponent* Survival = GetOwner()->FindComponentByClass<UForestSliceSurvivalComponent>()) {
            if (!Survival->ConsumeStamina(Attack->StaminaCost)) return;
        }
    }
    CombatPhase = EForestSliceCombatPhase::Startup;
    PhaseTimer = Attack->StartupSeconds;
    CurrentAttackId = Attack->AttackId;
    CombatEvent.Broadcast(Attack->AttackId, ComboIndex, 0.0f, EquippedWeaponIndex);
}

void UForestSliceCombatComponent::ResolveActiveHit()
{
    const FForestSliceAttackDefinition* Attack = GetCurrentAttack();
    if (bHeavyAttack) Attack = &HeavyAttack;
    if (!Attack || bHitResolved) return;

    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    const UForestSliceWeaponComponent* Weapon = GetOwner()->FindComponentByClass<UForestSliceWeaponComponent>();
    const FForestSliceWeaponDefinition WeaponDefinition = Weapon ? Weapon->GetEquippedDefinition() : FForestSliceWeaponDefinition{};
    const float DamageMultiplier = bHeavyAttack ? WeaponDefinition.HeavyDamageMultiplier : WeaponDefinition.LightDamageMultiplier;
    const float ResolvedDamage = FMath::Max(0.0f, Attack->Damage * FMath::Max(0.0f, DamageMultiplier));
    const FVector Start = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
    const FVector End = Start + GetOwner()->GetActorForwardVector() * Attack->Range;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ForestSliceAttack), false, GetOwner());
    TArray<FHitResult> Hits;
    const bool bAnyHit = GetWorld() && GetWorld()->SweepMultiByChannel(
        Hits,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(46.0f),
        QueryParams);

    TSet<AActor*> DamagedActors;
    if (bAnyHit) {
        for (const FHitResult& Hit : Hits) {
            AActor* HitActor = Hit.GetActor();
            if (!IsValid(HitActor) || HitActor == GetOwner() || DamagedActors.Contains(HitActor)) continue;
            UForestSliceHealthComponent* Health = HitActor->FindComponentByClass<UForestSliceHealthComponent>();
            DamagedActors.Add(HitActor);
            const FVector Impulse = GetOwner()->GetActorForwardVector() * Attack->Knockback;
            if (Health)
            {
                Health->ApplyDamage(ResolvedDamage, Attack->PoiseDamage, Impulse, Attack->AttackId);
            }
            else
            {
                FDamageEvent DamageEvent;
                HitActor->TakeDamage(ResolvedDamage, DamageEvent, GetOwner()->GetInstigatorController(), GetOwner());
            }
        }
    }

    bHitResolved = true;
    CombatEvent.Broadcast(bAnyHit ? TEXT("HitConfirmed") : TEXT("HitWindow"), ComboIndex, ResolvedDamage, EquippedWeaponIndex);
}

void UForestSliceCombatComponent::FinishAttack()
{
    CombatPhase = EForestSliceCombatPhase::None;
    CurrentAttackId = NAME_None;
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

void UForestSliceCombatComponent::OnRep_AttackPresentation()
{
    if (CurrentAttackId != NAME_None) {
        CombatEvent.Broadcast(CurrentAttackId, ComboIndex, 0.0f, EquippedWeaponIndex);
    }
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
    DOREPLIFETIME(UForestSliceCombatComponent, CurrentAttackId);
    DOREPLIFETIME(UForestSliceCombatComponent, EquippedWeaponIndex);
}
