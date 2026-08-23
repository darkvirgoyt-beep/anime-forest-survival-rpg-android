package com.darkvirgoyt.forestslice

import android.app.Activity
import com.google.android.gms.games.GamesSignInClient
import com.google.android.gms.games.PlayGames

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
 * Client-side boundary for Google Play Games Services v2 authentication.
 *
 * Play Games Services owns the provider authentication flow. The app only
 * consumes the authenticated state and must exchange a short-lived assertion
 * with a backend before enabling production cloud saves or co-op services.
 */
class AccountSessionManager {
    var snapshot: SessionSnapshot = SessionSnapshot(SessionState.SIGNED_OUT, message = "Signed out")
        private set

    private var gamesSignInClient: GamesSignInClient? = null
    private var stateListener: ((SessionSnapshot) -> Unit)? = null

    fun initialize(activity: Activity, onStateChanged: (SessionSnapshot) -> Unit) {
        stateListener = onStateChanged
        gamesSignInClient = PlayGames.getGamesSignInClient(activity)
        publish(SessionSnapshot(SessionState.SIGNING_IN, message = "Checking Google Play sign-in…"))

        gamesSignInClient?.isAuthenticated()?.addOnCompleteListener { task ->
            if (task.isSuccessful && task.result.isAuthenticated) {
                publish(
                    SessionSnapshot(
                        SessionState.AUTHENTICATED,
                        accountId = "play-games-authenticated",
                        message = "Google Play sign-in successful. Select a server to continue."
                    )
                )
            } else {
                publish(
                    SessionSnapshot(
                        SessionState.SIGNED_OUT,
                        message = "Google Play sign-in is required to continue."
                    )
                )
            }
        }
    }

    fun startGuest(): SessionSnapshot {
        val next = SessionSnapshot(SessionState.GUEST, accountId = "guest-local", message = "Guest mode • offline development")
        return publish(next)
    }

    fun requestGooglePlaySignIn(): SessionSnapshot {
        val client = gamesSignInClient
        if (client == null) {
            return publish(
                SessionSnapshot(
                    SessionState.ERROR,
                    message = "Google Play Games Services is not initialized. Restart the game and try again."
                )
            )
        }

        publish(SessionSnapshot(SessionState.SIGNING_IN, message = "Opening Google Play sign-in…"))
        client.signIn().addOnCompleteListener { signInTask ->
            if (!signInTask.isSuccessful) {
                publish(
                    SessionSnapshot(
                        SessionState.ERROR,
                        message = "Google Play sign-in failed or was cancelled. Check your Google account and try again."
                    )
                )
                return@addOnCompleteListener
            }

            client.isAuthenticated().addOnCompleteListener { authTask ->
                if (authTask.isSuccessful && authTask.result.isAuthenticated) {
                    publish(
                        SessionSnapshot(
                            SessionState.AUTHENTICATED,
                            accountId = "play-games-authenticated",
                            message = "Google Play sign-in successful. Select a server to continue."
                        )
                    )
                } else {
                    publish(
                        SessionSnapshot(
                            SessionState.ERROR,
                            message = "Google Play did not authenticate this account. Try again or check Play Games settings."
                        )
                    )
                }
            }
        }
        return snapshot
    }

    fun signOut(): SessionSnapshot {
        val next = SessionSnapshot(SessionState.SIGNED_OUT, message = "Signed out")
        return publish(next)
    }

    private fun publish(next: SessionSnapshot): SessionSnapshot {
        snapshot = next
        stateListener?.invoke(next)
        return next
    }
}
