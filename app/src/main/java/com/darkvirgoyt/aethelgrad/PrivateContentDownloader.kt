package com.darkvirgoyt.aethelgrad

import android.content.Context
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.RandomAccessFile
import java.net.HttpURLConnection
import java.net.SocketTimeoutException
import java.net.URL
import java.security.MessageDigest
import java.util.concurrent.Executors

/**
 * Private distribution path for the four-player build.
 *
 * This is used when Play Asset Delivery is not available. The backend serves a
 * signed metadata document and the matching APK-identity OBB over HTTPS. The
 * archive is resumed with HTTP Range, verified by exact byte count and SHA-256,
 * and moved atomically into Context.obbDir only after verification succeeds.
 */
class PrivateContentDownloader(
    context: Context,
    private val manifestUrl: String,
    private val configuredArchiveUrl: String
) {
    data class Progress(
        val status: Status,
        val bytesDownloaded: Long,
        val totalBytes: Long,
        val percent: Int,
        val detail: String? = null
    )

    enum class Status { DOWNLOADING, COMPLETED, FAILED }

    private val appContext = context.applicationContext
    private val executor = Executors.newSingleThreadExecutor()

    val configured: Boolean
        get() = manifestUrl.startsWith("https://") && configuredArchiveUrl.startsWith("https://")

    fun downloadHighEndContent(onProgress: (Progress) -> Unit) {
        if (!configured) {
            onProgress(Progress(Status.FAILED, 0L, 0L, 0, "Private content service is not configured."))
            return
        }
        executor.execute {
            try {
                val manifest = fetchManifest()
                val archiveUrl = manifest.optString("archiveUrl").ifBlank { configuredArchiveUrl }
                require(archiveUrl.startsWith("https://")) { "Private content archive must use HTTPS." }
                val expectedBytes = manifest.getLong("archiveBytes")
                val expectedSha256 = manifest.getString("archiveSha256").lowercase()
                require(expectedBytes > 0L) { "Private content manifest has no archive size." }
                require(expectedSha256.matches(Regex("[0-9a-f]{64}"))) { "Private content manifest has an invalid SHA-256." }
                val packageName = manifest.getString("packageName")
                val versionCode = manifest.getLong("versionCode")
                require(packageName == appContext.packageName) { "Private content package does not match this APK." }
                @Suppress("DEPRECATION")
                val installedVersionCode = appContext.packageManager.getPackageInfo(appContext.packageName, 0).versionCode.toLong()
                require(versionCode == installedVersionCode) {
                    "Private content version does not match this APK."
                }

                val partial = File(appContext.obbDir, ".high-content.obb.part")
                require(partial.parentFile?.mkdirs() != false) { "Private content OBB directory is unavailable." }
                val existing = partial.takeIf { it.isFile }?.length() ?: 0L
                require(existing <= expectedBytes) { "Partial private content is larger than the manifest archive." }
                if (existing == expectedBytes && sha256(partial) == expectedSha256) {
                    mountVerified(partial, versionCode, packageName)
                    onProgress(Progress(Status.COMPLETED, expectedBytes, expectedBytes, 100))
                    return@execute
                }

                val response = openArchive(archiveUrl, existing)
                val append = existing > 0L && response.responseCode == HttpURLConnection.HTTP_PARTIAL
                val startingBytes = if (append) existing else 0L
                if (!append) partial.delete()
                val totalBytes = expectedBytes
                response.inputStream.use { input ->
                    FileOutputStream(partial, append).use { output ->
                        val buffer = ByteArray(1024 * 1024)
                        var downloaded = startingBytes
                        var read: Int
                        while (input.read(buffer).also { read = it } >= 0) {
                            if (read == 0) continue
                            output.write(buffer, 0, read)
                            downloaded += read
                            val percent = (downloaded.toDouble() * 100.0 / totalBytes.toDouble()).toInt().coerceIn(1, 99)
                            onProgress(Progress(Status.DOWNLOADING, downloaded, totalBytes, percent))
                        }
                    }
                }
                response.disconnect()
                require(partial.length() == expectedBytes) {
                    "Private content size mismatch: ${partial.length()} != $expectedBytes."
                }
                require(sha256(partial) == expectedSha256) { "Private content SHA-256 verification failed." }
                mountVerified(partial, versionCode, packageName)
                onProgress(Progress(Status.COMPLETED, expectedBytes, expectedBytes, 100))
            } catch (_: SocketTimeoutException) {
                onProgress(Progress(Status.FAILED, 0L, 0L, 0, "Private high-end content service timed out. The archive may be unavailable or waking up; retry later."))
            } catch (error: IOException) {
                onProgress(Progress(Status.FAILED, 0L, 0L, 0, "Private high-end content network failure: ${error.message ?: "connection error"}"))
            } catch (error: Exception) {
                onProgress(Progress(Status.FAILED, 0L, 0L, 0, error.message ?: "Private content download failed."))
            }
        }
    }

    fun close() {
        executor.shutdownNow()
    }

    private fun fetchManifest(): JSONObject {
        val connection = openHttps(manifestUrl)
        return try {
            val responseCode = connection.responseCode
            if (responseCode !in 200..299) {
                throw IOException("Private high-end content manifest HTTP $responseCode${readFailureBody(connection)}")
            }
            JSONObject(connection.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() })
        } finally {
            connection.disconnect()
        }
    }

    private fun openArchive(url: String, existing: Long): HttpURLConnection {
        val connection = openHttps(url)
        if (existing > 0L) connection.setRequestProperty("Range", "bytes=$existing-")
        val code = connection.responseCode
        if (code != HttpURLConnection.HTTP_OK && code != HttpURLConnection.HTTP_PARTIAL) {
            val message = "Private high-end content archive HTTP $code${readFailureBody(connection)}"
            connection.disconnect()
            throw IOException(message)
        }
        return connection
    }

    private fun readFailureBody(connection: HttpURLConnection): String = runCatching {
        connection.errorStream?.bufferedReader(Charsets.UTF_8)?.use { reader ->
            reader.readText().take(240).trim()
        }?.takeIf { it.isNotBlank() }?.let { ": $it" }.orEmpty()
    }.getOrDefault("")

    private fun openHttps(rawUrl: String): HttpURLConnection {
        require(rawUrl.startsWith("https://")) { "Private content endpoints must use HTTPS." }
        return (URL(rawUrl).openConnection() as HttpURLConnection).apply {
            connectTimeout = 30_000
            readTimeout = 30_000
            requestMethod = "GET"
            setRequestProperty("Accept", "application/json, application/octet-stream")
            setRequestProperty("User-Agent", "Aethelgrad-Android-PrivateContent/1")
        }
    }

    private fun mountVerified(partial: File, versionCode: Long, packageName: String) {
        val destination = File(appContext.obbDir, "main.$versionCode.$packageName.obb")
        destination.parentFile?.mkdirs()
        val mounted = File(destination.parentFile, "${destination.name}.verified")
        partial.copyTo(mounted, overwrite = true)
        if (mounted.length() != partial.length()) {
            mounted.delete()
            error("Verified private content could not be copied to the OBB directory.")
        }
        if (!mounted.renameTo(destination)) {
            destination.delete()
            require(mounted.renameTo(destination)) { "Could not mount verified private content." }
        }
        partial.delete()
    }

    private fun sha256(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        RandomAccessFile(file, "r").use { input ->
            val buffer = ByteArray(1024 * 1024)
            var read: Int
            while (input.read(buffer).also { read = it } >= 0) {
                if (read > 0) digest.update(buffer, 0, read)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }
}
