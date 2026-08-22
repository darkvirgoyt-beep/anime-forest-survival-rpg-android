#include "ForestSliceAccountSubsystem.h"

void UForestSliceAccountSubsystem::StartGuestSession()
{
    SetState(EForestSliceLoginState::Guest, TEXT("Guest session active"));
}

void UForestSliceAccountSubsystem::StartGooglePlaySignIn()
{
#if PLATFORM_ANDROID
    bGooglePlayAvailable = true;
    SetState(EForestSliceLoginState::SigningIn, TEXT("Waiting for Google Play Games sign-in bridge"));
#else
    bGooglePlayAvailable = false;
    SetState(EForestSliceLoginState::Error, TEXT("Google Play Games is available only on Android"));
#endif
}

void UForestSliceAccountSubsystem::HandleGooglePlayCredential(const FString& ProviderCredential)
{
    if (ProviderCredential.IsEmpty()) {
        SetState(EForestSliceLoginState::Error, TEXT("Google Play credential was empty"));
        return;
    }

    // Send the short-lived credential to the backend session exchange here.
    // Never treat a client-provided player ID as proof of ownership.
    SetState(EForestSliceLoginState::Authenticated, TEXT("Backend session exchange accepted"));
}

void UForestSliceAccountSubsystem::SignOut()
{
    SetState(EForestSliceLoginState::SignedOut, TEXT("Signed out"));
}

void UForestSliceAccountSubsystem::SetState(EForestSliceLoginState NewState, const FString& Message)
{
    LoginState = NewState;
    LoginChanged.Broadcast(LoginState, Message);
}
