#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ForestSliceAccountSubsystem.generated.h"

UENUM(BlueprintType)
enum class EForestSliceLoginState : uint8
{
    SignedOut,
    Guest,
    SigningIn,
    Authenticated,
    Error
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForestSliceLoginChanged, EForestSliceLoginState, State, const FString&, Message);

UCLASS(BlueprintType)
class FORESTSLICE_API UForestSliceAccountSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Account")
    void StartGuestSession();

    UFUNCTION(BlueprintCallable, Category = "Account")
    void StartGooglePlaySignIn();

    UFUNCTION(BlueprintCallable, Category = "Account")
    void HandleGooglePlayCredential(const FString& BackendSessionToken);

    UFUNCTION(BlueprintCallable, Category = "Account")
    void HandleDedicatedServerAdmissionAccepted();

    UFUNCTION(BlueprintCallable, Category = "Account")
    void SignOut();

    UFUNCTION(BlueprintPure, Category = "Account")
    EForestSliceLoginState GetLoginState() const { return LoginState; }

    UFUNCTION(BlueprintPure, Category = "Account")
    bool IsGooglePlayAvailable() const { return bGooglePlayAvailable; }

    UPROPERTY(BlueprintAssignable, Category = "Account")
    FForestSliceLoginChanged LoginChanged;

private:
    EForestSliceLoginState LoginState = EForestSliceLoginState::SignedOut;
    bool bGooglePlayAvailable = false;
    FString PendingBackendSessionToken;

    void SetState(EForestSliceLoginState NewState, const FString& Message);
};
