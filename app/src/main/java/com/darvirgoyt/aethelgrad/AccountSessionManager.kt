package com.darvirgoyt.aethelgrad

import android.app.Activity
import android.os.Handler
import android.os.Looper
import com.google.android.gms.games.GamesSignInClient
import com.google.android.gms.games.PlayGames
import java.net.HttpURLConnection
import java.net.URL
import java.time.Instant
import java.util.concurrent.Executors


enum class SessionState {
    SIGNED_OUT,
    GUEST,
    SIGNING_IN,
    AUTHENTICATED,
    DENIED,
    CONFIGURATION_ERROR,
    EXPIRED,
    NETWORK_ERROR,
    ERROR
}

data class SessionSnapshot(
    val state: SessionState,
    val accountId: String? = null,
    val message: String,
    val expiresAtEpochMs: Long? = null
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
    private var authRefreshUrl: String = ""
    private var accessSessionToken: String? = null
    private var refreshSessionToken: String? = null
    private var accessExpiresAtEpochMs: Long? = null

    fun initialize(activity: Activity, onStateChanged: (SessionSnapshot) -> Unit) {
        stateListener = onStateChanged
        serverClientId = activity.getString(R.string.play_games_server_client_id)
        authExchangeUrl = activity.getString(R.string.auth_exchange_url)
        authRefreshUrl = activity.getString(R.string.auth_refresh_url)
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
                        SessionState.CONFIGURATION_ERROR,
                        message = "Play Games is ready, but the server OAuth ID and HTTPS backend URL are not configured."
                )
            )
            return
        }
        if (!authExchangeUrl.startsWith("https://")) {
            publish(
                    SessionSnapshot(
                        SessionState.CONFIGURATION_ERROR,
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
                        SessionState.DENIED,
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
                val sessionToken = jsonString(response, "accessToken")
                val accountId = jsonString(response, "accountId")
                val refreshToken = jsonString(response, "refreshToken")
                val expiresAt = parseIsoEpochMs(jsonString(response, "expiresAt"))
                if (sessionToken.isNullOrBlank() || accountId.isNullOrBlank() || refreshToken.isNullOrBlank() || expiresAt == null) {
                    publishFromNetwork(SessionSnapshot(SessionState.ERROR, message = "Game server returned an invalid session response."))
                    return@execute
                }
                accessSessionToken = sessionToken
                refreshSessionToken = refreshToken
                accessExpiresAtEpochMs = expiresAt
                publishFromNetwork(
                    SessionSnapshot(
                        SessionState.AUTHENTICATED,
                        accountId = accountId,
                        message = "Game server connected. Select a region to continue.",
                        expiresAtEpochMs = expiresAt
                    )
                )
            } catch (_: Exception) {
                publishFromNetwork(SessionSnapshot(SessionState.NETWORK_ERROR, message = "Game server is unreachable. Check your connection and try again."))
            }
        }
    }

    /** Refreshes an in-memory rotating backend session before it expires. No refresh token is written to disk. */
    fun refreshSession(): SessionSnapshot {
        val refreshToken = refreshSessionToken
        if (refreshToken.isNullOrBlank()) {
            return publish(SessionSnapshot(SessionState.EXPIRED, message = "Your game session has expired. Sign in again to continue."))
        }
        if (authRefreshUrl.startsWith("REPLACE_") || !authRefreshUrl.startsWith("https://")) {
            return publish(SessionSnapshot(SessionState.CONFIGURATION_ERROR, message = "The HTTPS session refresh endpoint is not configured."))
        }
        publish(SessionSnapshot(SessionState.SIGNING_IN, message = "Refreshing secure game session…"))
        networkExecutor.execute {
            try {
                val response = postJson(authRefreshUrl, "{\"refreshToken\":\"${escapeJson(refreshToken)}\"}")
                if (response.statusCode !in 200..299) {
                    clearSession()
                    publishFromNetwork(SessionSnapshot(SessionState.EXPIRED, message = "Your game session has expired. Sign in again to continue."))
                    return@execute
                }
                val accessToken = jsonString(response.body, "accessToken")
                val rotatedRefresh = jsonString(response.body, "refreshToken")
                val expiresAt = parseIsoEpochMs(jsonString(response.body, "expiresAt"))
                if (accessToken.isNullOrBlank() || rotatedRefresh.isNullOrBlank() || expiresAt == null) {
                    clearSession()
                    publishFromNetwork(SessionSnapshot(SessionState.ERROR, message = "Game server returned an invalid refreshed session."))
                    return@execute
                }
                accessSessionToken = accessToken
                refreshSessionToken = rotatedRefresh
                accessExpiresAtEpochMs = expiresAt
                publishFromNetwork(SessionSnapshot(SessionState.AUTHENTICATED, snapshot.accountId, "Game session refreshed.", expiresAt))
            } catch (_: Exception) {
                publishFromNetwork(SessionSnapshot(SessionState.NETWORK_ERROR, message = "Could not refresh the game session. Check your connection."))
            }
        }
        return snapshot
    }

    fun currentAccessToken(): String? {
        val expiresAt = accessExpiresAtEpochMs ?: return null
        if (expiresAt <= System.currentTimeMillis() + 60_000L) return null
        return accessSessionToken
    }

    fun signOut(): SessionSnapshot {
        clearSession()
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

    private fun postJson(url: String, payload: String): HttpResponse {
        val connection = (URL(url).openConnection() as HttpURLConnection).apply {
            requestMethod = "POST"
            connectTimeout = 10_000
            readTimeout = 10_000
            doOutput = true
            setRequestProperty("Content-Type", "application/json")
            setRequestProperty("Accept", "application/json")
        }
        connection.outputStream.use { it.write(payload.toByteArray(Charsets.UTF_8)) }
        val statusCode = connection.responseCode
        val body = (if (statusCode in 200..299) connection.inputStream else connection.errorStream)
            ?.bufferedReader()
            ?.use { it.readText() }
            .orEmpty()
        connection.disconnect()
        return HttpResponse(statusCode, body)
    }

    private fun clearSession() {
        accessSessionToken = null
        refreshSessionToken = null
        accessExpiresAtEpochMs = null
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

    private fun parseIsoEpochMs(value: String?): Long? = try {
        value?.let { Instant.parse(it).toEpochMilli() }
    } catch (_: Exception) {
        null
    }

    private data class HttpResponse(val statusCode: Int, val body: String)
}
