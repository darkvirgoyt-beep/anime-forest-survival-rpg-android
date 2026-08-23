package com.darvirgoyt.aethelgrad

import android.app.Activity
import android.content.Context
import android.os.StatFs
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

    data class Preflight(
        val ready: Boolean,
        val detail: String,
        val availableBytes: Long,
        val requiredBytes: Long
    )

    companion object {
        /** High-resource content stays outside the base APK and is downloaded independently. */
        val productionPackNames: List<String>
            get() = ContentDownloadPlan.requiredPackNames
    }

    private val appContext = context.applicationContext
    private val manager: AssetPackManager = AssetPackManagerFactory.getInstance(appContext)
    private var listener: AssetPackStateUpdateListener? = null

    fun checkProductionPreflight(tier: ContentDownloadPlan.ResourceTier = ContentDownloadPlan.ResourceTier.HIGH): Preflight {
        val requiredBytes = (ContentDownloadPlan.totalMiBFor(tier) + 512).toLong() * 1024L * 1024L
        return try {
            val stats = StatFs(appContext.filesDir.absolutePath)
            val availableBytes = stats.availableBytes
            if (availableBytes < requiredBytes) {
                Preflight(
                    ready = false,
                    detail = "Not enough free storage: ${availableBytes / (1024L * 1024L)} MB available; ${(requiredBytes / (1024L * 1024L))} MB required.",
                    availableBytes = availableBytes,
                    requiredBytes = requiredBytes
                )
            } else {
                Preflight(
                    ready = true,
                    detail = "Storage check passed: ${availableBytes / (1024L * 1024L)} MB available.",
                    availableBytes = availableBytes,
                    requiredBytes = requiredBytes
                )
            }
        } catch (error: Exception) {
            Preflight(
                ready = false,
                detail = "Storage check failed: ${error.message ?: "unable to inspect free space"}",
                availableBytes = 0L,
                requiredBytes = requiredBytes
            )
        }
    }

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
     * Starts the BGMI-style post-install download for the selected resource tier.
     * The AAB keeps these packs outside the base install; Play handles resumable
     * delivery, storage, and pack updates independently.
     */
    fun requestProductionContent(
        tier: ContentDownloadPlan.ResourceTier,
        onProgress: (ProductionProgress) -> Unit
    ) {
        val requestedPackNames = ContentDownloadPlan.packNamesFor(tier)
        val targetMiB = ContentDownloadPlan.totalMiBFor(tier)
        if (productionContentReady(tier)) {
            onProgress(
                ProductionProgress(
                    status = AssetPackStatus.COMPLETED,
                    percent = 100,
                    bytesDownloaded = targetMiB.toLong() * 1024L * 1024L,
                    totalBytes = targetMiB.toLong() * 1024L * 1024L
                )
            )
            return
        }
        val preflight = checkProductionPreflight(tier)
        if (!preflight.ready) {
            onProgress(
                ProductionProgress(
                    status = AssetPackStatus.FAILED,
                    percent = 0,
                    bytesDownloaded = 0L,
                    totalBytes = targetMiB.toLong() * 1024L * 1024L,
                    failedPack = preflight.detail,
                    errorCode = -2
                )
            )
            return
        }
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
            emitProductionProgress(requestedPackNames, targetMiB, states, onProgress)
        }
        listener = updateListener
        manager.registerListener(updateListener)
        manager.fetch(requestedPackNames).addOnFailureListener { error ->
            onProgress(
                ProductionProgress(
                    status = AssetPackStatus.FAILED,
                    percent = 0,
                    bytesDownloaded = 0L,
                    totalBytes = targetMiB.toLong() * 1024L * 1024L,
                    failedPack = error.message ?: "production-content",
                    errorCode = -1
                )
            )
        }
    }

    /** Backward-compatible high-resource request for existing callers and tests. */
    fun requestProductionContent(onProgress: (ProductionProgress) -> Unit) =
        requestProductionContent(ContentDownloadPlan.ResourceTier.HIGH, onProgress)

    private fun emitProductionProgress(
        requestedPackNames: List<String>,
        targetMiB: Int,
        states: Map<String, Progress>,
        onProgress: (ProductionProgress) -> Unit
    ) {
        val reportedTotalBytes = states.values.sumOf { it.totalBytes }
        val bytesDownloaded = states.values.sumOf { it.bytesDownloaded }
        val failed = states.values.firstOrNull { it.status == AssetPackStatus.FAILED }
        val waitingForWifi = states.values.any { it.status == AssetPackStatus.WAITING_FOR_WIFI }
        val confirmationRequired = states.values.any { it.status == AssetPackStatus.REQUIRES_USER_CONFIRMATION }
        val canceled = states.values.any { it.status == AssetPackStatus.CANCELED }
        val complete = requestedPackNames.all { packName ->
            states[packName]?.status == AssetPackStatus.COMPLETED || isReady(packName)
        }
        // Play reports packs independently. Until every pack total is known, use
        // the selected tier envelope as a stable denominator so discovering
        // another pack cannot make the visible bar move backward.
        val allTotalsKnown = requestedPackNames.all { (states[it]?.totalBytes ?: 0L) > 0L }
        val totalBytes = if (allTotalsKnown) {
            reportedTotalBytes
        } else {
            maxOf(reportedTotalBytes, targetMiB.toLong() * 1024L * 1024L)
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
                    waitingForWifi -> AssetPackStatus.WAITING_FOR_WIFI
                    confirmationRequired -> AssetPackStatus.REQUIRES_USER_CONFIRMATION
                    canceled -> AssetPackStatus.CANCELED
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

    /**
     * Shows Play's consent dialog for a large fast-follow/on-demand download.
     * A successful task result means the user accepted the dialog; the caller
     * should call requestProductionContent again so Play resumes the request.
     */
    fun showDownloadConfirmation(activity: Activity, onResult: (accepted: Boolean) -> Unit) {
        manager.showConfirmationDialog(activity)
            .addOnSuccessListener { result -> onResult(result == Activity.RESULT_OK) }
            .addOnFailureListener { onResult(false) }
    }

    fun isReady(packName: String): Boolean = manager.getPackLocation(packName) != null

    fun productionContentReady(): Boolean = productionContentReady(ContentDownloadPlan.ResourceTier.HIGH)

    fun productionContentReady(tier: ContentDownloadPlan.ResourceTier): Boolean =
        ContentDownloadPlan.packNamesFor(tier).all(::isReady)

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
