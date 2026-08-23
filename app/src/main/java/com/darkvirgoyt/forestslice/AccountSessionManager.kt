package com.darkvirgoyt.forestslice

enum class SessionState {
    SIGNED_OUT,
    GUEST,
    SIGNING_IN,
    AUTHENTICATED,
    ERROR
}

data class SessionSnapshot(
    val state: SessionState,
    val accountId: String? = null,
    val message: String
)

/**
 * Client-side boundary for the real Google Play Games -> backend exchange.
 * Guest mode is intentionally usable in development. Production sign-in must
 * be completed through the Android provider bridge and a short-lived backend
 * session; no provider secret belongs in this APK.
 */
class AccountSessionManager {
    var snapshot: SessionSnapshot = SessionSnapshot(SessionState.SIGNED_OUT, message = "Signed out")
        private set

    fun startGuest(): SessionSnapshot {
        snapshot = SessionSnapshot(SessionState.GUEST, accountId = "guest-local", message = "Guest mode • offline development")
        return snapshot
    }

    fun requestGooglePlaySignIn(): SessionSnapshot {
        snapshot = SessionSnapshot(
            SessionState.ERROR,
            message = "Google Play bridge not configured in this development APK"
        )
        return snapshot
    }

    fun signOut(): SessionSnapshot {
        snapshot = SessionSnapshot(SessionState.SIGNED_OUT, message = "Signed out")
        return snapshot
    }
}
