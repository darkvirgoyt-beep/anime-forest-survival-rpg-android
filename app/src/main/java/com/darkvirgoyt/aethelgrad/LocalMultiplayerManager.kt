package com.darkvirgoyt.aethelgrad

import android.Manifest
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.net.wifi.p2p.WifiP2pConfig
import android.net.wifi.p2p.WifiP2pDevice
import android.net.wifi.p2p.WifiP2pDeviceList
import android.net.wifi.p2p.WifiP2pInfo
import android.net.wifi.p2p.WifiP2pManager
import android.os.Handler
import android.os.Looper
import androidx.core.content.ContextCompat
import org.json.JSONArray
import org.json.JSONObject
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import java.net.SocketException
import java.util.UUID
import java.util.LinkedHashMap
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.Executors

/** A discovered local room advertised over mDNS or hosted through Wi-Fi Direct. */
data class LocalRoom(
    val name: String,
    val address: String,
    val port: Int,
    val code: String,
    val transport: String
)

data class LocalWifiPeer(
    val name: String,
    val address: String
)

data class LocalPeerState(
    val playerId: String,
    val playerName: String,
    val x: Float,
    val y: Float,
    val atTower: Boolean,
    val towerRevision: Int
)

/**
 * LAN/Wi-Fi Direct transport for small trusted local sessions.
 *
 * The internet backend remains the production-authoritative path. This class is
 * intentionally a local-room adapter: one device hosts a four-player session,
 * discovered peers connect to it, and newline-delimited JSON carries compact
 * state/event messages. The host relays messages and the Android client applies
 * the same peer presentation callbacks used by the existing co-op HUD.
 */
class LocalMultiplayerManager(
    private val context: Context,
    private val callbacks: Callbacks
) {
    interface Callbacks {
        fun onLocalRoomFound(room: LocalRoom)
        fun onWifiPeerFound(peer: LocalWifiPeer)
        fun onLocalSessionChanged(connected: Boolean, host: Boolean, room: LocalRoom?)
        fun onPeerStatesChanged(states: List<LocalPeerState>)
        fun onRemoteEvent(action: String, payload: JSONObject)
        fun onLocalError(message: String)
    }

    companion object {
        const val PORT = 36667
        private const val SERVICE_TYPE = "_aethelgard._tcp."
        private const val PROTOCOL_VERSION = 1
        private const val MAX_PLAYERS = 4
    }

    private val appContext = context.applicationContext
    private val mainHandler = Handler(Looper.getMainLooper())
    private val io = Executors.newCachedThreadPool()
    private val preferences = appContext.getSharedPreferences("aethelgard_local_multiplayer", Context.MODE_PRIVATE)
    val localPlayerId: String = preferences.getString("player_id", null) ?: ("local-" + UUID.randomUUID().toString().take(12)).also {
        preferences.edit().putString("player_id", it).apply()
    }

    private var displayName = "Wayfarer"
    private var localRoom: LocalRoom? = null
    private var serverSocket: ServerSocket? = null
    private var clientSocket: Socket? = null
    private var clientWriter: BufferedWriter? = null
    private var nsdRegistrationListener: NsdManager.RegistrationListener? = null
    private var nsdDiscoveryListener: NsdManager.DiscoveryListener? = null
    private val clientConnections = ConcurrentHashMap<String, ClientConnection>()
    private val peerStates = ConcurrentHashMap<String, LocalPeerState>()
    private var stopped = true

    private val nsdManager: NsdManager by lazy {
        appContext.getSystemService(Context.NSD_SERVICE) as NsdManager
    }
    private val wifiManager: WifiP2pManager? by lazy {
        appContext.getSystemService(Context.WIFI_P2P_SERVICE) as? WifiP2pManager
    }
    private var wifiChannel: WifiP2pManager.Channel? = null
    private var wifiReceiverRegistered = false
    private var wifiDiscoveryCallback: ((List<LocalWifiPeer>) -> Unit)? = null
    private val wifiPeers = LinkedHashMap<String, LocalWifiPeer>()
    private val wifiReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            when (intent.action) {
                WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION -> requestWifiPeers()
                WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION -> requestWifiConnectionInfo()
            }
        }
    }

    private class ClientConnection(
        val socket: Socket,
        var playerId: String? = null,
        var writer: BufferedWriter? = null
    ) {
        val lock = Any()
    }

    fun setDisplayName(name: String) {
        displayName = normalizeName(name)
    }

    fun startLanHost(name: String): LocalRoom {
        displayName = normalizeName(name)
        stopTransportOnly()
        val room = LocalRoom(
            name = "$displayName’s Aethelgard Room",
            address = localIpv4Address(),
            port = PORT,
            code = randomRoomCode(),
            transport = "LAN"
        )
        localRoom = room
        stopped = false
        startServer()
        publishRoom(room)
        notifySession(true, true, room)
        return room
    }

    fun startLanDiscovery() {
        stopDiscoveryOnly()
        val listener = object : NsdManager.DiscoveryListener {
            override fun onDiscoveryStarted(serviceType: String) = Unit
            override fun onServiceFound(serviceInfo: NsdServiceInfo) {
                if (serviceInfo.serviceType != SERVICE_TYPE) return
                try {
                    nsdManager.resolveService(serviceInfo, object : NsdManager.ResolveListener {
                        override fun onResolveFailed(serviceInfo: NsdServiceInfo, errorCode: Int) = Unit
                        override fun onServiceResolved(resolved: NsdServiceInfo) {
                            val host = resolved.host?.hostAddress ?: return
                            val attributes = resolved.attributes
                            val code = attributes["code"]?.let { String(it, Charsets.UTF_8) } ?: "LOCAL"
                            val room = LocalRoom(
                                name = resolved.serviceName,
                                address = host,
                                port = resolved.port,
                                code = code,
                                transport = "LAN"
                            )
                            mainHandler.post { callbacks.onLocalRoomFound(room) }
                        }
                    })
                } catch (_: Exception) {
                    // A disappearing service is normal during discovery.
                }
            }
            override fun onServiceLost(serviceInfo: NsdServiceInfo) = Unit
            override fun onDiscoveryStopped(serviceType: String) = Unit
            override fun onStartDiscoveryFailed(serviceType: String, errorCode: Int) {
                mainHandler.post { callbacks.onLocalError("LAN discovery could not start ($errorCode).") }
                stopDiscoveryOnly()
            }
            override fun onStopDiscoveryFailed(serviceType: String, errorCode: Int) = stopDiscoveryOnly()
        }
        nsdDiscoveryListener = listener
        try {
            nsdManager.discoverServices(SERVICE_TYPE, NsdManager.PROTOCOL_DNS_SD, listener)
        } catch (error: Exception) {
            callbacks.onLocalError("LAN discovery unavailable: ${error.message ?: "unknown error"}")
        }
    }

    fun connectToRoom(room: LocalRoom) {
        displayName = normalizeName(displayName)
        stopTransportOnly()
        stopped = false
        io.execute {
            try {
                val socket = Socket()
                socket.connect(java.net.InetSocketAddress(room.address, room.port), 4_000)
                socket.tcpNoDelay = true
                clientSocket = socket
                val writer = BufferedWriter(OutputStreamWriter(socket.getOutputStream(), Charsets.UTF_8))
                clientWriter = writer
                notifySession(true, false, room)
                sendLine(writer, helloMessage())
                readLoop(socket, null)
            } catch (error: Exception) {
                mainHandler.post { callbacks.onLocalError("Could not join ${room.name}: ${error.message ?: "connection failed"}") }
                notifySession(false, false, null)
            }
        }
    }

    fun sendState(x: Float, y: Float, atTower: Boolean, towerRevision: Int) {
        val message = JSONObject()
            .put("type", "STATE")
            .put("version", PROTOCOL_VERSION)
            .put("id", localPlayerId)
            .put("name", displayName)
            .put("x", x)
            .put("y", y)
            .put("atTower", atTower)
            .put("towerRevision", towerRevision)
            .toString()
        sendToTransport(message)
    }

    fun sendEvent(action: String, payload: JSONObject = JSONObject()) {
        val message = JSONObject()
            .put("type", "EVENT")
            .put("version", PROTOCOL_VERSION)
            .put("id", localPlayerId)
            .put("action", action)
            .put("payload", payload)
            .toString()
        sendToTransport(message)
    }

    fun startWifiDirectDiscovery() {
        val manager = wifiManager ?: run {
            callbacks.onLocalError("Wi-Fi Direct is not available on this device.")
            return
        }
        ensureWifiReceiver()
        wifiDiscoveryCallback = { peers -> peers.forEach { callbacks.onWifiPeerFound(it) } }
        wifiPeers.clear()
        try {
            manager.discoverPeers(wifiChannel ?: initializeWifiChannel(), object : WifiP2pManager.ActionListener {
                override fun onSuccess() = Unit
                override fun onFailure(reason: Int) {
                    mainHandler.post { callbacks.onLocalError("Wi-Fi Direct discovery failed ($reason).") }
                }
            })
        } catch (error: SecurityException) {
            callbacks.onLocalError("Wi-Fi Direct permission is required.")
        }
    }

    fun createWifiDirectGroup() {
        val manager = wifiManager ?: run {
            callbacks.onLocalError("Wi-Fi Direct is not available on this device.")
            return
        }
        val channel = wifiChannel ?: initializeWifiChannel()
        ensureWifiReceiver()
        try {
            manager.createGroup(channel, object : WifiP2pManager.ActionListener {
                override fun onSuccess() = Unit
                override fun onFailure(reason: Int) {
                    mainHandler.post { callbacks.onLocalError("Wi-Fi Direct group creation failed ($reason).") }
                }
            })
        } catch (error: SecurityException) {
            callbacks.onLocalError("Wi-Fi Direct permission is required.")
        }
    }

    fun connectToWifiPeer(address: String) {
        val manager = wifiManager ?: return
        val channel = wifiChannel ?: initializeWifiChannel()
        ensureWifiReceiver()
        val config = WifiP2pConfig().apply {
            deviceAddress = address
            groupOwnerIntent = 15
        }
        try {
            manager.connect(channel, config, object : WifiP2pManager.ActionListener {
                override fun onSuccess() = Unit
                override fun onFailure(reason: Int) {
                    mainHandler.post { callbacks.onLocalError("Wi-Fi Direct connection failed ($reason).") }
                }
            })
        } catch (error: SecurityException) {
            callbacks.onLocalError("Wi-Fi Direct permission is required.")
        }
    }

    fun leaveSession() {
        stopDiscoveryOnly()
        stopTransportOnly()
        notifySession(false, false, null)
    }

    fun stop() {
        stopDiscoveryOnly()
        stopTransportOnly()
        try {
            if (wifiReceiverRegistered) appContext.unregisterReceiver(wifiReceiver)
        } catch (_: Exception) {
            // Receiver may already be gone during process teardown.
        }
        wifiReceiverRegistered = false
        wifiDiscoveryCallback = null
        try {
            wifiManager?.removeGroup(wifiChannel ?: initializeWifiChannel(), null)
        } catch (_: Exception) {
            // Removing an already-absent group is harmless.
        }
        io.shutdownNow()
    }

    private fun startServer() {
        io.execute {
            try {
                val socket = ServerSocket(PORT, MAX_PLAYERS)
                serverSocket = socket
                while (!stopped) {
                    val accepted = try { socket.accept() } catch (_: SocketException) { break }
                    if (clientConnections.size >= MAX_PLAYERS - 1) {
                        accepted.close()
                    } else {
                        val connection = ClientConnection(accepted)
                        io.execute { readLoop(accepted, connection) }
                    }
                }
            } catch (error: Exception) {
                mainHandler.post { callbacks.onLocalError("Local host could not open port $PORT: ${error.message ?: "unknown error"}") }
                notifySession(false, true, null)
            }
        }
    }

    private fun readLoop(socket: Socket, connection: ClientConnection?) {
        try {
            val reader = BufferedReader(InputStreamReader(socket.getInputStream(), Charsets.UTF_8))
            if (connection != null) {
                connection.writer = BufferedWriter(OutputStreamWriter(socket.getOutputStream(), Charsets.UTF_8))
            }
            while (!stopped) {
                val line = reader.readLine() ?: break
                if (line.length > 16_384) continue
                handleMessage(JSONObject(line), connection)
            }
        } catch (_: Exception) {
            // Disconnects are handled uniformly below.
        } finally {
            if (connection != null) {
                connection.playerId?.let { id ->
                    clientConnections.remove(id)
                    peerStates.remove(id)
                    publishPeerStates()
                    broadcast(JSONObject().put("type", "LEAVE").put("id", id).toString(), connection)
                }
                try { connection.socket.close() } catch (_: Exception) { }
            } else {
                try { socket.close() } catch (_: Exception) { }
                notifySession(false, false, null)
            }
        }
    }

    private fun handleMessage(message: JSONObject, connection: ClientConnection?) {
        if (message.optInt("version", PROTOCOL_VERSION) != PROTOCOL_VERSION) return
        when (message.optString("type")) {
            "HELLO" -> {
                val id = message.optString("id").takeIf { it.isNotBlank() } ?: return
                val name = normalizeName(message.optString("name"))
                if (connection != null) {
                    connection.playerId = id
                    clientConnections[id] = connection
                    sendLine(connection.writer, JSONObject().put("type", "WELCOME").put("id", localPlayerId).put("room", localRoom?.code ?: "LOCAL").toString())
                    val snapshot = JSONArray()
                    peerStates.values.forEach { state -> snapshot.put(stateToJson(state)) }
                    sendLine(connection.writer, JSONObject().put("type", "SNAPSHOT").put("peers", snapshot).toString())
                }
                val initial = peerStates[id]
                if (initial == null) peerStates[id] = LocalPeerState(id, name, 0f, 0f, false, 0)
                publishPeerStates()
                if (connection != null) broadcast(message.put("type", "PEER_JOIN").toString(), connection)
            }
            "PEER_JOIN" -> {
                val id = message.optString("id").takeIf { it.isNotBlank() } ?: return
                if (id != localPlayerId) {
                    peerStates[id] = LocalPeerState(id, normalizeName(message.optString("name")), 0f, 0f, false, 0)
                    publishPeerStates()
                }
            }
            "STATE", "PEER_STATE" -> {
                val id = message.optString("id").takeIf { it.isNotBlank() } ?: return
                val state = LocalPeerState(
                    playerId = id,
                    playerName = normalizeName(message.optString("name")),
                    x = message.optDouble("x", 0.0).toFloat().coerceIn(-1f, 1f),
                    y = message.optDouble("y", 0.0).toFloat().coerceIn(-1f, 1f),
                    atTower = message.optBoolean("atTower"),
                    towerRevision = message.optInt("towerRevision", 0).coerceAtLeast(0)
                )
                peerStates[id] = state
                publishPeerStates()
                if (connection != null) {
                    message.put("type", "PEER_STATE")
                    broadcast(message.toString(), connection)
                }
            }
            "EVENT" -> {
                val id = message.optString("id")
                if (id == localPlayerId) return
                val action = message.optString("action").takeIf { it.isNotBlank() } ?: return
                val payload = message.optJSONObject("payload") ?: JSONObject()
                mainHandler.post { callbacks.onRemoteEvent(action, payload) }
                if (connection != null) broadcast(message.toString(), connection)
            }
            "SNAPSHOT" -> {
                val array = message.optJSONArray("peers") ?: return
                for (index in 0 until array.length()) {
                    val item = array.optJSONObject(index) ?: continue
                    val id = item.optString("id")
                    if (id.isBlank() || id == localPlayerId) continue
                    peerStates[id] = LocalPeerState(
                        id,
                        normalizeName(item.optString("name")),
                        item.optDouble("x").toFloat(),
                        item.optDouble("y").toFloat(),
                        item.optBoolean("atTower"),
                        item.optInt("towerRevision")
                    )
                }
                publishPeerStates()
            }
            "LEAVE" -> {
                val id = message.optString("id")
                if (id.isNotBlank()) {
                    peerStates.remove(id)
                    publishPeerStates()
                }
            }
        }
    }

    private fun sendToTransport(message: String) {
        if (localRoom?.transport == "LAN" || localRoom?.transport == "WIFI_DIRECT") {
            if (serverSocket != null) {
                handleMessage(JSONObject(message), null)
                broadcast(message, null)
            } else {
                sendLine(clientWriter, message)
            }
        }
    }

    private fun broadcast(message: String, except: ClientConnection?) {
        clientConnections.values.forEach { connection ->
            if (connection !== except) sendLine(connection.writer, message)
        }
    }

    private fun sendLine(writer: BufferedWriter?, message: String) {
        if (writer == null) return
        try {
            synchronized(writer) {
                writer.write(message)
                writer.newLine()
                writer.flush()
            }
        } catch (_: Exception) {
            // The reader loop will remove the dead connection.
        }
    }

    private fun helloMessage(): String = JSONObject()
        .put("type", "HELLO")
        .put("version", PROTOCOL_VERSION)
        .put("id", localPlayerId)
        .put("name", displayName)
        .toString()

    private fun stateToJson(state: LocalPeerState): JSONObject = JSONObject()
        .put("id", state.playerId)
        .put("name", state.playerName)
        .put("x", state.x)
        .put("y", state.y)
        .put("atTower", state.atTower)
        .put("towerRevision", state.towerRevision)

    private fun publishPeerStates() {
        val states = peerStates.values.filter { it.playerId != localPlayerId }.sortedBy { it.playerId }
        mainHandler.post { callbacks.onPeerStatesChanged(states) }
    }

    private fun notifySession(connected: Boolean, host: Boolean, room: LocalRoom?) {
        mainHandler.post { callbacks.onLocalSessionChanged(connected, host, room) }
    }

    private fun publishRoom(room: LocalRoom) {
        val info = NsdServiceInfo().apply {
            serviceName = room.name
            serviceType = SERVICE_TYPE
            port = room.port
            setAttribute("code", room.code)
            setAttribute("version", PROTOCOL_VERSION.toString())
        }
        val listener = object : NsdManager.RegistrationListener {
            override fun onServiceRegistered(serviceInfo: NsdServiceInfo) = Unit
            override fun onRegistrationFailed(serviceInfo: NsdServiceInfo, errorCode: Int) {
                mainHandler.post { callbacks.onLocalError("LAN room advertisement failed ($errorCode).") }
            }
            override fun onServiceUnregistered(serviceInfo: NsdServiceInfo) = Unit
            override fun onUnregistrationFailed(serviceInfo: NsdServiceInfo, errorCode: Int) = Unit
        }
        nsdRegistrationListener = listener
        try { nsdManager.registerService(info, NsdManager.PROTOCOL_DNS_SD, listener) } catch (_: Exception) { }
    }

    private fun stopDiscoveryOnly() {
        nsdDiscoveryListener?.let {
            try { nsdManager.stopServiceDiscovery(it) } catch (_: Exception) { }
        }
        nsdDiscoveryListener = null
        nsdRegistrationListener?.let {
            try { nsdManager.unregisterService(it) } catch (_: Exception) { }
        }
        nsdRegistrationListener = null
    }

    private fun stopTransportOnly() {
        stopped = true
        try { serverSocket?.close() } catch (_: Exception) { }
        try { clientSocket?.close() } catch (_: Exception) { }
        serverSocket = null
        clientSocket = null
        clientWriter = null
        clientConnections.values.forEach { try { it.socket.close() } catch (_: Exception) { } }
        clientConnections.clear()
        peerStates.clear()
        localRoom = null
        mainHandler.post { callbacks.onPeerStatesChanged(emptyList()) }
    }

    private fun ensureWifiReceiver() {
        if (wifiReceiverRegistered) return
        val manager = wifiManager ?: return
        wifiChannel = wifiChannel ?: manager.initialize(appContext, mainHandler.looper, null)
        val filter = IntentFilter().apply {
            addAction(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION)
            addAction(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION)
            addAction(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION)
        }
        ContextCompat.registerReceiver(appContext, wifiReceiver, filter, ContextCompat.RECEIVER_EXPORTED)
        wifiReceiverRegistered = true
    }

    private fun initializeWifiChannel(): WifiP2pManager.Channel {
        val manager = wifiManager ?: error("Wi-Fi Direct unavailable")
        return (wifiChannel ?: manager.initialize(appContext, mainHandler.looper, null).also { wifiChannel = it })
    }

    private fun requestWifiPeers() {
        val manager = wifiManager ?: return
        try {
            manager.requestPeers(wifiChannel ?: initializeWifiChannel()) { list: WifiP2pDeviceList ->
                wifiPeers.clear()
                list.deviceList.forEach { device ->
                    val peer = LocalWifiPeer(device.deviceName.ifBlank { "Nearby Aethelgard device" }, device.deviceAddress)
                    wifiPeers[peer.address] = peer
                }
                val values = wifiPeers.values.toList()
                mainHandler.post { wifiDiscoveryCallback?.invoke(values) }
            }
        } catch (_: SecurityException) {
            callbacks.onLocalError("Wi-Fi Direct permission is required.")
        }
    }

    private fun requestWifiConnectionInfo() {
        val manager = wifiManager ?: return
        try {
            manager.requestConnectionInfo(wifiChannel ?: initializeWifiChannel()) { info: WifiP2pInfo ->
                if (!info.groupFormed || info.groupOwnerAddress == null) return@requestConnectionInfo
                if (info.isGroupOwner) {
                    if (serverSocket == null) {
                        val room = LocalRoom("$displayName’s Wi-Fi Direct Room", info.groupOwnerAddress.hostAddress ?: return@requestConnectionInfo, PORT, randomRoomCode(), "WIFI_DIRECT")
                        localRoom = room
                        stopped = false
                        startServer()
                        notifySession(true, true, room)
                    }
                } else {
                    val room = LocalRoom("Wi-Fi Direct host", info.groupOwnerAddress.hostAddress ?: return@requestConnectionInfo, PORT, "DIRECT", "WIFI_DIRECT")
                    if (clientSocket == null) connectToRoom(room)
                }
            }
        } catch (_: SecurityException) {
            callbacks.onLocalError("Wi-Fi Direct permission is required.")
        }
    }

    private fun normalizeName(value: String): String = value.trim().replace(Regex("\\s+"), " ").take(24).ifBlank { "Wayfarer" }

    private fun randomRoomCode(): String = UUID.randomUUID().toString().replace("-", "").take(6).uppercase()

    private fun localIpv4Address(): String = try {
        val addresses = java.net.NetworkInterface.getNetworkInterfaces().toList().asSequence()
            .flatMap { it.inetAddresses.toList().asSequence() }
            .filter { !it.isLoopbackAddress && it is java.net.Inet4Address }
            .toList()
        (addresses.firstOrNull { it.isSiteLocalAddress } ?: addresses.firstOrNull())?.hostAddress ?: "0.0.0.0"
    } catch (_: Exception) { "0.0.0.0" }
}

private fun java.net.NetworkInterface.getInetAddressesList(): List<InetAddress> = inetAddresses.toList()
private fun <T> java.util.Enumeration<T>.toList(): List<T> = buildList {
    while (hasMoreElements()) add(nextElement())
}
