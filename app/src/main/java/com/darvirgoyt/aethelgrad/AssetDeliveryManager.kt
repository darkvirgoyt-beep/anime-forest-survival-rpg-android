package com.darvirgoyt.aethelgrad

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Base64
import org.json.JSONObject
import java.io.BufferedInputStream
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.URL
import java.security.KeyFactory
import java.security.MessageDigest
import java.security.Signature
import java.security.spec.X509EncodedKeySpec
import java.util.concurrent.Executors
import java.util.zip.ZipInputStream

/**
 * Original Aethelgard asset delivery foundation.
 *
 * Remote packs are expected to be pre-cooked runtime bundles. Android verifies,
 * unpacks, and mounts them; it does not compile an Unreal authoring project on
 * the phone. Local asset:// sources are included for deterministic development.
 */
class AssetDeliveryManager(private val context: Context) {
    data class Progress(
        val state: State,
        val percent: Int,
        val title: String,
        val detail: String,
        val error: String? = null
    )

    enum class State { CHECKING, DOWNLOADING, VERIFYING, UNPACKING, MOUNTING, READY, FAILED }

    private data class Pack(
        val tier: String,
        val version: String,
        val source: String,
        val sha256: String,
        val byteSize: Long,
        val runtimeFormat: String,
        val maxUnpackedBytes: Long,
        val maxFileCount: Int
    )

    private val executor = Executors.newSingleThreadExecutor()
    private val main = Handler(Looper.getMainLooper())
    private val root = File(context.filesDir, "aethelgard_asset_packs")
    private val deliveryPreferences = context.getSharedPreferences("aethelgard_delivery", Context.MODE_PRIVATE)
    private val manifestName = "asset_manifest.json"

    fun prepareForTier(tierIndex: Int, callback: (Progress) -> Unit) {
        val tier = listOf("low", "medium", "high", "ultra", "max")[tierIndex.coerceIn(0, 4)]
        executor.execute {
            try {
                emit(callback, Progress(State.CHECKING, 2, "Checking signed asset manifest…", "Resolving the $tier runtime tier."))
                val manifest = readManifest()
                verifyManifestSignature(manifest)
                val pack = parsePacks(manifest)[tier] ?: error("No pack is published for tier $tier")
                require(pack.runtimeFormat == "aethelgard-cooked-v1") { "Unsupported runtime pack format" }

                val mounted = mountedDirectory(pack)
                if (isMountedAndValid(mounted, pack)) {
                    emit(callback, Progress(State.READY, 100, "Asset tier ready", "Mounted cached ${pack.tier.uppercase()} runtime pack."))
                    return@execute
                }

                val archive = File(root, "downloads/${pack.tier}-${pack.version}.zip.part")
                archive.parentFile?.mkdirs()
                emit(callback, Progress(State.DOWNLOADING, 8, "Downloading runtime 3D bundle…", "Resumable transfer: ${pack.tier.uppercase()} tier."))
                download(pack.source, archive, pack.byteSize) { bytes ->
                    val percentage = if (pack.byteSize > 0) (8 + (bytes * 68L / pack.byteSize).coerceAtMost(68L)).toInt() else 18
                    emit(callback, Progress(State.DOWNLOADING, percentage, "Downloading runtime 3D bundle…", "${bytes / 1024} KB received."))
                }

                emit(callback, Progress(State.VERIFYING, 78, "Verifying SHA-256 checksum…", "Rejecting incomplete or modified bundles."))
                require(sha256(archive).equals(pack.sha256, ignoreCase = true)) { "Asset checksum mismatch" }

                val unpacked = File(root, "staging/${pack.tier}-${pack.version}")
                if (unpacked.exists()) unpacked.deleteRecursively()
                unpacked.mkdirs()
                emit(callback, Progress(State.UNPACKING, 84, "Unpacking cooked render assets…", "Safely extracting meshes, textures, materials, and metadata."))
                unpackZipSafely(archive, unpacked, pack.maxUnpackedBytes, pack.maxFileCount)

                emit(callback, Progress(State.MOUNTING, 94, "Mounting device graphics tier…", "Preparing the native renderer asset registry."))
                File(unpacked, "aethelgard_pack_mount.marker").writeText(
                    "tier=${pack.tier}\nversion=${pack.version}\nruntimeFormat=${pack.runtimeFormat}\n",
                    Charsets.UTF_8
                )
                val finalDir = mountedDirectory(pack)
                if (finalDir.exists()) finalDir.deleteRecursively()
                require(unpacked.renameTo(finalDir)) { "Could not atomically mount the unpacked pack" }
                archive.delete()
                emit(callback, Progress(State.READY, 100, "Asset tier ready", "${pack.tier.uppercase()} runtime pack mounted."))
            } catch (error: Throwable) {
                emit(callback, Progress(State.FAILED, 0, "Asset preparation failed", error.message ?: "Unknown delivery error", error.message))
            }
        }
    }

    fun shutdown() {
        executor.shutdownNow()
    }

    private fun readManifest(): JSONObject {
        val configuredUrl = deliveryPreferences.getString("manifest_url", "").orEmpty()
        val text = if (configuredUrl.isBlank()) {
            context.assets.open(manifestName).bufferedReader(Charsets.UTF_8).use { it.readText() }
        } else {
            require(configuredUrl.startsWith("https://")) { "Asset catalog URL must use HTTPS" }
            val connection = (URL(configuredUrl).openConnection() as HttpURLConnection).apply {
                connectTimeout = 15_000
                readTimeout = 30_000
                requestMethod = "GET"
            }
            try {
                connection.connect()
                require(connection.responseCode in 200..299) { "Asset catalog request failed: ${connection.responseCode}" }
                connection.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() }
            } finally {
                connection.disconnect()
            }
        }
        return JSONObject(text)
    }

    private fun verifyManifestSignature(manifest: JSONObject) {
        val signature = manifest.optJSONObject("signature") ?: error("Asset catalog signature is missing")
        val algorithm = signature.optString("algorithm")
        require(algorithm == "ed25519") { "Unsupported asset catalog signature algorithm" }
        val signatureValue = signature.optString("value")
        if (signatureValue == "DEVELOPMENT_ONLY_REPLACE_WITH_SERVER_SIGNATURE" &&
            manifest.optString("catalogId").startsWith("aethelgard-dev-")) return

        val publicKeyText = signature.optString("publicKey")
        val payloadText = signature.optString("payload")
        require(publicKeyText.isNotBlank() && payloadText.isNotBlank() && signatureValue.isNotBlank()) {
            "Production asset catalog requires publicKey, payload, and value"
        }
        val keyBytes = Base64.decode(publicKeyText, Base64.DEFAULT)
        val payloadBytes = Base64.decode(payloadText, Base64.DEFAULT)
        val signatureBytes = Base64.decode(signatureValue, Base64.DEFAULT)
        val publicKey = KeyFactory.getInstance("Ed25519").generatePublic(X509EncodedKeySpec(keyBytes))
        val verifier = Signature.getInstance("Ed25519")
        verifier.initVerify(publicKey)
        verifier.update(payloadBytes)
        require(verifier.verify(signatureBytes)) { "Asset catalog signature verification failed" }
    }

    private fun parsePacks(manifest: JSONObject): Map<String, Pack> {
        val packs = manifest.getJSONArray("packs")
        return buildMap {
            for (index in 0 until packs.length()) {
                val item = packs.getJSONObject(index)
                val pack = Pack(
                    tier = item.getString("tier"),
                    version = item.getString("version"),
                    source = item.getString("source"),
                    sha256 = item.getString("sha256"),
                    byteSize = item.getLong("byteSize"),
                    runtimeFormat = item.getString("runtimeFormat"),
                    maxUnpackedBytes = item.optLong("maxUnpackedBytes", 32L * 1024L * 1024L),
                    maxFileCount = item.optInt("maxFileCount", 256)
                )
                put(pack.tier, pack)
            }
        }
    }

    private fun mountedDirectory(pack: Pack): File = File(root, "mounted/${pack.tier}/${pack.version}")

    private fun isMountedAndValid(directory: File, pack: Pack): Boolean {
        return File(directory, "aethelgard_pack_mount.marker").isFile &&
            File(directory, "pack_manifest.json").isFile &&
            directorySize(directory) <= pack.maxUnpackedBytes
    }

    private fun directorySize(directory: File): Long = directory.walkTopDown().filter { it.isFile }.sumOf { it.length() }

    private fun download(source: String, destination: File, expectedBytes: Long, onProgress: (Long) -> Unit) {
        if (source.startsWith("asset://")) {
            context.assets.open(source.removePrefix("asset://")).use { input ->
                FileOutputStream(destination, false).use { output -> copy(input, output, 0L, expectedBytes, onProgress) }
            }
            return
        }
        require(source.startsWith("https://")) { "Only https:// and asset:// sources are supported" }
        val existing = if (destination.isFile) destination.length() else 0L
        val connection = (URL(source).openConnection() as HttpURLConnection).apply {
            connectTimeout = 15_000
            readTimeout = 30_000
            requestMethod = "GET"
            if (existing > 0L) setRequestProperty("Range", "bytes=$existing-")
        }
        try {
            connection.connect()
            val append = existing > 0L && connection.responseCode == HttpURLConnection.HTTP_PARTIAL
            val startingBytes = if (append) existing else 0L
            if (!append && destination.exists()) destination.delete()
            connection.inputStream.use { input ->
                FileOutputStream(destination, append).use { output -> copy(input, output, startingBytes, expectedBytes, onProgress) }
            }
        } finally {
            connection.disconnect()
        }
    }

    private fun copy(input: InputStream, output: FileOutputStream, startingBytes: Long, expectedBytes: Long, onProgress: (Long) -> Unit) {
        val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
        var total = startingBytes
        while (true) {
            val read = input.read(buffer)
            if (read < 0) break
            output.write(buffer, 0, read)
            total += read
            onProgress(total)
        }
        require(expectedBytes <= 0L || total == expectedBytes) { "Unexpected asset size: expected $expectedBytes, received $total" }
    }

    private fun unpackZipSafely(archive: File, destination: File, maxBytes: Long, maxFiles: Int) {
        var files = 0
        var unpackedBytes = 0L
        ZipInputStream(BufferedInputStream(FileInputStream(archive))).use { zip ->
            while (true) {
                val entry = zip.nextEntry ?: break
                val target = File(destination, entry.name).canonicalFile
                val base = destination.canonicalFile
                require(target.path == base.path || target.path.startsWith(base.path + File.separator)) { "Unsafe zip entry" }
                if (entry.isDirectory) {
                    target.mkdirs()
                } else {
                    require(++files <= maxFiles) { "Asset file-count limit exceeded" }
                    target.parentFile?.mkdirs()
                    FileOutputStream(target).use { output ->
                        val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
                        while (true) {
                            val read = zip.read(buffer)
                            if (read < 0) break
                            unpackedBytes += read
                            require(unpackedBytes <= maxBytes) { "Asset unpack-size limit exceeded" }
                            output.write(buffer, 0, read)
                        }
                    }
                }
                zip.closeEntry()
            }
        }
        require(File(destination, "pack_manifest.json").isFile) { "Runtime pack manifest is missing" }
    }

    private fun sha256(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
            while (true) {
                val count = input.read(buffer)
                if (count < 0) break
                digest.update(buffer, 0, count)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }

    private fun emit(callback: (Progress) -> Unit, progress: Progress) {
        main.post { callback(progress) }
    }

    companion object {
        private const val DEFAULT_BUFFER_SIZE = 16 * 1024
    }
}
