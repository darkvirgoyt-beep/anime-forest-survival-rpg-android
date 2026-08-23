#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ForestSliceCharacterProfileComponent.generated.h"

USTRUCT(BlueprintType)
struct FForestSliceCharacterProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CharacterName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EyebrowStyle = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HairStyle = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 OutfitStyle = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceProfileChanged, const FForestSliceCharacterProfile&, Profile);

UCLASS(ClassGroup = (ForestSlice), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FORESTSLICE_API UForestSliceCharacterProfileComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UForestSliceCharacterProfileComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Character")
    bool SetProfile(const FForestSliceCharacterProfile& NewProfile, FString& ErrorMessage);

    UFUNCTION(BlueprintPure, Category = "Character")
    const FForestSliceCharacterProfile& GetProfile() const { return Profile; }

    UPROPERTY(BlueprintAssignable, Category = "Character")
    FForestSliceProfileChanged ProfileChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_Profile, VisibleInstanceOnly, BlueprintReadOnly, Category = "Character")
    FForestSliceCharacterProfile Profile;

    UFUNCTION()
    void OnRep_Profile();

private:
    bool IsValidProfile(const FForestSliceCharacterProfile& Candidate, FString& ErrorMessage) const;
};
