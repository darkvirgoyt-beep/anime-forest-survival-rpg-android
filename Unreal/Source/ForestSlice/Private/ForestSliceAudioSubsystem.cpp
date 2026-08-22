#include "ForestSliceAudioSubsystem.h"

#include "Misc/ConfigCacheIni.h"

namespace
{
    const TCHAR* Section = TEXT("Aethelgard.Audio");
}

void UForestSliceAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (GConfig) {
        GConfig->GetFloat(Section, TEXT("Master"), Settings.Master, GGameUserSettingsIni);
        GConfig->GetFloat(Section, TEXT("Music"), Settings.Music, GGameUserSettingsIni);
        GConfig->GetFloat(Section, TEXT("Effects"), Settings.Effects, GGameUserSettingsIni);
        GConfig->GetFloat(Section, TEXT("Ambience"), Settings.Ambience, GGameUserSettingsIni);
        GConfig->GetFloat(Section, TEXT("Voice"), Settings.Voice, GGameUserSettingsIni);
        GConfig->GetBool(Section, TEXT("Muted"), Settings.bMuted, GGameUserSettingsIni);
    }
}

void UForestSliceAudioSubsystem::SetMasterVolume(float Value)
{
    Settings.Master = FMath::Clamp(Value, 0.0f, 1.0f);
    BroadcastAndSave();
}

void UForestSliceAudioSubsystem::SetMusicVolume(float Value)
{
    Settings.Music = FMath::Clamp(Value, 0.0f, 1.0f);
    BroadcastAndSave();
}

void UForestSliceAudioSubsystem::SetEffectsVolume(float Value)
{
    Settings.Effects = FMath::Clamp(Value, 0.0f, 1.0f);
    BroadcastAndSave();
}

void UForestSliceAudioSubsystem::SetAmbienceVolume(float Value)
{
    Settings.Ambience = FMath::Clamp(Value, 0.0f, 1.0f);
    BroadcastAndSave();
}

void UForestSliceAudioSubsystem::SetVoiceVolume(float Value)
{
    Settings.Voice = FMath::Clamp(Value, 0.0f, 1.0f);
    BroadcastAndSave();
}

void UForestSliceAudioSubsystem::SetMuted(bool bInMuted)
{
    Settings.bMuted = bInMuted;
    BroadcastAndSave();
}

float UForestSliceAudioSubsystem::GetEffectiveVolume(float CategoryVolume) const
{
    return Settings.bMuted ? 0.0f : Settings.Master * FMath::Clamp(CategoryVolume, 0.0f, 1.0f);
}

void UForestSliceAudioSubsystem::SaveSettings()
{
    if (!GConfig) return;
    GConfig->SetFloat(Section, TEXT("Master"), Settings.Master, GGameUserSettingsIni);
    GConfig->SetFloat(Section, TEXT("Music"), Settings.Music, GGameUserSettingsIni);
    GConfig->SetFloat(Section, TEXT("Effects"), Settings.Effects, GGameUserSettingsIni);
    GConfig->SetFloat(Section, TEXT("Ambience"), Settings.Ambience, GGameUserSettingsIni);
    GConfig->SetFloat(Section, TEXT("Voice"), Settings.Voice, GGameUserSettingsIni);
    GConfig->SetBool(Section, TEXT("Muted"), Settings.bMuted, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

void UForestSliceAudioSubsystem::BroadcastAndSave()
{
    SaveSettings();
    SettingsChanged.Broadcast(Settings);
}
