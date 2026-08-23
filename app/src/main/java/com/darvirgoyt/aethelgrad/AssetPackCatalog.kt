package com.darvirgoyt.aethelgrad

import android.content.Context
import com.google.android.play.core.assetpacks.AssetPackManager
import com.google.android.play.core.assetpacks.AssetPackManagerFactory
import com.google.android.play.core.assetpacks.AssetPackState
import com.google.android.play.core.assetpacks.AssetPackStateUpdateListener
import com.google.android.play.core.assetpacks.model.AssetPackStatus

/**
 * Runtime boundary for Play Asset Delivery.
 *
 * Asset packs are treated as read-only, may be unavailable on any launch, and
 * are resolved through Play instead of assuming a permanent filesystem path.
 */
class AssetPackCatalog(context: Context) {
    data class Progress(
        val packName: String,
        val status: Int,
        val bytesDownloaded: Long,
        val totalBytes: Long,
        val errorCode: Int
    ) {
        val fraction: Float
            get() = if (totalBytes <= 0L) 0.0f else (bytesDownloaded.toDouble() / totalBytes.toDouble()).toFloat().coerceIn(0.0f, 1.0f)
    }

    data class ProductionProgress(
        val status: Int,
        val percent: Int,
        val bytesDownloaded: Long,
        val totalBytes: Long,
        val failedPack: String? = null,
        val errorCode: Int = 0
    ) {
        val complete: Boolean get() = status == AssetPackStatus.COMPLETED
        val failed: Boolean get() = status == AssetPackStatus.FAILED
    }

    companion object {
        /** Content stays outside the base APK and is downloaded independently. */
        val productionPackNames = listOf(
            "assetpack_forest",
            "assetpack_sand",
            "assetpack_snow",
            "assetpack_characters",
            "assetpack_audio_hd",
            "assetpack_cinematics",
            "assetpack_hd_textures",
            "assetpack_dungeons",
            "assetpack_vfx",
            "assetpack_voice"
        )
    }

    private val manager: AssetPackManager = AssetPackManagerFactory.getInstance(context.applicationContext)
    private var listener: AssetPackStateUpdateListener? = null

    fun request(packName: String, onProgress: (Progress) -> Unit = {}) {
        listener?.let(manager::unregisterListener)
        val updateListener = AssetPackStateUpdateListener { state: AssetPackState ->
            onProgress(
                Progress(
                    packName = state.name(),
                    status = state.status(),
                    bytesDownloaded = state.bytesDownloaded(),
                    totalBytes = state.totalBytesToDownload(),
                    errorCode = state.errorCode()
                )
            )
        }
        listener = updateListener
        manager.registerListener(updateListener)
        manager.fetch(listOf(packName))
    }

    /**
     * Starts the BGMI-style post-install download of all production content.
     * The AAB keeps these packs outside the base install; Play handles
     * resumable delivery, storage, and pack updates independently.
     */
    fun requestProductionContent(onProgress: (ProductionProgress) -> Unit) {
        listener?.let(manager::unregisterListener)
        val states = mutableMapOf<String, Progress>()
        val updateListener = AssetPackStateUpdateListener { state: AssetPackState ->
            states[state.name()] = Progress(
                packName = state.name(),
                status = state.status(),
                bytesDownloaded = state.bytesDownloaded(),
                totalBytes = state.totalBytesToDownload(),
                errorCode = state.errorCode()
            )
            emitProductionProgress(states, onProgress)
        }
        listener = updateListener
        manager.registerListener(updateListener)
        manager.fetch(productionPackNames).addOnFailureListener { error ->
            onProgress(
                ProductionProgress(
                    status = AssetPackStatus.FAILED,
                    percent = 0,
                    bytesDownloaded = 0L,
                    totalBytes = 0L,
                    failedPack = error.message ?: "production-content",
                    errorCode = -1
                )
            )
        }
    }

    private fun emitProductionProgress(states: Map<String, Progress>, onProgress: (ProductionProgress) -> Unit) {
        val reportedTotalBytes = states.values.sumOf { it.totalBytes }
        val bytesDownloaded = states.values.sumOf { it.bytesDownloaded }
        val failed = states.values.firstOrNull { it.status == AssetPackStatus.FAILED }
        val complete = productionPackNames.all { packName ->
            states[packName]?.status == AssetPackStatus.COMPLETED || isReady(packName)
        }
        // Play reports packs independently. Until every pack has reported its
        // real total, use the production envelope as a stable denominator so
        // discovering another pack cannot make the visible bar move backward.
        val allTotalsKnown = productionPackNames.all { (states[it]?.totalBytes ?: 0L) > 0L }
        val totalBytes = if (allTotalsKnown) {
            reportedTotalBytes
        } else {
            maxOf(reportedTotalBytes, ContentDownloadPlan.totalMiB.toLong() * 1024L * 1024L)
        }
        val percent = when {
            complete -> 100
            totalBytes > 0L -> (bytesDownloaded.toDouble() * 100.0 / totalBytes.toDouble()).toInt().coerceIn(1, 99)
            else -> 5
        }
        onProgress(
            ProductionProgress(
                status = when {
                    failed != null -> AssetPackStatus.FAILED
                    complete -> AssetPackStatus.COMPLETED
                    else -> AssetPackStatus.DOWNLOADING
                },
                percent = percent,
                bytesDownloaded = bytesDownloaded,
                totalBytes = totalBytes,
                failedPack = failed?.packName,
                errorCode = failed?.errorCode ?: 0
            )
        )
    }

    fun isReady(packName: String): Boolean = manager.getPackLocation(packName) != null

    fun assetPath(packName: String, relativePath: String): String? {
        val location = manager.getPackLocation(packName) ?: return null
        return "${location.assetsPath()}/$relativePath"
    }

    fun isComplete(progress: Progress): Boolean = progress.status == AssetPackStatus.COMPLETED

    fun close() {
        listener?.let(manager::unregisterListener)
        listener = null
    }
}
