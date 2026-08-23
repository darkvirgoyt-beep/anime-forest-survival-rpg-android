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
import org.json.JSONArray
import org.json.JSONObject
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

data class CloudWorldManifest(
    val id: String,
    val name: String,
    val region: String,
    val saveRevision: Int,
    val schemaVersion: Int
)

data class PlayerProfile(
    val username: String?,
    val avatarId: String,
    val profileVisibility: String
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

    /** Lists only worlds that belong to the currently authenticated internal game account. */
    fun fetchOwnedWorlds(onComplete: (List<CloudWorldManifest>?, String?) -> Unit) {
        val token = currentAccessToken()
        if (token.isNullOrBlank()) {
            onComplete(null, "Your game session has expired. Sign in again to recover cloud worlds.")
            return
        }
        networkExecutor.execute {
            try {
                val response = getJson(cloudEndpoint("/worlds/mine"), token)
                if (response.statusCode !in 200..299) {
                    publishCloudResult(onComplete, null, "Could not check your cloud worlds (${response.statusCode}).")
                    return@execute
                }
                publishCloudResult(onComplete, parseWorldManifests(response.body), null)
            } catch (_: Exception) {
                publishCloudResult(onComplete, null, "Cloud worlds are unavailable. Check your connection.")
            }
        }
    }

    /** Restores the signed-in account's username and server-approved curated avatar ID. */
    fun fetchProfile(onComplete: (PlayerProfile?, String?) -> Unit) {
        val token = currentAccessToken()
        if (token.isNullOrBlank()) {
            onComplete(null, "Your game session has expired. Sign in again to restore your profile.")
            return
        }
        networkExecutor.execute {
            try {
                val response = getJson(cloudEndpoint("/profile"), token)
                if (response.statusCode !in 200..299) {
                    publishRecoveredProfile(onComplete, null, "Could not restore your profile (${response.statusCode}).")
                    return@execute
                }
                val payload = JSONObject(response.body).optJSONObject("profile")
                if (payload == null) {
                    publishRecoveredProfile(onComplete, null, "Cloud profile response was incomplete.")
                    return@execute
                }
                val username = payload.optString("username").trim().ifBlank { null }
                val avatarId = payload.optString("avatarId", "trailblazer")
                val visibility = payload.optString("profileVisibility", "public")
                publishRecoveredProfile(onComplete, PlayerProfile(username, avatarId, visibility), null)
            } catch (_: Exception) {
                publishRecoveredProfile(onComplete, null, "Could not restore your profile. Check your connection.")
            }
        }
    }

    /** Saves player-visible identity fields; only a server-approved built-in avatar ID may be selected. */
    fun updateProfile(username: String, avatarId: String, onComplete: (String?) -> Unit) {
        val token = currentAccessToken()
        if (token.isNullOrBlank()) {
            onComplete("Your game session has expired. Sign in again.")
            return
        }
        networkExecutor.execute {
            try {
                val payload = JSONObject().put("username", username).put("avatarId", avatarId).put("profileVisibility", "public").toString()
                val response = requestJson("PUT", cloudEndpoint("/profile"), token, payload)
                val error = if (response.statusCode in 200..299) null else if (response.statusCode == 409) "That wayfarer name is already taken." else "Could not save your profile (${response.statusCode})."
                publishProfileResult(onComplete, error)
            } catch (_: Exception) {
                publishProfileResult(onComplete, "Could not save your profile. Check your connection.")
            }
        }
    }

    /** Creates an account-owned manifest and writes its first immutable cloud snapshot. */
    fun createInitialCloudWorld(name: String, region: String, avatarId: String, nativeWorldState: String, onComplete: (CloudWorldManifest?, String?) -> Unit) {
        val token = currentAccessToken()
        if (token.isNullOrBlank()) {
            onComplete(null, "Your game session has expired. Sign in again.")
            return
        }
        networkExecutor.execute {
            try {
                val created = requestJson("POST", cloudEndpoint("/worlds"), token, JSONObject().put("name", name).put("region", region).toString())
                if (created.statusCode !in 200..299) {
                    publishWorldResult(onComplete, null, "Could not create your cloud world (${created.statusCode}).")
                    return@execute
                }
                val world = JSONObject(created.body).getJSONObject("world")
                val worldId = world.getString("id")
                val initialState = JSONObject(nativeWorldState)
                    .put("worldSeed", "aethelgard-${worldId.take(8)}")
                    .put("character", JSONObject().put("name", name).put("avatarId", avatarId))
                val saved = requestJson("PUT", cloudEndpoint("/worlds/$worldId/save"), token, JSONObject().put("expectedRevision", 0).put("schemaVersion", 1).put("worldState", initialState).toString())
                if (saved.statusCode !in 200..299) {
                    publishWorldResult(onComplete, null, "Cloud world was created but its first snapshot could not be saved (${saved.statusCode}).")
                    return@execute
                }
                publishWorldResult(onComplete, CloudWorldManifest(worldId, world.getString("name"), world.getString("region"), 1, 1), null)
            } catch (_: Exception) {
                publishWorldResult(onComplete, null, "Could not create your cloud world. Check your connection.")
            }
        }
    }

    /** Uploads a new immutable revision; a stale device is rejected instead of silently overwriting cloud state. */
    fun uploadCloudWorld(world: CloudWorldManifest, nativeWorldState: String, onComplete: (CloudWorldManifest?, String?) -> Unit) {
        val token = currentAccessToken()
        if (token.isNullOrBlank()) {
            onComplete(null, "Your game session has expired. Sign in again to save.")
            return
        }
        networkExecutor.execute {
            try {
                val payload = JSONObject().put("expectedRevision", world.saveRevision).put("schemaVersion", world.schemaVersion).put("worldState", JSONObject(nativeWorldState)).toString()
                val response = requestJson("PUT", cloudEndpoint("/worlds/${world.id}/save"), token, payload)
                if (response.statusCode == 409) {
                    publishWorldResult(onComplete, null, "A newer cloud revision exists. Re-open the world to recover it safely.")
                    return@execute
                }
                if (response.statusCode !in 200..299) {
                    publishWorldResult(onComplete, null, "Could not upload cloud save (${response.statusCode}).")
                    return@execute
                }
                val result = JSONObject(response.body)
                publishWorldResult(onComplete, world.copy(saveRevision = result.optInt("saveRevision", world.saveRevision + 1), schemaVersion = result.optInt("schemaVersion", world.schemaVersion)), null)
            } catch (_: Exception) {
                publishWorldResult(onComplete, null, "Could not upload cloud save. Check your connection.")
            }
        }
    }

    /** Downloads the latest owned world snapshot after the backend has validated world ownership. */
    fun downloadCloudWorld(world: CloudWorldManifest, onComplete: (String?, String?) -> Unit) {
        val token = currentAccessToken()
        if (token.isNullOrBlank()) {
            onComplete(null, "Your game session has expired. Sign in again to recover this world.")
            return
        }
        networkExecutor.execute {
            try {
                val manifest = getJson(cloudEndpoint("/worlds/${world.id}/save"), token)
                if (manifest.statusCode !in 200..299) {
                    publishSnapshotResult(onComplete, null, "Could not recover cloud world (${manifest.statusCode}).")
                    return@execute
                }
                val downloadUrl = JSONObject(manifest.body).optString("downloadUrl")
                if (downloadUrl.isBlank()) {
                    publishSnapshotResult(onComplete, null, "Cloud world did not return a snapshot location.")
                    return@execute
                }
                val absoluteUrl = if (downloadUrl.startsWith("https://")) downloadUrl else authExchangeUrl.substringBefore("/api/game-auth") + downloadUrl
                // The backend already authenticated ownership before minting this short-lived object URL.
                // Do not expose the game access token to the storage host.
                val snapshot = requestJson("GET", absoluteUrl, null, null)
                if (snapshot.statusCode !in 200..299) {
                    publishSnapshotResult(onComplete, null, "Could not download cloud world (${snapshot.statusCode}).")
                    return@execute
                }
                publishSnapshotResult(onComplete, snapshot.body, null)
            } catch (_: Exception) {
                publishSnapshotResult(onComplete, null, "Could not download cloud world. Check your connection.")
            }
        }
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

    private fun cloudEndpoint(path: String): String = authExchangeUrl.substringBeforeLast("/exchange") + path

    private fun publish(next: SessionSnapshot): SessionSnapshot {
        snapshot = next
        stateListener?.invoke(next)
        return next
    }

    private fun publishFromNetwork(next: SessionSnapshot) {
        mainHandler.post { publish(next) }
    }

    private fun postJson(url: String, payload: String): HttpResponse {
        return requestJson("POST", url, null, payload)
    }

    private fun getJson(url: String, bearerToken: String): HttpResponse = requestJson("GET", url, bearerToken, null)

    private fun requestJson(method: String, url: String, bearerToken: String?, payload: String?): HttpResponse {
        val connection = (URL(url).openConnection() as HttpURLConnection).apply {
            requestMethod = method
            connectTimeout = 10_000
            readTimeout = 10_000
            doOutput = payload != null
            setRequestProperty("Content-Type", "application/json")
            setRequestProperty("Accept", "application/json")
            if (!bearerToken.isNullOrBlank()) setRequestProperty("Authorization", "Bearer $bearerToken")
        }
        if (payload != null) connection.outputStream.use { it.write(payload.toByteArray(Charsets.UTF_8)) }
        val statusCode = connection.responseCode
        val body = (if (statusCode in 200..299) connection.inputStream else connection.errorStream)
            ?.bufferedReader()
            ?.use { it.readText() }
            .orEmpty()
        connection.disconnect()
        return HttpResponse(statusCode, body)
    }

    private fun parseWorldManifests(json: String): List<CloudWorldManifest> {
        val worlds: JSONArray = JSONObject(json).optJSONArray("worlds") ?: JSONArray()
        return buildList {
            for (index in 0 until worlds.length()) {
                val world = worlds.optJSONObject(index) ?: continue
                val id = world.optString("id")
                val name = world.optString("name")
                val region = world.optString("region")
                if (id.isNotBlank() && name.isNotBlank() && region.isNotBlank()) {
                    add(CloudWorldManifest(id, name, region, world.optInt("saveRevision", 0), world.optInt("schemaVersion", 1)))
                }
            }
        }
    }

    private fun publishCloudResult(callback: (List<CloudWorldManifest>?, String?) -> Unit, worlds: List<CloudWorldManifest>?, error: String?) = mainHandler.post { callback(worlds, error) }
    private fun publishRecoveredProfile(callback: (PlayerProfile?, String?) -> Unit, profile: PlayerProfile?, error: String?) = mainHandler.post { callback(profile, error) }
    private fun publishProfileResult(callback: (String?) -> Unit, error: String?) = mainHandler.post { callback(error) }
    private fun publishWorldResult(callback: (CloudWorldManifest?, String?) -> Unit, world: CloudWorldManifest?, error: String?) = mainHandler.post { callback(world, error) }
    private fun publishSnapshotResult(callback: (String?, String?) -> Unit, snapshot: String?, error: String?) = mainHandler.post { callback(snapshot, error) }

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
