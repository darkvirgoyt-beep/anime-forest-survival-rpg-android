package com.darvirgoyt.aethelgrad

import android.app.Activity
import android.os.Handler
import android.os.Looper
import androidx.core.content.ContextCompat
import androidx.credentials.CredentialManager
import androidx.credentials.CredentialManagerCallback
import androidx.credentials.CustomCredential
import androidx.credentials.GetCredentialRequest
import androidx.credentials.GetCredentialResponse
import androidx.credentials.exceptions.GetCredentialException
import com.google.android.libraries.identity.googleid.GetSignInWithGoogleOption
import com.google.android.libraries.identity.googleid.GoogleIdTokenCredential
import java.net.HttpURLConnection
import java.net.URL
import java.time.Instant
import java.util.concurrent.Executors

enum class SessionState {
    SIGNED_OUT,
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
 * Standard Google account sign-in boundary for pre-Play-Console testing.
 * The Android client sends only a Google-issued ID token to the configured HTTPS backend.
 * The backend verifies the token audience, issuer, expiry, and signature before it creates an Aethelgard session.
 */
class AccountSessionManager {
    var snapshot: SessionSnapshot = SessionSnapshot(SessionState.SIGNED_OUT, message = "Signed out")
        private set

    private val mainHandler = Handler(Looper.getMainLooper())
    private val networkExecutor = Executors.newSingleThreadExecutor()
    private var activity: Activity? = null
    private var credentialManager: CredentialManager? = null
    private var stateListener: ((SessionSnapshot) -> Unit)? = null
    private var googleWebClientId: String = ""
    private var authExchangeUrl: String = ""
    private var authRefreshUrl: String = ""
    private var accessSessionToken: String? = null
    private var refreshSessionToken: String? = null
    private var accessExpiresAtEpochMs: Long? = null

    fun initialize(activity: Activity, onStateChanged: (SessionSnapshot) -> Unit) {
        this.activity = activity
        stateListener = onStateChanged
        googleWebClientId = activity.getString(R.string.google_web_client_id)
        authExchangeUrl = activity.getString(R.string.auth_exchange_url)
        authRefreshUrl = activity.getString(R.string.auth_refresh_url)
        credentialManager = CredentialManager.create(activity)

        if (!hasCompleteConfiguration()) {
            publish(
                SessionSnapshot(
                    SessionState.CONFIGURATION_ERROR,
                    message = "Google sign-in is prepared, but the Web OAuth client ID and HTTPS backend URL are not configured."
                )
            )
            return
        }
        publish(SessionSnapshot(SessionState.SIGNED_OUT, message = "Google sign-in is required to continue."))
    }

    fun requestGoogleSignIn(): SessionSnapshot {
        val owner = activity
        val manager = credentialManager
        if (owner == null || manager == null) {
            return publish(SessionSnapshot(SessionState.ERROR, message = "Google sign-in is not initialized. Restart the game and try again."))
        }
        if (!hasCompleteConfiguration()) {
            return publish(SessionSnapshot(SessionState.CONFIGURATION_ERROR, message = "Configure the Web OAuth client ID and HTTPS game backend before signing in."))
        }

        publish(SessionSnapshot(SessionState.SIGNING_IN, message = "Choose a Google account to connect…"))
        val option = GetSignInWithGoogleOption.Builder(googleWebClientId).build()
        val request = GetCredentialRequest.Builder().addCredentialOption(option).build()
        manager.getCredentialAsync(
            owner,
            request,
            null,
            ContextCompat.getMainExecutor(owner),
            object : CredentialManagerCallback<GetCredentialResponse, GetCredentialException> {
                override fun onResult(result: GetCredentialResponse) {
                    val credential = result.credential
                    if (credential !is CustomCredential || credential.type != GoogleIdTokenCredential.TYPE_GOOGLE_ID_TOKEN_CREDENTIAL) {
                        publish(SessionSnapshot(SessionState.DENIED, message = "Google did not return a usable sign-in credential. Try again."))
                        return
                    }
                    try {
                        exchangeGoogleIdToken(GoogleIdTokenCredential.createFrom(credential.data).idToken)
                    } catch (_: Exception) {
                        publish(SessionSnapshot(SessionState.DENIED, message = "Google returned an invalid sign-in credential. Try again."))
                    }
                }

                override fun onError(error: GetCredentialException) {
                    publish(SessionSnapshot(SessionState.DENIED, message = "Google sign-in was cancelled or unavailable. Check your Google account and try again."))
                }
            }
        )
        return snapshot
    }

    private fun exchangeGoogleIdToken(idToken: String) {
        publish(SessionSnapshot(SessionState.SIGNING_IN, message = "Verifying your Google account with the game server…"))
        networkExecutor.execute {
            try {
                val response = postJson(authExchangeUrl, "{\"idToken\":\"${escapeJson(idToken)}\"}")
                if (response.statusCode !in 200..299) {
                    publishFromNetwork(SessionSnapshot(SessionState.ERROR, message = "Game server rejected the Google login (${response.statusCode})."))
                    return@execute
                }
                val sessionToken = jsonString(response.body, "accessToken")
                val accountId = jsonString(response.body, "accountId")
                val refreshToken = jsonString(response.body, "refreshToken")
                val expiresAt = parseIsoEpochMs(jsonString(response.body, "expiresAt"))
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
                        message = "Google account verified. Select a region to continue.",
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
        return publish(SessionSnapshot(SessionState.SIGNED_OUT, message = "Signed out"))
    }

    fun shutdown() {
        activity = null
        networkExecutor.shutdownNow()
    }

    private fun hasCompleteConfiguration(): Boolean =
        !googleWebClientId.startsWith("REPLACE_") &&
            !authExchangeUrl.startsWith("REPLACE_") &&
            authExchangeUrl.startsWith("https://")

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
