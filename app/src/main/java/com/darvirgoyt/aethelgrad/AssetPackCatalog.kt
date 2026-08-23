package com.darvirgoyt.aethelgrad

import android.content.Context
import com.google.android.play.core.assetpacks.AssetPackManager
import com.google.android.play.core.assetpacks.AssetPackManagerFactory
import com.google.android.play.core.assetpacks.AssetPackState
import com.google.android.play.core.assetpacks.AssetPackStateUpdateListener

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

    fun isReady(packName: String): Boolean = manager.getPackLocation(packName) != null

    fun assetPath(packName: String, relativePath: String): String? {
        val location = manager.getPackLocation(packName) ?: return null
        return "${location.assetsPath()}/$relativePath"
    }

    // Play Asset Delivery represents COMPLETED with status code 4 in the 2.x API.
    fun isComplete(progress: Progress): Boolean = progress.status == 4

    fun close() {
        listener?.let(manager::unregisterListener)
        listener = null
    }
}
