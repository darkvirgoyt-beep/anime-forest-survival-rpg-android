#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceMobPresentationComponent.generated.h"

class UForestSliceHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(
    FForestSliceMobPresentationChanged,
    FName, DisplayName,
    int32, Level,
    float, Health,
    float, MaxHealth,
    bool, bElite,
    bool, bBaseAffiliated
);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceMobPresentationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceMobPresentationComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Mob Presentation")
    void MarkTargeted(bool bTargeted);

    UFUNCTION(BlueprintCallable, Category = "Mob Presentation")
    void SetBaseAffiliation(FName NewBaseId);

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    float GetHealthRatio() const;

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    bool ShouldShowHealthBar(float DistanceMeters, bool bTargetedByPlayer) const;

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    bool ShouldShowBaseMarker(float DistanceMeters) const;

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    FLinearColor GetHealthBarColor() const;

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    const FName& GetDisplayName() const { return DisplayName; }

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    int32 GetLevel() const { return Level; }

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    bool IsElite() const { return bElite; }

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    bool IsBoss() const { return bBoss; }

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    bool IsBaseAffiliated() const { return !BaseId.IsNone(); }

    UFUNCTION(BlueprintPure, Category = "Mob Presentation")
    const FName& GetBaseId() const { return BaseId; }

    UPROPERTY(BlueprintAssignable, Category = "Mob Presentation")
    FForestSliceMobPresentationChanged PresentationChanged;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Presentation")
    FName DisplayName = FName(TEXT("Forest Creature"));

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Presentation", meta = (ClampMin = "1"))
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Presentation")
    bool bElite = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Presentation")
    bool bBoss = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Presentation")
    FName BaseId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Presentation", meta = (ClampMin = "1.0"))
    float MaxHealthBarDistanceMeters = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mob Presentation", meta = (ClampMin = "0.0"))
    float HealthBarGraceSeconds = 6.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mob Presentation")
    bool bIsTargeted = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mob Presentation")
    float LastCombatTime = -BIG_NUMBER;

    UPROPERTY()
    TObjectPtr<UForestSliceHealthComponent> HealthComponent;

    UFUNCTION()
    void OnHealthChanged(FName DamageType, float Damage, float RemainingHealth, bool bKilled);

private:
    void BroadcastPresentation();
};
