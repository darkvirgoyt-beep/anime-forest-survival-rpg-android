package com.darkvirgoyt.aethelgrad

import android.Manifest
import android.animation.ValueAnimator
import android.app.Activity
import android.app.AlertDialog
import android.app.Dialog
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.ActivityInfo
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RadialGradient
import android.graphics.Shader
import android.graphics.drawable.GradientDrawable
import android.net.Uri
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.MotionEvent
import android.view.Surface
import android.view.SurfaceHolder
import android.view.View
import android.view.Window
import android.view.WindowManager
import android.view.animation.AccelerateDecelerateInterpolator
import android.opengl.GLES30
import android.opengl.GLSurfaceView
import android.content.Context
import android.widget.Button
import android.widget.CheckBox
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.RadioButton
import android.widget.RadioGroup
import android.widget.SeekBar
import android.widget.ScrollView
import org.json.JSONObject
import android.widget.TextView
import com.google.android.play.core.assetpacks.model.AssetPackStatus
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10
import kotlin.math.hypot
import kotlin.math.pow
import kotlin.math.roundToInt

object NativeGameBridge {
    init { System.loadLibrary("forestgame") }
    external fun init(width: Int, height: Int)
    external fun resize(width: Int, height: Int)
    external fun render(deltaSeconds: Float)
    external fun setMove(x: Float, y: Float)
    external fun setSprintHeld(held: Boolean)
    external fun orbitCamera(deltaYaw: Float, deltaPitch: Float)
    external fun toggleViewMode()
    external fun setWorldMapVisible(visible: Boolean)
    external fun setWorldTime(worldTime: Float)
    external fun setCoOpPeer(index: Int, active: Boolean, x: Float, y: Float, atTower: Boolean)
    external fun clearCoOpPeers()
    external fun teleportToTower()
    external fun syncTeleportToTower(revision: Int)
    external fun getCoOpLocalState(): String
    external fun setAuthoritativeOnline(enabled: Boolean)
    external fun setAuthoritativeBossHealth(health: Int)
    external fun applyAuthoritativeInventory(wood: Int, fiber: Int, stone: Int, emberKit: Boolean)
    external fun applyAuthoritativeCompanion(creatureIndex: Int, stay: Boolean, revision: Int)
    external fun applyAuthoritativeCamp(built: Boolean, x: Float, y: Float, z: Float, yaw: Float, scale: Float, revision: Int)
    external fun setGyroEnabled(enabled: Boolean)
    external fun setGyro(rotationX: Float, rotationY: Float, sensitivity: Float)
    external fun setPlayerCharacterTexture(width: Int, height: Int, pixels: IntArray)
    external fun attack()
    external fun heavyAttack()
    external fun jump()
    external fun dodge()
    external fun slide()
    external fun gather()
    external fun craft()
    // Legacy Emberling interaction remains available for compatibility with older saves and tests.
    external fun interactEmberling()
    external fun captureNearestCreature()
    external fun toggleCompanionCommand()
    external fun buildCamp()
    external fun getHudState(): String
    external fun setGraphicsQuality(level: Int)
    external fun setContentTierReady(ready: Boolean, tier: Int)
    external fun getCloudState(): String
    external fun loadCloudState(state: String): Boolean
}

class MainActivity : Activity(), SensorEventListener {
    private lateinit var rootContainer: FrameLayout
    private lateinit var gameView: GameSurfaceView
    private lateinit var assetPacks: AssetPackCatalog
    private lateinit var gyroButton: Button
    private var sensorManager: SensorManager? = null
    private var gyroSensor: Sensor? = null
    private var gyroEnabled = false
    private var gyroSensitivity = 1.0f
    private var lookSensitivity = 1.0f
    private var joystickSensitivity = 1.0f
    private var selectedTargetFps = 60
    private var selectedGraphicsTier = 2
    private var selectedResourceTier: ContentDownloadPlan.ResourceTier = ContentDownloadPlan.ResourceTier.STAGE_1
    private var supportedTargetFps = listOf(60)
    private val graphicsPreferences by lazy { getSharedPreferences("aethelgard_graphics", MODE_PRIVATE) }
    private val controlPreferences by lazy { getSharedPreferences("aethelgard_controls", MODE_PRIVATE) }
    private lateinit var audio: GameAudio
    private lateinit var joystickView: JoystickView
    private lateinit var stateLabel: TextView
    private lateinit var questLabel: TextView
    private var hudPlayerTitle: TextView? = null
    private var currentPlayerName = "PLAYER NAME"
    private lateinit var vitalMeter: VitalMeterView
    private lateinit var onboardingOverlay: View
    private var characterSetupOverlay: View? = null
    private var assetPatchOverlay: View? = null
    private var lastBackPressAtMs = 0L
    private val backPressWindowMs = 1_600L
    private var cinematicEntryOverlay: View? = null
    private var worldLoadingOverlay: View? = null
    private var worldFadeCurtain: View? = null
    private var worldLoadingCard: View? = null
    private var worldLoadingProgress: ProgressBar? = null
    private var worldLoadingStatus: TextView? = null
    private var worldLoadingSkip: Button? = null
    private var worldLoadingLoreCard: View? = null
    private var worldLoadingLoreKicker: TextView? = null
    private var worldLoadingLoreText: TextView? = null
    private var worldLoadingLoreIndex = 0
    private val worldLoadingLoreHandler = Handler(Looper.getMainLooper())
    private val worldLoadingProgressHandler = Handler(Looper.getMainLooper())
    private val worldLoadingLoreRotation = object : Runnable {
        override fun run() = rotateWorldLoadingLore()
    }
    private var playerSkippedLoadingPresentation = false
    private var worldEntryRevealed = false
    private var worldRevealScheduled = false
    private var worldLoadingStartedAtMs = 0L
    private val minimumWorldLoadingDurationMs = 10_000L
    private val worldLoadingProgressTicker = object : Runnable {
        override fun run() {
            if (worldLoadingOverlay == null || worldEntryRevealed) return
            refreshWorldLoadingPresentation()
            worldLoadingProgressHandler.postDelayed(this, 120L)
        }
    }
    private var pendingWorldReadyAction: (() -> Unit)? = null
    private var rendererReadyForWorld = false
    private var characterTextureReadyForWorld = false
    private var worldStateReadyForWorld = false
    private val worldLoadingTasks = linkedMapOf(
        "renderer" to WorldLoadingTask("AWAKENING THE WILDERNESS", 28),
        "texture" to WorldLoadingTask("WEAVING THE WAYFARER", 20),
        "content" to WorldLoadingTask("MOUNTING FOREST MEMORY", 32),
        "world" to WorldLoadingTask("RESTORING THE HORIZON", 20)
    )
    private val worldLoadingLore = listOf(
        LoadingLore("FOREST LAW", "Gather what the forest has already released. Living roots remember every careless cut."),
        LoadingLore("WARDEN PROTOCOL", "When the Warden lashes out, break the binding command—not the guardian beneath it."),
        LoadingLore("EMBER CRAFT", "Branchwood, root-fiber, and mineral dust carry different kinds of memory into every Ember Kit."),
        LoadingLore("TRAIL NOTE", "A gold-lit root path marks restoration. Violet light marks a memory that has not yet found its way home."),
        LoadingLore("WAYFARER'S BREATH", "Dodge with intent. Saving breath before a Warden strike matters more than winning a race through the undergrowth.")
    )
    private var resourcePreparationComplete = false
    private var highResolutionContentReady = false
    private var pendingWorldEntry: (() -> Unit)? = null
    private lateinit var standaloneExpansionFile: StandaloneExpansionFile
    private var authenticationTransitionStarted = false
    private lateinit var onboardingStatus: TextView
    private lateinit var characterNameInput: EditText
    private val accountSession = AccountSessionManager()
    private val characterCreation = CharacterCreationState()
    private var activeCloudWorld: CloudWorldManifest? = null
    private var cloudSaveInFlight = false
    private val requestedProgressiveSectors = mutableSetOf<ContentDownloadPlan.WorldSector>()
    private var progressiveContentNotice: String? = null
    private var progressiveConfirmationInFlight = false
    private var cloudRecoveryNotice: String? = null
    private var activeCoOpRoom: CoOpRoomSnapshot? = null
    private var coOpHeartbeatInFlight = false
    private var coOpReconnectInFlight = false
    private var coOpReconnectAttempts = 0
    private var coOpNextReconnectAtMs = 0L
    private var roomConnectInFlight = false
    private var lastCoOpTowerRevision = 0
    private var coOpRequestCounter = 0L
    private var coOpMemberRevision = 0
    private var authoritativeCompanion: CompanionStateSnapshot? = null
    private var authoritativeCamp: CampStateSnapshot? = null
    private var authoritativeTargets: List<CompanionTarget> = emptyList()
    private var coOpSaveInFlight = false
    private lateinit var coOpStatusLabel: TextView
    private var selectedServer = ServerDirectory.regions.first()
    private var selectedServerLatencyMs: Int? = null
    private var serverLatencyProbeToken = 0L
    private lateinit var networkMonitor: NetworkConnectivityMonitor
    private var networkOnline = false
    private var localSessionActive = false
    private var localSessionHost = false
    private var localRoom: LocalRoom? = null
    private val localRooms = linkedMapOf<String, LocalRoom>()
    private val localWifiPeers = linkedMapOf<String, LocalWifiPeer>()
    private var localDiscoverySummary: TextView? = null
    private var localPeerSummary: TextView? = null
    private var pendingLocalPermissionAction: (() -> Unit)? = null
    private lateinit var localMultiplayer: LocalMultiplayerManager
    private companion object {
        const val LOCAL_PERMISSION_REQUEST = 4207
        const val GUEST_PREFS = "aethelgard_guest_profile"
        const val GUEST_WORLD_STATE = "guest_world_state"
        const val GUEST_WORLD_NAME = "guest_world_name"
        val GUEST_DEFAULT_WORLD_STATE = """{"schemaVersion":5,"playerX":-0.550000,"playerY":-0.080000,"health":1.000000,"stamina":1.000000,"hunger":0.820000,"wood":12,"fiber":8,"stone":4,"experience":0,"level":0,"experienceToNext":991,"totalExperience":0,"day":1,"worldTime":0.000000,"gatheringActions":0,"questStage":0,"emberKitCrafted":0,"wardenDefeated":0,"emberlingTrust":0,"emberlingBonded":0,"emberlingStay":0,"discoveredSectors":1,"capturedMobIndex":-1,"capturedCompanionStay":0,"campBuilt":0,"campX":-0.640000,"campY":0.520000,"campZ":0.000000,"campYaw":0.000000,"campScale":1.000000,"companionRevision":0,"campRevision":0}"""
    }
    private var pingProbeInFlight = false
    private var latestPingMs: Int? = null
    private var googleLoginInFlight = false
    private var googleSignInButton: Button? = null
    private var accountLinkConsentAccepted = false
    private var currentPlayerProfile: PlayerProfile? = null
    private lateinit var networkStatusLabel: TextView
    private lateinit var identityStatusLabel: TextView
    private val localMultiplayerCallbacks = object : LocalMultiplayerManager.Callbacks {
        override fun onLocalRoomFound(room: LocalRoom) {
            localRooms[room.code] = room
            localDiscoverySummary?.text = localRooms.values.joinToString("\n") { "• ${it.name}  [${it.code}]  ${it.address}:${it.port}" }
            if (::coOpStatusLabel.isInitialized) {
                coOpStatusLabel.text = "LAN ROOM FOUND  •  ${room.name}  •  ${room.code}"
            }
        }

        override fun onWifiPeerFound(peer: LocalWifiPeer) {
            localWifiPeers[peer.address] = peer
            localPeerSummary?.text = localWifiPeers.values.joinToString("\n") { "• ${it.name}  [${it.address}]" }
            if (::coOpStatusLabel.isInitialized) {
                coOpStatusLabel.text = "WI-FI DIRECT PEER FOUND  •  ${peer.name}"
            }
        }

        override fun onLocalSessionChanged(connected: Boolean, host: Boolean, room: LocalRoom?) {
            localSessionActive = connected
            localSessionHost = host
            localRoom = room
            if (connected) {
                activeCoOpRoom = null
                if (::gameView.isInitialized) {
                    gameView.queueEvent { NativeGameBridge.setAuthoritativeOnline(true) }
                }
                hudHandler.removeCallbacks(coOpUpdater)
                hudHandler.postDelayed(coOpUpdater, 120L)
                if (::coOpStatusLabel.isInitialized) {
                    val role = if (host) "HOST" else "CLIENT"
                    coOpStatusLabel.text = "LOCAL CO-OP  •  $role  •  ${room?.code ?: "CONNECTED"}  •  LAN/WI-FI DIRECT"
                }
            } else {
                hudHandler.removeCallbacks(coOpUpdater)
                if (::coOpStatusLabel.isInitialized) {
                    coOpStatusLabel.text = "CO-OP SYNC PENDING  •  LOCAL SESSION CLOSED"
                }
            }
        }

        override fun onPeerStatesChanged(states: List<LocalPeerState>) {
            if (!::gameView.isInitialized) return
            gameView.queueEvent {
                NativeGameBridge.clearCoOpPeers()
                states.take(3).forEachIndexed { index, peer ->
                    NativeGameBridge.setCoOpPeer(index, true, peer.x, peer.y, peer.atTower)
                }
            }
            if (::coOpStatusLabel.isInitialized && localSessionActive) {
                coOpStatusLabel.text = "LOCAL CO-OP  •  ${if (localSessionHost) "HOST" else "CLIENT"}  •  ${states.size + 1}/4 PLAYERS"
            }
        }

        override fun onRemoteEvent(action: String, payload: JSONObject) {
            if (!::gameView.isInitialized) return
            when (action) {
                "attack" -> {
                    audio.playEffect("attack")
                    gameView.queueEvent { NativeGameBridge.attack() }
                }
                "heavy_attack" -> {
                    audio.playEffect("attack")
                    gameView.queueEvent { NativeGameBridge.heavyAttack() }
                }
                "gather" -> gameView.queueEvent { NativeGameBridge.gather() }
                "craft" -> gameView.queueEvent { NativeGameBridge.craft() }
                "build_camp" -> gameView.queueEvent { NativeGameBridge.buildCamp() }
                "capture" -> gameView.queueEvent { NativeGameBridge.captureNearestCreature() }
                "companion_command" -> gameView.queueEvent { NativeGameBridge.toggleCompanionCommand() }
            }
        }

        override fun onLocalError(message: String) {
            if (::coOpStatusLabel.isInitialized) coOpStatusLabel.text = "LOCAL CO-OP  •  $message"
        }
    }
    private val hudHandler = Handler(Looper.getMainLooper())
    private val hudUpdater = object : Runnable {
        override fun run() {
            if (::gameView.isInitialized) {
                gameView.queueEvent {
                    val snapshot = NativeGameBridge.getHudState()
                    runOnUiThread { applyHudSnapshot(snapshot) }
                }
            }
            hudHandler.postDelayed(this, 200L)
        }
    }
    private val coOpUpdater = object : Runnable {
        override fun run() {
            if (localSessionActive && ::gameView.isInitialized) {
                gameView.queueEvent {
                    val local = NativeGameBridge.getCoOpLocalState().split('|')
                    val x = local.getOrNull(0)?.toFloatOrNull() ?: -0.55f
                    val y = local.getOrNull(1)?.toFloatOrNull() ?: -0.08f
                    val atTower = local.getOrNull(2) == "1"
                    val towerRevision = local.getOrNull(3)?.toIntOrNull() ?: 0
                    runOnUiThread { localMultiplayer.sendState(x, y, atTower, towerRevision) }
                }
                hudHandler.postDelayed(this, 120L)
                return
            }
            val room = activeCoOpRoom
            if (room != null && !networkOnline) {
                if (::coOpStatusLabel.isInitialized) {
                    coOpStatusLabel.text = "CO-OP SYNC  •  RECONNECTING"
                }
                if (::gameView.isInitialized) {
                    gameView.queueEvent { NativeGameBridge.setAuthoritativeOnline(false) }
                }
            } else if (room != null && !coOpHeartbeatInFlight && ::gameView.isInitialized) {
                coOpHeartbeatInFlight = true
                gameView.queueEvent {
                    val local = NativeGameBridge.getCoOpLocalState().split('|')
                    val x = local.getOrNull(0)?.toFloatOrNull() ?: -0.55f
                    val y = local.getOrNull(1)?.toFloatOrNull() ?: -0.08f
                    val atTower = local.getOrNull(2) == "1"
                    val towerRevision = local.getOrNull(3)?.toIntOrNull() ?: 0
                    runOnUiThread {
                        accountSession.heartbeatCoOpRoom(room.code, x, y, atTower, towerRevision) { snapshot, error ->
                            coOpHeartbeatInFlight = false
                            if (snapshot != null) {
                                coOpReconnectAttempts = 0
                                coOpNextReconnectAtMs = 0L
                                applyCoOpSnapshot(snapshot)
                            } else if (error != null && ::coOpStatusLabel.isInitialized) {
                                coOpStatusLabel.text = "CO-OP ${room.code}  •  CONNECTION LOST  •  RECONNECTING"
                                requestCoOpReconnect(room.code)
                            }
                        }
                    }
                }
            }
            hudHandler.postDelayed(this, 2_000L)
        }
    }
    private val coOpSaveUpdater = object : Runnable {
        override fun run() {
            savePersistentCoOpState()
            hudHandler.postDelayed(this, 30_000L)
        }
    }
    private val cloudSaveUpdater = object : Runnable {
        override fun run() {
            if (accountSession.snapshot.isGuest && worldStateReadyForWorld) {
                saveGuestWorldState()
            }
            val world = activeCloudWorld
            if (!accountSession.snapshot.isGuest && world != null && networkOnline && !cloudSaveInFlight && ::gameView.isInitialized) {
                cloudSaveInFlight = true
                gameView.queueEvent {
                    val nativeState = NativeGameBridge.getCloudState()
                    runOnUiThread {
                        accountSession.uploadCloudWorld(world, nativeState) { updated, error ->
                            handleCloudSaveResult(updated, error)
                        }
                    }
                }
            }
            hudHandler.postDelayed(this, 45_000L)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        enterImmersiveMode()

        sensorManager = getSystemService(SENSOR_SERVICE) as SensorManager
        gyroSensor = sensorManager?.getDefaultSensor(Sensor.TYPE_GYROSCOPE)
        supportedTargetFps = detectSupportedTargetFps()
        selectedTargetFps = graphicsPreferences.getInt("target_fps", supportedTargetFps.maxOrNull() ?: 60)
            .takeIf { it in supportedTargetFps } ?: (supportedTargetFps.maxOrNull() ?: 60)
        selectedGraphicsTier = graphicsPreferences.getInt("graphics_tier", 2).coerceIn(0, 4)
        gyroSensitivity = controlPreferences.getFloat("gyro_sensitivity", 1.0f).coerceIn(0.25f, 2.5f)
        lookSensitivity = controlPreferences.getFloat("look_sensitivity", 1.0f).coerceIn(0.25f, 2.5f)
        joystickSensitivity = controlPreferences.getFloat("joystick_sensitivity", 1.0f).coerceIn(0.50f, 1.50f)
        audio = GameAudio(this)
        audio.playMusic()

        rootContainer = FrameLayout(this).apply { setBackgroundColor(Color.rgb(7, 16, 20)) }
        gameView = GameSurfaceView(this) { markWorldLoadingTaskReady("renderer") }
        assetPacks = AssetPackCatalog(this)
        standaloneExpansionFile = StandaloneExpansionFile(this)
        gameView.applyTargetFps(selectedTargetFps)
        gameView.applyGraphicsTier(selectedGraphicsTier)
        rootContainer.addView(gameView, FrameLayout.LayoutParams(-1, -1))
        rootContainer.addView(LookPadView(this) { dx, dy ->
            // Direct manipulation: the camera follows the finger instead of
            // orbiting against it. Both axes use the same intuitive mapping.
            gameView.queueEvent { NativeGameBridge.orbitCamera(-dx * 0.0048f * lookSensitivity, -dy * 0.0032f * lookSensitivity) }
        })
        rootContainer.addView(buildHud())
        joystickView = JoystickView(this) { x, y ->
            gameView.queueEvent { NativeGameBridge.setMove(x, y) }
        }
        joystickView.setSensitivity(joystickSensitivity)
        rootContainer.addView(joystickView)
        onboardingOverlay = buildOnboardingOverlay()
        rootContainer.addView(onboardingOverlay)
        setContentView(rootContainer)
        loadHeroineCharacterTexture()
        updateGyroButton()
        registerGyro()
        networkMonitor = NetworkConnectivityMonitor(this)
        // Initialize the session boundary before starting connectivity callbacks;
        // the monitor can report immediately on a warm network.
        accountSession.initialize(this, ::applyAccountSnapshot)
        localMultiplayer = LocalMultiplayerManager(this, localMultiplayerCallbacks)
        networkMonitor.start(::applyConnectivitySnapshot)
        onboardingOverlay.visibility = View.VISIBLE
        // Stage 1 forest content is the current phone-first boundary. The future
        // high-end archive remains an explicit opt-in tier after a real cook.
        showAssetPatchOverlay { beginOnlineStartup() }
        if (networkOnline) beginOnlineStartup()
    }

    private fun markProductionContentReady() {
        if (resourcePreparationComplete) return
        highResolutionContentReady = true
        resourcePreparationComplete = true
        markWorldLoadingTaskReady("content")
        val readyTier = selectedResourceTier
        applyGraphicsTier(readyTier.graphicsTierIndex)
        if (::gameView.isInitialized) {
            gameView.queueEvent { NativeGameBridge.setContentTierReady(true, readyTier.graphicsTierIndex) }
        }
    }

    /** Opens the verified base online world only when optional high-resolution delivery is unavailable. */
    private fun markCoreOnlineContentReady() {
        if (resourcePreparationComplete) return
        highResolutionContentReady = false
        resourcePreparationComplete = true
        markWorldLoadingTaskReady("content")
        val coreGraphicsTier = selectedGraphicsTier.coerceAtMost(2)
        if (::gameView.isInitialized) {
            gameView.applyGraphicsTier(coreGraphicsTier)
            gameView.queueEvent { NativeGameBridge.setContentTierReady(true, coreGraphicsTier) }
        }
    }

    private fun continuePendingWorldEntry() {
        pendingWorldEntry?.let { continuation ->
            pendingWorldEntry = null
            continuation()
        }
    }

    private fun beginOnlineStartup() {
        if (accountSession.snapshot.state == SessionState.AUTHENTICATED && accountSession.snapshot.isGuest) {
            if (!resourcePreparationComplete) {
                if (::onboardingStatus.isInitialized) onboardingStatus.text = "GUEST MODE  •  PREPARING LOCAL FOREST WORLD"
                return
            }
            if (pendingWorldEntry != null) {
                continuePendingWorldEntry()
            } else {
                applyAccountSnapshot(accountSession.snapshot)
            }
            return
        }
        if (!networkOnline) {
            if (::onboardingStatus.isInitialized) {
                onboardingStatus.text = "NETWORK REQUIRED  •  STAGE 1 CONTENT AND ONLINE PLAY ARE LOCKED"
            }
            return
        }
        if (!resourcePreparationComplete) {
            if (::onboardingStatus.isInitialized) {
                onboardingStatus.text = "STAGE 1 FOREST CONTENT CHECK REQUIRED BEFORE SIGN-IN"
            }
            return
        }
        continuePendingWorldEntry()
        when (accountSession.snapshot.state) {
            SessionState.SIGNED_OUT, SessionState.NETWORK_ERROR -> {
                if (::onboardingStatus.isInitialized) {
                    onboardingStatus.text = "SIGN IN WITH GOOGLE TO CONTINUE"
                }
            }
            SessionState.AUTHENTICATED -> {
                if (!authenticationTransitionStarted) applyAccountSnapshot(accountSession.snapshot)
            }
            else -> Unit
        }
    }

    private fun applyConnectivitySnapshot(snapshot: ConnectivitySnapshot) {
        networkOnline = snapshot.isOnline
        if (!networkOnline) {
            if (::networkStatusLabel.isInitialized) {
                networkStatusLabel.text = if (accountSession.snapshot.isGuest) "NETWORK: OFFLINE  •  GUEST PLAY" else "NETWORK: RECONNECTING"
                networkStatusLabel.setTextColor(Color.rgb(255, 180, 150))
            }
            if (::coOpStatusLabel.isInitialized && activeCoOpRoom == null) {
                coOpStatusLabel.text = "CO-OP: RECONNECTING"
            }
            if (::onboardingStatus.isInitialized) {
                onboardingStatus.text = if (accountSession.snapshot.isGuest) {
                    "GUEST MODE READY  •  LOCAL WORLD AVAILABLE  •  HOSTED CO-OP DISABLED"
                } else {
                    "NETWORK REQUIRED  •  STAGE 1 CONTENT AND ONLINE PLAY ARE LOCKED"
                }
            }
            return
        }

        if (::networkStatusLabel.isInitialized) {
            networkStatusLabel.text = if (accountSession.snapshot.isGuest) "NETWORK: OPTIONAL  •  GUEST PLAY" else "NETWORK: CONNECTED"
            networkStatusLabel.setTextColor(Color.rgb(164, 231, 190))
        }
        if (accountSession.snapshot.state != SessionState.AUTHENTICATED && ::onboardingStatus.isInitialized) {
            onboardingStatus.text = "CONNECTING TO ONLINE WORLD…"
        }
        beginOnlineStartup()
    }

    private fun updateNetworkAndIdentityLabels() {
        if (::networkStatusLabel.isInitialized) {
            networkStatusLabel.text = when {
                accountSession.snapshot.isGuest && networkOnline -> "NETWORK: OPTIONAL  •  GUEST PLAY"
                accountSession.snapshot.isGuest -> "NETWORK: OFFLINE  •  GUEST PLAY"
                networkOnline -> "NETWORK: CONNECTED"
                else -> "NETWORK: RECONNECTING"
            }
            networkStatusLabel.setTextColor(if (networkOnline) Color.rgb(164, 231, 190) else Color.rgb(255, 180, 150))
        }
        if (::identityStatusLabel.isInitialized) {
            val accountId = accountSession.snapshot.accountId
            val username = currentPlayerProfile?.username?.takeIf { it.isNotBlank() }
                ?: currentPlayerName
            val safeId = accountId?.take(10)?.let { "$it…" } ?: "PENDING"
            identityStatusLabel.text = if (accountSession.snapshot.isGuest) {
                "PLAYER: $username  •  GUEST LOCAL  •  ID: $safeId"
            } else {
                "PLAYER: $username  •  ID: $safeId"
            }
        }
        if (::coOpStatusLabel.isInitialized && accountSession.snapshot.isGuest && activeCoOpRoom == null) {
            coOpStatusLabel.text = "CO-OP UNAVAILABLE  •  GUEST MODE IS LOCAL-ONLY  •  GOOGLE LOGIN REQUIRED"
            coOpStatusLabel.setTextColor(Color.rgb(255, 205, 145))
        }
    }

    private fun apiLatencyProbeHost(): String =
        Uri.parse(getString(R.string.api_base_url)).host.orEmpty()

    private fun serverLatencySummary(latencyMs: Int?): String = when {
        !networkOnline -> "ROUTE CHECK  •  WAITING FOR NETWORK"
        latencyMs == null -> "ROUTE CHECK  •  UNAVAILABLE"
        latencyMs <= 80 -> "ROUTE CHECK  •  ${latencyMs} ms  •  EXCELLENT"
        latencyMs <= 160 -> "ROUTE CHECK  •  ${latencyMs} ms  •  STEADY"
        else -> "ROUTE CHECK  •  ${latencyMs} ms  •  HIGH LATENCY"
    }

    private fun measureServerLatency(region: ServerRegion, onRendered: (String) -> Unit = {}) {
        val probeToken = ++serverLatencyProbeToken
        val host = apiLatencyProbeHost()
        if (!networkOnline || host.isBlank()) {
            if (region.id == selectedServer.id) selectedServerLatencyMs = null
            onRendered(serverLatencySummary(null))
            return
        }
        onRendered("ROUTE CHECK  •  OPTIMIZING…")
        // The backend receives the selected region for matchmaking and cloud worlds.
        // This check measures the real API route only; it never delays world entry.
        networkMonitor.measureTcpLatency(host, timeoutMs = 900) { latencyMs ->
            if (probeToken != serverLatencyProbeToken) return@measureTcpLatency
            if (region.id == selectedServer.id) selectedServerLatencyMs = latencyMs
            onRendered(serverLatencySummary(latencyMs))
        }
    }

    private fun applyServerRegion(region: ServerRegion, serverButton: Button?) {
        selectedServer = region
        selectedServerLatencyMs = null
        serverButton?.text = "◉  ${region.name.removePrefix("Aethelgrad ").uppercase()}  ▾"
        if (::onboardingStatus.isInitialized) {
            onboardingStatus.text = "${region.name} selected  •  MATCH REGION READY"
        }
        measureServerLatency(region) { summary ->
            if (::onboardingStatus.isInitialized && !authenticationTransitionStarted) {
                onboardingStatus.text = "${region.name.removePrefix("Aethelgrad ").uppercase()}  •  $summary"
            }
        }
    }

    private fun showServerLocationPicker(serverButton: Button) {
        val regions = ServerDirectory.regions
        if (regions.isEmpty()) return
        var candidateIndex = regions.indexOfFirst { it.id == selectedServer.id }.coerceAtLeast(0)
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(24), dp(16), dp(24), dp(8))
        }
        val locationLabel = TextView(this).apply {
            setTextColor(Color.rgb(246, 232, 196))
            textSize = 18f
            gravity = Gravity.CENTER
        }
        val routeLabel = TextView(this).apply {
            setTextColor(Color.rgb(167, 214, 232))
            textSize = 12f
            gravity = Gravity.CENTER
            setPadding(0, dp(8), 0, dp(8))
        }
        val slider = SeekBar(this).apply {
            max = regions.lastIndex
            progress = candidateIndex
            contentDescription = "Server location"
        }
        fun renderCandidate(checkRoute: Boolean) {
            val region = regions[candidateIndex]
            locationLabel.text = region.name.removePrefix("Aethelgrad ").uppercase()
            routeLabel.text = if (checkRoute) "ROUTE CHECK  •  OPTIMIZING…" else serverLatencySummary(
                if (region.id == selectedServer.id) selectedServerLatencyMs else null
            )
            if (checkRoute) measureServerLatency(region) { summary -> routeLabel.text = summary }
        }
        slider.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(bar: SeekBar?, value: Int, fromUser: Boolean) {
                serverLatencyProbeToken++
                candidateIndex = value.coerceIn(0, regions.lastIndex)
                renderCandidate(false)
            }

            override fun onStartTrackingTouch(bar: SeekBar?) = Unit

            override fun onStopTrackingTouch(bar: SeekBar?) = renderCandidate(true)
        })
        panel.addView(locationLabel)
        panel.addView(slider, LinearLayout.LayoutParams(-1, dp(40)))
        panel.addView(routeLabel)
        renderCandidate(true)
        AlertDialog.Builder(this)
            .setTitle("SERVER LOCATION")
            .setMessage("Choose the co-op and cloud-world region. Route checks run in the background and never delay entering the world.")
            .setView(panel)
            .setNegativeButton("CANCEL", null)
            .setPositiveButton("USE LOCATION") { _, _ ->
                applyServerRegion(regions[candidateIndex], serverButton)
            }
            .show()
    }

    private fun requestGuestEntry() {
        if (googleLoginInFlight) return
        authenticationTransitionStarted = false
        val immediate = accountSession.requestGuestSignIn()
        if (immediate.state == SessionState.AUTHENTICATED && immediate.isGuest) {
            if (::onboardingStatus.isInitialized) {
                onboardingStatus.text = "GUEST MODE READY  •  LOCAL WORLD ONLY  •  HOSTED MULTIPLAYER REQUIRES GOOGLE"
                onboardingStatus.setTextColor(Color.rgb(164, 231, 190))
            }
            applyAccountSnapshot(immediate)
        }
    }

    private fun requestGoogleAccountLink() {
        if (!networkOnline) {
            if (::onboardingStatus.isInitialized) onboardingStatus.text = "ACCOUNT LINK  •  CONNECTION RESTORING"
            return
        }
        if (!accountLinkConsentAccepted) {
            if (::onboardingStatus.isInitialized) onboardingStatus.text = "CHECK THE ACCOUNT-LINK CONSENT BOX FIRST"
            return
        }
        if (googleLoginInFlight) {
            if (::onboardingStatus.isInitialized) onboardingStatus.text = "GOOGLE ACCOUNT CHOOSER IS ALREADY OPEN"
            return
        }
        googleLoginInFlight = true
        googleSignInButton?.apply {
            isEnabled = false
            alpha = 0.62f
            text = "OPENING GOOGLE ACCOUNT CHOOSER…"
        }
        authenticationTransitionStarted = false
        val immediate = accountSession.requestGoogleSignIn()
        if (immediate.state != SessionState.SIGNING_IN) {
            restoreGoogleSignInAction(immediate.state)
        }
    }

    private fun restoreGoogleSignInAction(state: SessionState) {
        googleLoginInFlight = false
        googleSignInButton?.apply {
            isEnabled = accountLinkConsentAccepted
            alpha = if (accountLinkConsentAccepted) 1.0f else 0.5f
            text = when (state) {
                SessionState.DENIED -> "↻  RETRY GOOGLE SIGN-IN"
                SessionState.CONFIGURATION_ERROR -> "GOOGLE LOGIN CONFIGURATION REQUIRED"
                else -> "✦  SIGN IN WITH GOOGLE"
            }
        }
    }

    private fun requireOnline(action: String): Boolean {
        if (accountSession.snapshot.isGuest && activeCoOpRoom == null && action != "CO-OP") return true
        if (networkOnline) return true
        if (::coOpStatusLabel.isInitialized) {
            coOpStatusLabel.text = "$action  •  CONNECTION RESTORING"
        }
        if (::onboardingStatus.isInitialized) {
            onboardingStatus.text = "ONLINE BLOCKED"
        }
        return false
    }

    private fun detectSupportedTargetFps(): List<Int> {
        val display = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) display else @Suppress("DEPRECATION") windowManager.defaultDisplay
        val maxRefreshRate = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            display?.supportedModes?.maxOfOrNull { it.refreshRate } ?: display?.refreshRate ?: 60f
        } else {
            @Suppress("DEPRECATION") (display?.refreshRate ?: 60f)
        }
        val options = listOf(60, 90, 120)
        return options.filter { it.toFloat() <= maxRefreshRate + 1.5f }.ifEmpty { listOf(60) }
    }

    private fun applyTargetFps(value: Int) {
        selectedTargetFps = value.coerceIn(supportedTargetFps.minOrNull() ?: 60, supportedTargetFps.maxOrNull() ?: 60)
        graphicsPreferences.edit().putInt("target_fps", selectedTargetFps).apply()
        if (::gameView.isInitialized) gameView.applyTargetFps(selectedTargetFps)
    }

    private fun applyGraphicsTier(value: Int) {
        selectedGraphicsTier = value.coerceIn(0, 4)
        graphicsPreferences.edit().putInt("graphics_tier", selectedGraphicsTier).apply()
        if (::gameView.isInitialized) gameView.applyGraphicsTier(selectedGraphicsTier)
    }

    private fun graphicsTierName(value: Int): String = listOf("LOW", "MEDIUM", "HIGH", "ULTRA", "MAX")[value.coerceIn(0, 4)]

    private fun registerGyro() {
        gyroSensor?.let { sensorManager?.registerListener(this, it, SensorManager.SENSOR_DELAY_GAME) }
    }

    private fun loadHeroineCharacterTexture() {
        val options = BitmapFactory.Options().apply {
            inPreferredConfig = Bitmap.Config.ARGB_8888
            inSampleSize = 4
        }
        val bitmap = BitmapFactory.decodeResource(resources, R.drawable.aethelgard_heroine_character, options) ?: return
        val pixels = IntArray(bitmap.width * bitmap.height)
        bitmap.getPixels(pixels, 0, bitmap.width, 0, 0, bitmap.width, bitmap.height)
        gameView.queueEvent {
            NativeGameBridge.setPlayerCharacterTexture(bitmap.width, bitmap.height, pixels)
            bitmap.recycle()
            runOnUiThread { markWorldLoadingTaskReady("texture") }
        }
    }

    private fun updateGyroButton() {
        if (!::gyroButton.isInitialized) return
        if (gyroSensor == null) {
            gyroEnabled = false
            gyroButton.text = "GYRO: UNSUPPORTED"
            gyroButton.isEnabled = false
            gyroButton.alpha = 0.45f
        } else {
            gyroButton.text = if (gyroEnabled) "GYRO: ON" else "GYRO: OFF"
            gyroButton.isEnabled = true
            gyroButton.alpha = 1.0f
        }
    }

    override fun onSensorChanged(event: SensorEvent) {
        if (event.sensor.type != Sensor.TYPE_GYROSCOPE || !gyroEnabled) return
        val x = event.values.getOrNull(0) ?: 0.0f
        val y = event.values.getOrNull(1) ?: 0.0f
        gameView.queueEvent { NativeGameBridge.setGyro(x, y, gyroSensitivity) }
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) = Unit

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) enterImmersiveMode()
    }

    /**
     * Handles Android system Back as navigation first and exit second. Dialogs
     * consume Back themselves; this method covers the full-screen game overlays
     * that are intentionally hosted inside the Activity root.
     */
    override fun onBackPressed() {
        if (characterSetupOverlay != null) {
            dismissCharacterSetupOverlay()
            return
        }
        if (worldLoadingOverlay != null) {
            skipLoadingPresentation()
            return
        }
        val now = System.currentTimeMillis()
        if (now - lastBackPressAtMs <= backPressWindowMs) {
            lastBackPressAtMs = 0L
            showCloseGameDialog()
        } else {
            lastBackPressAtMs = now
            android.widget.Toast.makeText(this, "Press Back again to close AETHELGRAD", android.widget.Toast.LENGTH_SHORT).show()
        }
    }

    private fun showCloseGameDialog() {
        val dialog = AlertDialog.Builder(this)
            .setTitle("CLOSE AETHELGRAD?")
            .setMessage("Do you want to close this game?")
            .setNegativeButton("CANCEL", null)
            .setPositiveButton("YES") { _, _ ->
                finishAndRemoveTask()
            }
            .create()
        dialog.setOnShowListener {
            dialog.getButton(AlertDialog.BUTTON_NEGATIVE)?.setTextColor(Color.rgb(64, 145, 255))
            dialog.getButton(AlertDialog.BUTTON_POSITIVE)?.setTextColor(Color.rgb(226, 184, 101))
        }
        dialog.show()
    }

    private fun dismissCharacterSetupOverlay() {
        val overlay = characterSetupOverlay ?: return
        rootContainer.removeView(overlay)
        characterSetupOverlay = null
        authenticationTransitionStarted = false
        onboardingOverlay.visibility = View.VISIBLE
        if (::onboardingStatus.isInitialized) {
            onboardingStatus.text = "SIGN IN WITH GOOGLE TO CONTINUE"
        }
    }

    private fun addBackArrow(parent: FrameLayout, onBack: () -> Unit) {
        val back = cinematicButton("‹  BACK", false, onBack).apply {
            contentDescription = "Go back one screen"
        }
        parent.addView(back, FrameLayout.LayoutParams(dp(122), dp(42), Gravity.TOP or Gravity.START).apply {
            topMargin = dp(18)
            leftMargin = dp(18)
        })
    }

    private fun enterImmersiveMode() {
        window.decorView.systemUiVisibility = (
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        )
    }

    override fun onPause() {
        audio.stopMusic()
        gyroEnabled = false
        hudHandler.removeCallbacks(hudUpdater)
        hudHandler.removeCallbacks(cloudSaveUpdater)
        hudHandler.removeCallbacks(coOpUpdater)
        hudHandler.removeCallbacks(coOpSaveUpdater)
        stopWorldLoadingLoreRotation()
        stopWorldLoadingProgressTicker()
        if (networkOnline) activeCoOpRoom?.let { savePersistentCoOpState() }
        if (localSessionActive) {
            localMultiplayer.leaveSession()
            localSessionActive = false
            localRoom = null
        }
        // Keep the hosted room reference for resume; onDestroy performs the explicit leave.
        if (::gameView.isInitialized) gameView.queueEvent { NativeGameBridge.clearCoOpPeers() }
        if (accountSession.snapshot.isGuest && worldStateReadyForWorld) saveGuestWorldState()
        val world = activeCloudWorld
        if (world != null && networkOnline && !cloudSaveInFlight) {
            cloudSaveInFlight = true
            gameView.queueEvent {
                val nativeState = NativeGameBridge.getCloudState()
                runOnUiThread {
                    accountSession.uploadCloudWorld(world, nativeState) { updated, error ->
                        handleCloudSaveResult(updated, error)
                    }
                }
            }
        }
        sensorManager?.unregisterListener(this)
        gameView.queueEvent { NativeGameBridge.setGyroEnabled(false) }
        gameView.onPause()
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
        audio.playMusic()
        gameView.onResume()
        registerGyro()
        updateGyroButton()
        if (worldLoadingOverlay != null && !worldEntryRevealed) {
            worldLoadingProgressHandler.removeCallbacks(worldLoadingProgressTicker)
            worldLoadingProgressHandler.post(worldLoadingProgressTicker)
        }
        hudHandler.postDelayed(hudUpdater, 350L)
        hudHandler.postDelayed(cloudSaveUpdater, 45_000L)
        if (activeCoOpRoom != null) {
            hudHandler.postDelayed(coOpUpdater, 1_000L)
            hudHandler.postDelayed(coOpSaveUpdater, 30_000L)
        }
    }

    override fun onDestroy() {
        hudHandler.removeCallbacks(hudUpdater)
        hudHandler.removeCallbacks(cloudSaveUpdater)
        hudHandler.removeCallbacks(coOpUpdater)
        hudHandler.removeCallbacks(coOpSaveUpdater)
        stopWorldLoadingLoreRotation()
        stopWorldLoadingProgressTicker()
        if (networkOnline) activeCoOpRoom?.let { room -> savePersistentCoOpState(); accountSession.leaveCoOpRoom(room.code) }
        if (localSessionActive) localMultiplayer.leaveSession()
        if (::localMultiplayer.isInitialized) localMultiplayer.stop()
        accountSession.shutdown()
        if (::networkMonitor.isInitialized) networkMonitor.stop()
        if (::assetPacks.isInitialized) assetPacks.close()
        if (::audio.isInitialized) audio.release()
        super.onDestroy()
    }

    private fun saveGuestWorldState() {
        if (!accountSession.snapshot.isGuest || !worldStateReadyForWorld || !::gameView.isInitialized) return
        gameView.queueEvent {
            val state = NativeGameBridge.getCloudState()
            runOnUiThread {
                getSharedPreferences(GUEST_PREFS, MODE_PRIVATE).edit()
                    .putString(GUEST_WORLD_STATE, state)
                    .putString(GUEST_WORLD_NAME, characterCreation.name.ifBlank { "Guest Horizon" })
                    .apply()
            }
        }
    }

    private fun guestWorldState(): String? = getSharedPreferences(GUEST_PREFS, MODE_PRIVATE)
        .getString(GUEST_WORLD_STATE, null)
        ?.takeIf { it.trimStart().startsWith("{") }

    private fun handleCloudSaveResult(updated: CloudWorldManifest?, error: String?) {
        if (updated != null) {
            activeCloudWorld = updated
            cloudRecoveryNotice = null
        } else if (error?.startsWith("A newer cloud revision") == true) {
            activeCloudWorld = null
            cloudRecoveryNotice = "CLOUD CONFLICT • SAVES PAUSED • REOPEN THIS WORLD TO RECOVER THE NEWER REVISION"
        }
        cloudSaveInFlight = false
    }

    private fun requestDiscoveredSectorContent(discoveredMask: Int) {
        if (!resourcePreparationComplete || !highResolutionContentReady || !networkOnline) return
        val tier = selectedResourceTier
        ContentDownloadPlan.WorldSector.values().forEach { sector ->
            if (discoveredMask and sector.bit == 0) return@forEach
            if (assetPacks.sectorContentReady(tier, sector)) return@forEach
            if (!requestedProgressiveSectors.add(sector)) return@forEach
            val label = sector.label
            progressiveContentNotice = "EXPANSION UNLOCKED  •  $label  •  VERIFYING DOWNLOAD SIZE"
            assetPacks.requestWorldSector(tier, sector) { event ->
                when {
                    event.complete -> {
                        progressiveContentNotice = "$label READY  •  LOCAL WORLD SIZE INCREASED"
                    }
                    event.status == AssetPackStatus.REQUIRES_USER_CONFIRMATION -> {
                        progressiveContentNotice = "$label  •  CONFIRM DOWNLOAD TO CONTINUE"
                        if (!progressiveConfirmationInFlight) {
                            progressiveConfirmationInFlight = true
                            assetPacks.showDownloadConfirmation(this) { accepted ->
                                progressiveConfirmationInFlight = false
                                requestedProgressiveSectors.remove(sector)
                                if (accepted) requestDiscoveredSectorContent(discoveredMask) else {
                                    progressiveContentNotice = "$label PAUSED  •  DOWNLOAD CAN RESUME LATER"
                                }
                            }
                        }
                    }
                    event.status == AssetPackStatus.WAITING_FOR_WIFI -> {
                        progressiveContentNotice = "$label WAITING FOR WI-FI  •  GAMEPLAY CONTINUES"
                    }
                    event.failed -> {
                        requestedProgressiveSectors.remove(sector)
                        progressiveContentNotice = "$label DOWNLOAD PAUSED  •  RETRY WHEN ONLINE"
                    }
                    else -> {
                        progressiveContentNotice = if (event.sizeVerified && event.totalBytes > 0L) {
                            val downloaded = event.bytesDownloaded / (1024 * 1024)
                            val total = event.totalBytes / (1024 * 1024)
                            "$label  •  ${event.percent}%  •  $downloaded / $total MB"
                        } else {
                            "$label  •  WAITING FOR VERIFIED DOWNLOAD METADATA"
                        }
                    }
                }
            }
        }
    }

    private fun buildOnboardingOverlay(): View {
        val overlay = FrameLayout(this)
        overlay.addView(CinematicLoginBackdropView(this), FrameLayout.LayoutParams(-1, -1))
        overlay.addView(ImageView(this).apply {
            setImageResource(R.drawable.aethelgard_login_cinematic_background)
            scaleType = ImageView.ScaleType.CENTER_CROP
            alpha = 0.72f
        }, FrameLayout.LayoutParams(-1, -1))
        overlay.addView(View(this).apply {
            background = GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(Color.argb(36, 3, 8, 14), Color.argb(118, 3, 7, 12), Color.argb(224, 2, 5, 8))
            )
        }, FrameLayout.LayoutParams(-1, -1))

        val serverButton = cinematicButton("◉  ${selectedServer.name.removePrefix("Aethelgrad ").uppercase()}  ▾", false) {}
        serverButton.setOnClickListener { showServerLocationPicker(serverButton) }
        overlay.addView(serverButton, FrameLayout.LayoutParams(dp(176), dp(42), Gravity.TOP or Gravity.END).apply {
            topMargin = dp(18)
            rightMargin = dp(24)
        })

        val settingsButton = cinematicButton("⚙  SETTINGS", false) { showControlSettings() }
        overlay.addView(settingsButton, FrameLayout.LayoutParams(dp(132), dp(38), Gravity.BOTTOM or Gravity.END).apply {
            bottomMargin = dp(16)
            rightMargin = dp(24)
        })

        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
            setPadding(dp(28), dp(18), dp(28), dp(18))
            background = GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(Color.argb(218, 10, 17, 24), Color.argb(234, 8, 13, 18))
            ).apply {
                cornerRadius = dp(18).toFloat()
                setStroke(dp(1), Color.rgb(150, 116, 62))
            }
        }
        val crest = TextView(this).apply {
            text = "✦"
            textSize = 42f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(226, 184, 101))
            setShadowLayer(12f, 0f, 0f, Color.argb(180, 226, 184, 101))
        }
        val title = TextView(this).apply {
            text = "AETHELGRAD"
            textSize = 35f
            gravity = Gravity.CENTER
            letterSpacing = 0.12f
            setTextColor(Color.rgb(239, 234, 219))
            typeface = android.graphics.Typeface.create("serif", android.graphics.Typeface.BOLD)
            setShadowLayer(10f, 0f, 2f, Color.BLACK)
        }
        val subtitle = TextView(this).apply {
            text = "WILD HORIZONS  •  CRAFTING"
            textSize = 12f
            gravity = Gravity.CENTER
            letterSpacing = 0.16f
            setTextColor(Color.rgb(217, 178, 99))
            setPadding(0, 0, 0, dp(8))
        }
        val divider = TextView(this).apply {
            text = "────  ◈  ────"
            textSize = 13f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(188, 143, 76))
        }
        val welcome = TextView(this).apply {
            text = "BEGIN YOUR JOURNEY"
            textSize = 20f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(236, 193, 108))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            setPadding(0, dp(8), 0, dp(2))
        }
        val instruction = TextView(this).apply {
            text = "CHOOSE YOUR PATH  •  Google unlocks cloud worlds and hosted co-op; Guest starts local worlds without Gmail or a server login."
            textSize = 12f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(210, 214, 218))
            setPadding(0, 0, 0, dp(6))
        }
        onboardingStatus = TextView(this).apply {
            text = "CHOOSE GOOGLE FOR ONLINE PLAY  OR  GUEST FOR LOCAL PLAY"
            textSize = 11f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(255, 205, 145))
            setPadding(dp(8), dp(6), dp(8), dp(6))
            background = GradientDrawable().apply {
                cornerRadius = dp(10).toFloat()
                setColor(Color.argb(115, 42, 39, 29))
            }
        }
        val consent = CheckBox(this).apply {
            text = "I consent to link my Google identity to this cloud world. The game does not read Gmail, Drive, or contacts."
            textSize = 11f
            setTextColor(Color.rgb(220, 222, 224))
            buttonTintList = android.content.res.ColorStateList.valueOf(Color.rgb(220, 182, 101))
            isChecked = false
        }
        val google = cinematicButton("✦  SIGN IN WITH GOOGLE", true) {
            requestGoogleAccountLink()
        }
        googleSignInButton = google
        google.isEnabled = false
        google.alpha = 0.5f
        consent.setOnCheckedChangeListener { _, checked ->
            accountLinkConsentAccepted = checked
            google.isEnabled = checked
            google.alpha = if (checked) 1f else 0.5f
            if (!checked && ::onboardingStatus.isInitialized) {
                onboardingStatus.text = "ACCOUNT-LINK CONSENT IS REQUIRED TO CONTINUE ONLINE"
            }
        }
        val guest = cinematicButton("CONTINUE AS GUEST  •  LOCAL PLAY", false) {
            requestGuestEntry()
        }
        val trustRow = LinearLayout(this).apply {
            gravity = Gravity.CENTER
            background = GradientDrawable().apply {
                cornerRadius = dp(10).toFloat()
                setColor(Color.argb(145, 7, 11, 16))
                setStroke(dp(1), Color.rgb(69, 62, 49))
            }
            setPadding(dp(6), dp(8), dp(6), dp(8))
        }
        listOf("☁\nCLOUD SAVE", "⟡\nACCOUNT LINK", "✥\nSECURE SESSIONS", "⚔\nCO-OP READY").forEachIndexed { index, value ->
            val item = TextView(this).apply {
                text = value
                textSize = 9f
                gravity = Gravity.CENTER
                setTextColor(Color.rgb(210, 188, 139))
            }
            trustRow.addView(item, LinearLayout.LayoutParams(0, dp(52), 1f).apply {
                if (index < 3) rightMargin = dp(2)
            })
        }

        panel.addView(crest, LinearLayout.LayoutParams(-1, dp(44)))
        panel.addView(title, LinearLayout.LayoutParams(-1, dp(50)))
        panel.addView(subtitle, LinearLayout.LayoutParams(-1, dp(28)))
        panel.addView(divider, LinearLayout.LayoutParams(-1, dp(22)))
        panel.addView(welcome, LinearLayout.LayoutParams(-1, dp(38)))
        panel.addView(instruction, LinearLayout.LayoutParams(-1, dp(30)))
        panel.addView(onboardingStatus, LinearLayout.LayoutParams(-1, dp(42)))
        panel.addView(consent, LinearLayout.LayoutParams(-1, dp(34)).apply { topMargin = dp(6) })
        panel.addView(google, LinearLayout.LayoutParams(-1, dp(52)).apply { topMargin = dp(2) })
        panel.addView(guest, LinearLayout.LayoutParams(-1, dp(48)).apply { topMargin = dp(6) })
        panel.addView(trustRow, LinearLayout.LayoutParams(-1, dp(68)).apply { topMargin = dp(12) })

        val scroll = ScrollView(this).apply {
            isFillViewport = true
            clipToPadding = false
            setPadding(dp(46), dp(14), dp(46), dp(14))
            addView(panel, FrameLayout.LayoutParams(-1, -2))
        }
        overlay.addView(scroll, FrameLayout.LayoutParams(-1, -1, Gravity.CENTER))
        return overlay
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).roundToInt()

    private fun applyAccountSnapshot(snapshot: SessionSnapshot) {
        updateNetworkAndIdentityLabels()
        if (snapshot.state != SessionState.SIGNING_IN && googleLoginInFlight) {
            restoreGoogleSignInAction(snapshot.state)
        }
        if (snapshot.state == SessionState.AUTHENTICATED && !snapshot.isGuest) {
            authenticationTransitionStarted = false
        }
        if (snapshot.state == SessionState.AUTHENTICATED && snapshot.isGuest) {
            if (authenticationTransitionStarted) return
            authenticationTransitionStarted = true
            val continueToGuestSetup = {
                showCharacterSetup(snapshot.accountId, recoveredProfile = null, recoveredWorlds = emptyList(), cloudError = null)
            }
            if (resourcePreparationComplete) {
                continueToGuestSetup()
            } else {
                pendingWorldEntry = continueToGuestSetup
            }
            return
        }
        if (!networkOnline) return
        if (snapshot.state == SessionState.AUTHENTICATED && !authenticationTransitionStarted) {
            authenticationTransitionStarted = true
            val continueToCharacterSetup = {
                accountSession.fetchProfile { profile, profileError ->
                    currentPlayerProfile = profile
                    updateNetworkAndIdentityLabels()
                    accountSession.fetchOwnedWorlds { worlds, worldsError ->
                        showCharacterSetup(snapshot.accountId, profile, worlds.orEmpty(), worldsError ?: profileError)
                    }
                }
            }
            if (resourcePreparationComplete) {
                continueToCharacterSetup()
            } else {
                pendingWorldEntry = continueToCharacterSetup
            }
            return
        }
        if (!::onboardingStatus.isInitialized) return
        onboardingStatus.text = snapshot.message
        onboardingStatus.setTextColor(
            when (snapshot.state) {
                SessionState.AUTHENTICATED -> Color.rgb(164, 231, 190)
                SessionState.SIGNING_IN -> Color.rgb(255, 205, 145)
                SessionState.CONFIGURATION_ERROR, SessionState.NETWORK_ERROR -> Color.rgb(255, 205, 145)
                SessionState.DENIED, SessionState.EXPIRED, SessionState.ERROR -> Color.rgb(255, 180, 150)
                SessionState.SIGNED_OUT -> Color.WHITE
            }
        )
    }

    /** Starts the real resource-backed warm-up; no pitch or reference video is part of production entry. */
    private fun enterWorldThroughCinematic(onWorldReady: () -> Unit) {
        if (worldLoadingOverlay != null) return
        audio.stopMusic()
        showWorldLoading(onWorldReady)
    }

    private fun showWorldLoading(onWorldReady: () -> Unit) {
        beginWorldLoading(onWorldReady)
        audio.playLoadingMusic()
        val overlay = FrameLayout(this).apply {
            alpha = 0f
            setBackgroundColor(Color.rgb(4, 18, 20))
        }
        val card = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
            setPadding(dp(38), dp(30), dp(38), dp(28))
            background = GradientDrawable().apply {
                setColor(Color.argb(220, 6, 27, 29))
                cornerRadius = dp(14).toFloat()
                setStroke(dp(1), Color.rgb(225, 179, 84))
            }
        }
        val ember = TextView(this).apply {
            text = "✦"
            textSize = 40f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(240, 195, 99))
            setShadowLayer(20f, 0f, 0f, Color.rgb(232, 184, 90))
            animate().rotationBy(360f).setDuration(1200L).withEndAction { rotation = 0f }.start()
        }
        val title = TextView(this).apply {
            text = "LET THE ROOTS REMEMBER"
            textSize = 18f
            gravity = Gravity.CENTER
            letterSpacing = 0.10f
            typeface = android.graphics.Typeface.create("serif", android.graphics.Typeface.BOLD)
            setTextColor(Color.rgb(245, 238, 217))
        }
        val status = TextView(this).apply {
            text = "PREPARING WISTERIA FOREST…"
            textSize = 9f
            gravity = Gravity.CENTER
            letterSpacing = 0.10f
            setTextColor(Color.rgb(197, 187, 158))
            setPadding(0, dp(8), 0, dp(10))
        }
        val progress = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            isIndeterminate = false
            max = 100
            progress = 0
            progressTintList = android.content.res.ColorStateList.valueOf(Color.rgb(232, 184, 90))
            progressBackgroundTintList = android.content.res.ColorStateList.valueOf(Color.rgb(46, 62, 58))
        }
        val loreCard = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(15), dp(10), dp(15), dp(10))
            background = GradientDrawable(
                GradientDrawable.Orientation.TL_BR,
                intArrayOf(Color.argb(150, 38, 31, 58), Color.argb(185, 9, 31, 33))
            ).apply {
                cornerRadius = dp(8).toFloat()
                setStroke(dp(1), Color.rgb(112, 85, 137))
            }
        }
        val loreKicker = TextView(this).apply {
            textSize = 8f
            letterSpacing = 0.15f
            setTextColor(Color.rgb(223, 182, 96))
        }
        val loreText = TextView(this).apply {
            textSize = 10f
            setTextColor(Color.rgb(220, 224, 217))
            setPadding(0, dp(4), 0, 0)
        }
        loreCard.addView(loreKicker, LinearLayout.LayoutParams(-1, dp(18)))
        loreCard.addView(loreText, LinearLayout.LayoutParams(-1, -2))
        val skip = cinematicButton("CONTINUE WHILE PREPARING  ›", false) { skipLoadingPresentation() }
        card.addView(ember, LinearLayout.LayoutParams(-1, dp(54)))
        card.addView(title, LinearLayout.LayoutParams(-1, dp(36)))
        card.addView(status, LinearLayout.LayoutParams(-1, dp(34)))
        card.addView(progress, LinearLayout.LayoutParams(-1, dp(5)))
        card.addView(loreCard, LinearLayout.LayoutParams(-1, -2).apply { topMargin = dp(16) })
        card.addView(skip, LinearLayout.LayoutParams(dp(330), dp(42)).apply { topMargin = dp(18) })
        overlay.addView(card, FrameLayout.LayoutParams(dp(500), -2, Gravity.CENTER))

        worldLoadingOverlay = overlay
        worldLoadingCard = card
        worldLoadingProgress = progress
        worldLoadingStatus = status
        worldLoadingSkip = skip
        worldLoadingLoreCard = loreCard
        worldLoadingLoreKicker = loreKicker
        worldLoadingLoreText = loreText
        rootContainer.addView(overlay)
        overlay.animate().alpha(1f).setDuration(180L).withEndAction {
            refreshWorldLoadingPresentation()
            worldLoadingProgressHandler.removeCallbacks(worldLoadingProgressTicker)
            worldLoadingProgressHandler.post(worldLoadingProgressTicker)
            startWorldLoadingLoreRotation()
        }.start()
    }

    private data class WorldLoadingTask(val label: String, val weight: Int, var ready: Boolean = false)
    private data class LoadingLore(val kicker: String, val text: String)

    private fun startWorldLoadingLoreRotation() {
        worldLoadingLoreHandler.removeCallbacks(worldLoadingLoreRotation)
        worldLoadingLoreIndex = 0
        showWorldLoadingLore(immediate = true)
        worldLoadingLoreHandler.postDelayed(worldLoadingLoreRotation, 3_800L)
    }

    private fun rotateWorldLoadingLore() {
        if (worldEntryRevealed || worldLoadingOverlay == null || worldLoadingLoreCard == null) return
        worldLoadingLoreCard?.animate()?.alpha(0f)?.translationY(dp(8).toFloat())?.setDuration(170L)?.withEndAction {
            worldLoadingLoreIndex = (worldLoadingLoreIndex + 1) % worldLoadingLore.size
            showWorldLoadingLore(immediate = false)
            worldLoadingLoreHandler.postDelayed(worldLoadingLoreRotation, 3_800L)
        }?.start()
    }

    private fun showWorldLoadingLore(immediate: Boolean) {
        val lore = worldLoadingLore.getOrNull(worldLoadingLoreIndex) ?: return
        worldLoadingLoreKicker?.text = "✦  ${lore.kicker}"
        worldLoadingLoreText?.text = lore.text
        val card = worldLoadingLoreCard ?: return
        if (immediate) {
            card.alpha = 1f
            card.translationY = 0f
        } else {
            card.translationY = -dp(8).toFloat()
            card.animate().alpha(1f).translationY(0f).setDuration(220L).start()
        }
    }

    private fun stopWorldLoadingLoreRotation() {
        worldLoadingLoreHandler.removeCallbacks(worldLoadingLoreRotation)
    }

    private fun stopWorldLoadingProgressTicker() {
        worldLoadingProgressHandler.removeCallbacks(worldLoadingProgressTicker)
    }

    /** Records a real engine, texture, content, or world-state callback for the entry progress meter. */
    private fun markWorldLoadingTaskReady(id: String) {
        when (id) {
            "renderer" -> rendererReadyForWorld = true
            "texture" -> characterTextureReadyForWorld = true
            "content" -> resourcePreparationComplete = true
            "world" -> worldStateReadyForWorld = true
        }
        worldLoadingTasks[id]?.ready = true
        refreshWorldLoadingPresentation()
    }

    private fun beginWorldLoading(onWorldReady: () -> Unit) {
        pendingWorldReadyAction = onWorldReady
        worldEntryRevealed = false
        worldRevealScheduled = false
        playerSkippedLoadingPresentation = false
        worldLoadingStartedAtMs = System.currentTimeMillis()
        worldLoadingProgressHandler.removeCallbacks(worldLoadingProgressTicker)
        worldLoadingTasks["renderer"]?.ready = rendererReadyForWorld
        worldLoadingTasks["texture"]?.ready = characterTextureReadyForWorld
        worldLoadingTasks["content"]?.ready = resourcePreparationComplete
        worldLoadingTasks["world"]?.ready = worldStateReadyForWorld
    }

    private fun refreshWorldLoadingPresentation() {
        if (pendingWorldReadyAction == null || worldEntryRevealed) return
        val totalWeight = worldLoadingTasks.values.sumOf { it.weight }.coerceAtLeast(1)
        val readyWeight = worldLoadingTasks.values.filter { it.ready }.sumOf { it.weight }
        val actualReadinessPercent = (readyWeight * 100 / totalWeight).coerceIn(0, 100)
        val elapsed = (System.currentTimeMillis() - worldLoadingStartedAtMs).coerceAtLeast(0L)
        val timelinePercent = ((elapsed * 100L) / minimumWorldLoadingDurationMs).toInt().coerceIn(0, 100)
        // Never show more progress than the real engine/content readiness. When
        // preparation finishes early, the remaining time is a controlled final
        // shader/world warm-up rather than an instant scene pop.
        val percent = minOf(actualReadinessPercent, timelinePercent)
        val nextTask = worldLoadingTasks.values.firstOrNull { !it.ready }
        val allReady = nextTask == null
        worldLoadingProgress?.progress = percent
        worldLoadingStatus?.text = when {
            playerSkippedLoadingPresentation && !allReady -> "PREPARING IN BACKGROUND  •  $percent%"
            !allReady -> "${nextTask.label}  •  $percent%"
            timelinePercent < 100 -> "WARMING STAGE 1 GRAPHICS  •  $percent%"
            else -> "NECESSARY RESOURCES READY  •  100%"
        }
        if (allReady && timelinePercent >= 100) revealWorldWhenReady()
    }

    /** Hides the large loading card without treating unfinished startup tasks as complete. */
    private fun skipLoadingPresentation() {
        if (worldEntryRevealed) return
        playerSkippedLoadingPresentation = true
        audio.playEffect("ui", rate = 0.88f)
        stopWorldLoadingLoreRotation()
        worldLoadingSkip?.isEnabled = false
        worldLoadingSkip?.text = "PREPARING IN BACKGROUND"
        worldLoadingCard?.animate()?.alpha(0.34f)?.scaleX(0.94f)?.scaleY(0.94f)?.setDuration(180L)?.start()
        refreshWorldLoadingPresentation()
    }

    private fun revealWorldWhenReady() {
        if (worldEntryRevealed || worldRevealScheduled || !worldLoadingTasks.values.all { it.ready }) return
        val elapsed = System.currentTimeMillis() - worldLoadingStartedAtMs
        val remainingMinimumPresentation = (minimumWorldLoadingDurationMs - elapsed).coerceAtLeast(0L)
        worldRevealScheduled = true
        rootContainer.postDelayed({
            worldRevealScheduled = false
            if (worldEntryRevealed || !worldLoadingTasks.values.all { it.ready }) return@postDelayed
            worldLoadingProgressHandler.removeCallbacks(worldLoadingProgressTicker)
            worldEntryRevealed = true
            val onWorldReady = pendingWorldReadyAction ?: return@postDelayed
            pendingWorldReadyAction = null
            audio.stopLoadingMusic()
            audio.playEffect("craft", rate = 1.08f)
            audio.playMusic()
            fadeLoadingIntoWorld(onWorldReady)
        }, remainingMinimumPresentation)
    }

    /** Fades the completed loading state to ink, applies the world entry, then reveals Wisteria Forest. */
    private fun fadeLoadingIntoWorld(onWorldReady: () -> Unit) {
        stopWorldLoadingLoreRotation()
        val overlay = worldLoadingOverlay
        val curtain = View(this).apply {
            alpha = 0f
            background = GradientDrawable(
                GradientDrawable.Orientation.TL_BR,
                intArrayOf(Color.rgb(3, 12, 15), Color.rgb(12, 7, 22), Color.rgb(3, 16, 18))
            )
        }
        worldFadeCurtain = curtain
        rootContainer.addView(curtain, FrameLayout.LayoutParams(-1, -1))
        overlay?.animate()?.alpha(0f)?.setDuration(240L)?.setInterpolator(AccelerateDecelerateInterpolator())?.start()
        curtain.animate().alpha(1f).setDuration(260L).setInterpolator(AccelerateDecelerateInterpolator()).withEndAction {
            onWorldReady()
            rootContainer.postDelayed({
                curtain.animate().alpha(0f).setDuration(520L).setInterpolator(AccelerateDecelerateInterpolator()).withEndAction {
                    rootContainer.removeView(curtain)
                    if (worldFadeCurtain === curtain) worldFadeCurtain = null
                    if (overlay != null) rootContainer.removeView(overlay)
                    if (worldLoadingOverlay === overlay) worldLoadingOverlay = null
                    worldLoadingCard = null
                    worldLoadingProgress = null
                    worldLoadingStatus = null
                    worldLoadingSkip = null
                    worldLoadingLoreCard = null
                    worldLoadingLoreKicker = null
                    worldLoadingLoreText = null
                }.start()
            }, 80L)
        }.start()
    }

    private fun connectToOnlineRoom() {
        if (accountSession.snapshot.isGuest) {
            if (::coOpStatusLabel.isInitialized) coOpStatusLabel.text = "CO-OP UNAVAILABLE  •  GUEST MODE IS LOCAL-ONLY  •  GOOGLE LOGIN REQUIRED"
            return
        }
        if (!requireOnline("CO-OP")) return
        if (roomConnectInFlight || activeCoOpRoom != null) return
        roomConnectInFlight = true
        accountSession.createCoOpRoom(selectedServer.id) { room, error ->
            roomConnectInFlight = false
            if (room != null) {
                startCoOpRoom(room)
            } else if (::coOpStatusLabel.isInitialized) {
                coOpStatusLabel.text = "ONLINE WORLD  •  ${error ?: "Room service unavailable; retry from CO-OP ROOM."}"
            }
        }
    }

    /** Restores the signed-in account’s profile and cloud-world setup after Google authentication. */
    private fun showCharacterSetup(accountId: String?, recoveredProfile: PlayerProfile? = null, recoveredWorlds: List<CloudWorldManifest> = emptyList(), cloudError: String? = null) {
        runOnUiThread {
            val guestMode = accountSession.snapshot.isGuest
            val savedGuestName = getSharedPreferences(GUEST_PREFS, MODE_PRIVATE).getString(GUEST_WORLD_NAME, null)
            setPlayerName(if (guestMode) savedGuestName else recoveredProfile?.username)
            if (characterSetupOverlay != null) return@runOnUiThread
            val availableWorlds = recoveredWorlds.take(6)
            var selectedWorld = availableWorlds.firstOrNull()
            onboardingOverlay.visibility = View.GONE
            val overlay = FrameLayout(this).apply {
                addView(ImageView(this@MainActivity).apply {
                    setImageResource(R.drawable.aethelgard_login_cinematic_background)
                    scaleType = ImageView.ScaleType.CENTER_CROP
                    alpha = 0.66f
                }, FrameLayout.LayoutParams(-1, -1))
                addView(View(this@MainActivity).apply {
                    background = GradientDrawable(
                        GradientDrawable.Orientation.TOP_BOTTOM,
                        intArrayOf(Color.argb(182, 3, 8, 13), Color.argb(238, 3, 7, 12))
                    )
                }, FrameLayout.LayoutParams(-1, -1))
            }
            val panel = LinearLayout(this).apply {
                orientation = LinearLayout.VERTICAL
                gravity = Gravity.CENTER_HORIZONTAL
                setPadding(dp(28), dp(22), dp(28), dp(18))
                background = GradientDrawable(
                    GradientDrawable.Orientation.TOP_BOTTOM,
                    intArrayOf(Color.argb(232, 10, 17, 24), Color.argb(242, 6, 11, 16))
                ).apply {
                    cornerRadius = dp(18).toFloat()
                    setStroke(dp(1), Color.rgb(150, 116, 62))
                }
            }
            panel.addView(ImageView(this).apply {
                setImageResource(R.drawable.aethelgard_profile_gold)
                scaleType = ImageView.ScaleType.CENTER_CROP
                contentDescription = "AETHELGRAD gold profile photo"
                background = GradientDrawable().apply {
                    shape = GradientDrawable.OVAL
                    setColor(Color.rgb(226, 184, 101))
                    setStroke(dp(2), Color.rgb(255, 235, 156))
                }
                clipToOutline = true
            }, LinearLayout.LayoutParams(dp(76), dp(76)).apply { bottomMargin = dp(6) })
            panel.addView(TextView(this).apply {
                text = "✦  AETHELGRAD  ✦"
                textSize = 15f
                gravity = Gravity.CENTER
                letterSpacing = 0.16f
                setTextColor(Color.rgb(226, 184, 101))
            }, LinearLayout.LayoutParams(-1, dp(30)))
            panel.addView(TextView(this).apply {
                text = when {
                    guestMode -> "LOCAL GUEST WORLD"
                    selectedWorld == null -> "CREATE YOUR WAYFARER"
                    else -> "YOUR CLOUD WORLDS"
                }
                textSize = 24f
                gravity = Gravity.CENTER
                typeface = android.graphics.Typeface.create("serif", android.graphics.Typeface.BOLD)
                setTextColor(Color.rgb(239, 234, 219))
            }, LinearLayout.LayoutParams(-1, dp(42)))
            val accountStatus = TextView(this).apply {
                text = when {
                    guestMode -> "Guest mode  •  local world and gameplay enabled  •  hosted co-op requires Google login"
                    selectedWorld != null && recoveredProfile?.username != null -> "${recoveredProfile.username}  •  ${selectedWorld?.name}  •  Revision ${selectedWorld?.saveRevision}"
                    selectedWorld != null -> "Cloud world found: ${selectedWorld?.name}  •  Revision ${selectedWorld?.saveRevision}"
                    recoveredProfile?.username != null -> "Welcome back, ${recoveredProfile.username}  •  Choose your cloud path"
                    !cloudError.isNullOrBlank() -> "Account verified  •  $cloudError"
                    else -> "Account verified${accountId?.let { "  •  ${it.take(8)}" } ?: ""}"
                }
                textSize = 11f
                gravity = Gravity.CENTER
                setTextColor(Color.rgb(164, 231, 190))
            }
            panel.addView(accountStatus, LinearLayout.LayoutParams(-1, dp(28)))
            if (availableWorlds.size > 1) {
                var selectedWorldIndex = 0
                val worldSummary = TextView(this).apply {
                    textSize = 10f
                    gravity = Gravity.CENTER
                    setTextColor(Color.rgb(237, 231, 214))
                }
                fun refreshWorldSelection() {
                    selectedWorld = availableWorlds[selectedWorldIndex]
                    val world = selectedWorld ?: return
                    worldSummary.text = "${selectedWorldIndex + 1}/${availableWorlds.size}  •  ${world.name.take(24)}  •  ${world.region.take(12)}  •  R${world.saveRevision}"
                    accountStatus.text = "Selected cloud world: ${world.name}  •  Revision ${world.saveRevision}"
                }
                val worldPicker = LinearLayout(this).apply { gravity = Gravity.CENTER_VERTICAL }
                worldPicker.addView(cinematicButton("‹", false) {
                    selectedWorldIndex = (selectedWorldIndex - 1 + availableWorlds.size) % availableWorlds.size
                    refreshWorldSelection()
                }, LinearLayout.LayoutParams(dp(42), dp(36)))
                worldPicker.addView(worldSummary, LinearLayout.LayoutParams(0, dp(36), 1f))
                worldPicker.addView(cinematicButton("›", false) {
                    selectedWorldIndex = (selectedWorldIndex + 1) % availableWorlds.size
                    refreshWorldSelection()
                }, LinearLayout.LayoutParams(dp(42), dp(36)))
                refreshWorldSelection()
                panel.addView(worldPicker, LinearLayout.LayoutParams(-1, dp(42)))
            }
            characterNameInput = EditText(this).apply {
                hint = when {
                    guestMode -> "WAYFARER NAME (LOCAL)"
                    selectedWorld == null -> "WAYFARER NAME"
                    else -> "PROFILE NAME"
                }
                setText(if (guestMode) savedGuestName.orEmpty() else recoveredProfile?.username.orEmpty())
                textSize = 15f
                isSingleLine = true
                setTextColor(Color.WHITE)
                setHintTextColor(Color.rgb(164, 170, 177))
                setPadding(dp(16), 0, dp(16), 0)
                background = GradientDrawable().apply {
                    cornerRadius = dp(10).toFloat()
                    setColor(Color.argb(180, 7, 12, 18))
                    setStroke(dp(1), Color.rgb(127, 99, 56))
                }
            }
            panel.addView(characterNameInput, LinearLayout.LayoutParams(-1, dp(48)).apply { topMargin = dp(12) })
            val appearance = LinearLayout(this).apply { gravity = Gravity.CENTER }
            val eyebrow = cinematicButton("BROW 01", false) { }
            val outfit = cinematicButton("OUTFIT 01", false) { }
            val hair = cinematicButton("HAIR 01", false) { }
            val avatars = listOf("heroine", "trailblazer")
            val avatarMarks = listOf("✦", "T")
            val avatarResources = listOf(
                R.drawable.aethelgard_heroine_character, R.drawable.aethelgard_avatar_trailblazer
            )
            val avatarColors = listOf(
                Color.rgb(112, 91, 126), Color.rgb(78, 114, 92)
            )
            var avatarIndex = avatars.indexOf(recoveredProfile?.avatarId).takeIf { it >= 0 } ?: 0
            val avatarStrip = LinearLayout(this).apply { gravity = Gravity.CENTER }
            val avatarChoices = mutableListOf<FrameLayout>()
            fun refreshAvatarChoices() {
                avatarChoices.forEachIndexed { index, choice ->
                    choice.background = GradientDrawable().apply {
                        setColor(avatarColors[index])
                        cornerRadius = dp(10).toFloat()
                        setStroke(dp(if (index == avatarIndex) 3 else 1), if (index == avatarIndex) Color.rgb(239, 204, 124) else Color.rgb(67, 54, 37))
                    }
                    choice.alpha = if (index == avatarIndex) 1f else 0.64f
                }
            }
            avatarMarks.forEachIndexed { index, mark ->
                avatarChoices += FrameLayout(this).apply {
                    addView(ImageView(this@MainActivity).apply {
                        scaleType = if (index == 0) ImageView.ScaleType.FIT_CENTER else ImageView.ScaleType.CENTER_CROP
                        setImageResource(avatarResources[index])
                        background = GradientDrawable().apply { shape = GradientDrawable.OVAL }
                        clipToOutline = true
                        contentDescription = "${avatars[index]} profile avatar"
                    }, FrameLayout.LayoutParams(-1, -1).apply { setMargins(dp(3), dp(3), dp(3), dp(3)) })
                    addView(TextView(this@MainActivity).apply {
                        text = mark
                        textSize = 11f
                        gravity = Gravity.CENTER
                        typeface = android.graphics.Typeface.DEFAULT_BOLD
                        setTextColor(Color.rgb(242, 235, 216))
                        setShadowLayer(2f, 0f, 1f, Color.BLACK)
                    }, FrameLayout.LayoutParams(-1, -1))
                    setOnClickListener { avatarIndex = index; refreshAvatarChoices() }
                }
            }
            eyebrow.setOnClickListener {
                characterCreation.eyebrowStyle = (characterCreation.eyebrowStyle + 1) % 4
                eyebrow.text = "BROW ${String.format("%02d", characterCreation.eyebrowStyle + 1)}"
            }
            outfit.setOnClickListener {
                characterCreation.outfitStyle = (characterCreation.outfitStyle + 1) % 4
                outfit.text = "OUTFIT ${String.format("%02d", characterCreation.outfitStyle + 1)}"
            }
            hair.setOnClickListener {
                characterCreation.hairStyle = (characterCreation.hairStyle + 1) % 4
                hair.text = "HAIR ${String.format("%02d", characterCreation.hairStyle + 1)}"
            }
            listOf(eyebrow, outfit, hair).forEach { control ->
                appearance.addView(control, LinearLayout.LayoutParams(0, dp(42), 1f).apply { leftMargin = dp(3); rightMargin = dp(3) })
            }
            panel.addView(appearance, LinearLayout.LayoutParams(-1, dp(50)).apply { topMargin = dp(10) })
            panel.addView(TextView(this).apply {
                text = "CHOOSE PROFILE AVATAR"
                textSize = 10f
                gravity = Gravity.CENTER
                letterSpacing = 0.12f
                setTextColor(Color.rgb(187, 165, 119))
            }, LinearLayout.LayoutParams(-1, dp(22)))
            avatarChoices.forEach { choice -> avatarStrip.addView(choice, LinearLayout.LayoutParams(dp(42), dp(42)).apply { leftMargin = dp(3); rightMargin = dp(3) }) }
            refreshAvatarChoices()
            panel.addView(avatarStrip, LinearLayout.LayoutParams(-1, dp(44)))
            val validation = TextView(this).apply {
                textSize = 11f
                gravity = Gravity.CENTER
                setTextColor(Color.rgb(255, 180, 150))
            }
            panel.addView(validation, LinearLayout.LayoutParams(-1, dp(26)))
            panel.addView(cinematicButton(if (guestMode) "ENTER LOCAL WORLD  ›" else if (selectedWorld == null) "CREATE CLOUD WORLD  ›" else "RESUME SELECTED WORLD  ›", true) {
                if (guestMode) {
                    characterCreation.name = characterNameInput.text.toString()
                    val issue = characterCreation.validate()
                    if (issue != null) {
                        validation.text = issue
                        return@cinematicButton
                    }
                    setPlayerName(characterCreation.name)
                    getSharedPreferences(GUEST_PREFS, MODE_PRIVATE).edit()
                        .putString(GUEST_WORLD_NAME, characterCreation.name)
                        .apply()
                    validation.setTextColor(Color.rgb(255, 205, 145))
                    validation.text = if (guestWorldState() == null) "Creating local world on this device…" else "Restoring local guest world…"
                    val savedState = guestWorldState() ?: GUEST_DEFAULT_WORLD_STATE
                    gameView.queueEvent {
                        val restored = NativeGameBridge.loadCloudState(savedState)
                        runOnUiThread {
                            if (!restored) {
                                validation.setTextColor(Color.rgb(255, 180, 150))
                                validation.text = "Local guest snapshot is incompatible with this game build."
                                return@runOnUiThread
                            }
                            activeCloudWorld = null
                            worldStateReadyForWorld = true
                            markWorldLoadingTaskReady("world")
                            validation.setTextColor(Color.rgb(164, 231, 190))
                            validation.text = "Local world ready. Entering Aethelgrad…"
                            rootContainer.postDelayed({
                                enterWorldThroughCinematic {
                                    rootContainer.removeView(overlay)
                                    characterSetupOverlay = null
                                }
                            }, 420L)
                        }
                    }
                    return@cinematicButton
                }
                val recoveredWorld = selectedWorld
                if (recoveredWorld != null) {
                    validation.setTextColor(Color.rgb(164, 231, 190))
                    validation.text = "Recovering ${recoveredWorld.name} from revision ${recoveredWorld.saveRevision}…"
                    accountSession.downloadCloudWorld(recoveredWorld) { snapshot, recoveryError ->
                        if (snapshot == null) {
                            validation.setTextColor(Color.rgb(255, 180, 150))
                            validation.text = recoveryError ?: "Cloud world recovery failed."
                            return@downloadCloudWorld
                        }
                        gameView.queueEvent {
                            val restored = NativeGameBridge.loadCloudState(snapshot)
                            runOnUiThread {
                                if (!restored) {
                                    validation.setTextColor(Color.rgb(255, 180, 150))
                                    validation.text = "Cloud snapshot is incompatible with this game build."
                                    return@runOnUiThread
                                }
                                activeCloudWorld = recoveredWorld
                                worldStateReadyForWorld = true
                                markWorldLoadingTaskReady("world")
                                enterWorldThroughCinematic {
                                    rootContainer.removeView(overlay)
                                    characterSetupOverlay = null
                                }
                            }
                        }
                    }
                    return@cinematicButton
                }
                characterCreation.name = characterNameInput.text.toString()
                setPlayerName(characterCreation.name)
                val issue = characterCreation.validate()
                if (issue != null) {
                    validation.text = issue
                    return@cinematicButton
                }
                validation.setTextColor(Color.rgb(255, 205, 145))
                validation.text = "Reserving your profile and creating a cloud world…"
                val avatarId = avatars[avatarIndex]
                accountSession.updateProfile(characterCreation.name, avatarId) { profileError ->
                    if (profileError != null) {
                        validation.setTextColor(Color.rgb(255, 180, 150))
                        validation.text = profileError
                        return@updateProfile
                    }
                    gameView.queueEvent {
                        val nativeState = NativeGameBridge.getCloudState()
                        runOnUiThread {
                            accountSession.createInitialCloudWorld("${characterCreation.name}'s Horizon", selectedServer.id, avatarId, nativeState) { world, worldError ->
                                if (worldError != null || world == null) {
                                    validation.setTextColor(Color.rgb(255, 180, 150))
                                    validation.text = worldError ?: "Cloud world creation failed."
                                    return@createInitialCloudWorld
                                }
                                activeCloudWorld = world
                                worldStateReadyForWorld = true
                                markWorldLoadingTaskReady("world")
                                validation.setTextColor(Color.rgb(164, 231, 190))
                                validation.text = "Cloud world protected. Entering Aethelgrad…"
                                rootContainer.postDelayed({
                                    enterWorldThroughCinematic {
                                        rootContainer.removeView(overlay)
                                        characterSetupOverlay = null
                                    }
                                }, 420L)
                            }
                        }
                    }
                }
            }, LinearLayout.LayoutParams(-1, dp(52)).apply { topMargin = dp(4) })
            val scroll = ScrollView(this).apply {
                isFillViewport = false
                clipToPadding = false
                isVerticalScrollBarEnabled = true
                setPadding(dp(30), dp(72), dp(30), dp(24))
                addView(panel, FrameLayout.LayoutParams(dp(520), -2, Gravity.CENTER))
            }
            overlay.addView(scroll, FrameLayout.LayoutParams(-1, -1, Gravity.CENTER))
            addBackArrow(overlay) { dismissCharacterSetupOverlay() }
            characterSetupOverlay = overlay
            rootContainer.addView(overlay)
        }
    }

    private fun nextCoOpRequestId(prefix: String): String {
        coOpRequestCounter += 1L
        return "$prefix-${System.currentTimeMillis()}-$coOpRequestCounter"
    }

    private fun companionNativeIndex(creatureId: String): Int = when (creatureId) {
        "moon_deer" -> 8
        "mossback_boar" -> 9
        "river_otter" -> 10
        "canopy_fox" -> 11
        else -> -1
    }

    private fun applyAuthoritativeCompanionSnapshot(companion: CompanionStateSnapshot?, camp: CampStateSnapshot?) {
        authoritativeCompanion = companion
        authoritativeCamp = camp
        if (::gameView.isInitialized) {
            gameView.queueEvent {
                if (companion == null) {
                    NativeGameBridge.applyAuthoritativeCompanion(-1, false, 0)
                } else {
                    val index = companionNativeIndex(companion.creatureId)
                    if (index >= 0) NativeGameBridge.applyAuthoritativeCompanion(index, companion.command == "stay", companion.revision)
                }
                if (camp == null) {
                    NativeGameBridge.applyAuthoritativeCamp(false, 0f, 0f, 0f, 0f, 1f, 0)
                } else {
                    runCatching {
                        val transform = JSONObject(camp.transformJson)
                        NativeGameBridge.applyAuthoritativeCamp(
                            true,
                            transform.optDouble("x", 0.0).toFloat(),
                            transform.optDouble("y", 0.0).toFloat(),
                            transform.optDouble("z", 0.0).toFloat(),
                            transform.optDouble("yaw", 0.0).toFloat(),
                            transform.optDouble("scale", 1.0).toFloat(),
                            camp.revision
                        )
                    }
                }
            }
        }
    }

    private fun reconcileAuthoritativeCompanionCamp(roomCode: String) {
        accountSession.fetchCompanionCampState(roomCode) { state, error ->
            if (state != null) {
                authoritativeTargets = state.targets
                applyAuthoritativeCompanionSnapshot(state.companion, state.camp)
                if (::coOpStatusLabel.isInitialized) {
                    val companionLabel = state.companion?.displayName ?: "NO COMPANION"
                    val campLabel = if (state.camp == null) "NO CAMP" else "CAMP R${state.camp.revision}"
                    coOpStatusLabel.text = "CO-OP $roomCode  •  $companionLabel  •  $campLabel  •  AUTHORITY SYNCED"
                }
            } else if (error != null && ::coOpStatusLabel.isInitialized) {
                coOpStatusLabel.text = "CO-OP $roomCode  •  AUTHORITY SYNC RETRY  •  $error"
            }
        }
    }

    private fun submitAuthoritativeCapture() {
        if (!requireOnline("COMPANION CAPTURE")) return
        val room = activeCoOpRoom
        if (localSessionActive) {
            gameView.queueEvent { NativeGameBridge.captureNearestCreature() }
            localMultiplayer.sendEvent("capture")
            return
        }
        if (room == null) {
            gameView.queueEvent { NativeGameBridge.captureNearestCreature() }
            return
        }
        gameView.queueEvent {
            val local = NativeGameBridge.getCoOpLocalState().split('|')
            val x = local.getOrNull(0)?.toFloatOrNull() ?: -0.55f
            val y = local.getOrNull(1)?.toFloatOrNull() ?: -0.08f
            val target = authoritativeTargets.minByOrNull { target -> hypot(target.x - x, target.y - y) }
            runOnUiThread {
                if (target == null) {
                    coOpStatusLabel.text = "CO-OP ${room.code}  •  NO SERVER-KNOWN CREATURE TARGET"
                    return@runOnUiThread
                }
                accountSession.captureCompanion(room.code, nextCoOpRequestId("capture"), target.creatureId) { result, error ->
                    if (result != null) {
                        coOpMemberRevision = maxOf(coOpMemberRevision, result.memberRevision)
                        applyAuthoritativeCompanionSnapshot(result.companion, authoritativeCamp)
                        authoritativeTargets = authoritativeTargets.filterNot { it.creatureId == result.companion.creatureId }
                        gameView.queueEvent { NativeGameBridge.applyAuthoritativeInventory(result.wood, result.fiber, result.stone, result.emberKit) }
                        coOpStatusLabel.text = "CO-OP ${room.code}  •  ${result.companion.displayName.uppercase()} CAPTURED  •  FIBER ${result.fiber}  •  R${result.companion.revision}"
                    } else if (error != null) {
                        coOpStatusLabel.text = "CO-OP ${room.code}  •  $error"
                        if (error.contains("already", ignoreCase = true) || error.contains("changed", ignoreCase = true)) reconcileAuthoritativeCompanionCamp(room.code)
                    }
                }
            }
        }
    }

    private fun submitAuthoritativeCompanionCommand() {
        if (!requireOnline("COMPANION COMMAND")) return
        val room = activeCoOpRoom
        val companion = authoritativeCompanion
        if (localSessionActive) {
            gameView.queueEvent { NativeGameBridge.toggleCompanionCommand() }
            localMultiplayer.sendEvent("companion_command")
            return
        }
        if (room == null || companion == null) {
            if (room == null) gameView.queueEvent { NativeGameBridge.toggleCompanionCommand() }
            else coOpStatusLabel.text = "CO-OP ${room.code}  •  NO ACTIVE COMPANION"
            return
        }
        val nextCommand = if (companion.command == "stay") "follow" else "stay"
        accountSession.setCompanionCommand(room.code, nextCoOpRequestId("companion"), nextCommand, companion.revision) { result, error ->
            if (result != null) {
                applyAuthoritativeCompanionSnapshot(result, authoritativeCamp)
                coOpStatusLabel.text = "CO-OP ${room.code}  •  COMPANION ${result.command.uppercase()}  •  R${result.revision}"
            } else if (error != null) {
                coOpStatusLabel.text = "CO-OP ${room.code}  •  $error"
                if (error.contains("changed", ignoreCase = true)) reconcileAuthoritativeCompanionCamp(room.code)
            }
        }
    }

    private fun submitAuthoritativeCamp() {
        if (!requireOnline("FIELD CAMP")) return
        val room = activeCoOpRoom
        if (localSessionActive) {
            gameView.queueEvent { NativeGameBridge.buildCamp() }
            localMultiplayer.sendEvent("build_camp")
            return
        }
        if (room == null) {
            gameView.queueEvent { NativeGameBridge.buildCamp() }
            return
        }
        gameView.queueEvent {
            val local = NativeGameBridge.getCoOpLocalState().split('|')
            val x = local.getOrNull(0)?.toFloatOrNull() ?: -0.55f
            val y = local.getOrNull(1)?.toFloatOrNull() ?: -0.08f
            val transform = JSONObject().put("x", x).put("y", y).put("z", 0.0).put("yaw", 0.0).put("scale", 1.0)
            runOnUiThread {
                accountSession.placeFieldCamp(room.code, nextCoOpRequestId("camp"), transform, authoritativeCamp?.revision ?: 0) { result, error ->
                    if (result != null) {
                        coOpMemberRevision = maxOf(coOpMemberRevision, result.memberRevision)
                        applyAuthoritativeCompanionSnapshot(authoritativeCompanion, result.camp)
                        gameView.queueEvent { NativeGameBridge.applyAuthoritativeInventory(result.wood, result.fiber, result.stone, result.emberKit) }
                        coOpStatusLabel.text = "CO-OP ${room.code}  •  FIELD CAMP BUILT  •  W ${result.wood} F ${result.fiber}  •  R${result.camp.revision}"
                    } else if (error != null) {
                        coOpStatusLabel.text = "CO-OP ${room.code}  •  $error"
                        if (error.contains("changed", ignoreCase = true) || error.contains("existing", ignoreCase = true)) reconcileAuthoritativeCompanionCamp(room.code)
                    }
                }
            }
        }
    }

    private fun submitAuthoritativeCombat(action: String) {
        if (!requireOnline("COMBAT")) return
        val room = activeCoOpRoom
        if (localSessionActive) {
            audio.playEffect("attack")
            gameView.queueEvent { if (action == "heavy_attack") NativeGameBridge.heavyAttack() else NativeGameBridge.attack() }
            localMultiplayer.sendEvent(action)
            return
        }
        if (room == null) {
            gameView.queueEvent { if (action == "heavy_attack") NativeGameBridge.heavyAttack() else NativeGameBridge.attack() }
            return
        }
        audio.playEffect("attack")
        gameView.queueEvent { if (action == "heavy_attack") NativeGameBridge.heavyAttack() else NativeGameBridge.attack() }
        accountSession.authoritativeCombat(room.code, nextCoOpRequestId("combat"), action) { result, error ->
            if (result != null) {
                gameView.queueEvent { NativeGameBridge.setAuthoritativeBossHealth(result.bossHealth) }
                coOpStatusLabel.text = "CO-OP ${room.code}  •  ${result.action.uppercase()} ACCEPTED  •  WARDEN HP ${result.bossHealth}/100"
            } else if (error != null) {
                coOpStatusLabel.text = "CO-OP ${room.code}  •  $error"
            }
        }
    }

    private fun submitAuthoritativeInventory(operation: String) {
        if (!requireOnline(operation.uppercase())) return
        val room = activeCoOpRoom
        if (localSessionActive) {
            audio.playEffect(if (operation == "craft") "craft" else "gather")
            gameView.queueEvent { if (operation == "craft") NativeGameBridge.craft() else NativeGameBridge.gather() }
            localMultiplayer.sendEvent(operation)
            return
        }
        if (room == null) {
            gameView.queueEvent { if (operation == "craft") NativeGameBridge.craft() else NativeGameBridge.gather() }
            return
        }
        audio.playEffect(if (operation == "craft") "craft" else "gather")
        gameView.queueEvent {
            val local = NativeGameBridge.getCoOpLocalState().split('|')
            val x = local.getOrNull(0)?.toFloatOrNull() ?: -0.55f
            val y = local.getOrNull(1)?.toFloatOrNull() ?: -0.08f
            val resourceId = if (operation == "gather") {
                val distances = listOf(
                    "forest_cache" to kotlin.math.abs(x + 0.56f) + kotlin.math.abs(y + 0.28f),
                    "root_cache" to kotlin.math.abs(x + 0.40f) + kotlin.math.abs(y + 0.18f),
                    "warden_stone" to kotlin.math.abs(x + 0.24f) + kotlin.math.abs(y + 0.28f)
                )
                distances.minByOrNull { it.second }?.first
            } else null
            runOnUiThread {
                accountSession.authoritativeInventory(room.code, nextCoOpRequestId(operation), operation, resourceId) { result, error ->
                    if (result != null) {
                        coOpMemberRevision = maxOf(coOpMemberRevision, result.memberRevision)
                        gameView.queueEvent { NativeGameBridge.applyAuthoritativeInventory(result.wood, result.fiber, result.stone, result.emberKit) }
                        coOpStatusLabel.text = "${room.worldName}  •  ${result.operation.uppercase()} ACCEPTED  •  W ${result.wood} F ${result.fiber} S ${result.stone}"
                    } else if (error != null) {
                        coOpStatusLabel.text = "CO-OP ${room.code}  •  $error"
                    }
                }
            }
        }
    }

    private fun requestCoOpReconnect(roomCode: String) {
        if (coOpReconnectInFlight) return
        val now = System.currentTimeMillis()
        if (now < coOpNextReconnectAtMs) return
        coOpReconnectInFlight = true
        coOpReconnectAttempts += 1
        accountSession.reconnectCoOpRoom(roomCode) { snapshot, error ->
            coOpReconnectInFlight = false
            if (snapshot != null) {
                coOpReconnectAttempts = 0
                coOpNextReconnectAtMs = 0L
                applyCoOpSnapshot(snapshot)
                reconcileAuthoritativeCompanionCamp(snapshot.code)
                if (::coOpStatusLabel.isInitialized) coOpStatusLabel.text = "CO-OP ${snapshot.code}  •  RECONNECTED  •  ${snapshot.participants.size}/${snapshot.maxPlayers}"
            } else {
                val delay = (1_000L shl coOpReconnectAttempts.coerceIn(0, 3)).coerceAtMost(8_000L)
                coOpNextReconnectAtMs = System.currentTimeMillis() + delay
                if (::coOpStatusLabel.isInitialized) {
                    coOpStatusLabel.text = if (coOpReconnectAttempts >= 4) {
                        "CO-OP $roomCode  •  RECONNECT FAILED  •  OPEN CO-OP ROOM"
                    } else {
                        "CO-OP $roomCode  •  RETRYING CONNECTION ${coOpReconnectAttempts}/4"
                    }
                }
                if (error == "Your membership has expired." || error == "Tower room is no longer available.") {
                    activeCoOpRoom = null
                    hudHandler.removeCallbacks(coOpUpdater)
                    if (::coOpStatusLabel.isInitialized) coOpStatusLabel.text = "CO-OP SYNC  •  RETRYING"
                }
            }
        }
    }

    private fun applyCoOpSnapshot(snapshot: CoOpRoomSnapshot) {
        activeCoOpRoom = snapshot
        if (!::coOpStatusLabel.isInitialized) return
        val accountId = accountSession.snapshot.accountId
        val remoteParticipants = snapshot.participants.filter { it.accountId != accountId }
        val incomingTowerRevision = remoteParticipants.maxOfOrNull { it.towerRevision } ?: 0
        if (incomingTowerRevision > lastCoOpTowerRevision && remoteParticipants.any { it.atTower && it.towerRevision >= incomingTowerRevision }) {
            lastCoOpTowerRevision = incomingTowerRevision
            gameView.queueEvent { NativeGameBridge.syncTeleportToTower(incomingTowerRevision) }
        }
        gameView.queueEvent {
            NativeGameBridge.setAuthoritativeBossHealth(snapshot.bossHealth)
            NativeGameBridge.setWorldTime(snapshot.worldTime)
            NativeGameBridge.clearCoOpPeers()
            remoteParticipants.take(3).forEachIndexed { index, participant ->
                NativeGameBridge.setCoOpPeer(index, true, participant.playerX, participant.playerY, participant.atTower)
            }
        }
        val tower = if (snapshot.towerRevision > 0) "TOWER ${snapshot.towerRevision}" else "TOWER READY"
        val ownerLabel = if (snapshot.ownerAccountId == accountSession.snapshot.accountId) "OWNER" else "SHARED"
        coOpStatusLabel.text = "${snapshot.worldName}  •  $ownerLabel  •  ${snapshot.code}  •  ${snapshot.participants.size}/${snapshot.maxPlayers}  •  $tower  •  WARDEN ${snapshot.bossHealth}/100  •  WEATHER SYNCED"
    }

    private fun startCoOpRoom(snapshot: CoOpRoomSnapshot) {
        if (accountSession.snapshot.isGuest) {
            if (::coOpStatusLabel.isInitialized) coOpStatusLabel.text = "CO-OP UNAVAILABLE  •  GUEST MODE IS LOCAL-ONLY  •  GOOGLE LOGIN REQUIRED"
            return
        }
        activeCoOpRoom = snapshot
        coOpReconnectInFlight = false
        coOpReconnectAttempts = 0
        coOpNextReconnectAtMs = 0L
        lastCoOpTowerRevision = 0
        coOpMemberRevision = 0
        coOpSaveInFlight = false
        accountSession.rememberCoOpRoom(snapshot.code)
        gameView.queueEvent { NativeGameBridge.setAuthoritativeOnline(true) }
        applyCoOpSnapshot(snapshot)
        accountSession.loadCoOpPlayerSave(snapshot.code) { playerSave, _ ->
            if (playerSave != null) {
                coOpMemberRevision = playerSave.memberRevision
                gameView.queueEvent { NativeGameBridge.loadCloudState(playerSave.progressionStateJson) }
                if (::coOpStatusLabel.isInitialized) coOpStatusLabel.text = "${snapshot.worldName}  •  SAVED ITEMS + PROGRESSION RESTORED  •  ${snapshot.code}"
            }
            reconcileAuthoritativeCompanionCamp(snapshot.code)
        }
        accountSession.loadCoOpWorldSave(snapshot.code) { worldSave, _ ->
            if (worldSave != null && ::coOpStatusLabel.isInitialized) {
                coOpStatusLabel.text = "${snapshot.worldName}  •  ${worldSave.buildingsJson.count { it == '{' }} SAVED BUILDINGS  •  ${snapshot.code}"
            }
        }
        hudHandler.removeCallbacks(coOpUpdater)
        hudHandler.removeCallbacks(coOpSaveUpdater)
        hudHandler.postDelayed(coOpUpdater, 250L)
        hudHandler.postDelayed(coOpSaveUpdater, 30_000L)
    }

    private fun savePersistentCoOpState() {
        val room = activeCoOpRoom ?: return
        if (!networkOnline || coOpSaveInFlight || !::gameView.isInitialized) return
        coOpSaveInFlight = true
        gameView.queueEvent {
            val nativeState = NativeGameBridge.getCloudState()
            runOnUiThread {
                try {
                    val root = JSONObject(nativeState)
                    val itemState = JSONObject()
                        .put("wood", root.optInt("wood"))
                        .put("fiber", root.optInt("fiber"))
                        .put("stone", root.optInt("stone"))
                        .put("emberKit", root.optBoolean("emberKitCrafted"))
                        .put("items", org.json.JSONArray())
                    accountSession.saveCoOpPlayerState(room.code, coOpMemberRevision, itemState.toString(), root.toString()) { revision, error ->
                        coOpSaveInFlight = false
                        if (revision != null) {
                            coOpMemberRevision = revision
                            if (::coOpStatusLabel.isInitialized) coOpStatusLabel.text = "${room.worldName}  •  SAVED  •  ${room.code}"
                        } else if (error != null && ::coOpStatusLabel.isInitialized) {
                            coOpStatusLabel.text = "${room.worldName}  •  SAVE RETRY  •  $error"
                        }
                    }
                } catch (_: Exception) {
                    coOpSaveInFlight = false
                }
            }
        }
    }

    private fun ensureLocalPermissions(action: () -> Unit): Boolean {
        if (accountSession.snapshot.isGuest) {
            if (::coOpStatusLabel.isInitialized) coOpStatusLabel.text = "CO-OP UNAVAILABLE  •  GUEST MODE IS LOCAL-ONLY  •  GOOGLE LOGIN REQUIRED"
            return false
        }
        val required = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            arrayOf(Manifest.permission.NEARBY_WIFI_DEVICES)
        } else {
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        }
        val missing = required.filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
        if (missing.isEmpty()) {
            action()
            return true
        }
        pendingLocalPermissionAction = action
        requestPermissions(missing.toTypedArray(), LOCAL_PERMISSION_REQUEST)
        return false
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != LOCAL_PERMISSION_REQUEST) return
        val action = pendingLocalPermissionAction
        pendingLocalPermissionAction = null
        if (grantResults.isNotEmpty() && grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
            action?.invoke()
        } else if (::coOpStatusLabel.isInitialized) {
            coOpStatusLabel.text = "LOCAL CO-OP  •  NEARBY DEVICES PERMISSION DENIED"
        }
    }

    private fun showLocalDiscoveryDialog(result: TextView) {
        ensureLocalPermissions {
            localRooms.clear()
            localWifiPeers.clear()
            localDiscoverySummary?.text = "No LAN rooms discovered yet."
            localPeerSummary?.text = "No Wi-Fi Direct peers discovered yet."
            localMultiplayer.setDisplayName(currentPlayerName)
            localMultiplayer.startLanDiscovery()
            result.text = "Searching for LAN rooms…"
        }
    }

    private fun hostLocalLan(result: TextView) {
        ensureLocalPermissions {
            localMultiplayer.setDisplayName(currentPlayerName)
            val room = localMultiplayer.startLanHost(currentPlayerName)
            localRoom = room
            result.text = "Hosting ${room.code} on ${room.address}:${room.port}"
        }
    }

    private fun joinLocalLan(result: TextView) {
        ensureLocalPermissions {
            val room = localRooms.values.firstOrNull()
            if (room == null) {
                result.text = "No LAN room found. Search first, then try again."
                return@ensureLocalPermissions
            }
            localMultiplayer.setDisplayName(currentPlayerName)
            localMultiplayer.connectToRoom(room)
            result.text = "Joining ${room.name}…"
        }
    }

    private fun hostWifiDirect(result: TextView) {
        ensureLocalPermissions {
            localMultiplayer.setDisplayName(currentPlayerName)
            localMultiplayer.createWifiDirectGroup()
            result.text = "Creating Wi-Fi Direct group…"
        }
    }

    private fun discoverWifiDirect(result: TextView) {
        ensureLocalPermissions {
            localWifiPeers.clear()
            localPeerSummary?.text = "Searching for Wi-Fi Direct peers…"
            localMultiplayer.startWifiDirectDiscovery()
            result.text = "Searching for nearby Wi-Fi Direct devices…"
        }
    }

    private fun joinWifiDirect(result: TextView) {
        ensureLocalPermissions {
            val peer = localWifiPeers.values.firstOrNull()
            if (peer == null) {
                result.text = "No Wi-Fi Direct peer found. Discover first, then try again."
                return@ensureLocalPermissions
            }
            localMultiplayer.setDisplayName(currentPlayerName)
            localMultiplayer.connectToWifiPeer(peer.address)
            result.text = "Connecting to ${peer.name}…"
        }
    }

    private fun showCoOpDialog() {
        if (accountSession.snapshot.isGuest) {
            AlertDialog.Builder(this)
                .setTitle("HOSTED CO-OP UNAVAILABLE")
                .setMessage("Guest mode keeps every local world, combat, gathering, crafting, companion, camp, and exploration feature on this device. Hosted multiplayer, cloud worlds, and shared saves require Google login.")
                .setNegativeButton("BACK", null)
                .setPositiveButton("SWITCH TO GOOGLE") { _, _ ->
                    confirmLogout()
                }
                .show()
            return
        }
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(18), dp(8), dp(18), dp(4))
        }
        val hostedHeading = TextView(this).apply {
            text = "HOSTED INTERNET CO-OP  •  FREE RENDER SERVICE"
            textSize = 12f
            setTextColor(Color.rgb(164, 231, 190))
            setPadding(0, 0, 0, dp(5))
        }
        panel.addView(hostedHeading)
        val explanation = TextView(this).apply {
            text = "No Termux, local server, or same Wi-Fi is required. Create a six-character room code and share it with friends. Render synchronizes the day-night clock, weather phase, player positions, tower arrivals, combat, inventory, crafting, and cloud saves for up to four players."
            textSize = 12f
            setTextColor(Color.rgb(205, 220, 218))
            setPadding(0, 0, 0, dp(10))
        }
        panel.addView(explanation)
        val status = TextView(this).apply {
            text = activeCoOpRoom?.let { "Hosted room: ${it.code}  •  ${it.participants.size}/${it.maxPlayers} PLAYERS" } ?: "No active hosted room"
            textSize = 12f
            setTextColor(Color.rgb(244, 218, 155))
            gravity = Gravity.CENTER
            setPadding(0, 0, 0, dp(10))
        }
        panel.addView(status)
        val codeInput = EditText(this).apply {
            hint = "ROOM CODE (6 CHARACTERS)"
            textSize = 14f
            isSingleLine = true
            setTextColor(Color.WHITE)
            setHintTextColor(Color.rgb(164, 170, 177))
            setPadding(dp(14), 0, dp(14), 0)
            background = GradientDrawable().apply {
                cornerRadius = dp(10).toFloat()
                setColor(Color.argb(180, 7, 12, 18))
                setStroke(dp(1), Color.rgb(127, 99, 56))
            }
        }
        panel.addView(codeInput, LinearLayout.LayoutParams(-1, dp(48)))
        val result = TextView(this).apply {
            textSize = 11f
            setTextColor(Color.rgb(255, 205, 145))
            gravity = Gravity.CENTER
            setPadding(0, dp(8), 0, 0)
        }
        panel.addView(result, LinearLayout.LayoutParams(-1, dp(32)))
        val create = actionButton("CREATE TOWER ROOM") {
            if (!requireOnline("CO-OP")) {
                result.text = "Connection is restoring. Create a room when sync returns."
                return@actionButton
            }
            result.text = "Allocating a shared tower room…"
            accountSession.createCoOpRoom(selectedServer.id) { room, error ->
                if (room == null) {
                    result.text = error ?: "Room creation failed."
                } else {
                    startCoOpRoom(room)
                    result.setTextColor(Color.rgb(164, 231, 190))
                    result.text = "Room ${room.code} ready. Share this code with your friends."
                    codeInput.setText(room.code)
                    status.text = "Active room: ${room.code}"
                }
            }
        }
        val join = actionButton("JOIN TOWER ROOM") {
            if (!requireOnline("CO-OP")) {
                result.text = "Connection is restoring. Join a room when sync returns."
                return@actionButton
            }
            result.text = "Joining tower room…"
            accountSession.joinCoOpRoom(codeInput.text.toString()) { room, error ->
                if (room == null) {
                    result.text = error ?: "Room join failed."
                } else {
                    startCoOpRoom(room)
                    result.setTextColor(Color.rgb(164, 231, 190))
                    result.text = "Joined ${room.code}. Friends will appear near the tower."
                    status.text = "Active room: ${room.code}"
                }
            }
        }
        panel.addView(create, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(8) })
        panel.addView(join, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(6) })

        val savedRoomCode = accountSession.lastPersistentCoOpRoomCode()
        if (activeCoOpRoom == null && !savedRoomCode.isNullOrBlank()) {
            val reconnect = actionButton("RECONNECT HOSTED ROOM  •  ${savedRoomCode}") {
                result.text = "Reconnecting to hosted room ${savedRoomCode}…"
                accountSession.reconnectCoOpRoom(savedRoomCode) { room, error ->
                    if (room == null) {
                        result.text = error ?: "Hosted room is unavailable. Create or join another room."
                    } else {
                        startCoOpRoom(room)
                        result.setTextColor(Color.rgb(164, 231, 190))
                        result.text = "Reconnected to ${room.code}."
                        status.text = "Hosted room: ${room.code}  •  ${room.participants.size}/${room.maxPlayers} PLAYERS"
                    }
                }
            }
            panel.addView(reconnect, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(6) })
        }

        if (activeCoOpRoom != null) {
            val share = actionButton("SHARE ROOM INVITE") {
                val room = activeCoOpRoom ?: return@actionButton
                startActivity(Intent.createChooser(Intent(Intent.ACTION_SEND).apply {
                    type = "text/plain"
                                        putExtra(Intent.EXTRA_TEXT, "Join my AETHELGRAD hosted co-op tower room ${room.code} in ${room.region}. No Termux or local server is required. Up to ${room.maxPlayers} players.")
                }, "Share AETHELGRAD hosted invite")
)
            }
            panel.addView(share, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(6) })
        }
        if (activeCoOpRoom != null || localSessionActive) {
            val leave = actionButton("LEAVE CURRENT ROOM") {
                activeCoOpRoom?.let { accountSession.leaveCoOpRoom(it.code) }
                if (localSessionActive) localMultiplayer.leaveSession()
                activeCoOpRoom = null
                localSessionActive = false
                localRoom = null
                hudHandler.removeCallbacks(coOpUpdater)
                gameView.queueEvent {
                    NativeGameBridge.clearCoOpPeers()
                    NativeGameBridge.setAuthoritativeOnline(false)
                }
                if (::coOpStatusLabel.isInitialized) coOpStatusLabel.text = "CO-OP SYNC PENDING  •  WEATHER LOCAL"
                result.text = "Left the room."
            }
            panel.addView(leave, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(6) })
        }
        val scroll = ScrollView(this).apply {
            addView(panel)
        }
        AlertDialog.Builder(this)
            .setTitle("HOSTED CO-OP TOWER RENDEZVOUS")
            .setView(scroll)
            .setNegativeButton("‹ BACK", null)
            .show()
    }

    private fun setPlayerName(name: String?) {
        val normalized = name?.trim()?.replace(Regex("\\s+"), " ")?.take(24).orEmpty()
        if (normalized.isNotBlank()) currentPlayerName = normalized
        hudPlayerTitle?.text = "AETHELGRAD  •  $currentPlayerName"
    }

    private fun buildHud(): View {
        val overlay = FrameLayout(this)
        val top = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(16), dp(8), dp(10), dp(0))
            background = GradientDrawable(
                GradientDrawable.Orientation.LEFT_RIGHT,
                intArrayOf(Color.argb(188, 8, 18, 23), Color.argb(132, 10, 22, 26), Color.argb(22, 8, 18, 23))
            ).apply {
                cornerRadius = dp(16).toFloat()
                setStroke(dp(1), Color.argb(130, 221, 186, 105))
            }
        }
        val title = TextView(this).apply {
            text = "AETHELGRAD"
            textSize = 15f
            letterSpacing = 0.10f
            setTextColor(Color.rgb(255, 226, 151))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            setShadowLayer(5f, 0f, 2f, Color.argb(180, 0, 0, 0))
        }
        hudPlayerTitle = title
        stateLabel = TextView(this).apply {
            text = "FOREST  •  DAY 1  •  DAY  •  CLEAR  •  HP 100  •  STA 100  •  LV 1"
            textSize = 12f
            letterSpacing = 0.035f
            setTextColor(Color.rgb(235, 244, 238))
            setShadowLayer(5f, 1f, 2f, Color.argb(210, 0, 0, 0))
            setSingleLine(true)
            ellipsize = android.text.TextUtils.TruncateAt.END
        }
        questLabel = TextView(this).apply {
            text = "THE FIRST EMBER  -  Aurora arrives - gather 3 caches for the camp"
            textSize = 12f
            letterSpacing = 0.025f
            setTextColor(Color.rgb(255, 226, 164))
            setShadowLayer(5f, 1f, 2f, Color.argb(210, 0, 0, 0))
            background = GradientDrawable().apply {
                cornerRadius = dp(10).toFloat()
                setColor(Color.argb(150, 8, 17, 21))
                setStroke(dp(1), Color.argb(120, 220, 182, 96))
            }
            setPadding(dp(12), dp(4), dp(18), dp(4))
            setSingleLine(true)
            ellipsize = android.text.TextUtils.TruncateAt.END
        }
        gyroButton = actionButton("GYRO: OFF") {
            audio.playEffect("ui")
            gyroEnabled = gyroSensor != null && !gyroEnabled
            gameView.queueEvent { NativeGameBridge.setGyroEnabled(gyroEnabled) }
            updateGyroButton()
        }
        setPlayerName(currentPlayerName)
        top.addView(title, LinearLayout.LayoutParams(-2, -1))
        top.addView(stateLabel, LinearLayout.LayoutParams(0, -1, 1f).apply { leftMargin = dp(16); rightMargin = dp(10) })
        overlay.addView(top, FrameLayout.LayoutParams(dp(438), dp(54), Gravity.TOP or Gravity.START).apply {
            topMargin = dp(10)
            leftMargin = dp(16)
        })
        overlay.addView(gyroButton, FrameLayout.LayoutParams(dp(118), dp(34), Gravity.TOP or Gravity.START).apply {
            topMargin = dp(136)
            leftMargin = dp(18)
        })
        overlay.addView(AimCrosshairView(this), FrameLayout.LayoutParams(dp(62), dp(62), Gravity.CENTER))
        overlay.addView(questLabel, FrameLayout.LayoutParams(dp(370), dp(48), Gravity.TOP or Gravity.START).apply {
            topMargin = dp(72)
            leftMargin = dp(16)
        })

        val profileBadge = ImageView(this).apply {
            setImageResource(R.drawable.aethelgard_profile_gold)
            scaleType = ImageView.ScaleType.CENTER_CROP
            contentDescription = "Open character and inventory"
            background = GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(Color.rgb(226, 184, 101))
                setStroke(dp(2), Color.rgb(255, 235, 156))
            }
            clipToOutline = true
            setOnClickListener { showCharacterInventoryPanel() }
        }
        overlay.addView(profileBadge, FrameLayout.LayoutParams(dp(48), dp(48), Gravity.TOP or Gravity.END).apply {
            topMargin = dp(88)
            rightMargin = dp(154)
        })
        vitalMeter = VitalMeterView(this)
        overlay.addView(vitalMeter, FrameLayout.LayoutParams(dp(272), dp(78), Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL).apply {
            bottomMargin = dp(18)
            rightMargin = dp(132)
        })

        var firstPerson = false
        var worldMapDialog: AlertDialog? = null
        val navigation = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        val viewModeButton = circularControlButton("◌", "VIEW") { }
        viewModeButton.setOnClickListener {
            firstPerson = !firstPerson
            viewModeButton.text = if (firstPerson) "◌\nFIRST" else "◌\nTHIRD"
            gameView.queueEvent { NativeGameBridge.toggleViewMode() }
        }
        val mapButton = circularControlButton("◉", "MAP") { }
        mapButton.setOnClickListener {
            val openDialog = worldMapDialog
            if (openDialog?.isShowing == true) {
                openDialog.dismiss()
            } else {
                mapButton.text = "◉\nCLOSE"
                gameView.queueEvent { NativeGameBridge.setWorldMapVisible(true) }
                    worldMapDialog = showWorldMapDialog {
                    mapButton.text = "◉\nMAP"
                    worldMapDialog = null
                    gameView.queueEvent { NativeGameBridge.setWorldMapVisible(false) }
                }
            }
        }
        val miniMap = CircularMiniMapView(this).apply {
            isClickable = true
            contentDescription = "Open world map"
            setOnClickListener { mapButton.performClick() }
        }
        overlay.addView(miniMap, FrameLayout.LayoutParams(dp(112), dp(112), Gravity.TOP or Gravity.END).apply {
            topMargin = dp(86)
            rightMargin = dp(18)
        })
        val towerButton = circularControlButton("♜", "TOWER") {
            audio.playEffect("ui")
            stateLabel.text = "TOWER LANDMARK READY  •  USE TELEPORT TO RETURN"
        }
        val teleportButton = circularControlButton("✦", "TELEPORT") {
            audio.playEffect("ui")
            gameView.queueEvent { NativeGameBridge.teleportToTower() }
        }
        val controlsButton = circularControlButton("☰", "MENU") { showControlSettings() }
        navigation.addView(mapButton, LinearLayout.LayoutParams(dp(62), dp(62)).apply { rightMargin = dp(4) })
        navigation.addView(towerButton, LinearLayout.LayoutParams(dp(62), dp(62)).apply { rightMargin = dp(4) })
        navigation.addView(teleportButton, LinearLayout.LayoutParams(dp(62), dp(62)).apply { rightMargin = dp(4) })
        navigation.addView(controlsButton, LinearLayout.LayoutParams(dp(62), dp(62)))
        overlay.addView(navigation, FrameLayout.LayoutParams(-2, dp(66), Gravity.TOP or Gravity.END).apply {
            topMargin = dp(12)
            rightMargin = dp(16)
        })
        overlay.addView(viewModeButton, FrameLayout.LayoutParams(dp(62), dp(62), Gravity.TOP or Gravity.START).apply {
            topMargin = dp(136)
            leftMargin = dp(144)
        })
        val orbitHint = TextView(this).apply {
            text = "DRAG LOOK PAD TO ORBIT 540°  •  FULL HORIZONTAL + VERTICAL LOOK  •  GYRO OPTIONAL"
            textSize = 10f
            letterSpacing = 0.08f
            setTextColor(Color.rgb(229, 211, 167))
            setShadowLayer(4f, 1f, 1f, Color.BLACK)
        }
        overlay.addView(orbitHint, FrameLayout.LayoutParams(-1, dp(24), Gravity.TOP).apply {
            topMargin = dp(142)
            leftMargin = dp(24)
        })
        coOpStatusLabel = TextView(this).apply {
            text = "CO-OP SYNC PENDING  •  WEATHER LOCAL"
            textSize = 10f
            setTextColor(Color.rgb(183, 218, 208))
            setShadowLayer(4f, 1f, 1f, Color.BLACK)
        }
        overlay.addView(coOpStatusLabel, FrameLayout.LayoutParams(-1, dp(24), Gravity.TOP).apply {
            topMargin = dp(168)
            leftMargin = dp(24)
            rightMargin = dp(214)
        })
        networkStatusLabel = TextView(this).apply {
            text = "NETWORK: CONNECTING"
            textSize = 10f
            setTextColor(Color.rgb(255, 205, 145))
            setShadowLayer(4f, 1f, 1f, Color.BLACK)
            setSingleLine(true)
            ellipsize = android.text.TextUtils.TruncateAt.END
        }
        overlay.addView(networkStatusLabel, FrameLayout.LayoutParams(-1, dp(22), Gravity.TOP).apply {
            topMargin = dp(190)
            leftMargin = dp(24)
            rightMargin = dp(214)
        })
        identityStatusLabel = TextView(this).apply {
            text = "PLAYER: PLAYER NAME  •  ID: PENDING"
            textSize = 10f
            setTextColor(Color.rgb(229, 211, 167))
            setShadowLayer(4f, 1f, 1f, Color.BLACK)
            setSingleLine(true)
            ellipsize = android.text.TextUtils.TruncateAt.END
        }
        overlay.addView(identityStatusLabel, FrameLayout.LayoutParams(-1, dp(22), Gravity.TOP).apply {
            topMargin = dp(211)
            leftMargin = dp(24)
            rightMargin = dp(214)
        })
        val coOpButton = actionButton("CO-OP ROOM") { showCoOpDialog() }
        overlay.addView(coOpButton, FrameLayout.LayoutParams(dp(138), dp(38), Gravity.TOP or Gravity.END).apply {
            topMargin = dp(204)
            rightMargin = dp(24)
        })
        val logoutButton = actionButton("LOG OUT") { confirmLogout() }
        overlay.addView(logoutButton, FrameLayout.LayoutParams(dp(108), dp(34), Gravity.TOP or Gravity.END).apply {
            topMargin = dp(142)
            rightMargin = dp(154)
        })

        val actions = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.END
            setPadding(dp(3), dp(3), dp(3), dp(3))
        }
        val sprintSlide = circularControlButton("♞", "SPRINT") { }
        sprintSlide.setOnTouchListener { _, event ->
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> gameView.queueEvent { NativeGameBridge.setSprintHeld(true) }
                MotionEvent.ACTION_UP -> gameView.queueEvent { NativeGameBridge.setSprintHeld(false) }
                MotionEvent.ACTION_CANCEL -> gameView.queueEvent { NativeGameBridge.setSprintHeld(false) }
            }
            true
        }
        val attack = circularControlButton("⚔", "ATTACK") { submitAuthoritativeCombat("attack") }
        val heavy = circularControlButton("✦", "HEAVY") { submitAuthoritativeCombat("heavy_attack") }
        val jump = circularControlButton("↟", "JUMP") { audio.playEffect("ui"); gameView.queueEvent { NativeGameBridge.jump() } }
        val dodge = circularControlButton("◆", "DODGE") { audio.playEffect("slide"); gameView.queueEvent { NativeGameBridge.dodge() } }
        val gather = circularControlButton("✧", "GATHER") { submitAuthoritativeInventory("gather") }
        val craft = circularControlButton("⌂", "CRAFT") { submitAuthoritativeInventory("craft") }
        val companion = circularControlButton("✦", "COMMAND") {
            audio.playEffect("ui")
            submitAuthoritativeCompanionCommand()
        }
        val capture = circularControlButton("◎", "TAME") {
            audio.playEffect("ui")
            submitAuthoritativeCapture()
        }
        capture.contentDescription = "TAME ANIMAL"
        val camp = circularControlButton("⌂", "CAMP") {
            audio.playEffect("craft")
            submitAuthoritativeCamp()
        }
        fun controlRow(vararg controls: View): LinearLayout = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.END
            controls.forEach { control ->
                addView(control, LinearLayout.LayoutParams(dp(72), dp(72)).apply {
                    leftMargin = dp(3)
                    rightMargin = dp(3)
                })
            }
        }
        fun rowParams(): LinearLayout.LayoutParams = LinearLayout.LayoutParams(-1, dp(78)).apply { bottomMargin = dp(3) }
        actions.addView(controlRow(sprintSlide, attack), rowParams())
        actions.addView(controlRow(jump, dodge, heavy), rowParams())
        actions.addView(controlRow(craft, gather, companion), rowParams())
        actions.addView(controlRow(capture, camp), LinearLayout.LayoutParams(-1, dp(78)))
        overlay.addView(actions, FrameLayout.LayoutParams(dp(252), -2, Gravity.BOTTOM or Gravity.END).apply {
            rightMargin = dp(8)
            bottomMargin = dp(12)
        })

        val quickSlots = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER
            setPadding(dp(4), dp(4), dp(4), dp(4))
            background = GradientDrawable().apply {
                cornerRadius = dp(10).toFloat()
                setColor(Color.argb(160, 5, 12, 16))
                setStroke(dp(1), Color.argb(190, 206, 174, 105))
            }
        }
        val selectedSlot = intArrayOf(0)
        val slotButtons = mutableListOf<Button>()
        val slotSymbols = listOf("⚔", "⛏", "⌁", "♨", "▣", "◈", "◇", "✦")
        slotSymbols.forEachIndexed { index, symbol ->
            val slot = Button(this).apply {
                text = "${index + 1}\n$symbol"
                textSize = 10f
                gravity = Gravity.CENTER
                isAllCaps = false
                minHeight = 0
                minimumHeight = 0
                minWidth = 0
                minimumWidth = 0
                includeFontPadding = false
                setPadding(0, dp(3), 0, 0)
                setTextColor(Color.rgb(246, 235, 204))
                contentDescription = "Quick slot ${index + 1}"
                setOnClickListener {
                    selectedSlot[0] = index
                    slotButtons.forEachIndexed { selectedIndex, button ->
                        button.background = quickSlotBackground(selectedIndex == index)
                    }
                    audio.playEffect("ui")
                }
                background = quickSlotBackground(index == 0)
            }
            slotButtons += slot
            quickSlots.addView(slot, LinearLayout.LayoutParams(dp(48), dp(54)).apply {
                leftMargin = dp(2)
                rightMargin = dp(2)
            })
        }
        overlay.addView(quickSlots, FrameLayout.LayoutParams(dp(420), dp(62), Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL).apply {
            bottomMargin = dp(14)
            rightMargin = dp(126)
        })
        return overlay
    }

    private fun applyHudSnapshot(snapshot: String) {
        val values = snapshot.split('|')
        if (values.size < 16 || !::stateLabel.isInitialized || !::questLabel.isInitialized) return
        fun number(index: Int): Int = values.getOrNull(index)?.toIntOrNull() ?: 0
        val level = number(0)
        val xp = number(1)
        val next = number(2).coerceAtLeast(1)
        val health = number(3).coerceIn(0, 100)
        val stamina = number(4).coerceIn(0, 100)
        val hunger = number(5).coerceIn(0, 100)
        val wood = number(6)
        val fiber = number(7)
        val stone = number(8)
        val warden = number(9).coerceIn(0, 100)
        val levelPulse = number(10) > 0
        val questPulse = number(11) > 0
        val biome = values.getOrNull(12)?.ifBlank { "UNKNOWN" } ?: "UNKNOWN"
        val phase = values.getOrNull(13)?.ifBlank { "DAY" } ?: "DAY"
        val daysPlayed = values.getOrNull(14)?.toIntOrNull()?.coerceAtLeast(1) ?: 1
        val objective = values.getOrNull(15)?.ifBlank { "Explore the wilds" } ?: "Explore the wilds"
        val xpLabel = if (level >= 100) "XP MAX" else "XP $xp/$next"
        val water = values.getOrNull(16)?.ifBlank { "DRY" } ?: "DRY"
        val locomotion = values.getOrNull(17)?.ifBlank { "IDLE" } ?: "IDLE"
        val weather = values.getOrNull(18)?.ifBlank { "CLEAR" } ?: "CLEAR"
        val viewMode = values.getOrNull(19)?.replace('_', ' ')?.ifBlank { "THIRD PERSON" } ?: "THIRD PERSON"
        val mapState = values.getOrNull(20)?.ifBlank { "MAP OFF" } ?: "MAP OFF"
        val towerState = values.getOrNull(21)?.replace('_', ' ')?.ifBlank { "TOWER READY" } ?: "TOWER READY"
        val companion = values.getOrNull(22)?.replace('_', ' ')?.ifBlank { "EMBERLING WILD" } ?: "EMBERLING WILD"
        val target = values.getOrNull(23)?.replace('_', ' ')?.ifBlank { "NO TARGET" } ?: "NO TARGET"
        val companionState = values.getOrNull(24)?.replace('_', ' ')?.ifBlank { companion } ?: companion
        val campState = values.getOrNull(25)?.replace('_', ' ')?.ifBlank { "NO CAMP" } ?: "NO CAMP"
        val discoveredSectors = number(26)
        requestDiscoveredSectorContent(discoveredSectors)
        if (::vitalMeter.isInitialized) vitalMeter.updateVitals(health, stamina, hunger)
        stateLabel.text = "$biome  •  DAY $daysPlayed  •  $phase  •  $weather  •  $viewMode  •  TARGET $target  •  HP $health  •  STA $stamina  •  LV $level"
        stateLabel.setTextColor(
            when {
                levelPulse -> Color.rgb(255, 236, 157)
                weather == "THUNDERSTORM" -> Color.rgb(190, 220, 255)
                weather == "RAIN" -> Color.rgb(170, 220, 236)
                else -> Color.WHITE
            }
        )
        val recoveryNotice = cloudRecoveryNotice
        val localContent = progressiveContentNotice
        questLabel.text = recoveryNotice ?: localContent ?: if (warden > 0 && objective.contains("Forest Warden")) "$objective  •  WARDEN HP $warden/100  •  $locomotion  •  $companionState" else if (target != "NO TARGET") "$objective  •  TARGET $target  •  $biome  •  $weather  •  W $wood  F $fiber  S $stone  •  $companionState  •  $campState" else "$objective  •  $biome  •  $weather  •  $water  •  W $wood  F $fiber  S $stone  •  $xpLabel  •  $companionState  •  $campState"
        questLabel.setTextColor(if (recoveryNotice != null) Color.rgb(255, 180, 150) else if (questPulse) Color.rgb(255, 236, 157) else Color.rgb(255, 226, 164))
    }

    private fun showWorldMapDialog(onClosed: () -> Unit): AlertDialog {
        val content = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(8), dp(16), dp(4))
        }
        content.addView(TextView(this).apply {
            text = "AETHELGRAD WORLD  •  TAP CLOSE TO RETURN"
            textSize = 12f
            setTextColor(Color.rgb(244, 218, 155))
            setPadding(0, 0, 0, dp(8))
        })
        content.addView(AethelgardWorldMapView(this), LinearLayout.LayoutParams(-1, dp(300)))
        content.addView(TextView(this).apply {
            text = "GOLD: TOWER / LANDMARK     CYAN: RIVER     WHITE: YOU     REGIONS: FOREST  •  SAND  •  SNOW"
            textSize = 10f
            setTextColor(Color.rgb(186, 211, 204))
            setPadding(0, dp(8), 0, 0)
        })
        val dialog = AlertDialog.Builder(this)
            .setTitle("WORLD MAP")
            .setView(content)
            .setNegativeButton("‹ BACK", null)
            .create()
        dialog.setOnDismissListener { onClosed() }
        dialog.show()
        return dialog
    }

    private fun showCharacterInventoryPanel() {
        if (!::gameView.isInitialized) return
        gameView.queueEvent {
            val values = NativeGameBridge.getHudState().split('|')
            runOnUiThread {
                fun number(index: Int): Int = values.getOrNull(index)?.toIntOrNull() ?: 0
                val companion = values.getOrNull(22)?.replace('_', ' ') ?: "EMBERLING WILD"
                val account = accountSession.snapshot.accountId?.take(10)?.let { "Account $it…" } ?: "Online account"
                val world = activeCloudWorld?.let { "${it.name}  •  Revision ${it.saveRevision}" } ?: "No cloud world active"
                val panel = LinearLayout(this).apply {
                    orientation = LinearLayout.VERTICAL
                    setPadding(dp(22), dp(14), dp(22), dp(8))
                }
                panel.addView(ImageView(this).apply {
                    setImageResource(R.drawable.aethelgard_heroine_character)
                    scaleType = ImageView.ScaleType.FIT_CENTER
                    contentDescription = "Silver-haired AETHELGRAD heroine"
                }, LinearLayout.LayoutParams(-1, dp(180)))
                panel.addView(TextView(this).apply {
                    text = "${characterCreation.name.ifBlank { "WAYFARER" }}  •  $account"
                    textSize = 17f
                    setTextColor(Color.rgb(244, 218, 155))
                })
                panel.addView(TextView(this).apply {
                    text = "WORLD\n$world"
                    textSize = 12f
                    setTextColor(Color.rgb(194, 218, 214))
                    setPadding(0, dp(12), 0, dp(6))
                })
                panel.addView(TextView(this).apply {
                    text = "VITALS  •  HP ${number(3)}/100  •  STAMINA ${number(4)}  •  HUNGER ${number(5)}"
                    textSize = 13f
                    setTextColor(Color.WHITE)
                })
                panel.addView(TextView(this).apply {
                    text = "PACK  •  WOOD ${number(6)}  •  FIBER ${number(7)}  •  STONE ${number(8)}"
                    textSize = 13f
                    setTextColor(Color.WHITE)
                    setPadding(0, dp(8), 0, 0)
                })
                panel.addView(TextView(this).apply {
                    text = "PROGRESSION  •  LEVEL ${number(0)}  •  XP ${number(1)}/${number(2)}"
                    textSize = 13f
                    setTextColor(Color.WHITE)
                    setPadding(0, dp(8), 0, 0)
                })
                panel.addView(TextView(this).apply {
                    text = "COMPANION  •  $companion"
                    textSize = 13f
                    setTextColor(Color.rgb(164, 231, 190))
                    setPadding(0, dp(8), 0, 0)
                })
                val accountDialog = AlertDialog.Builder(this)
                    .setTitle("CHARACTER / INVENTORY")
                    .setView(panel)
                    .setNegativeButton("‹ BACK", null)
                    .setPositiveButton("LOG OUT") { _, _ -> confirmLogout() }
                accountDialog.show()
            }
        }
    }

    private fun confirmLogout() {
        AlertDialog.Builder(this)
            .setTitle("LOG OUT OF AETHELGRAD?")
            .setMessage(if (accountSession.snapshot.isGuest) {
                "This ends guest mode. Your local guest world remains on this device; hosted multiplayer and cloud worlds require Google login."
            } else {
                "This removes the local session from this device. Your cloud world remains protected in your account."
            })
            .setNegativeButton("CANCEL", null)
            .setPositiveButton("LOG OUT") { _, _ ->
                if (accountSession.snapshot.isGuest && worldStateReadyForWorld) saveGuestWorldState()
                if (networkOnline) activeCoOpRoom?.let { accountSession.leaveCoOpRoom(it.code) }
                if (localSessionActive) localMultiplayer.leaveSession()
                activeCoOpRoom = null
                localSessionActive = false
                localRoom = null
                activeCloudWorld = null
                currentPlayerProfile = null
                authenticationTransitionStarted = false
                cloudRecoveryNotice = null
                hudHandler.removeCallbacks(cloudSaveUpdater)
                hudHandler.removeCallbacks(coOpUpdater)
                gameView.queueEvent {
                    NativeGameBridge.clearCoOpPeers()
                    NativeGameBridge.setAuthoritativeOnline(false)
                    NativeGameBridge.setMove(0f, 0f)
                }
                accountSession.signOut()
                onboardingOverlay.visibility = View.VISIBLE
            }
            .show()
    }

    private fun showAssetPatchOverlay(onReady: () -> Unit = {}) {
        if (assetPatchOverlay != null) return
        val resourceTier = selectedResourceTier
        val overlay = FrameLayout(this).apply {
            background = GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(Color.rgb(4, 18, 26), Color.rgb(3, 7, 13))
            )
        }
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
            setPadding(dp(34), dp(26), dp(34), dp(24))
            background = GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(Color.rgb(13, 28, 36), Color.rgb(6, 15, 22))
            ).apply {
                cornerRadius = dp(18).toFloat()
                setStroke(dp(1), Color.rgb(150, 116, 62))
            }
        }
        val ambientEmber = TextView(this).apply {
            text = "✦"
            textSize = 48f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(245, 199, 104))
            setShadowLayer(26f, 0f, 0f, Color.rgb(218, 130, 54))
            alpha = 0.92f
        }
        val title = TextView(this).apply {
            text = if (resourceTier == ContentDownloadPlan.ResourceTier.STAGE_1) {
                "PREPARE STAGE 1 FOREST CONTENT"
            } else {
                "PREPARE ${resourceTier.name} GRAPHICS"
            }
            textSize = 18f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(244, 218, 155))
        }
        val status = TextView(this).apply {
            text = "CHECKING STAGE 1 CONTENT AVAILABILITY"
            textSize = 14f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(205, 223, 220))
            setPadding(0, dp(12), 0, dp(10))
        }
        val progress = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            max = 100
            progress = 0
            progressTintList = android.content.res.ColorStateList.valueOf(Color.rgb(226, 184, 101))
            progressBackgroundTintList = android.content.res.ColorStateList.valueOf(Color.rgb(37, 56, 61))
        }
        val details = TextView(this).apply {
            text = if (resourceTier == ContentDownloadPlan.ResourceTier.HIGH) {
                "HIGH-END CONTENT is not published for this installation. GAME LOCKED until a signed Unreal cook is available."
            } else {
                resourceTier.description
            }
            textSize = 12f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(161, 190, 187))
            setPadding(0, dp(12), 0, 0)
        }
        val note = TextView(this).apply {
            text = "Stage 1 is planned at 1 GiB, but the app displays only bytes actually mounted from authored content. Future Unreal content is added in later verified packs; no padding is used."
            textSize = 11f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(146, 168, 171))
            setPadding(0, dp(18), 0, 0)
        }
        val retry = actionButton("RETRY ASSET PREPARATION") { }
        retry.visibility = View.GONE
        val enterCoreWorld = actionButton("ENTER CORE ONLINE WORLD") { }
        enterCoreWorld.visibility = View.GONE
        panel.addView(ambientEmber, LinearLayout.LayoutParams(-1, dp(58)))
        panel.addView(title, LinearLayout.LayoutParams(-1, dp(34)))
        panel.addView(status, LinearLayout.LayoutParams(-1, dp(46)))
        panel.addView(progress, LinearLayout.LayoutParams(dp(430), dp(28)))
        panel.addView(details, LinearLayout.LayoutParams(-1, dp(48)))
        panel.addView(note, LinearLayout.LayoutParams(-1, dp(60)))
        panel.addView(retry, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(12) })
        panel.addView(enterCoreWorld, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(8) })
        overlay.addView(panel, FrameLayout.LayoutParams(dp(520), -2, Gravity.CENTER))
        rootContainer.addView(overlay)
        assetPatchOverlay = overlay

        val loadingAnimationHandler = Handler(Looper.getMainLooper())
        val loadingAnimation = object : Runnable {
            override fun run() {
                if (assetPatchOverlay !== overlay || overlay.parent == null) return
                ambientEmber.animate().rotationBy(180f).scaleX(1.12f).scaleY(1.12f).alpha(0.58f)
                    .setDuration(420L).withEndAction {
                        ambientEmber.animate().scaleX(1.0f).scaleY(1.0f).alpha(0.92f)
                            .setDuration(420L).start()
                    }.start()
                loadingAnimationHandler.postDelayed(this, 900L)
            }
        }
        loadingAnimationHandler.post(loadingAnimation)

        var displayedProgress = 0
        var progressAnimator: ValueAnimator? = null
        fun animateVerifiedProgress(targetPercent: Int) {
            val target = targetPercent.coerceIn(0, 100)
            if (target == displayedProgress) return
            progressAnimator?.cancel()
            progressAnimator = ValueAnimator.ofInt(displayedProgress, target).apply {
                duration = if (kotlin.math.abs(target - displayedProgress) <= 1) 110L else 260L
                interpolator = AccelerateDecelerateInterpolator()
                addUpdateListener { animator -> progress.progress = animator.animatedValue as Int }
                start()
            }
            displayedProgress = target
        }

        fun formatVerifiedBytes(bytes: Long): String = when {
            bytes >= 1024L * 1024L * 1024L -> "%.2f GB".format(bytes.toDouble() / (1024.0 * 1024.0 * 1024.0))
            bytes >= 1024L * 1024L -> "%.1f MB".format(bytes.toDouble() / (1024.0 * 1024.0))
            bytes >= 1024L -> "%.1f KB".format(bytes.toDouble() / 1024.0)
            else -> "$bytes bytes"
        }

        fun finishPreparation(highResolution: Boolean) {
            loadingAnimationHandler.removeCallbacks(loadingAnimation)
            progressAnimator?.cancel()
            hudHandler.postDelayed({
                rootContainer.removeView(overlay)
                assetPatchOverlay = null
                if (highResolution) markProductionContentReady() else markCoreOnlineContentReady()
                onReady()
                continuePendingWorldEntry()
            }, 450L)
        }

        lateinit var startPreparation: () -> Unit
        startPreparation = {
            retry.visibility = View.GONE
            enterCoreWorld.visibility = View.GONE
            progressAnimator?.cancel()
            displayedProgress = 0
            progress.progress = 0
            var failureShown = false
            assetPacks.requestProductionContent(resourceTier) { event ->
                runOnUiThread {
                if (event.failed) {
                    if (!failureShown) {
                        failureShown = true
                        val failure = event.failedPack ?: "Verified high-graphics content could not start for this installation."
                        val contentNotPublished = event.errorCode == -30
                        val serviceUnavailable = contentNotPublished || failure.contains("timed out", ignoreCase = true) ||
                            failure.contains("HTTP 5", ignoreCase = true) ||
                            failure.contains("private_content_not_configured", ignoreCase = true)
                        status.text = when {
                            contentNotPublished -> "STAGE 1 FOREST CONTENT NOT AVAILABLE"
                            serviceUnavailable -> "STAGE 1 CONTENT SERVICE UNAVAILABLE"
                            else -> "GRAPHICS DOWNLOAD FAILED  •  GAME LOCKED"
                        }
                        details.text = failure
                        note.text = if (serviceUnavailable) {
                            "No measured Stage 1 payload is available for this APK. The bundled forest launch slice remains the phone-first path; retry after the staged content release is available."
                        } else {
                            "The downloaded archive did not pass a release integrity check. Retry after the content service is corrected."
                        }
                        note.setTextColor(Color.rgb(255, 188, 142))
                        animateVerifiedProgress(0)
                        retry.visibility = View.VISIBLE
                        if (serviceUnavailable) enterCoreWorld.visibility = View.VISIBLE
                    }
                } else {
                    when {
                        event.complete -> {
                            val envelope = ContentDownloadPlan.qualityEnvelopeFor(resourceTier)
                            if (event.source == "bundled-launch-slice") {
                                status.text = "FOREST LAUNCH SLICE READY"
                                details.text = "${formatVerifiedBytes(event.totalBytes)} of authored mobile content mounted: forest terrain, water, foliage, Aurora motion data, GLES materials, and audio. Starting the game…"
                            } else {
                                status.text = "${envelope.id.uppercase()} CONTENT READY"
                                details.text = "${formatVerifiedBytes(event.totalBytes)} verified and mounted: ${envelope.textureLabel}, ${envelope.foliageDensity}% foliage, ${envelope.waterQuality}. Starting the game…"
                            }
                        }
                                                event.status == AssetPackStatus.WAITING_FOR_WIFI || event.status == AssetPackStatus.REQUIRES_USER_CONFIRMATION -> {
                            status.text = if (event.status == AssetPackStatus.WAITING_FOR_WIFI) {
                                "WAITING FOR WI-FI  •  GAME LOCKED  •  DOWNLOAD READY TO RESUME"
                            } else {
                                "CONFIRM LARGE DOWNLOAD  •  GAME LOCKED  •  DOWNLOAD READY TO RESUME"
                            }
                            details.text = "Stage 1 forest content is required before sign-in and gameplay. Connect to Wi-Fi or approve the staged download, then retry."
                            retry.visibility = View.VISIBLE

                        }
                        event.status == AssetPackStatus.CANCELED -> {
                            status.text = "DOWNLOAD CANCELED  •  GAME LOCKED"
                            details.text = "Stage 1 forest content is required before sign-in and gameplay. Press retry to resume."
                            retry.visibility = View.VISIBLE
                        }
                        else -> {
                            status.text = if (event.sizeVerified) {
                                "DOWNLOADING STAGE 1 FOREST CONTENT  •  ${event.percent}%"
                            } else {
                                "WAITING FOR VERIFIED CONTENT METADATA"
                            }
                            details.text = if (event.sizeVerified && event.totalBytes > 0L) {
                                "${formatVerifiedBytes(event.bytesDownloaded)} / ${formatVerifiedBytes(event.totalBytes)} downloaded  •  signed cooked graphics payload"
                            } else {
                                "Play or the signed archive has not yet reported an exact byte total. The bar stays at 0% until it does."
                            }
                        }
                    }
                    animateVerifiedProgress(if (event.sizeVerified || event.complete) event.percent else 0)
                    if (event.complete) finishPreparation(highResolution = resourceTier == ContentDownloadPlan.ResourceTier.HIGH)
                }
            }
            }
        }
        retry.setOnClickListener {
            retry.visibility = View.GONE
            startPreparation()
        }
        enterCoreWorld.setOnClickListener {
            enterCoreWorld.visibility = View.GONE
            finishPreparation(highResolution = false)
        }
        startPreparation()
    }

    private fun showGraphicsSettings() {
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(24), dp(12), dp(24), dp(8))
        }
        val detectedMax = supportedTargetFps.maxOrNull() ?: 60
        val detected = TextView(this).apply {
            text = "Detected display capability: up to ${detectedMax} FPS\nUnsupported higher modes are hidden automatically."
            textSize = 13f
            setTextColor(Color.rgb(210, 224, 220))
            setPadding(0, 0, 0, dp(10))
        }
        panel.addView(detected)
        val autoFps = CheckBox(this).apply {
            text = "AUTO: use the highest supported FPS ($detectedMax)"
            textSize = 14f
            setTextColor(Color.rgb(244, 218, 155))
            isChecked = graphicsPreferences.getBoolean("auto_fps", true)
        }
        panel.addView(autoFps)
        val fpsGroup = RadioGroup(this).apply {
            orientation = RadioGroup.HORIZONTAL
            setPadding(dp(8), 0, 0, dp(8))
        }
        val fpsButtons = supportedTargetFps.map { fps ->
            RadioButton(this).apply {
                text = "$fps FPS"
                textSize = 13f
                setTextColor(Color.WHITE)
                tag = fps
                isChecked = fps == selectedTargetFps
            }
        }
        fpsButtons.forEach { fpsGroup.addView(it, LinearLayout.LayoutParams(0, dp(44), 1f)) }
        fpsGroup.isEnabled = !autoFps.isChecked
        panel.addView(TextView(this).apply {
            text = "Manual FPS override"
            textSize = 13f
            setTextColor(Color.rgb(165, 214, 223))
        })
        panel.addView(fpsGroup)
        autoFps.setOnCheckedChangeListener { _, checked ->
            fpsGroup.isEnabled = !checked
            if (checked) fpsGroup.clearCheck()
        }

        panel.addView(TextView(this).apply {
            text = "GRAPHICS QUALITY"
            textSize = 13f
            setTextColor(Color.rgb(165, 214, 223))
            setPadding(0, dp(8), 0, dp(4))
        })
        val qualityGroup = RadioGroup(this).apply { orientation = RadioGroup.VERTICAL }
        val qualityNames = listOf("LOW", "MEDIUM", "HIGH", "ULTRA", "MAX")
        qualityNames.forEachIndexed { index, name ->
            qualityGroup.addView(RadioButton(this).apply {
                text = name
                textSize = 13f
                setTextColor(Color.WHITE)
                isChecked = index == selectedGraphicsTier
                tag = index
            }, LinearLayout.LayoutParams(-1, dp(34)))
        }
        panel.addView(qualityGroup)
        panel.addView(TextView(this).apply {
            text = "Quality tiers scale effect density, weather detail, lighting accents, and future 3D asset/LOD budgets."
            textSize = 12f
            setTextColor(Color.rgb(171, 190, 187))
            setPadding(0, dp(8), 0, 0)
        })
        AlertDialog.Builder(this)
            .setTitle("GRAPHICS / FPS SETTINGS")
            .setView(panel)
            .setNegativeButton("‹ BACK", null)
            .setPositiveButton("APPLY") { _, _ ->
                val auto = autoFps.isChecked
                graphicsPreferences.edit().putBoolean("auto_fps", auto).apply()
                val chosenFps = if (auto) detectedMax else {
                    fpsGroup.checkedRadioButtonId.let { id ->
                        if (id == -1) selectedTargetFps else fpsGroup.findViewById<RadioButton>(id).tag as Int
                    }
                }
                val chosenTier = qualityGroup.checkedRadioButtonId.let { id ->
                    if (id == -1) selectedGraphicsTier else qualityGroup.findViewById<RadioButton>(id).tag as Int
                }
                applyTargetFps(chosenFps)
                applyGraphicsTier(chosenTier)
            }
            .show()
    }

    private fun showControlSettings() {
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(22), dp(10), dp(22), dp(8))
        }
        val summary = TextView(this).apply {
            text = "Tune how quickly the camera and movement respond. Changes apply immediately and are saved on this device."
            textSize = 12f
            setTextColor(Color.rgb(171, 190, 187))
            setPadding(0, 0, 0, dp(8))
        }
        panel.addView(summary)

        fun valueLabel(name: String, value: Float): TextView = TextView(this).apply {
            text = "$name  ${(value * 100f).roundToInt()}%"
            textSize = 13f
            setTextColor(Color.rgb(244, 218, 155))
        }

        fun sensitivitySlider(
            name: String,
            initial: Float,
            minimum: Float,
            maximum: Float,
            onChanged: (Float) -> Unit
        ) {
            val label = valueLabel(name, initial)
            panel.addView(label)
            panel.addView(SeekBar(this).apply {
                max = 100
                progress = (((initial - minimum) / (maximum - minimum)) * 100f).roundToInt().coerceIn(0, 100)
                setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                    override fun onProgressChanged(bar: SeekBar?, value: Int, fromUser: Boolean) {
                        val selected = minimum + (maximum - minimum) * (value / 100f)
                        label.text = "$name  ${(selected * 100f).roundToInt()}%"
                        onChanged(selected)
                    }
                    override fun onStartTrackingTouch(bar: SeekBar?) = Unit
                    override fun onStopTrackingTouch(bar: SeekBar?) = Unit
                })
            })
        }

        sensitivitySlider("GYRO SENSITIVITY", gyroSensitivity, 0.25f, 2.5f) { value ->
            gyroSensitivity = value
            controlPreferences.edit().putFloat("gyro_sensitivity", value).apply()
        }
        sensitivitySlider("LOOK / LENS SENSITIVITY", lookSensitivity, 0.25f, 2.5f) { value ->
            lookSensitivity = value
            controlPreferences.edit().putFloat("look_sensitivity", value).apply()
        }
        sensitivitySlider("JOYSTICK RESPONSE", joystickSensitivity, 0.50f, 1.50f) { value ->
            joystickSensitivity = value
            joystickView.setSensitivity(value)
            controlPreferences.edit().putFloat("joystick_sensitivity", value).apply()
        }
        panel.addView(TextView(this).apply {
            text = "LOWER values give slower, precise aiming. HIGHER values turn faster. Joystick response changes the thumb-to-movement curve, not the maximum speed."
            textSize = 11f
            setTextColor(Color.rgb(171, 190, 187))
            setPadding(0, dp(8), 0, dp(6))
        })

        lateinit var dialog: AlertDialog
        panel.addView(actionButton("RESET CONTROL DEFAULTS") {
            gyroSensitivity = 1.0f
            lookSensitivity = 1.0f
            joystickSensitivity = 1.0f
            joystickView.setSensitivity(joystickSensitivity)
            controlPreferences.edit()
                .putFloat("gyro_sensitivity", gyroSensitivity)
                .putFloat("look_sensitivity", lookSensitivity)
                .putFloat("joystick_sensitivity", joystickSensitivity)
                .apply()
            dialog.dismiss()
            showControlSettings()
        }, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(4) })
        panel.addView(actionButton("AUDIO SETTINGS") {
            dialog.dismiss()
            showAudioSettings()
        }, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(6) })

        dialog = AlertDialog.Builder(this)
            .setTitle("AETHELGRAD CONTROLS")
            .setView(panel)
            .setNegativeButton("‹ BACK", null)
            .create()
        dialog.show()
    }

    private fun showAudioSettings() {
        val current = audio.getSettings()
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 8, 32, 8)
        }
        fun slider(label: String, initial: Float, apply: (Float) -> Unit) {
            panel.addView(TextView(this).apply { text = label })
            panel.addView(SeekBar(this).apply {
                max = 100
                progress = (initial * 100f).toInt()
                setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                    override fun onProgressChanged(bar: SeekBar?, value: Int, fromUser: Boolean) = apply(value / 100f)
                    override fun onStartTrackingTouch(bar: SeekBar?) = Unit
                    override fun onStopTrackingTouch(bar: SeekBar?) = Unit
                })
            })
        }
        slider("Master", current.master, audio::setMaster)
        slider("Music", current.music, audio::setMusic)
        slider("Effects", current.effects, audio::setEffects)
        slider("Ambience", current.ambience, audio::setAmbience)
        slider("Voice", current.voice, audio::setVoice)
        AlertDialog.Builder(this)
            .setTitle("AETHELGRAD AUDIO")
            .setView(panel)
            .setNegativeButton("‹ BACK", null)
            .show()
    }

    private fun quickSlotBackground(selected: Boolean): GradientDrawable = GradientDrawable(
        GradientDrawable.Orientation.TOP_BOTTOM,
        if (selected) intArrayOf(Color.argb(225, 194, 143, 76), Color.argb(205, 108, 66, 43))
        else intArrayOf(Color.argb(165, 16, 27, 31), Color.argb(150, 7, 13, 17))
    ).apply {
        cornerRadius = dp(6).toFloat()
        setStroke(dp(if (selected) 2 else 1), if (selected) Color.rgb(255, 237, 170) else Color.argb(180, 187, 164, 119))
    }

    private fun circularControlButton(symbol: String, label: String, onClick: () -> Unit): Button = Button(this).apply {
        text = "$symbol\n$label"
        textSize = 10f
        gravity = Gravity.CENTER
        isAllCaps = false
        minHeight = 0
        minimumHeight = 0
        minWidth = 0
        minimumWidth = 0
        includeFontPadding = false
        setPadding(dp(2), dp(5), dp(2), dp(2))
        setTextColor(Color.rgb(249, 239, 211))
        typeface = android.graphics.Typeface.DEFAULT_BOLD
        contentDescription = label
        setOnClickListener { onClick() }
        background = android.graphics.drawable.StateListDrawable().apply {
            addState(intArrayOf(android.R.attr.state_pressed), GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(Color.argb(225, 194, 143, 76))
                setStroke(dp(2), Color.rgb(255, 239, 177))
            })
            addState(intArrayOf(), GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(Color.argb(150, 7, 14, 18))
                setStroke(dp(1), Color.argb(220, 226, 193, 121))
            })
        }
        elevation = dp(3).toFloat()
    }

    private fun gameplayButton(label: String, onClick: () -> Unit): Button = Button(this).apply {
        text = label
        textSize = 11f
        isAllCaps = false
        minHeight = 0
        minimumHeight = 0
        minWidth = 0
        minimumWidth = 0
        letterSpacing = 0.04f
        setPadding(dp(8), 0, dp(8), 0)
        setTextColor(Color.rgb(248, 239, 213))
        typeface = android.graphics.Typeface.DEFAULT_BOLD
        elevation = dp(3).toFloat()
        background = android.graphics.drawable.StateListDrawable().apply {
            addState(intArrayOf(android.R.attr.state_pressed), GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(Color.rgb(190, 104, 58), Color.rgb(126, 63, 54))
            ).apply {
                cornerRadius = dp(10).toFloat()
                setStroke(dp(2), Color.rgb(255, 231, 154))
            })
            addState(intArrayOf(), GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                intArrayOf(Color.argb(245, 26, 59, 66), Color.argb(235, 12, 27, 35))
            ).apply {
                cornerRadius = dp(10).toFloat()
                setStroke(dp(1), Color.rgb(108, 192, 182))
            })
        }
        setOnClickListener { onClick() }
    }

    private fun actionButton(label: String, onClick: () -> Unit): Button = Button(this).apply {
        text = label
        textSize = 12f
        isAllCaps = false
        minHeight = 0
        minimumHeight = 0
        minWidth = 0
        minimumWidth = 0
        setPadding(dp(8), 0, dp(8), 0)
        setTextColor(Color.rgb(14, 26, 27))
        typeface = android.graphics.Typeface.DEFAULT_BOLD
        setOnClickListener { onClick() }
        background = GradientDrawable(
            GradientDrawable.Orientation.LEFT_RIGHT,
            intArrayOf(Color.rgb(231, 187, 103), Color.rgb(169, 218, 197))
        ).apply {
            cornerRadius = dp(18).toFloat()
            setStroke(dp(1), Color.rgb(255, 236, 174))
        }
        elevation = dp(2).toFloat()
    }

    private fun roundControlButton(label: String, onClick: () -> Unit): Button = Button(this).apply {
        text = label
        textSize = 11f
        isAllCaps = false
        minHeight = 0
        minimumHeight = 0
        minWidth = 0
        minimumWidth = 0
        setPadding(dp(6), 0, dp(6), 0)
        setTextColor(Color.rgb(255, 239, 193))
        setOnClickListener { onClick() }
        background = GradientDrawable(
            GradientDrawable.Orientation.TOP_BOTTOM,
            intArrayOf(Color.argb(245, 27, 59, 67), Color.argb(235, 10, 23, 31))
        ).apply {
            cornerRadius = dp(10).toFloat()
            setStroke(dp(1), Color.rgb(111, 193, 182))
        }
        elevation = dp(2).toFloat()
    }

    private fun cinematicButton(label: String, primary: Boolean, onClick: () -> Unit): Button = Button(this).apply {
        text = label
        textSize = 12f
        isAllCaps = false
        minHeight = 0
        minimumHeight = 0
        minWidth = 0
        minimumWidth = 0
        letterSpacing = 0.05f
        setPadding(dp(12), 0, dp(12), 0)
        setTextColor(if (primary) Color.rgb(27, 22, 15) else Color.rgb(232, 224, 205))
        setOnClickListener { onClick() }
        background = GradientDrawable(
            GradientDrawable.Orientation.LEFT_RIGHT,
            if (primary) intArrayOf(Color.rgb(214, 170, 88), Color.rgb(247, 209, 132)) else intArrayOf(Color.argb(230, 29, 34, 41), Color.argb(230, 12, 18, 25))
        ).apply {
            cornerRadius = dp(10).toFloat()
            setStroke(dp(1), if (primary) Color.rgb(255, 226, 161) else Color.rgb(151, 114, 62))
        }
    }
}

/** Original procedural dark-fantasy landscape: no external background art is copied into the build. */
private class CinematicLoginBackdropView(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val ridge = Path()

    override fun onDraw(canvas: android.graphics.Canvas) {
        val w = width.toFloat().coerceAtLeast(1f)
        val h = height.toFloat().coerceAtLeast(1f)
        paint.shader = LinearGradient(0f, 0f, 0f, h, intArrayOf(Color.rgb(5, 14, 22), Color.rgb(13, 23, 30), Color.rgb(3, 7, 11)), null, Shader.TileMode.CLAMP)
        canvas.drawRect(0f, 0f, w, h, paint)

        paint.shader = RadialGradient(w * 0.14f, h * 0.26f, w * 0.53f, intArrayOf(Color.argb(205, 218, 151, 72), Color.argb(55, 126, 107, 75), Color.TRANSPARENT), floatArrayOf(0f, 0.46f, 1f), Shader.TileMode.CLAMP)
        canvas.drawCircle(w * 0.14f, h * 0.26f, w * 0.53f, paint)
        paint.shader = null

        drawRidge(canvas, w, h, 0.56f, Color.rgb(23, 41, 49), 0.06f)
        drawRidge(canvas, w, h, 0.67f, Color.rgb(14, 28, 36), 0.10f)
        drawRidge(canvas, w, h, 0.78f, Color.rgb(7, 16, 23), 0.15f)

        paint.color = Color.argb(80, 211, 185, 130)
        for (i in 0..18) {
            val x = ((i * 97) % w.toInt()).toFloat()
            val y = h * (0.08f + ((i * 37) % 32) / 100f)
            canvas.drawCircle(x, y, if (i % 3 == 0) 2.5f else 1.2f, paint)
        }
        paint.shader = LinearGradient(0f, h * 0.65f, 0f, h, Color.argb(0, 0, 0, 0), Color.argb(220, 0, 0, 0), Shader.TileMode.CLAMP)
        canvas.drawRect(0f, h * 0.62f, w, h, paint)
        paint.shader = null
    }

    private fun drawRidge(canvas: android.graphics.Canvas, w: Float, h: Float, base: Float, color: Int, amplitude: Float) {
        ridge.reset()
        ridge.moveTo(0f, h)
        ridge.lineTo(0f, h * base)
        for (i in 0..8) {
            val x = w * i / 8f
            val peak = h * (base - amplitude * if (i % 2 == 0) 1.25f else 0.45f)
            ridge.lineTo(x + w / 16f, peak)
            ridge.lineTo(x + w / 8f, h * base)
        }
        ridge.lineTo(w, h)
        ridge.close()
        paint.color = color
        canvas.drawPath(ridge, paint)
    }
}

private class GameSurfaceView(context: Context, onRendererReady: () -> Unit) : GLSurfaceView(context) {
    private val renderer = GameRenderer { post { onRendererReady() } }
    private var targetFps = 60
    private var surfaceReady = false
    private var activeLookPointerId = MotionEvent.INVALID_POINTER_ID
    private var lastLookX = 0f
    private var lastLookY = 0f

    private val surfaceCallback = object : SurfaceHolder.Callback {
        override fun surfaceCreated(holder: SurfaceHolder) {
            surfaceReady = holder.surface.isValid
            applyFrameRateIfSurfaceReady()
        }

        override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
            surfaceReady = holder.surface.isValid
            applyFrameRateIfSurfaceReady()
        }

        override fun surfaceDestroyed(holder: SurfaceHolder) {
            surfaceReady = false
            activeLookPointerId = MotionEvent.INVALID_POINTER_ID
        }
    }

    init {
        setEGLContextClientVersion(3)
        setRenderer(renderer)
        setPreserveEGLContextOnPause(true)
        renderMode = RENDERMODE_CONTINUOUSLY
        isFocusable = true
        holder.addCallback(surfaceCallback)
    }

    fun applyTargetFps(value: Int) {
        targetFps = value.coerceIn(60, 120)
        renderer.targetFps = targetFps
        applyFrameRateIfSurfaceReady()
    }

    private fun applyFrameRateIfSurfaceReady() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R || !surfaceReady) return
        val surface = holder.surface
        if (!surface.isValid) {
            surfaceReady = false
            return
        }
        try {
            surface.setFrameRate(
                targetFps.toFloat(),
                Surface.FRAME_RATE_COMPATIBILITY_FIXED_SOURCE,
                Surface.CHANGE_FRAME_RATE_ALWAYS
            )
        } catch (_: IllegalStateException) {
            // The Surface can be released between isValid and setFrameRate during a
            // pause/recreate race. It will be retried from surfaceCreated/Changed.
            surfaceReady = false
        }
    }

    fun applyGraphicsTier(level: Int) {
        renderer.graphicsTier = level.coerceIn(0, 4)
        queueEvent { NativeGameBridge.setGraphicsQuality(renderer.graphicsTier) }
    }

    override fun onPause() {
        activeLookPointerId = MotionEvent.INVALID_POINTER_ID
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
        renderer.resetFrameClock()
        applyFrameRateIfSurfaceReady()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                if (event.x >= width * 0.42f) {
                    activeLookPointerId = event.getPointerId(event.actionIndex)
                    lastLookX = event.x
                    lastLookY = event.y
                }
            }
            MotionEvent.ACTION_POINTER_DOWN -> {
                val index = event.actionIndex
                if (activeLookPointerId == MotionEvent.INVALID_POINTER_ID && event.getX(index) >= width * 0.42f) {
                    activeLookPointerId = event.getPointerId(index)
                    lastLookX = event.getX(index)
                    lastLookY = event.getY(index)
                }
            }
            MotionEvent.ACTION_MOVE -> {
                if (activeLookPointerId != MotionEvent.INVALID_POINTER_ID) {
                    val index = event.findPointerIndex(activeLookPointerId)
                    if (index >= 0) {
                        val dx = (event.getX(index) - lastLookX).coerceIn(-96f, 96f)
                        val dy = (event.getY(index) - lastLookY).coerceIn(-96f, 96f)
                        if (kotlin.math.abs(dx) >= 0.35f || kotlin.math.abs(dy) >= 0.35f) {
                            // Keep the fallback surface path consistent with the
                            // LookPadView direct-manipulation sign convention.
                            queueEvent { NativeGameBridge.orbitCamera(-dx * 0.0048f, -dy * 0.0032f) }
                        }
                        lastLookX = event.getX(index)
                        lastLookY = event.getY(index)
                    }
                }
            }
            MotionEvent.ACTION_POINTER_UP -> {
                if (event.getPointerId(event.actionIndex) == activeLookPointerId) {
                    activeLookPointerId = MotionEvent.INVALID_POINTER_ID
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                activeLookPointerId = MotionEvent.INVALID_POINTER_ID
                performClick()
            }
        }
        return true
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }
}

private class LookPadView(context: Context, private val onOrbit: (Float, Float) -> Unit) : View(context) {
    private var activePointerId = MotionEvent.INVALID_POINTER_ID
    private var lastX = 0f
    private var lastY = 0f
    private var centerX = 0f
    private var centerY = 0f
    private var radius = 1f
    private val density = context.resources.displayMetrics.density

    private fun dp(value: Int): Int = (value * density).roundToInt()

    init {
        setWillNotDraw(false)
        layoutParams = FrameLayout.LayoutParams(-1, -1)
        isClickable = true
    }

    override fun onSizeChanged(width: Int, height: Int, oldWidth: Int, oldHeight: Int) {
        centerX = width * 0.80f
        centerY = height * 0.61f
        radius = (width.coerceAtMost(dp(720)) * 0.105f).coerceAtLeast(dp(58).toFloat())
    }

    override fun onDraw(canvas: android.graphics.Canvas) {
        super.onDraw(canvas)
        if (width <= 0 || height <= 0) return
        val paint = android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG)
        paint.color = Color.argb(34, 214, 239, 231)
        canvas.drawCircle(centerX, centerY, radius * 1.55f, paint)
        paint.color = Color.argb(45, 18, 31, 35)
        canvas.drawCircle(centerX, centerY, radius * 1.18f, paint)
        paint.style = android.graphics.Paint.Style.STROKE
        paint.strokeWidth = dp(2).toFloat()
        paint.color = Color.argb(150, 226, 198, 126)
        canvas.drawCircle(centerX, centerY, radius, paint)
        paint.color = Color.argb(92, 237, 244, 227)
        canvas.drawCircle(centerX, centerY, radius * 0.56f, paint)
        canvas.drawLine(centerX - radius * 0.72f, centerY, centerX + radius * 0.72f, centerY, paint)
        canvas.drawLine(centerX, centerY - radius * 0.72f, centerX, centerY + radius * 0.72f, paint)
        paint.style = android.graphics.Paint.Style.FILL
        paint.textSize = dp(10).toFloat()
        paint.textAlign = android.graphics.Paint.Align.CENTER
        paint.color = Color.argb(205, 241, 224, 178)
        canvas.drawText("LOOK", centerX, centerY + radius * 1.72f, paint)
    }

    private fun isLookRegion(x: Float, y: Float): Boolean =
        x >= width * 0.43f && y >= height * 0.23f

    private fun begin(event: MotionEvent, index: Int): Boolean {
        val x = event.getX(index)
        val y = event.getY(index)
        if (!isLookRegion(x, y)) return false
        activePointerId = event.getPointerId(index)
        lastX = x
        lastY = y
        return true
    }

    private fun release() {
        activePointerId = MotionEvent.INVALID_POINTER_ID
        invalidate()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> return begin(event, event.actionIndex)
            MotionEvent.ACTION_POINTER_DOWN -> {
                if (activePointerId == MotionEvent.INVALID_POINTER_ID) begin(event, event.actionIndex)
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                val index = event.findPointerIndex(activePointerId)
                if (index >= 0) {
                    val dx = (event.getX(index) - lastX).coerceIn(-96f, 96f)
                    val dy = (event.getY(index) - lastY).coerceIn(-96f, 96f)
                    if (kotlin.math.abs(dx) >= 0.35f || kotlin.math.abs(dy) >= 0.35f) onOrbit(dx, dy)
                    lastX = event.getX(index)
                    lastY = event.getY(index)
                }
                return true
            }
            MotionEvent.ACTION_POINTER_UP -> {
                if (event.getPointerId(event.actionIndex) == activePointerId) release()
                return true
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                release()
                performClick()
                return true
            }
        }
        return activePointerId != MotionEvent.INVALID_POINTER_ID
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }
}

private class JoystickView(context: Context, private val onMove: (Float, Float) -> Unit) : View(context) {
    private var baseX = 0f
    private var baseY = 0f
    private var knobX = 0f
    private var knobY = 0f
    private var radius = 1f
    private var activePointerId = MotionEvent.INVALID_POINTER_ID
    private val density = context.resources.displayMetrics.density
    private val deadZone = 0.10f
    private val leftZoneFraction = 0.46f
    private var sensitivity = 1.0f
    private var responseCurve = 1.12f

    private fun dp(value: Int): Int = (value * density).roundToInt()

    fun setSensitivity(value: Float) {
        sensitivity = value.coerceIn(0.50f, 1.50f)
        // Keep full-speed output at the edge while changing precision near center.
        responseCurve = (1.42f - sensitivity * 0.30f).coerceIn(0.97f, 1.27f)
    }

    init {
        setWillNotDraw(false)
        alpha = 0.94f
        // A bottom movement zone allows the player to touch anywhere under the
        // left thumb instead of forcing a fixed stick position.
        layoutParams = FrameLayout.LayoutParams(-1, dp(290), Gravity.BOTTOM or Gravity.START)
        isClickable = true
    }

    override fun onSizeChanged(width: Int, height: Int, oldWidth: Int, oldHeight: Int) {
        resetStickOrigin()
    }

    private fun resetStickOrigin() {
        baseX = width * 0.16f
        baseY = height * 0.57f
        knobX = baseX
        knobY = baseY
        radius = (width.coerceAtMost(dp(360)) * 0.115f).coerceAtLeast(dp(62).toFloat())
    }

    override fun onDraw(canvas: android.graphics.Canvas) {
        super.onDraw(canvas)
        if (width <= 0 || height <= 0) return
        val paint = android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG)
        paint.color = Color.argb(48, 220, 238, 226)
        canvas.drawCircle(baseX, baseY, radius * 1.42f, paint)
        paint.color = Color.argb(78, 225, 244, 220)
        canvas.drawCircle(baseX, baseY, radius * 1.16f, paint)
        paint.color = Color.argb(115, 239, 194, 112)
        canvas.drawCircle(baseX, baseY, radius, paint)
        paint.color = Color.argb(195, 255, 226, 164)
        canvas.drawCircle(knobX, knobY, radius * 0.43f, paint)
        paint.style = android.graphics.Paint.Style.STROKE
        paint.strokeWidth = dp(2).toFloat()
        paint.color = Color.argb(150, 255, 242, 194)
        canvas.drawCircle(baseX, baseY, radius * 1.42f, paint)
        paint.color = Color.argb(185, 255, 242, 194)
        canvas.drawCircle(knobX, knobY, radius * 0.43f, paint)
        paint.style = android.graphics.Paint.Style.FILL
        paint.textSize = dp(10).toFloat()
        paint.textAlign = android.graphics.Paint.Align.CENTER
        paint.color = Color.argb(205, 241, 224, 178)
        canvas.drawText("MOVE", baseX, baseY + radius * 1.72f, paint)
    }

    private fun emitMove(event: MotionEvent, index: Int) {
        val dx = event.getX(index) - baseX
        val dy = event.getY(index) - baseY
        val distance = hypot(dx.toDouble(), dy.toDouble()).toFloat()
        val clampedDistance = distance.coerceAtMost(radius)
        if (distance <= radius * deadZone) {
            knobX = baseX
            knobY = baseY
            onMove(0f, 0f)
            invalidate()
            return
        }
        val directionX = dx / distance
        val directionY = dy / distance
        knobX = baseX + directionX * clampedDistance
        knobY = baseY + directionY * clampedDistance
        val normalizedDistance = (((clampedDistance / radius) - deadZone) / (1f - deadZone)).coerceIn(0f, 1f)
        val curvedDistance = normalizedDistance.pow(responseCurve)
        onMove(
            (directionX * curvedDistance).coerceIn(-1f, 1f),
            (directionY * curvedDistance).coerceIn(-1f, 1f)
        )
        invalidate()
    }

    private fun beginPointer(event: MotionEvent, index: Int): Boolean {
        if (event.getX(index) > width * leftZoneFraction || event.getY(index) < height * 0.08f) return false
        activePointerId = event.getPointerId(index)
        baseX = event.getX(index).coerceIn(radius * 1.55f, width - radius * 1.55f)
        baseY = event.getY(index).coerceIn(radius * 1.55f, height - radius * 1.55f)
        emitMove(event, index)
        invalidate()
        return true
    }

    private fun releasePointer() {
        activePointerId = MotionEvent.INVALID_POINTER_ID
        onMove(0f, 0f)
        resetStickOrigin()
        invalidate()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> return beginPointer(event, event.actionIndex)
            MotionEvent.ACTION_POINTER_DOWN -> {
                if (activePointerId == MotionEvent.INVALID_POINTER_ID) beginPointer(event, event.actionIndex)
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                val index = event.findPointerIndex(activePointerId)
                if (index >= 0) emitMove(event, index)
                return true
            }
            MotionEvent.ACTION_POINTER_UP -> {
                if (event.getPointerId(event.actionIndex) == activePointerId) releasePointer()
                return true
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                releasePointer()
                performClick()
                return true
            }
        }
        return activePointerId != MotionEvent.INVALID_POINTER_ID
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }
}

private class GameRenderer(private val onRendererReady: () -> Unit) : GLSurfaceView.Renderer {
    var targetFps: Int = 60
    var graphicsTier: Int = 2
    private var lastFrameNanos = 0L

    fun resetFrameClock() {
        lastFrameNanos = 0L
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        NativeGameBridge.init(1, 1)
        NativeGameBridge.setGraphicsQuality(graphicsTier)
        onRendererReady()
        lastFrameNanos = System.nanoTime()
        GLES30.glDisable(GLES30.GL_DEPTH_TEST)
        GLES30.glEnable(GLES30.GL_BLEND)
        GLES30.glBlendFunc(GLES30.GL_SRC_ALPHA, GLES30.GL_ONE_MINUS_SRC_ALPHA)
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        GLES30.glViewport(0, 0, width, height)
        NativeGameBridge.resize(width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        val now = System.nanoTime()
        val delta = if (lastFrameNanos == 0L) 1.0 / targetFps.toDouble() else (now - lastFrameNanos) / 1_000_000_000.0
        lastFrameNanos = now
        NativeGameBridge.render(delta.toFloat().coerceIn(0.0f, 0.10f))
    }
}
