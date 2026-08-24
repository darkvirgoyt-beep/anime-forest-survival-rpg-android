package com.darkvirgoyt.forestslice

import android.os.Handler
import android.os.Looper
import java.io.File
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

sealed interface AssetPreparationState {
    data object Idle : AssetPreparationState
    data object CheckingCache : AssetPreparationState
    data class Retrying(val attempt: Int, val maxAttempts: Int) : AssetPreparationState
    data class Downloading(val attempt: Int) : AssetPreparationState
    data class HighEndReady(val manifestVersion: String) : AssetPreparationState
    data class StarterPackReady(val reason: FallbackReason) : AssetPreparationState
}

enum class FallbackReason {
    NoManifestConfigured,
    PrivateServiceUnavailable,
    Unauthorized,
    InvalidManifest,
    NetworkUnavailable,
    CacheInvalid,
}

/**
 * Prepares optional high-end content without making the playable starter pack dependent on
 * a private service. The manager only owns manifest/cache state; gameplay remains available
 * while optional content is unavailable.
 */
class AssetPreparationManager(
    private val cacheDir: File,
    private val manifestUrl: String,
    private val onStateChanged: (AssetPreparationState) -> Unit,
) {
    companion object {
        private const val MAX_ATTEMPTS = 3
        private const val CONNECT_TIMEOUT_MS = 4_000
        private const val READ_TIMEOUT_MS = 6_000
        private const val MAX_MANIFEST_BYTES = 256 * 1024
        private const val CACHE_FILE = "high_end_manifest.json"
    }

    private val executor: ExecutorService = Executors.newSingleThreadExecutor()
    private val mainHandler = Handler(Looper.getMainLooper())
    @Volatile private var closed = false
    @Volatile private var state: AssetPreparationState = AssetPreparationState.Idle

    fun currentState(): AssetPreparationState = state

    fun start() {
        if (closed) return
        publish(AssetPreparationState.CheckingCache)
        executor.execute {
            if (hasValidCache()) {
                publish(AssetPreparationState.HighEndReady("cached"))
            } else if (manifestUrl.isBlank()) {
                publish(AssetPreparationState.StarterPackReady(FallbackReason.NoManifestConfigured))
            } else {
                prepareWithFallback()
            }
        }
    }

    fun retry() {
        if (closed) return
        publish(AssetPreparationState.CheckingCache)
        executor.execute { prepareWithFallback() }
    }

    fun close() {
        closed = true
        executor.shutdownNow()
        mainHandler.removeCallbacksAndMessages(null)
    }

    private fun prepareWithFallback() {
        if (manifestUrl.isBlank()) {
            publish(AssetPreparationState.StarterPackReady(FallbackReason.NoManifestConfigured))
            return
        }

        var fallbackReason = FallbackReason.NetworkUnavailable
        for (attempt in 1..MAX_ATTEMPTS) {
            if (closed) return
            if (attempt > 1) {
                publish(AssetPreparationState.Retrying(attempt, MAX_ATTEMPTS))
                try {
                    Thread.sleep(250L shl (attempt - 2))
                } catch (_: InterruptedException) {
                    return
                }
            }
            publish(AssetPreparationState.Downloading(attempt))
            when (val result = downloadManifest()) {
                is DownloadResult.Success -> {
                    if (writeCache(result.bytes)) {
                        publish(AssetPreparationState.HighEndReady(result.version))
                        return
                    }
                    fallbackReason = FallbackReason.CacheInvalid
                }
                is DownloadResult.Failure -> {
                    fallbackReason = result.reason
                    if (!result.retryable) break
                }
            }
        }
        publish(AssetPreparationState.StarterPackReady(fallbackReason))
    }

    private fun downloadManifest(): DownloadResult {
        var connection: HttpURLConnection? = null
        return try {
            connection = (URL(manifestUrl).openConnection() as HttpURLConnection).apply {
                requestMethod = "GET"
                connectTimeout = CONNECT_TIMEOUT_MS
                readTimeout = READ_TIMEOUT_MS
                useCaches = true
                setRequestProperty("Accept", "application/json")
                setRequestProperty("Cache-Control", "no-cache")
            }
            val responseCode = connection.responseCode
            if (responseCode !in 200..299) {
                DownloadResult.Failure(
                    reason = when (responseCode) {
                        HttpURLConnection.HTTP_UNAUTHORIZED,
                        HttpURLConnection.HTTP_FORBIDDEN -> FallbackReason.Unauthorized
                        HttpURLConnection.HTTP_NOT_FOUND,
                        HttpURLConnection.HTTP_BAD_REQUEST -> FallbackReason.InvalidManifest
                        else -> FallbackReason.PrivateServiceUnavailable
                    },
                    retryable = responseCode == HttpURLConnection.HTTP_UNAVAILABLE || responseCode >= 500,
                )
            } else {
                val bytes = connection.inputStream.use { input ->
                    val output = ByteArray(MAX_MANIFEST_BYTES)
                    var total = 0
                    while (total < output.size) {
                        val read = input.read(output, total, output.size - total)
                        if (read < 0) break
                        total += read
                    }
                    if (input.read() >= 0) return@use null
                    output.copyOf(total)
                }
                if (bytes == null || !isValidManifest(bytes)) {
                    DownloadResult.Failure(FallbackReason.InvalidManifest, retryable = false)
                } else {
                    DownloadResult.Success(bytes, version = manifestVersion(bytes))
                }
            }
        } catch (_: SecurityException) {
            DownloadResult.Failure(FallbackReason.NetworkUnavailable, retryable = false)
        } catch (_: Exception) {
            DownloadResult.Failure(FallbackReason.NetworkUnavailable, retryable = true)
        } finally {
            connection?.disconnect()
        }
    }

    private fun hasValidCache(): Boolean {
        val file = File(cacheDir, CACHE_FILE)
        return file.isFile && file.length() > 2L && file.length() <= MAX_MANIFEST_BYTES && isValidManifest(file.readBytes())
    }

    private fun writeCache(bytes: ByteArray): Boolean {
        return try {
            if (!isValidManifest(bytes)) return false
            cacheDir.mkdirs()
            val temporary = File(cacheDir, "$CACHE_FILE.tmp")
            temporary.writeBytes(bytes)
            if (temporary.length() != bytes.size.toLong()) return false
            val target = File(cacheDir, CACHE_FILE)
            if (target.exists() && !target.delete()) return false
            temporary.renameTo(target)
        } catch (_: Exception) {
            false
        }
    }

    private fun isValidManifest(bytes: ByteArray): Boolean {
        val text = bytes.toString(Charsets.UTF_8).trim()
        return text.length >= 2 && text.first() == '{' && text.last() == '}' &&
            (text.contains("\"version\"") || text.contains("\"assets\""))
    }

    private fun manifestVersion(bytes: ByteArray): String {
        val text = bytes.toString(Charsets.UTF_8)
        val marker = "\"version\""
        val start = text.indexOf(marker)
        if (start < 0) return "remote"
        val colon = text.indexOf(':', start + marker.length)
        val firstQuote = text.indexOf('"', colon + 1)
        val secondQuote = text.indexOf('"', firstQuote + 1)
        return if (colon >= 0 && firstQuote >= 0 && secondQuote > firstQuote) {
            text.substring(firstQuote + 1, secondQuote)
        } else {
            "remote"
        }
    }

    private fun publish(next: AssetPreparationState) {
        state = next
        mainHandler.post {
            if (!closed) onStateChanged(next)
        }
    }

    private sealed interface DownloadResult {
        data class Success(val bytes: ByteArray, val version: String) : DownloadResult
        data class Failure(val reason: FallbackReason, val retryable: Boolean) : DownloadResult
    }
}
