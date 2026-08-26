package com.darvirgoyt.aethelgrad

import android.app.Activity
import android.content.Context
import android.os.StatFs
import com.google.android.play.core.assetpacks.AssetPackManager
import com.google.android.play.core.assetpacks.AssetPackManagerFactory
import com.google.android.play.core.assetpacks.AssetPackState
import com.google.android.play.core.assetpacks.AssetPackStateUpdateListener
import com.google.android.play.core.assetpacks.model.AssetPackStatus
import java.io.File
import org.json.JSONObject

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
        val sizeVerified: Boolean,
        val failedPack: String? = null,
        val errorCode: Int = 0,
        val source: String = "asset-pack"
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
        /** Stage 1 is the current phone-first content boundary; high-end remains deferred. */
        val productionPackNames: List<String>
            get() = ContentDownloadPlan.requiredPackNames
    }

    private val appContext = context.applicationContext
    private val manager: AssetPackManager = AssetPackManagerFactory.getInstance(appContext)
    private val standaloneExpansionFile = StandaloneExpansionFile(appContext)
    private val privateContentDownloader = PrivateContentDownloader(
        appContext,
        appContext.getString(R.string.published_high_end_content).equals("true", ignoreCase = true),
        appContext.getString(R.string.private_content_manifest_url),
        appContext.getString(R.string.private_content_archive_url)
    )
    private var listener: AssetPackStateUpdateListener? = null

    fun checkProductionPreflight(tier: ContentDownloadPlan.ResourceTier = ContentDownloadPlan.ResourceTier.STAGE_1): Preflight {
        val requiredBytes = 0L
        return try {
            val stats = StatFs(appContext.filesDir.absolutePath)
            val availableBytes = stats.availableBytes
            Preflight(
                ready = true,
                detail = "${availableBytes / (1024L * 1024L)} MB free. Required size is disclosed only after a signed archive manifest or Play reports it.",
                availableBytes = availableBytes,
                requiredBytes = requiredBytes
            )
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

    /** Starts only the compact launch slice; discovered sectors are requested separately. */
    fun requestProductionContent(
        tier: ContentDownloadPlan.ResourceTier,
        onProgress: (ProductionProgress) -> Unit
    ) {
        if (standaloneExpansionFile.inspect().ready) {
            val totalBytes = standaloneExpansionFile.inspect().bytes
            onProgress(ProductionProgress(AssetPackStatus.COMPLETED, 100, totalBytes, totalBytes, true))
            return
        }
        val bundledLaunchBytes = bundledLaunchSliceBytes()
        if (bundledLaunchBytes > 0L && tier == ContentDownloadPlan.ResourceTier.STAGE_1) {
            onProgress(ProductionProgress(AssetPackStatus.COMPLETED, 100, bundledLaunchBytes, bundledLaunchBytes, true, source = "bundled-launch-slice"))
            return
        }
        if (!privateContentDownloader.published) {
            onProgress(
                ProductionProgress(
                    status = AssetPackStatus.FAILED,
                    percent = 0,
                    bytesDownloaded = 0L,
                    totalBytes = 0L,
                    sizeVerified = false,
                    failedPack = "No signed high-graphics archive or cooked Play Asset Delivery build is published for this APK. Repository plans and Unreal source do not contain downloadable map, model, or graphics payload bytes.",
                    errorCode = -30
                )
            )
            return
        }
        if (privateContentDownloader.configured) {
            privateContentDownloader.downloadHighEndContent { progress ->
                onProgress(
                    ProductionProgress(
                        status = when (progress.status) {
                            PrivateContentDownloader.Status.COMPLETED -> AssetPackStatus.COMPLETED
                            PrivateContentDownloader.Status.FAILED -> AssetPackStatus.FAILED
                            PrivateContentDownloader.Status.DOWNLOADING -> AssetPackStatus.DOWNLOADING
                        },
                        percent = progress.percent,
                        bytesDownloaded = progress.bytesDownloaded,
                        totalBytes = progress.totalBytes,
                        sizeVerified = progress.totalBytes > 0L,
                        failedPack = progress.detail,
                        errorCode = if (progress.status == PrivateContentDownloader.Status.FAILED) -20 else 0
                    )
                )
            }
            return
        }
        val requestedPackNames = ContentDownloadPlan.startupPackNamesFor(tier)
        requestPackSet(requestedPackNames, onProgress)
    }

    /** Requests the immutable pack group associated with a newly discovered sector. */
    fun requestWorldSector(
        tier: ContentDownloadPlan.ResourceTier,
        sector: ContentDownloadPlan.WorldSector,
        onProgress: (ProductionProgress) -> Unit
    ) {
        if (tier == ContentDownloadPlan.ResourceTier.HIGH && !privateContentDownloader.published) {
            onProgress(
                ProductionProgress(
                    status = AssetPackStatus.FAILED,
                    percent = 0,
                    bytesDownloaded = 0L,
                    totalBytes = 0L,
                    sizeVerified = false,
                    failedPack = "High-end sector packs, including cinematics, are unavailable until a signed cooked release is published.",
                    errorCode = -30
                )
            )
            return
        }
        val requestedPackNames = ContentDownloadPlan.packNamesForSector(tier, sector)
        if (requestedPackNames.isEmpty()) {
            onProgress(ProductionProgress(AssetPackStatus.COMPLETED, 100, 0L, 0L, true))
            return
        }
        requestPackSet(requestedPackNames, onProgress)
    }

    /** Backward-compatible high-resource request for existing callers and tests. */
    fun requestProductionContent(onProgress: (ProductionProgress) -> Unit) =
        requestProductionContent(ContentDownloadPlan.ResourceTier.STAGE_1, onProgress)

    /**
     * Expands the local Stage 1 footprint only with optional packs that are both
     * published and measured in the embedded content manifest. No planned bytes,
     * empty packs, or unpublished Unreal resources can start a download here.
     */
    fun requestAvailableStageEnhancements(onProgress: (ProductionProgress) -> Unit) {
        val packNames = measuredPublishedPackNames(ContentDownloadPlan.optionalStagePackNames())
        if (packNames.isEmpty()) {
            onProgress(
                ProductionProgress(
                    status = AssetPackStatus.COMPLETED,
                    percent = 100,
                    bytesDownloaded = 0L,
                    totalBytes = 0L,
                    sizeVerified = true,
                    source = "no-published-stage-enhancement"
                )
            )
            return
        }
        requestPackSet(packNames, onProgress)
    }

    private fun requestPackSet(
        requestedPackNames: List<String>,
        onProgress: (ProductionProgress) -> Unit
    ) {
        if (requestedPackNames.isEmpty() || requestedPackNames.all(::isReady)) {
            val mountedBytes = measureInstalledPackBytes(requestedPackNames)
            onProgress(
                ProductionProgress(
                    status = if (mountedBytes > 0L) AssetPackStatus.COMPLETED else AssetPackStatus.FAILED,
                    percent = if (mountedBytes > 0L) 100 else 0,
                    bytesDownloaded = mountedBytes,
                    totalBytes = mountedBytes,
                    sizeVerified = mountedBytes > 0L,
                    failedPack = if (mountedBytes > 0L) null else "Play Asset Delivery reported ready packs, but no cooked payload bytes were mounted."
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
            emitProductionProgress(requestedPackNames, states, onProgress)
        }
        listener = updateListener
        manager.registerListener(updateListener)
        manager.fetch(requestedPackNames).addOnFailureListener { error ->
            onProgress(
                ProductionProgress(
                    status = AssetPackStatus.FAILED,
                    percent = 0,
                    bytesDownloaded = 0L,
                    totalBytes = 0L,
                    sizeVerified = false,
                    failedPack = error.message ?: requestedPackNames.joinToString(),
                    errorCode = -1
                )
            )
        }
    }

    private fun emitProductionProgress(
        requestedPackNames: List<String>,
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
        val allTotalsKnown = requestedPackNames.all { (states[it]?.totalBytes ?: 0L) > 0L }
        val totalBytes = if (allTotalsKnown) reportedTotalBytes else 0L
        val percent = when {
            complete -> 100
            totalBytes > 0L -> (bytesDownloaded.toDouble() * 100.0 / totalBytes.toDouble()).toInt().coerceIn(if (bytesDownloaded > 0L) 1 else 0, 99)
            else -> 0
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
                sizeVerified = allTotalsKnown || complete,
                failedPack = failed?.packName,
                errorCode = failed?.errorCode ?: 0
            )
        )
    }

    /** Shows Play's consent dialog for a large fast-follow/on-demand download. */
    fun showDownloadConfirmation(activity: Activity, onResult: (accepted: Boolean) -> Unit) {
        manager.showConfirmationDialog(activity)
            .addOnSuccessListener { result -> onResult(result == Activity.RESULT_OK) }
            .addOnFailureListener { onResult(false) }
    }

    fun isReady(packName: String): Boolean = manager.getPackLocation(packName) != null

    fun productionContentReady(): Boolean = productionContentReady(ContentDownloadPlan.ResourceTier.STAGE_1)

    fun productionContentReady(tier: ContentDownloadPlan.ResourceTier): Boolean {
        if (standaloneExpansionFile.inspect().ready) return true
        if (bundledLaunchSliceBytes() > 0L) return true
        if (!privateContentDownloader.published) return false
        val expectedPacks = ContentDownloadPlan.startupPackNamesFor(tier)
        return expectedPacks.isNotEmpty() && expectedPacks.all(::isReady) && measureInstalledPackBytes(expectedPacks) > 0L
    }

    private fun bundledLaunchSliceBytes(): Long = runCatching {
        val manifest = appContext.assets.open("asset_manifest.json").bufferedReader(Charsets.UTF_8).use { JSONObject(it.readText()) }
        val launchSlice = manifest.getJSONObject("contentDelivery").getJSONObject("launchSlice")
        if (!launchSlice.optBoolean("published", false)) 0L else launchSlice.optLong("measuredBytes", 0L).takeIf { it > 0L } ?: 0L
    }.getOrDefault(0L)

    private fun measuredPublishedPackNames(requestedPackNames: List<String>): List<String> = runCatching {
        val manifest = appContext.assets.open("asset_manifest.json").bufferedReader(Charsets.UTF_8).use { JSONObject(it.readText()) }
        val packs = manifest.getJSONArray("packs")
        buildList {
            for (index in 0 until packs.length()) {
                val pack = packs.getJSONObject(index)
                val name = pack.optString("name")
                if (name in requestedPackNames && pack.optBoolean("published", false) && pack.optLong("measuredBytes", 0L) > 0L) {
                    add(name)
                }
            }
        }
    }.getOrDefault(emptyList())

    fun sectorContentReady(tier: ContentDownloadPlan.ResourceTier, sector: ContentDownloadPlan.WorldSector): Boolean =
        ContentDownloadPlan.packNamesForSector(tier, sector).all(::isReady)

    fun assetPath(packName: String, relativePath: String): String? {
        val location = manager.getPackLocation(packName) ?: return null
        return "${location.assetsPath()}/$relativePath"
    }

    fun isComplete(progress: Progress): Boolean = progress.status == AssetPackStatus.COMPLETED

    private fun measureInstalledPackBytes(packNames: List<String>): Long = packNames.sumOf { packName ->
        val assetsPath = manager.getPackLocation(packName)?.assetsPath() ?: return@sumOf 0L
        File(assetsPath).walkTopDown().filter { it.isFile }.sumOf { it.length() }
    }

    fun close() {
        listener?.let(manager::unregisterListener)
        listener = null
        privateContentDownloader.close()
    }
}
