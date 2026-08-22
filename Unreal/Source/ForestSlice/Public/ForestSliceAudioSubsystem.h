#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ForestSliceAudioSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FForestSliceAudioSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Master = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Music = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Effects = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Ambience = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Voice = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bMuted = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceAudioSettingsChanged, const FForestSliceAudioSettings&, Settings);

UCLASS(BlueprintType)
class FORESTSLICE_API UForestSliceAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetMasterVolume(float Value);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetMusicVolume(float Value);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetEffectsVolume(float Value);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetAmbienceVolume(float Value);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetVoiceVolume(float Value);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetMuted(bool bInMuted);

    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SaveSettings();

    UFUNCTION(BlueprintPure, Category = "Audio")
    const FForestSliceAudioSettings& GetSettings() const { return Settings; }

    UFUNCTION(BlueprintPure, Category = "Audio")
    float GetEffectiveVolume(float CategoryVolume) const;

    UPROPERTY(BlueprintAssignable, Category = "Audio")
    FForestSliceAudioSettingsChanged SettingsChanged;

private:
    UPROPERTY()
    FForestSliceAudioSettings Settings;

    void BroadcastAndSave();
};
