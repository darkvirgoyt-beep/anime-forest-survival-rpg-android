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

void UForestSliceAccountSubsystem::HandleGooglePlayCredential(const FString& BackendSessionToken)
{
    if (BackendSessionToken.IsEmpty()) {
        SetState(EForestSliceLoginState::Error, TEXT("Backend session token was empty"));
        return;
    }

    // The Android bridge must exchange the single-use Play Games code with the
    // backend first. Never treat a client-provided player ID or raw provider
    // credential as proof of ownership inside Unreal.
    PendingBackendSessionToken = BackendSessionToken;
    SetState(EForestSliceLoginState::SigningIn, TEXT("Backend session accepted; requesting dedicated-server admission"));
}

void UForestSliceAccountSubsystem::HandleDedicatedServerAdmissionAccepted()
{
    if (PendingBackendSessionToken.IsEmpty()) {
        SetState(EForestSliceLoginState::Error, TEXT("Dedicated-server admission requires an authenticated backend session"));
        return;
    }

    SetState(EForestSliceLoginState::Authenticated, TEXT("Dedicated server admission accepted"));
}

void UForestSliceAccountSubsystem::SignOut()
{
    PendingBackendSessionToken.Reset();
    SetState(EForestSliceLoginState::SignedOut, TEXT("Signed out"));
}

void UForestSliceAccountSubsystem::SetState(EForestSliceLoginState NewState, const FString& Message)
{
    LoginState = NewState;
    LoginChanged.Broadcast(LoginState, Message);
}
