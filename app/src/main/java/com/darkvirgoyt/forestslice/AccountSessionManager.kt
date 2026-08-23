package com.darkvirgoyt.forestslice

import android.app.Activity
import android.os.Handler
import android.os.Looper
import com.google.android.gms.games.GamesSignInClient
import com.google.android.gms.games.PlayGames
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.Executors


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
 * Client boundary for Play Games platform authentication and backend session exchange.
 * The client never treats a Play Games player ID as proof of in-game ownership.
 */
class AccountSessionManager {
    var snapshot: SessionSnapshot = SessionSnapshot(SessionState.SIGNED_OUT, message = "Signed out")
        private set

    private val mainHandler = Handler(Looper.getMainLooper())
    private val networkExecutor = Executors.newSingleThreadExecutor()
    private var gamesSignInClient: GamesSignInClient? = null
    private var stateListener: ((SessionSnapshot) -> Unit)? = null
    private var serverClientId: String = ""
    private var authExchangeUrl: String = ""
    private var gameSessionToken: String? = null

    fun initialize(activity: Activity, onStateChanged: (SessionSnapshot) -> Unit) {
        stateListener = onStateChanged
        serverClientId = activity.getString(R.string.play_games_server_client_id)
        authExchangeUrl = activity.getString(R.string.auth_exchange_url)
        gamesSignInClient = PlayGames.getGamesSignInClient(activity)
        publish(SessionSnapshot(SessionState.SIGNING_IN, message = "Checking Google Play sign-in…"))

        gamesSignInClient?.isAuthenticated()?.addOnCompleteListener { task ->
            if (task.isSuccessful && task.result.isAuthenticated) {
                requestServerSession()
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
            requestServerSession()
        }
        return snapshot
    }

    private fun requestServerSession() {
        if (serverClientId.startsWith("REPLACE_") || authExchangeUrl.startsWith("REPLACE_")) {
            publish(
                SessionSnapshot(
                    SessionState.ERROR,
                    message = "Play Games is ready, but the server OAuth ID and HTTPS backend URL are not configured."
                )
            )
            return
        }
        if (!authExchangeUrl.startsWith("https://")) {
            publish(
                SessionSnapshot(
                    SessionState.ERROR,
                    message = "The authentication backend must use HTTPS."
                )
            )
            return
        }

        val client = gamesSignInClient ?: return
        publish(SessionSnapshot(SessionState.SIGNING_IN, message = "Connecting to the game server…"))
        client.requestServerSideAccess(serverClientId, false).addOnCompleteListener { task ->
            if (!task.isSuccessful || task.result.isNullOrBlank()) {
                publish(
                    SessionSnapshot(
                        SessionState.ERROR,
                        message = "Could not obtain a secure Play Games server code. Check Play Console configuration."
                    )
                )
                return@addOnCompleteListener
            }
            exchangeCode(task.result)
        }
    }

    private fun exchangeCode(serverAuthCode: String) {
        networkExecutor.execute {
            try {
                val connection = (URL(authExchangeUrl).openConnection() as HttpURLConnection).apply {
                    requestMethod = "POST"
                    connectTimeout = 10_000
                    readTimeout = 10_000
                    doOutput = true
                    setRequestProperty("Content-Type", "application/json")
                    setRequestProperty("Accept", "application/json")
                }
                val payload = "{\"serverAuthCode\":\"${escapeJson(serverAuthCode)}\"}"
                connection.outputStream.use { it.write(payload.toByteArray(Charsets.UTF_8)) }
                val responseCode = connection.responseCode
                val response = (if (responseCode in 200..299) connection.inputStream else connection.errorStream)
                    ?.bufferedReader()
                    ?.use { it.readText() }
                    .orEmpty()
                connection.disconnect()

                if (responseCode !in 200..299) {
                    publishFromNetwork(SessionSnapshot(SessionState.ERROR, message = "Game server rejected the Play Games login ($responseCode)."))
                    return@execute
                }
                val sessionToken = jsonString(response, "sessionToken")
                val accountId = jsonString(response, "accountId")
                if (sessionToken.isNullOrBlank() || accountId.isNullOrBlank()) {
                    publishFromNetwork(SessionSnapshot(SessionState.ERROR, message = "Game server returned an invalid session response."))
                    return@execute
                }
                gameSessionToken = sessionToken
                publishFromNetwork(
                    SessionSnapshot(
                        SessionState.AUTHENTICATED,
                        accountId = accountId,
                        message = "Game server connected. Select a region to continue."
                    )
                )
            } catch (_: Exception) {
                publishFromNetwork(SessionSnapshot(SessionState.ERROR, message = "Game server is unreachable. Check your connection and try again."))
            }
        }
    }

    fun signOut(): SessionSnapshot {
        gameSessionToken = null
        val next = SessionSnapshot(SessionState.SIGNED_OUT, message = "Signed out")
        return publish(next)
    }

    fun shutdown() {
        networkExecutor.shutdownNow()
    }

    private fun publish(next: SessionSnapshot): SessionSnapshot {
        snapshot = next
        stateListener?.invoke(next)
        return next
    }

    private fun publishFromNetwork(next: SessionSnapshot) {
        mainHandler.post { publish(next) }
    }

    private fun escapeJson(value: String): String = value
        .replace("\\", "\\\\")
        .replace("\"", "\\\"")
        .replace("\n", "\\n")
        .replace("\r", "\\r")

    private fun jsonString(json: String, key: String): String? {
        val pattern = Regex("\\\"${Regex.escape(key)}\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"")
        return pattern.find(json)?.groupValues?.getOrNull(1)
    }
}
