package com.darvirgoyt.aethelgrad

import android.content.Context
import java.io.File

/**
 * Compatibility boundary for the legacy standalone expansion artifact.
 *
 * Android does not install an OBB by itself when a user sideloads an APK. When
 * an installer or device image places the correctly named file in Context.obbDir,
 * the game can detect it and treat it as mounted content. Google Play installs
 * should use Play Asset Delivery instead, which supports automatic fast-follow
 * downloads and resumable updates.
 */
class StandaloneExpansionFile(context: Context) {
    data class State(
        val ready: Boolean,
        val path: String?,
        val bytes: Long
    )

    private val appContext = context.applicationContext

    fun inspect(versionCode: Int = 3): State {
        val file = File(appContext.obbDir, "main.$versionCode.${appContext.packageName}.obb")
        val bytes = if (file.isFile) file.length() else 0L
        return State(
            ready = bytes > 0L,
            path = file.takeIf { it.isFile }?.absolutePath,
            bytes = bytes
        )
    }
}
