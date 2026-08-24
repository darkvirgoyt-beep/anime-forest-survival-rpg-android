package com.darvirgoyt.aethelgrad

import android.content.Context
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.os.Handler
import android.os.Looper
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.Executors

/**
 * Process-local connectivity boundary for online gameplay.
 *
 * A network is considered online only when Android reports Wi-Fi or cellular
 * transport, INTERNET capability, and VALIDATED internet access. The monitor
 * does not grant gameplay authority; it only prevents avoidable online calls
 * when the device is offline and supplies UI-facing network state.
 */
class NetworkConnectivityMonitor(context: Context) {
    private val appContext = context.applicationContext
    private val connectivityManager =
        appContext.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
    private val callbackHandler = Handler(Looper.getMainLooper())
    private val probeExecutor = Executors.newSingleThreadExecutor()
    private var networkCallback: ConnectivityManager.NetworkCallback? = null
    private var listener: ((ConnectivitySnapshot) -> Unit)? = null
    private var started = false

    @Volatile
    var current: ConnectivitySnapshot = readSnapshot()
        private set

    fun start(onChanged: (ConnectivitySnapshot) -> Unit) {
        listener = onChanged
        if (started) {
            publish(current)
            return
        }
        started = true
        current = readSnapshot()
        publish(current)

        val callback = object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(network: Network) {
                refreshFromAndroid()
            }

            override fun onLost(network: Network) {
                refreshFromAndroid()
            }

            override fun onCapabilitiesChanged(
                network: Network,
                networkCapabilities: NetworkCapabilities
            ) {
                refreshFromAndroid()
            }
        }
        networkCallback = callback
        try {
            connectivityManager.registerDefaultNetworkCallback(callback)
        } catch (_: RuntimeException) {
            // Keep the last snapshot. The explicit read still protects the
            // online request path on devices that reject callback registration.
        }
    }

    fun stop() {
        val callback = networkCallback
        if (callback != null) {
            try {
                connectivityManager.unregisterNetworkCallback(callback)
            } catch (_: RuntimeException) {
                // The callback may already have been removed by the OS.
            }
        }
        networkCallback = null
        listener = null
        started = false
        probeExecutor.shutdownNow()
    }

    fun isOnline(): Boolean = current.isOnline

    /** Measures TCP connection latency to a configured server host. */
    fun measureTcpLatency(
        host: String,
        port: Int = 443,
        timeoutMs: Int = 1800,
        onComplete: (Int?) -> Unit
    ) {
        if (!current.isOnline || host.isBlank()) {
            callbackHandler.post { onComplete(null) }
            return
        }

        probeExecutor.execute {
            val startedAt = System.nanoTime()
            val result = try {
                Socket().use { socket ->
                    socket.connect(InetSocketAddress(host, port), timeoutMs)
                }
                ((System.nanoTime() - startedAt) / 1_000_000L)
                    .toInt()
                    .coerceAtLeast(1)
            } catch (_: Exception) {
                null
            }
            callbackHandler.post { onComplete(result) }
        }
    }

    private fun refreshFromAndroid() {
        val snapshot = readSnapshot()
        if (snapshot == current) {
            return
        }
        current = snapshot
        publish(snapshot)
    }

    private fun publish(snapshot: ConnectivitySnapshot) {
        callbackHandler.post { listener?.invoke(snapshot) }
    }

    private fun readSnapshot(): ConnectivitySnapshot {
        val network = connectivityManager.activeNetwork
            ?: return ConnectivitySnapshot.offline("NO NETWORK")
        val capabilities = connectivityManager.getNetworkCapabilities(network)
            ?: return ConnectivitySnapshot.offline("NETWORK STATE UNKNOWN")

        val isWifi = capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)
        val isCellular = capabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR)
        val hasInternet = capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
        val isValidated = capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)

        if (!hasInternet || !isValidated || (!isWifi && !isCellular)) {
            val reason = when {
                !hasInternet -> "NO INTERNET CAPABILITY"
                !isValidated -> "WAITING FOR INTERNET"
                else -> "WI-FI OR MOBILE DATA REQUIRED"
            }
            return ConnectivitySnapshot.offline(reason)
        }

        return ConnectivitySnapshot(
            isOnline = true,
            transport = if (isWifi) NetworkTransport.WIFI else NetworkTransport.CELLULAR,
            message = if (isWifi) "WI-FI CONNECTED" else "MOBILE DATA CONNECTED"
        )
    }
}

enum class NetworkTransport {
    NONE,
    WIFI,
    CELLULAR
}

data class ConnectivitySnapshot(
    val isOnline: Boolean,
    val transport: NetworkTransport,
    val message: String
) {
    companion object {
        fun offline(message: String) = ConnectivitySnapshot(
            isOnline = false,
            transport = NetworkTransport.NONE,
            message = message
        )
    }
}
