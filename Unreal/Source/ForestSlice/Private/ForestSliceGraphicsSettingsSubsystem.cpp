#include "ForestSliceGraphicsSettingsSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
    const TCHAR* Section = TEXT("Aethelgrad.Graphics");

    const TCHAR* QualityLabel(int32 QualityLevel)
    {
        switch (FMath::Clamp(QualityLevel, 0, 4))
        {
        case 0: return TEXT("LOW");
        case 1: return TEXT("MEDIUM");
        case 2: return TEXT("HIGH");
        case 3: return TEXT("ULTRA");
        default: return TEXT("MAX");
        }
    }

    void SetScalabilityConsoleVariable(const TCHAR* Name, int32 Value)
    {
        if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            Variable->Set(Value, ECVF_SetByGameSetting);
        }
    }
}

void UForestSliceGraphicsSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadSettings();
    ApplyGraphicsQuality();
}

void UForestSliceGraphicsSettingsSubsystem::Deinitialize()
{
    SaveSettings();
    Super::Deinitialize();
}

void UForestSliceGraphicsSettingsSubsystem::SetGraphicsQuality(int32 QualityLevel)
{
    const int32 NewQuality = FMath::Clamp(QualityLevel, MinQuality, MaxQuality);
    if (GraphicsQuality == NewQuality)
    {
        ApplyGraphicsQuality();
        return;
    }

    GraphicsQuality = NewQuality;
    ApplyGraphicsQuality();
    SaveSettings();
    GraphicsQualityChanged.Broadcast(GraphicsQuality);
}

void UForestSliceGraphicsSettingsSubsystem::ApplyGraphicsQuality()
{
    const int32 Quality = FMath::Clamp(GraphicsQuality, MinQuality, MaxQuality);

    // These are live game-settings variables and intentionally remain bounded
    // for mobile thermal and memory budgets. Unreal scalability groups consume
    // the same values when a platform profile is loaded.
    SetScalabilityConsoleVariable(TEXT("sg.ViewDistanceQuality"), Quality);
    SetScalabilityConsoleVariable(TEXT("sg.AntiAliasingQuality"), Quality);
    SetScalabilityConsoleVariable(TEXT("sg.ShadowQuality"), FMath::Max(0, Quality - 1));
    SetScalabilityConsoleVariable(TEXT("sg.PostProcessQuality"), Quality);
    SetScalabilityConsoleVariable(TEXT("sg.TextureQuality"), Quality);
    SetScalabilityConsoleVariable(TEXT("sg.EffectsQuality"), Quality);
    SetScalabilityConsoleVariable(TEXT("sg.FoliageQuality"), FMath::Min(Quality, 3));
    SetScalabilityConsoleVariable(TEXT("sg.ShadingQuality"), FMath::Min(Quality, 3));

    if (GEngine)
    {
        GEngine->Exec(nullptr, *FString::Printf(TEXT("ScalabilityQuality %d"), Quality));
    }
}

FText UForestSliceGraphicsSettingsSubsystem::GetGraphicsQualityLabel() const
{
    return FText::FromString(QualityLabel(GraphicsQuality));
}

void UForestSliceGraphicsSettingsSubsystem::LoadSettings()
{
    GraphicsQuality = 3;
    if (GConfig)
    {
        GConfig->GetInt(Section, TEXT("Quality"), GraphicsQuality, GGameUserSettingsIni);
    }
    GraphicsQuality = FMath::Clamp(GraphicsQuality, MinQuality, MaxQuality);
}

void UForestSliceGraphicsSettingsSubsystem::SaveSettings() const
{
    if (!GConfig) return;
    GConfig->SetInt(Section, TEXT("Quality"), GraphicsQuality, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}
