#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ForestSliceGraphicsSettingsSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForestSliceGraphicsQualityChanged, int32, QualityLevel);

UCLASS()
class FORESTSLICE_API UForestSliceGraphicsSettingsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Graphics|Settings")
    void SetGraphicsQuality(int32 QualityLevel);

    UFUNCTION(BlueprintCallable, Category = "Graphics|Settings")
    void ApplyGraphicsQuality();

    UFUNCTION(BlueprintPure, Category = "Graphics|Settings")
    int32 GetGraphicsQuality() const { return GraphicsQuality; }

    UFUNCTION(BlueprintPure, Category = "Graphics|Settings")
    FText GetGraphicsQualityLabel() const;

    UPROPERTY(BlueprintAssignable, Category = "Graphics|Settings")
    FForestSliceGraphicsQualityChanged GraphicsQualityChanged;

private:
    static constexpr int32 MinQuality = 0;
    static constexpr int32 MaxQuality = 4;
    static constexpr int32 DefaultQuality = 3;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Graphics|Settings", meta = (AllowPrivateAccess = "true"))
    int32 GraphicsQuality = DefaultQuality;

    void LoadSettings();
    void SaveSettings() const;
};
