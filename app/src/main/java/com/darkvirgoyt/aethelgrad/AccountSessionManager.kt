package com.darkvirgoyt.aethelgrad

import android.app.Activity
import android.os.Handler
import android.os.Looper
import android.util.Base64
import android.util.Log
import androidx.core.content.ContextCompat
import androidx.credentials.CredentialManager
import androidx.credentials.CredentialManagerCallback
import androidx.credentials.CustomCredential
import androidx.credentials.GetCredentialRequest
import androidx.credentials.GetCredentialResponse
import androidx.credentials.exceptions.GetCredentialCancellationException
import androidx.credentials.exceptions.GetCredentialException
import androidx.credentials.exceptions.GetCredentialInterruptedException
import androidx.credentials.exceptions.GetCredentialProviderConfigurationException
import androidx.credentials.exceptions.GetCredentialUnsupportedException
import androidx.credentials.exceptions.NoCredentialException
import com.google.android.libraries.identity.googleid.GetGoogleIdOption
import com.google.android.libraries.identity.googleid.GoogleIdTokenCredential
import org.json.JSONArray
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.security.KeyStore
import java.security.MessageDigest
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import java.util.Locale
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
    val expiresAtEpochMs: Long? = null,
    val isGuest: Boolean = false
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

data class CoOpParticipant(
    val accountId: String,
    val playerX: Float,
    val playerY: Float,
    val atTower: Boolean,
    val towerRevision: Int
)

data class CoOpRoomSnapshot(
    val code: String,
    val region: String,
    val maxPlayers: Int,
    val worldTime: Float,
    val towerRevision: Int,
    val bossHealth: Int,
    val combatRevision: Int,
    val participants: List<CoOpParticipant>,
    val worldName: String = "Aethelgrad Shared World",
    val ownerAccountId: String? = null
)

data class CoOpPlayerSave(
    val worldName: String,
    val ownerAccountId: String?,
    val memberRevision: Int,
    val itemStateJson: String,
    val progressionStateJson: String
)

data class CoOpWorldSave(
    val worldName: String,
    val ownerAccountId: String?,
    val saveRevision: Int,
    val worldStateJson: String,
    val buildingsJson: String
)

data class AuthoritativeCombatResult(
    val action: String,
    val damage: Int,
    val bossHealth: Int,
    val combatRevision: Int
)

data class CompanionTarget(
    val id: String,
    val creatureId: String,
    val x: Float,
    val y: Float,
    val healthFraction: Float,
    val revision: Int
)

data class CompanionStateSnapshot(
    val companionId: String,
    val creatureId: String,
    val displayName: String,
    val command: String,
    val bond: Int,
    val healthFraction: Float,
    val revision: Int
)

data class CampStateSnapshot(
    val id: String,
    val recipeId: String,
    val transformJson: String,
    val stateJson: String,
    val revision: Int
)

data class CompanionCampSnapshot(
    val companion: CompanionStateSnapshot?,
    val camp: CampStateSnapshot?,
    val targets: List<CompanionTarget>
)

data class AuthoritativeCompanionResult(
    val companion: CompanionStateSnapshot,
    val wood: Int,
    val fiber: Int,
    val stone: Int,
    val emberKit: Boolean,
    val inventoryRevision: Int,
    val memberRevision: Int,
    val targetRevision: Int
)

data class AuthoritativeCampResult(
    val camp: CampStateSnapshot,
    val wood: Int,
    val fiber: Int,
    val stone: Int,
    val emberKit: Boolean,
    val inventoryRevision: Int,
    val memberRevision: Int
)

data class AuthoritativeInventoryResult(
    val operation: String,
    val wood: Int,
    val fiber: Int,
    val stone: Int,
    val emberKit: Boolean,
    val inventoryRevision: Int,
    val memberRevision: Int = 0
)

/**
 * Standard Google account sign-in boundary for pre-Play-Console testing.
 * The Android client sends only a Google-issued ID token to the configured HTTPS backend.
 * The backend verifies the token audience, issuer, expiry, and signature before it creates an Aethelgrad session.
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
    private var apiBaseUrl: String = ""
    private var authExchangeUrl: String = ""
    private var authRefreshUrl: String = ""
    private var accessSessionToken: String? = null
    private var refreshSessionToken: String? = null
    private var accessExpiresAtEpochMs: Long? = null

    private companion object {
        const val SESSION_PREFS = "aethelgard_session"
        const val SESSION_BLOB = "encrypted_session"
        const val SESSION_KEY_ALIAS = "aethelgard_session_key"
        const val LAST_COOP_CODE_PREFS = "aethelgard_persistent_world"
        const val LAST_COOP_CODE = "last_world_code"
    }

    fun initialize(activity: Activity, onStateChanged: (SessionSnapshot) -> Unit) {
        this.activity = activity
        stateListener = onStateChanged
        googleWebClientId = activity.getString(R.string.google_web_client_id)
        apiBaseUrl = activity.getString(R.string.api_base_url).trimEnd('/')
        authExchangeUrl = activity.getString(R.string.auth_exchange_url)
        authRefreshUrl = activity.getString(R.string.auth_refresh_url)
        credentialManager = CredentialManager.create(activity)

        if (!hasGoogleConfiguration()) {
            publish(SessionSnapshot(SessionState.CONFIGURATION_ERROR, message = "The HTTPS online game service is not configured."))
            return
        }
        if (!restorePersistedSession()) {
            publish(SessionSnapshot(SessionState.SIGNED_OUT, message = "Sign in to continue to Aethelgrad online."))
        }
    }

    fun requestGoogleSignIn(): SessionSnapshot {
        val owner = activity
        val manager = credentialManager
        if (owner == null || manager == null) {
            Log.w("AethelgardAuth", "Google sign-in requested before the credential service was ready")
            return publish(SessionSnapshot(SessionState.ERROR, message = "Google sign-in is still starting. Close and reopen the game, then try again."))
        }
        if (!hasGoogleConfiguration()) {
            Log.w("AethelgardAuth", "Google sign-in configuration is invalid for package ${owner.packageName}")
            return publish(
                SessionSnapshot(
                    SessionState.CONFIGURATION_ERROR,
                    message = "Google sign-in is not configured for this installed APK. Install the latest build, then verify the Android OAuth package and this APK signing SHA-1."
                )
            )
        }

        publish(SessionSnapshot(SessionState.SIGNING_IN, message = "Choose a Google account to connect…"))
        try {
            val option = GetGoogleIdOption.Builder()
                .setServerClientId(googleWebClientId)
                .setFilterByAuthorizedAccounts(false)
                .setAutoSelectEnabled(false)
                .build()
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
                        publish(SessionSnapshot(SessionState.DENIED, message = describeGoogleCredentialFailure(error)))
                    }
                }
            )
        } catch (error: Exception) {
            Log.w("AethelgardAuth", "Google credential request could not be launched: ${error::class.java.simpleName}")
            return publish(SessionSnapshot(SessionState.DENIED, message = "Google sign-in could not open. Update Google Play services, then retry. If it continues, verify the Android OAuth package and APK signing SHA-1."))
        }
        return snapshot
    }

    /** Keeps provider diagnostics in Logcat and never exposes registration data in the player UI. */
    private fun describeGoogleCredentialFailure(error: GetCredentialException): String {
        Log.w("AethelgardAuth", "Google credential request failed: ${error::class.java.simpleName}")
        return when (error) {
            is GetCredentialCancellationException -> "Google account selection was dismissed before sign-in completed. Choose an account and keep the Google sheet open until you return to the game."
            is NoCredentialException -> "No Google credential is available. Add a Google account to this phone, then retry."
            is GetCredentialProviderConfigurationException -> "Google sign-in is not configured for this release APK. Check its Android OAuth package and signing certificate."
            is GetCredentialUnsupportedException -> "Google sign-in is unavailable on this device. Update Google Play services, then retry."
            is GetCredentialInterruptedException -> "Google sign-in was interrupted. Please try again."
            else -> "Google sign-in could not start. Check the release APK identity and Google Play services, then retry."
        }
    }

    private fun exchangeGoogleIdToken(idToken: String) {
        publish(SessionSnapshot(SessionState.SIGNING_IN, message = "Verifying your Google account with the game server…"))
        networkExecutor.execute {
            try {
                val response = postJson(authExchangeUrl, "{\"idToken\":\"${escapeJson(idToken)}\"}")
                if (!response.isJson()) {
                    Log.w("AethelgardAuth", "Google token exchange returned a non-JSON response")
                    publishFromNetwork(SessionSnapshot(SessionState.CONFIGURATION_ERROR, message = "The game login service returned an unexpected response. Please update the game and try again."))
                    return@execute
                }
                if (response.statusCode !in 200..299) {
                    val serverError = jsonString(response.body, "error")
                    Log.w("AethelgardAuth", "Google token exchange was rejected with HTTP ${response.statusCode}: $serverError")
                    publishFromNetwork(SessionSnapshot(SessionState.ERROR, message = describeExchangeFailure(response.statusCode, serverError)))
                    return@execute
                }
                val sessionToken = jsonString(response.body, "accessToken")
                val accountId = jsonString(response.body, "accountId")
                val refreshToken = jsonString(response.body, "refreshToken")
                val expiresAt = parseIsoEpochMs(jsonString(response.body, "expiresAt"))
                if (sessionToken.isNullOrBlank() || accountId.isNullOrBlank() || refreshToken.isNullOrBlank() || expiresAt == null) {
                    Log.w("AethelgardAuth", "Google token exchange returned an incomplete session payload")
                    publishFromNetwork(SessionSnapshot(SessionState.ERROR, message = "The game login service returned an incomplete secure session. Please try again."))
                    return@execute
                }
                accessSessionToken = sessionToken
                refreshSessionToken = refreshToken
                accessExpiresAtEpochMs = expiresAt
                persistSession(accountId, refreshToken, false)
                publishFromNetwork(
                    SessionSnapshot(
                        SessionState.AUTHENTICATED,
                        accountId = accountId,
                        message = "Google account verified. Select a region to continue.",
                        expiresAtEpochMs = expiresAt
                    )
                )
            } catch (error: Exception) {
                Log.w("AethelgardAuth", "Google token exchange failed: ${error::class.java.simpleName}")
                publishFromNetwork(SessionSnapshot(SessionState.NETWORK_ERROR, message = "Could not reach the game login service. Check your connection and try again."))
            }
        }
    }

    private fun describeExchangeFailure(statusCode: Int, serverError: String?): String = when {
        statusCode == 404 -> "Online login service is not deployed at the configured address. Please update the game and try again."
        statusCode == 503 || serverError == "game_auth_not_configured" -> "Online login service is temporarily unavailable. Try again in a moment."
        serverError == "google_id_token_audience_mismatch" -> "Google login configuration mismatch. Render GOOGLE_ID_TOKEN_AUDIENCE must match the Web OAuth client ID in this game."
        serverError == "google_id_token_issuer_mismatch" -> "Google returned an untrusted account issuer. Choose your Google account again."
        serverError == "google_id_token_expired" -> "The Google sign-in expired. Choose your Google account again."
        serverError == "google_id_token_signature_invalid" -> "Google could not verify this sign-in certificate. Check the Android OAuth package and signing certificate."
        serverError == "google_id_token_verification_failed" || serverError == "google_id_token_authentication_failed" || statusCode == 401 -> "Google account verification was rejected. Check the Web OAuth audience and the release APK identity, then retry."
        serverError == "invalid_google_id_token" -> "Google returned an incomplete sign-in credential. Please choose your account again."
        else -> "Game login was rejected by the service (HTTP $statusCode). Please try again."
    }

    /** Refreshes the rotating backend session; the refresh token is stored only in Android Keystore-backed encrypted storage. */
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
                if (!response.isJson() || response.statusCode !in 200..299) {
                    if (response.statusCode == 401) {
                        clearSession()
                        publishFromNetwork(SessionSnapshot(SessionState.EXPIRED, message = "Your game session has expired. Sign in again to continue."))
                    } else {
                        publishFromNetwork(SessionSnapshot(SessionState.NETWORK_ERROR, snapshot.accountId, "Could not restore your game session. Check your connection and try again.", isGuest = snapshot.isGuest))
                    }
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
                persistSession(snapshot.accountId, rotatedRefresh, snapshot.isGuest)
                publishFromNetwork(SessionSnapshot(SessionState.AUTHENTICATED, snapshot.accountId, "Game session refreshed.", expiresAt, snapshot.isGuest))
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
                val absoluteUrl = if (downloadUrl.startsWith("https://")) downloadUrl else "$apiBaseUrl/${downloadUrl.trimStart('/')}"
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

    fun createCoOpRoom(region: String, onComplete: (CoOpRoomSnapshot?, String?) -> Unit) {
        val token = currentAccessToken()
        if (token.isNullOrBlank()) {
            onComplete(null, "Your game session has expired. Sign in again to create a co-op room.")
            return
        }
        networkExecutor.execute {
            try {
                val response = requestJson("POST", cloudEndpoint("/coop/rooms"), token, JSONObject().put("region", region).toString())
                if (response.statusCode !in 200..299) {
                    publishCoOpResult(onComplete, null, "Could not create the co-op room (${response.statusCode}).")
                    return@execute
                }
                val room = parseCoOpRoom(response.body)
                rememberCoOpRoom(room.code)
                publishCoOpResult(onComplete, room, null)
            } catch (_: Exception) {
                publishCoOpResult(onComplete, null, "Could not create the co-op room. Check your connection.")
            }
        }
    }

    fun joinCoOpRoom(code: String, onComplete: (CoOpRoomSnapshot?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = code.trim().uppercase()
        if (token.isNullOrBlank()) {
            onComplete(null, "Your game session has expired. Sign in again to join a co-op room.")
            return
        }
        if (!Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Enter the six-character tower room code.")
            return
        }
        networkExecutor.execute {
            try {
                val response = requestJson("POST", cloudEndpoint("/coop/rooms/$normalized/join"), token, "{}")
                if (response.statusCode !in 200..299) {
                    publishCoOpResult(onComplete, null, if (response.statusCode == 404) "Tower room not found." else if (response.statusCode == 409) "Tower room is full." else "Could not join the co-op room (${response.statusCode}).")
                    return@execute
                }
                val room = parseCoOpRoom(response.body)
                rememberCoOpRoom(room.code)
                publishCoOpResult(onComplete, room, null)
            } catch (_: Exception) {
                publishCoOpResult(onComplete, null, "Could not join the co-op room. Check your connection.")
            }
        }
    }

    fun lastPersistentCoOpRoomCode(): String? = activity?.getSharedPreferences(LAST_COOP_CODE_PREFS, Activity.MODE_PRIVATE)?.getString(LAST_COOP_CODE, null)

    fun rememberCoOpRoom(code: String) {
        activity?.getSharedPreferences(LAST_COOP_CODE_PREFS, Activity.MODE_PRIVATE)?.edit()?.putString(LAST_COOP_CODE, code.trim().uppercase())?.apply()
    }

    /** Reopens the last creator-owned or joined persistent world after an app restart. */
    fun reconnectLastPersistentCoOpWorld(onComplete: (CoOpRoomSnapshot?, String?) -> Unit) {
        val code = lastPersistentCoOpRoomCode()
        if (code.isNullOrBlank()) {
            onComplete(null, "No saved multiplayer world is available on this device.")
            return
        }
        reconnectCoOpRoom(code, onComplete)
    }

    fun loadCoOpPlayerSave(roomCode: String, onComplete: (CoOpPlayerSave?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your persistent world session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val response = getJson(cloudEndpoint("/coop/rooms/$normalized/player-save"), token)
                if (response.statusCode !in 200..299) {
                    publishCoOpSaveResult(onComplete, null, "Could not load your saved items and progression (${response.statusCode}).")
                    return@execute
                }
                val root = JSONObject(response.body)
                publishCoOpSaveResult(onComplete, CoOpPlayerSave(root.optString("worldName", "Aethelgrad Shared World"), root.optString("ownerAccountId").takeIf { it.isNotBlank() }, root.optInt("memberRevision"), root.optJSONObject("itemState")?.toString() ?: "{}", root.optJSONObject("progressionState")?.toString() ?: "{}"), null)
            } catch (_: Exception) {
                publishCoOpSaveResult(onComplete, null, "Could not load your saved multiplayer state.")
            }
        }
    }

    fun saveCoOpPlayerState(roomCode: String, expectedRevision: Int, itemStateJson: String, progressionStateJson: String, onComplete: (Int?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your persistent world session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val payload = JSONObject().put("expectedRevision", expectedRevision).put("itemState", JSONObject(itemStateJson)).put("progressionState", JSONObject(progressionStateJson)).toString()
                val response = requestJson("PUT", cloudEndpoint("/coop/rooms/$normalized/player-save"), token, payload)
                if (response.statusCode !in 200..299) {
                    publishIntResult(onComplete, null, if (response.statusCode == 409) "Your save changed on another device. Reload the world before saving again." else "Could not save your items and progression (${response.statusCode}).")
                    return@execute
                }
                publishIntResult(onComplete, JSONObject(response.body).optInt("memberRevision"), null)
            } catch (_: Exception) {
                publishIntResult(onComplete, null, "Could not save your multiplayer state.")
            }
        }
    }

    fun loadCoOpWorldSave(roomCode: String, onComplete: (CoOpWorldSave?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your persistent world session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val response = getJson(cloudEndpoint("/coop/rooms/$normalized/save"), token)
                if (response.statusCode !in 200..299) {
                    publishCoOpWorldSaveResult(onComplete, null, "Could not load the shared world save (${response.statusCode}).")
                    return@execute
                }
                val root = JSONObject(response.body)
                publishCoOpWorldSaveResult(onComplete, CoOpWorldSave(root.optString("worldName", "Aethelgrad Shared World"), root.optString("ownerAccountId").takeIf { it.isNotBlank() }, root.optInt("saveRevision"), root.optJSONObject("worldState")?.toString() ?: "{}", root.optJSONArray("buildings")?.toString() ?: "[]"), null)
            } catch (_: Exception) {
                publishCoOpWorldSaveResult(onComplete, null, "Could not load the shared world save.")
            }
        }
    }

    /** Reconnects to an existing room after a transient heartbeat/network interruption. */
    fun reconnectCoOpRoom(roomCode: String, onComplete: (CoOpRoomSnapshot?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your co-op session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val response = requestJson("POST", cloudEndpoint("/coop/rooms/$normalized/reconnect"), token, "{}")
                if (response.statusCode !in 200..299) {
                    publishCoOpResult(onComplete, null, if (response.statusCode == 404) "Tower room is no longer available." else if (response.statusCode == 403) "Your membership has expired." else "Reconnect rejected (${response.statusCode}).")
                    return@execute
                }
                val room = parseCoOpRoom(response.body)
                rememberCoOpRoom(room.code)
                publishCoOpResult(onComplete, room, null)
            } catch (_: Exception) {
                publishCoOpResult(onComplete, null, "Reconnect server unavailable.")
            }
        }
    }

    fun heartbeatCoOpRoom(roomCode: String, playerX: Float, playerY: Float, atTower: Boolean, towerRevision: Int, onComplete: (CoOpRoomSnapshot?, String?) -> Unit = { _, _ -> }) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) return
        networkExecutor.execute {
            try {
                val payload = JSONObject()
                    .put("playerX", playerX)
                    .put("playerY", playerY)
                    .put("atTower", atTower)
                    .put("towerRevision", towerRevision)
                    .toString()
                val response = requestJson("POST", cloudEndpoint("/coop/rooms/$normalized/heartbeat"), token, payload)
                if (response.statusCode !in 200..299) {
                    publishCoOpResult(onComplete, null, "Co-op room heartbeat failed (${response.statusCode}).")
                    return@execute
                }
                val room = parseCoOpRoom(response.body)
                rememberCoOpRoom(room.code)
                publishCoOpResult(onComplete, room, null)
            } catch (_: Exception) {
                publishCoOpResult(onComplete, null, "Co-op room connection lost.")
            }
        }
    }

    fun authoritativeCombat(roomCode: String, requestId: String, action: String, targetId: String = "forest_warden", onComplete: (AuthoritativeCombatResult?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your co-op session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val payload = JSONObject().put("requestId", requestId).put("action", action).put("targetId", targetId).toString()
                val response = requestJson("POST", cloudEndpoint("/coop/rooms/$normalized/combat"), token, payload)
                if (response.statusCode !in 200..299) {
                    publishCombatResult(onComplete, null, if (response.statusCode == 429) "Combat cooldown active." else if (response.statusCode == 409) "Move closer to the target." else "Combat request rejected (${response.statusCode}).")
                    return@execute
                }
                val result = JSONObject(response.body)
                publishCombatResult(onComplete, AuthoritativeCombatResult(result.optString("action"), result.optInt("damage"), result.optInt("bossHealth"), result.optInt("combatRevision")), null)
            } catch (_: Exception) {
                publishCombatResult(onComplete, null, "Authoritative combat service unavailable.")
            }
        }
    }

    fun authoritativeInventory(roomCode: String, requestId: String, operation: String, resourceId: String? = null, onComplete: (AuthoritativeInventoryResult?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your co-op session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val payload = JSONObject().put("requestId", requestId).put("operation", operation)
                if (!resourceId.isNullOrBlank()) payload.put("resourceId", resourceId)
                val response = requestJson("POST", cloudEndpoint("/coop/rooms/$normalized/inventory"), token, payload.toString())
                if (response.statusCode !in 200..299) {
                    publishInventoryResult(onComplete, null, if (response.statusCode == 409) "Move closer or gather more materials." else "Inventory request rejected (${response.statusCode}).")
                    return@execute
                }
                val result = JSONObject(response.body)
                val inventory = result.getJSONObject("inventory")
                publishInventoryResult(onComplete, AuthoritativeInventoryResult(result.optString("operation"), inventory.optInt("wood"), inventory.optInt("fiber"), inventory.optInt("stone"), inventory.optBoolean("emberKit"), result.optInt("inventoryRevision"), result.optInt("memberRevision")), null)
            } catch (_: Exception) {
                publishInventoryResult(onComplete, null, "Authoritative inventory service unavailable.")
            }
        }
    }

    fun fetchCompanionCampState(roomCode: String, onComplete: (CompanionCampSnapshot?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your persistent world session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val response = getJson(cloudEndpoint("/coop/rooms/$normalized/companions"), token)
                if (response.statusCode !in 200..299) {
                    publishCompanionCampResult(onComplete, null, "Could not load companion and camp state (${response.statusCode}).")
                    return@execute
                }
                publishCompanionCampResult(onComplete, parseCompanionCampSnapshot(response.body), null)
            } catch (_: Exception) {
                publishCompanionCampResult(onComplete, null, "Could not load companion and camp state.")
            }
        }
    }

    fun captureCompanion(roomCode: String, requestId: String, creatureId: String, onComplete: (AuthoritativeCompanionResult?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your persistent world session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val payload = JSONObject().put("requestId", requestId).put("creatureId", creatureId).toString()
                val response = requestJson("POST", cloudEndpoint("/coop/rooms/$normalized/companions/capture"), token, payload)
                if (response.statusCode !in 200..299) {
                    publishAuthoritativeCompanionResult(onComplete, null, when (response.statusCode) {
                        409 -> "You already have an active companion."
                        404 -> "That creature is no longer available."
                        422 -> "The creature cannot be captured yet, or you need more Fiber."
                        else -> "Companion capture rejected (${response.statusCode})."
                    })
                    return@execute
                }
                val root = JSONObject(response.body)
                val inventory = root.optJSONObject("inventory") ?: JSONObject()
                val companion = parseCompanion(root.getJSONObject("companion"))
                publishAuthoritativeCompanionResult(onComplete, AuthoritativeCompanionResult(companion, inventory.optInt("wood"), inventory.optInt("fiber"), inventory.optInt("stone"), inventory.optBoolean("emberKit"), root.optInt("inventoryRevision"), root.optInt("memberRevision"), root.optInt("targetRevision")), null)
            } catch (_: Exception) {
                publishAuthoritativeCompanionResult(onComplete, null, "Companion capture service unavailable.")
            }
        }
    }

    fun setCompanionCommand(roomCode: String, requestId: String, command: String, expectedRevision: Int, onComplete: (CompanionStateSnapshot?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your persistent world session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val payload = JSONObject().put("requestId", requestId).put("command", command).put("expectedRevision", expectedRevision).toString()
                val response = requestJson("POST", cloudEndpoint("/coop/rooms/$normalized/companions/command"), token, payload)
                if (response.statusCode !in 200..299) {
                    publishCompanionStateResult(onComplete, null, if (response.statusCode == 409) "Your companion changed elsewhere. Reload its state." else "Companion command rejected (${response.statusCode}).")
                    return@execute
                }
                publishCompanionStateResult(onComplete, parseCompanion(JSONObject(response.body).getJSONObject("companion")), null)
            } catch (_: Exception) {
                publishCompanionStateResult(onComplete, null, "Companion command service unavailable.")
            }
        }
    }

    fun placeFieldCamp(roomCode: String, requestId: String, transform: JSONObject, expectedRevision: Int = 0, onComplete: (AuthoritativeCampResult?, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(null, "Your persistent world session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val payload = JSONObject().put("requestId", requestId).put("recipeId", "field_camp").put("expectedRevision", expectedRevision).put("transform", transform).toString()
                val response = requestJson("POST", cloudEndpoint("/coop/rooms/$normalized/camps"), token, payload)
                if (response.statusCode !in 200..299) {
                    publishAuthoritativeCampResult(onComplete, null, if (response.statusCode == 422) "Camp placement rejected: check range, materials, or your existing camp." else "Camp placement rejected (${response.statusCode}).")
                    return@execute
                }
                val root = JSONObject(response.body)
                val inventory = root.optJSONObject("inventory") ?: JSONObject()
                publishAuthoritativeCampResult(onComplete, AuthoritativeCampResult(parseCamp(root.getJSONObject("camp")), inventory.optInt("wood"), inventory.optInt("fiber"), inventory.optInt("stone"), inventory.optBoolean("emberKit"), root.optInt("inventoryRevision"), root.optInt("memberRevision")), null)
            } catch (_: Exception) {
                publishAuthoritativeCampResult(onComplete, null, "Camp building service unavailable.")
            }
        }
    }

    fun removeFieldCamp(roomCode: String, campId: String, requestId: String, expectedRevision: Int, onComplete: (Boolean, String?) -> Unit) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) {
            onComplete(false, "Your persistent world session is not active.")
            return
        }
        networkExecutor.execute {
            try {
                val payload = JSONObject().put("requestId", requestId).put("expectedRevision", expectedRevision).toString()
                val response = requestJson("DELETE", cloudEndpoint("/coop/rooms/$normalized/camps/${campId.trim()}"), token, payload)
                if (response.statusCode !in 200..299) {
                    publishBooleanResult(onComplete, false, if (response.statusCode == 409) "Your camp changed elsewhere. Reload the world." else "Camp removal rejected (${response.statusCode}).")
                    return@execute
                }
                publishBooleanResult(onComplete, true, null)
            } catch (_: Exception) {
                publishBooleanResult(onComplete, false, "Camp building service unavailable.")
            }
        }
    }

    fun leaveCoOpRoom(roomCode: String) {
        val token = currentAccessToken()
        val normalized = roomCode.trim().uppercase()
        if (token.isNullOrBlank() || !Regex("[A-Z0-9]{6}").matches(normalized)) return
        networkExecutor.execute { runCatching { requestJson("DELETE", cloudEndpoint("/coop/rooms/$normalized/leave"), token, null) } }
    }

    private fun parseCoOpRoom(json: String): CoOpRoomSnapshot {
        val root = JSONObject(json)
        val room = root.getJSONObject("room")
        val participantsJson = root.optJSONArray("participants") ?: JSONArray()
        val participants = buildList {
            for (index in 0 until participantsJson.length()) {
                val member = participantsJson.optJSONObject(index) ?: continue
                val accountId = member.optString("accountId")
                if (accountId.isNotBlank()) add(CoOpParticipant(
                    accountId = accountId,
                    playerX = member.optDouble("playerX", -0.55).toFloat(),
                    playerY = member.optDouble("playerY", -0.08).toFloat(),
                    atTower = member.optBoolean("atTower", false),
                    towerRevision = member.optInt("towerRevision", 0)
                ))
            }
        }
        return CoOpRoomSnapshot(
            code = room.optString("code"),
            region = room.optString("region"),
            maxPlayers = room.optInt("maxPlayers", 4),
            worldTime = room.optDouble("worldTime", 0.0).toFloat(),
            towerRevision = room.optLong("towerRevision", 0L).coerceAtMost(Int.MAX_VALUE.toLong()).toInt(),
            bossHealth = room.optInt("bossHealth", 100).coerceIn(0, 100),
            combatRevision = room.optLong("combatRevision", 0L).coerceAtMost(Int.MAX_VALUE.toLong()).toInt(),
            participants = participants,
            worldName = room.optString("worldName", "Aethelgrad Shared World"),
            ownerAccountId = room.optString("ownerAccountId").takeIf { it.isNotBlank() }
        )
    }

    private fun parseCompanionCampSnapshot(json: String): CompanionCampSnapshot {
        val root = JSONObject(json)
        val companion = root.optJSONObject("companion")?.let(::parseCompanion)
        val camp = root.optJSONObject("camp")?.let(::parseCamp)
        val targetsJson = root.optJSONArray("targets") ?: JSONArray()
        val targets = buildList {
            for (index in 0 until targetsJson.length()) {
                val target = targetsJson.optJSONObject(index) ?: continue
                val id = target.optString("id")
                val creatureId = target.optString("creature_id", target.optString("creatureId"))
                if (id.isNotBlank() && creatureId.isNotBlank()) add(CompanionTarget(
                    id = id,
                    creatureId = creatureId,
                    x = target.optDouble("position_x", target.optDouble("x", 0.0)).toFloat(),
                    y = target.optDouble("position_y", target.optDouble("y", 0.0)).toFloat(),
                    healthFraction = target.optDouble("health_fraction", target.optDouble("healthFraction", 1.0)).toFloat(),
                    revision = target.optInt("revision", 0)
                ))
            }
        }
        return CompanionCampSnapshot(companion, camp, targets)
    }

    private fun parseCompanion(json: JSONObject): CompanionStateSnapshot = CompanionStateSnapshot(
        companionId = json.optString("companion_id", json.optString("companionId")),
        creatureId = json.optString("creature_id", json.optString("creatureId")),
        displayName = json.optString("display_name", json.optString("displayName", "Companion")),
        command = json.optString("command", "follow"),
        bond = json.optInt("bond", 0),
        healthFraction = json.optDouble("health_fraction", json.optDouble("healthFraction", 0.75)).toFloat(),
        revision = json.optInt("revision", 0)
    )

    private fun parseCamp(json: JSONObject): CampStateSnapshot {
        val transform = json.optJSONObject("transform")?.toString() ?: json.optString("transform", "{}")
        val state = json.optJSONObject("state")?.toString() ?: json.optString("state", "{}")
        return CampStateSnapshot(
            id = json.optString("id"),
            recipeId = json.optString("recipe_id", json.optString("recipeId")),
            transformJson = transform,
            stateJson = state,
            revision = json.optInt("revision", 0)
        )
    }

    fun signOut(): SessionSnapshot {
        clearSession()
        return publish(SessionSnapshot(SessionState.SIGNED_OUT, message = "Signed out"))
    }

    fun shutdown() {
        activity = null
        networkExecutor.shutdownNow()
    }

    private fun hasGoogleConfiguration(): Boolean =
        googleWebClientId.endsWith(".apps.googleusercontent.com") &&
            !googleWebClientId.startsWith("REPLACE_") &&
            !apiBaseUrl.startsWith("REPLACE_") &&
            !authExchangeUrl.startsWith("REPLACE_") &&
            !authRefreshUrl.startsWith("REPLACE_") &&
            apiBaseUrl.startsWith("https://") &&
            authExchangeUrl.startsWith("https://") &&
            authRefreshUrl.startsWith("https://")

    private fun parseSessionBundle(response: HttpResponse): SessionBundle? {
        val root = JSONObject(response.body)
        val accessToken = root.optString("accessToken").takeIf { it.isNotBlank() }
        val refreshToken = root.optString("refreshToken").takeIf { it.isNotBlank() }
        val accountId = root.optString("accountId").takeIf { it.isNotBlank() }
        val expiresAt = parseIsoEpochMs(root.optString("expiresAt"))
        return if (accessToken != null && refreshToken != null && accountId != null && expiresAt != null) {
            SessionBundle(accessToken, refreshToken, accountId, expiresAt)
        } else null
    }

    private fun cloudEndpoint(path: String): String = "$apiBaseUrl/${path.trimStart('/')}"

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
        return HttpResponse(statusCode, body, connection.contentType.orEmpty())
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
    private fun publishCoOpResult(callback: (CoOpRoomSnapshot?, String?) -> Unit, room: CoOpRoomSnapshot?, error: String?) = mainHandler.post { callback(room, error) }
    private fun publishCombatResult(callback: (AuthoritativeCombatResult?, String?) -> Unit, result: AuthoritativeCombatResult?, error: String?) = mainHandler.post { callback(result, error) }
    private fun publishInventoryResult(callback: (AuthoritativeInventoryResult?, String?) -> Unit, result: AuthoritativeInventoryResult?, error: String?) = mainHandler.post { callback(result, error) }
    private fun publishCoOpSaveResult(callback: (CoOpPlayerSave?, String?) -> Unit, result: CoOpPlayerSave?, error: String?) = mainHandler.post { callback(result, error) }
    private fun publishIntResult(callback: (Int?, String?) -> Unit, result: Int?, error: String?) = mainHandler.post { callback(result, error) }
    private fun publishCoOpWorldSaveResult(callback: (CoOpWorldSave?, String?) -> Unit, result: CoOpWorldSave?, error: String?) = mainHandler.post { callback(result, error) }
    private fun publishCompanionCampResult(callback: (CompanionCampSnapshot?, String?) -> Unit, result: CompanionCampSnapshot?, error: String?) = mainHandler.post { callback(result, error) }
    private fun publishAuthoritativeCompanionResult(callback: (AuthoritativeCompanionResult?, String?) -> Unit, result: AuthoritativeCompanionResult?, error: String?) = mainHandler.post { callback(result, error) }
    private fun publishCompanionStateResult(callback: (CompanionStateSnapshot?, String?) -> Unit, result: CompanionStateSnapshot?, error: String?) = mainHandler.post { callback(result, error) }
    private fun publishAuthoritativeCampResult(callback: (AuthoritativeCampResult?, String?) -> Unit, result: AuthoritativeCampResult?, error: String?) = mainHandler.post { callback(result, error) }
    private fun publishBooleanResult(callback: (Boolean, String?) -> Unit, result: Boolean, error: String?) = mainHandler.post { callback(result, error) }

    private fun restorePersistedSession(): Boolean {
        val owner = activity ?: return false
        return try {
            val encoded = owner.getSharedPreferences(SESSION_PREFS, Activity.MODE_PRIVATE)
                .getString(SESSION_BLOB, null) ?: return false
            val payload = decryptSession(encoded) ?: return false
            val root = JSONObject(payload)
            val accountId = root.optString("accountId").takeIf { it.isNotBlank() } ?: return false
            val refreshToken = root.optString("refreshToken").takeIf { it.isNotBlank() } ?: return false
            val isGuest = root.optBoolean("isGuest", false)
            if (isGuest) {
                // Guest sessions were a development entry path. Do not let an
                // old encrypted guest session bypass the production login.
                clearPersistedSession()
                return false
            }
            refreshSessionToken = refreshToken
            publish(SessionSnapshot(SessionState.SIGNING_IN, accountId, "Restoring your secure game session…"))
            refreshSession()
            true
        } catch (_: Exception) {
            clearPersistedSession()
            false
        }
    }

    private fun persistSession(accountId: String?, refreshToken: String, isGuest: Boolean) {
        val owner = activity ?: return
        if (accountId.isNullOrBlank() || refreshToken.isBlank()) return
        runCatching {
            val payload = JSONObject()
                .put("accountId", accountId)
                .put("refreshToken", refreshToken)
                .put("isGuest", isGuest)
                .toString()
            owner.getSharedPreferences(SESSION_PREFS, Activity.MODE_PRIVATE)
                .edit()
                .putString(SESSION_BLOB, encryptSession(payload))
                .apply()
        }
    }

    private fun clearPersistedSession() {
        activity?.getSharedPreferences(SESSION_PREFS, Activity.MODE_PRIVATE)
            ?.edit()
            ?.remove(SESSION_BLOB)
            ?.apply()
    }

    private fun sessionKey(): SecretKey {
        val keyStore = KeyStore.getInstance("AndroidKeyStore").apply { load(null) }
        (keyStore.getKey(SESSION_KEY_ALIAS, null) as? SecretKey)?.let { return it }
        return KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore").apply {
            init(
                KeyGenParameterSpec.Builder(
                    SESSION_KEY_ALIAS,
                    KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT
                )
                    .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                    .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                    .setUserAuthenticationRequired(false)
                    .build()
            )
        }.generateKey()
    }

    private fun encryptSession(payload: String): String {
        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        cipher.init(Cipher.ENCRYPT_MODE, sessionKey())
        val encrypted = cipher.iv + cipher.doFinal(payload.toByteArray(Charsets.UTF_8))
        return Base64.encodeToString(encrypted, Base64.NO_WRAP)
    }

    private fun decryptSession(encoded: String): String? {
        val encrypted = Base64.decode(encoded, Base64.DEFAULT)
        if (encrypted.size <= 12) return null
        val iv = encrypted.copyOfRange(0, 12)
        val ciphertext = encrypted.copyOfRange(12, encrypted.size)
        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        cipher.init(Cipher.DECRYPT_MODE, sessionKey(), GCMParameterSpec(128, iv))
        return cipher.doFinal(ciphertext).toString(Charsets.UTF_8)
    }

    private fun clearSession() {
        accessSessionToken = null
        refreshSessionToken = null
        accessExpiresAtEpochMs = null
        clearPersistedSession()
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

    private data class SessionBundle(val accessToken: String, val refreshToken: String, val accountId: String, val expiresAt: Long)
    private data class HttpResponse(val statusCode: Int, val body: String, val contentType: String) {
        fun isJson(): Boolean = contentType.lowercase(Locale.US).contains("application/json") || body.trimStart().startsWith("{")
    }
}
