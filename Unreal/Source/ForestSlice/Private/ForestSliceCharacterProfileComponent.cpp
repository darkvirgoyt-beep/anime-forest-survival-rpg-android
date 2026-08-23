#include "ForestSliceCharacterProfileComponent.h"

#include "Net/UnrealNetwork.h"

UForestSliceCharacterProfileComponent::UForestSliceCharacterProfileComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

bool UForestSliceCharacterProfileComponent::SetProfile(const FForestSliceCharacterProfile& NewProfile, FString& ErrorMessage)
{
    if (!GetOwner()->HasAuthority()) {
        ErrorMessage = TEXT("Character profile changes must be submitted to the server");
        return false;
    }
    if (!IsValidProfile(NewProfile, ErrorMessage)) return false;
    Profile = NewProfile;
    ProfileChanged.Broadcast(Profile);
    return true;
}

void UForestSliceCharacterProfileComponent::OnRep_Profile()
{
    ProfileChanged.Broadcast(Profile);
}

bool UForestSliceCharacterProfileComponent::IsValidProfile(const FForestSliceCharacterProfile& Candidate, FString& ErrorMessage) const
{
    const FString CleanName = Candidate.CharacterName.TrimStartAndEnd();
    if (CleanName.Len() < 3 || CleanName.Len() > 16) {
        ErrorMessage = TEXT("Character name must be 3-16 characters");
        return false;
    }
    for (const TCHAR Character : CleanName) {
        const bool bAllowed = (Character >= 'A' && Character <= 'Z') ||
            (Character >= 'a' && Character <= 'z') ||
            (Character >= '0' && Character <= '9') ||
            Character == ' ' || Character == '_' || Character == '-';
        if (!bAllowed) {
            ErrorMessage = TEXT("Character name contains unsupported characters");
            return false;
        }
    }
    if (Candidate.EyebrowStyle < 0 || Candidate.EyebrowStyle > 3 ||
        Candidate.HairStyle < 0 || Candidate.HairStyle > 3 ||
        Candidate.OutfitStyle < 0 || Candidate.OutfitStyle > 3) {
        ErrorMessage = TEXT("Character style selection is outside the supported range");
        return false;
    }
    return true;
}

void UForestSliceCharacterProfileComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UForestSliceCharacterProfileComponent, Profile);
}
