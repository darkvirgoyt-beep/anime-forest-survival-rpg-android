#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ForestSliceMob.generated.h"

class USkeletalMesh;
class UAnimInstance;

UENUM(BlueprintType)
enum class EForestSliceMobArchetype : uint8
{
    Thornfang,
    Mossback,
    EmberWarden
};

UENUM(BlueprintType)
enum class EForestSliceMobState : uint8
{
    Idle,
    Roam,
    Chase,
    AttackWindup,
    AttackRecovery,
    Flee,
    Stagger,
    Dead
};

USTRUCT(BlueprintType)
struct FForestSliceMobDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    FName MobId = TEXT("Thornfang");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    EForestSliceMobArchetype Archetype = EForestSliceMobArchetype::Thornfang;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    FText DisplayName = FText::FromString(TEXT("Thornfang"));

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    float MaxHealth = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    float MoveSpeed = 310.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    float AggroRange = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    float AttackRange = 170.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    float AttackDamage = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    float AttackCooldown = 1.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    float AttackWindup = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    float AttackRecovery = 0.70f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    float FleeHealthThreshold = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    int32 ExperienceReward = 25;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    FName LootTableId = TEXT("loot.thornfang");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    bool bPassiveUntilProvoked = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
    FForestSliceMobCombatEvent,
    FName, EventId,
    FName, MobId,
    float, HealthNormalized,
    FVector, WorldLocation,
    int32, ExperienceReward
);

UCLASS(BlueprintType, Blueprintable)
class FORESTSLICE_API AForestSliceMob : public ACharacter
{
    GENERATED_BODY()

public:
    AForestSliceMob();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual float TakeDamage(
        float DamageAmount,
        FDamageEvent const& DamageEvent,
        AController* EventInstigator,
        AActor* DamageCauser
    ) override;

    UFUNCTION(BlueprintCallable, Category = "Mob|AI")
    void Provoke(AActor* InstigatorActor);

    UFUNCTION(BlueprintPure, Category = "Mob")
    float GetHealthNormalized() const;

    UFUNCTION(BlueprintPure, Category = "Mob")
    EForestSliceMobState GetMobState() const { return MobState; }

    UFUNCTION(BlueprintPure, Category = "Mob")
    FName GetMobId() const { return Definition.MobId; }

    UFUNCTION(BlueprintPure, Category = "Mob")
    bool IsAlive() const { return MobState != EForestSliceMobState::Dead && CurrentHealth > 0.0f; }

    UPROPERTY(BlueprintAssignable, Category = "Mob|Combat")
    FForestSliceMobCombatEvent MobCombatEvent;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob")
    FForestSliceMobDefinition Definition;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|AI")
    float RoamRadius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|AI")
    float RoamRetargetSeconds = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob|AI")
    float StaggerSeconds = 0.25f;

    UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Mob")
    float CurrentHealth = 80.0f;

    UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Mob")
    EForestSliceMobState MobState = EForestSliceMobState::Idle;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mob|AI")
    TObjectPtr<AActor> CombatTarget;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mob|Visual")
    TObjectPtr<USkeletalMesh> MobSkeletalMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mob|Visual")
    TSubclassOf<UAnimInstance> AnimationClass;

private:
    float AttackTimer = 0.0f;
    float CooldownTimer = 0.0f;
    float StaggerTimer = 0.0f;
    float RoamTimer = 0.0f;
    FVector HomeLocation = FVector::ZeroVector;
    FVector RoamTarget = FVector::ZeroVector;
    bool bProvoked = false;

    void TickAuthority(float DeltaSeconds);
    void UpdateTarget();
    void UpdateState(float DeltaSeconds);
    void MoveToward(const FVector& Destination, float Scale = 1.0f);
    void ChooseRoamTarget();
    void BeginAttack();
    void ResolveAttack();
    void Die(AController* Killer);
    void BroadcastEvent(FName EventId);
};
