package com.darvirgoyt.aethelgrad

import android.app.Activity
import android.app.AlertDialog
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RadialGradient
import android.graphics.Shader
import android.graphics.drawable.GradientDrawable
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
import android.widget.TextView
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10
import kotlin.math.hypot
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
    external fun teleportToTower()
    external fun setGyroEnabled(enabled: Boolean)
    external fun setGyro(rotationX: Float, rotationY: Float, sensitivity: Float)
    external fun attack()
    external fun heavyAttack()
    external fun jump()
    external fun dodge()
    external fun slide()
    external fun gather()
    external fun craft()
    external fun getHudState(): String
    external fun setGraphicsQuality(level: Int)
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
    private val gyroSensitivity = 1.0f
    private var selectedTargetFps = 60
    private var selectedGraphicsTier = 2
    private var supportedTargetFps = listOf(60)
    private val graphicsPreferences by lazy { getSharedPreferences("aethelgard_graphics", MODE_PRIVATE) }
    private lateinit var audio: GameAudio
    private lateinit var assetDelivery: AssetDeliveryManager
    private lateinit var stateLabel: TextView
    private lateinit var questLabel: TextView
    private lateinit var onboardingOverlay: View
    private var characterSetupOverlay: View? = null
    private var assetPatchOverlay: View? = null
    private var authenticationTransitionStarted = false
    private lateinit var onboardingStatus: TextView
    private lateinit var characterNameInput: EditText
    private val accountSession = AccountSessionManager()
    private val characterCreation = CharacterCreationState()
    private var activeCloudWorld: CloudWorldManifest? = null
    private var cloudSaveInFlight = false
    private var cloudRecoveryNotice: String? = null
    private var selectedServer = ServerDirectory.regions.first()
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
    private val cloudSaveUpdater = object : Runnable {
        override fun run() {
            val world = activeCloudWorld
            if (world != null && !cloudSaveInFlight && ::gameView.isInitialized) {
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
        audio = GameAudio(this)
        assetDelivery = AssetDeliveryManager(this)
        audio.playMusic()

        rootContainer = FrameLayout(this).apply { setBackgroundColor(Color.rgb(7, 16, 20)) }
        gameView = GameSurfaceView(this)
        assetPacks = AssetPackCatalog(this)
        gameView.applyTargetFps(selectedTargetFps)
        gameView.applyGraphicsTier(selectedGraphicsTier)
        rootContainer.addView(gameView, FrameLayout.LayoutParams(-1, -1))
        rootContainer.addView(buildHud())
        rootContainer.addView(JoystickView(this) { x, y ->
            gameView.queueEvent { NativeGameBridge.setMove(x, y) }
        })
        onboardingOverlay = buildOnboardingOverlay()
        rootContainer.addView(onboardingOverlay)
        setContentView(rootContainer)
        // Production requests the fast-follow forest pack. The prototype variant stays
        // offline and uses the built-in native slice so it is playable immediately.
        if (!BuildConfig.PROTOTYPE_MODE) {
            assetPacks.request("assetpack_forest")
        } else {
            onboardingOverlay.visibility = View.GONE
        }
        updateGyroButton()
        registerGyro()
        if (!BuildConfig.PROTOTYPE_MODE) {
            accountSession.initialize(this, ::applyAccountSnapshot)
        }
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
        val world = activeCloudWorld
        if (world != null && !cloudSaveInFlight) {
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
        hudHandler.postDelayed(hudUpdater, 350L)
        hudHandler.postDelayed(cloudSaveUpdater, 45_000L)
    }

    override fun onDestroy() {
        hudHandler.removeCallbacks(hudUpdater)
        hudHandler.removeCallbacks(cloudSaveUpdater)
        accountSession.shutdown()
        if (::assetDelivery.isInitialized) assetDelivery.shutdown()
        if (::assetPacks.isInitialized) assetPacks.close()
        if (::audio.isInitialized) audio.release()
        super.onDestroy()
    }

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

        val serverButton = cinematicButton("◉  ${selectedServer.name.removePrefix("Aethelgard ").uppercase()}  ▾", false) {
            val index = ServerDirectory.regions.indexOfFirst { it.id == selectedServer.id }
            selectedServer = ServerDirectory.regions[(index + 1) % ServerDirectory.regions.size]
            onboardingStatus.text = "${selectedServer.name} selected  •  PING: ${selectedServer.pingMs?.let { "$it ms" } ?: "—"}  •  ${selectedServer.status}"
        }
        overlay.addView(serverButton, FrameLayout.LayoutParams(dp(176), dp(42), Gravity.TOP or Gravity.END).apply {
            topMargin = dp(18)
            rightMargin = dp(24)
        })

        val settingsButton = cinematicButton("⚙  SETTINGS", false) { showAudioSettings() }
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
            text = "AETHELGARD"
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
            text = "Sign in securely to continue to character setup and your cloud world."
            textSize = 12f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(210, 214, 218))
            setPadding(0, 0, 0, dp(6))
        }
        onboardingStatus = TextView(this).apply {
            text = "ONLINE ONLY / ONLINE LOGIN  •  Checking your Google account…"
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
            text = "I agree to the Terms of Service and Privacy Policy"
            textSize = 11f
            setTextColor(Color.rgb(220, 222, 224))
            buttonTintList = android.content.res.ColorStateList.valueOf(Color.rgb(220, 182, 101))
            isChecked = false
        }
        val google = cinematicButton("✦  CONTINUE WITH GOOGLE", true) {
            accountSession.requestGoogleSignIn()
        }.apply {
            isEnabled = false
            alpha = 0.5f
        }
        consent.setOnCheckedChangeListener { _, checked ->
            google.isEnabled = checked
            google.alpha = if (checked) 1f else 0.5f
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

    private fun showArtReferenceDialog() {
        val preview = ImageView(this).apply {
            scaleType = ImageView.ScaleType.FIT_CENTER
            adjustViewBounds = true
            setBackgroundColor(Color.rgb(18, 24, 29))
            setPadding(dp(6), dp(6), dp(6), dp(6))
            setImageResource(R.drawable.reference_game_board)
        }
        val selector = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER
            setPadding(0, 0, 0, dp(8))
        }
        val references = listOf(
            "BOARD" to R.drawable.reference_game_board,
            "PLAYER" to R.drawable.reference_player_emotions,
            "WORLD" to R.drawable.reference_environment_lighting,
            "CREATURES" to R.drawable.reference_enemy_creatures,
            "ASSETS" to R.drawable.reference_assets_weapons,
            "UI" to R.drawable.reference_ui_gameplay
        )
        references.forEach { (label, resource) ->
            selector.addView(cinematicButton(label, false) { preview.setImageResource(resource) }, LinearLayout.LayoutParams(0, dp(36), 1f).apply {
                leftMargin = dp(2)
                rightMargin = dp(2)
            })
        }
        val content = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(12), dp(4), dp(12), 0)
            addView(selector, LinearLayout.LayoutParams(-1, dp(46)))
            addView(preview, LinearLayout.LayoutParams(-1, dp(420)))
        }
        AlertDialog.Builder(this)
            .setTitle("ART REFERENCE LIBRARY")
            .setMessage("Style targets for player, expressions, environments, creatures, assets, weapons, monument, UI, skin tones, and time-of-day lighting.")
            .setView(content)
            .setPositiveButton("CLOSE", null)
            .show()
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).roundToInt()

    private fun applyAccountSnapshot(snapshot: SessionSnapshot) {
        if (snapshot.state == SessionState.AUTHENTICATED && !authenticationTransitionStarted) {
            authenticationTransitionStarted = true
            accountSession.fetchProfile { profile, profileError ->
                accountSession.fetchOwnedWorlds { worlds, worldsError ->
                    showCharacterSetup(snapshot.accountId, profile, worlds.orEmpty(), worldsError ?: profileError)
                }
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

    /** The login panel is intentionally sign-in only. A verified backend session advances here automatically. */
    private fun showCharacterSetup(accountId: String?, recoveredProfile: PlayerProfile? = null, recoveredWorlds: List<CloudWorldManifest> = emptyList(), cloudError: String? = null) {
        runOnUiThread {
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
                text = "✦  AETHELGARD  ✦"
                textSize = 15f
                gravity = Gravity.CENTER
                letterSpacing = 0.16f
                setTextColor(Color.rgb(226, 184, 101))
            }, LinearLayout.LayoutParams(-1, dp(30)))
            panel.addView(TextView(this).apply {
                text = if (selectedWorld == null) "CREATE YOUR WAYFARER" else "YOUR CLOUD WORLDS"
                textSize = 24f
                gravity = Gravity.CENTER
                typeface = android.graphics.Typeface.create("serif", android.graphics.Typeface.BOLD)
                setTextColor(Color.rgb(239, 234, 219))
            }, LinearLayout.LayoutParams(-1, dp(42)))
            val accountStatus = TextView(this).apply {
                text = when {
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
                hint = if (selectedWorld == null) "WAYFARER NAME" else "PROFILE NAME"
                setText(recoveredProfile?.username.orEmpty())
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
            val avatars = listOf("trailblazer", "ember", "verdant", "tide", "moon", "sunward")
            val avatarMarks = listOf("T", "E", "V", "W", "M", "S")
            val avatarColors = listOf(
                Color.rgb(78, 114, 92), Color.rgb(153, 81, 54), Color.rgb(71, 139, 103),
                Color.rgb(57, 112, 148), Color.rgb(103, 83, 150), Color.rgb(166, 125, 63)
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
                    val portraitId = resources.getIdentifier("aethelgard_avatar_${avatars[index]}", "drawable", packageName)
                    addView(ImageView(this@MainActivity).apply {
                        scaleType = ImageView.ScaleType.CENTER_CROP
                        if (portraitId != 0) setImageResource(portraitId)
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
            panel.addView(cinematicButton(if (selectedWorld == null) "CREATE CLOUD WORLD  ›" else "RESUME SELECTED WORLD  ›", true) {
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
                                rootContainer.removeView(overlay)
                                characterSetupOverlay = null
                            }
                        }
                    }
                    return@cinematicButton
                }
                characterCreation.name = characterNameInput.text.toString()
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
                                validation.setTextColor(Color.rgb(164, 231, 190))
                                validation.text = "Cloud world protected. Entering Aethelgard…"
                                rootContainer.postDelayed({
                                    rootContainer.removeView(overlay)
                                    characterSetupOverlay = null
                                    showAssetPatchOverlay()
                                }, 420L)
                            }
                        }
                    }
                }
            }, LinearLayout.LayoutParams(-1, dp(52)).apply { topMargin = dp(4) })
            overlay.addView(panel, FrameLayout.LayoutParams(dp(520), -2, Gravity.CENTER))
            characterSetupOverlay = overlay
            rootContainer.addView(overlay)
        }
    }

    private fun buildHud(): View {
        val overlay = FrameLayout(this)
        val top = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(28), dp(16), dp(28), 0)
        }
        val title = TextView(this).apply {
            text = if (BuildConfig.PROTOTYPE_MODE) {
                "AETHELGRAD  •  PROTOTYPE  •  OFFLINE"
            } else {
                "AETHELGRAD  •  DAY 1  •  DAY"
            }
            textSize = 15f
            setTextColor(Color.rgb(244, 218, 155))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
        }
        stateLabel = TextView(this).apply {
            text = "SAND  |  DAY 1  |  DAY  |  CLEAR  |  HP 100  |  STA 100  |  HUN 82  |  LV 1  |  XP 0/100  |  W 12  F 08  S 04"
            textSize = 13f
            setTextColor(Color.WHITE)
            setShadowLayer(4f, 1f, 1f, Color.BLACK)
        }
        questLabel = TextView(this).apply {
            text = "THE FIRST EMBER  -  Aurora arrives - gather 3 caches for the camp"
            textSize = 13f
            setTextColor(Color.rgb(255, 226, 164))
            setShadowLayer(4f, 1f, 1f, Color.BLACK)
            setPadding(0, 0, dp(24), dp(24))
        }
        gyroButton = actionButton("GYRO: OFF") {
            audio.playEffect("ui")
            gyroEnabled = gyroSensor != null && !gyroEnabled
            gameView.queueEvent { NativeGameBridge.setGyroEnabled(gyroEnabled) }
            updateGyroButton()
        }
        top.addView(title)
        top.addView(stateLabel)
        top.addView(gyroButton, LinearLayout.LayoutParams(dp(142), dp(44)).apply { leftMargin = dp(18) })
        overlay.addView(top, FrameLayout.LayoutParams(-1, dp(54), Gravity.TOP))
        overlay.addView(questLabel, FrameLayout.LayoutParams(-1, dp(42), Gravity.TOP).apply { topMargin = dp(54) })

        val profileBadge = ImageView(this).apply {
            setImageResource(R.drawable.aethelgard_profile_gold)
            scaleType = ImageView.ScaleType.CENTER_CROP
            contentDescription = "AETHELGRAD gold profile photo"
            background = GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(Color.rgb(226, 184, 101))
                setStroke(dp(2), Color.rgb(255, 235, 156))
            }
            clipToOutline = true
        }
        overlay.addView(profileBadge, FrameLayout.LayoutParams(dp(58), dp(58), Gravity.TOP or Gravity.END).apply {
            topMargin = dp(10)
            rightMargin = dp(176)
        })

        var firstPerson = false
        var worldMapVisible = false
        val navigation = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        val viewModeButton = actionButton("VIEW: THIRD PERSON") { }
        viewModeButton.setOnClickListener {
            firstPerson = !firstPerson
            viewModeButton.text = if (firstPerson) "VIEW: FIRST PERSON" else "VIEW: THIRD PERSON"
            gameView.queueEvent { NativeGameBridge.toggleViewMode() }
        }
        val mapButton = actionButton("WORLD MAP") { }
        mapButton.setOnClickListener {
            worldMapVisible = !worldMapVisible
            mapButton.text = if (worldMapVisible) "MAP: CLOSE" else "WORLD MAP"
            gameView.queueEvent { NativeGameBridge.setWorldMapVisible(worldMapVisible) }
        }
        val towerButton = actionButton("TOWER / TELEPORT") {
            audio.playEffect("ui")
            gameView.queueEvent { NativeGameBridge.teleportToTower() }
        }
        navigation.addView(viewModeButton, LinearLayout.LayoutParams(dp(156), dp(42)).apply { rightMargin = dp(6) })
        navigation.addView(mapButton, LinearLayout.LayoutParams(dp(112), dp(42)).apply { rightMargin = dp(6) })
        navigation.addView(towerButton, LinearLayout.LayoutParams(dp(156), dp(42)))
        overlay.addView(navigation, FrameLayout.LayoutParams(-1, dp(46), Gravity.TOP).apply {
            topMargin = dp(96)
            leftMargin = dp(20)
            rightMargin = dp(210)
        })
        val orbitHint = TextView(this).apply {
            text = "SWIPE RIGHT TO ORBIT 360°  •  GYRO OPTIONAL"
            textSize = 10f
            letterSpacing = 0.08f
            setTextColor(Color.rgb(229, 211, 167))
            setShadowLayer(4f, 1f, 1f, Color.BLACK)
        }
        overlay.addView(orbitHint, FrameLayout.LayoutParams(-1, dp(24), Gravity.TOP).apply {
            topMargin = dp(142)
            leftMargin = dp(24)
        })

        val actions = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.BOTTOM or Gravity.END
            setPadding(0, dp(12), 0, dp(4))
        }
        val sprintSlide = actionButton("SPRINT / SLIDE") { }
        sprintSlide.setOnTouchListener { _, event ->
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> gameView.queueEvent { NativeGameBridge.setSprintHeld(true) }
                MotionEvent.ACTION_UP -> {
                    gameView.queueEvent {
                        NativeGameBridge.setSprintHeld(false)
                        NativeGameBridge.slide()
                    }
                }
                MotionEvent.ACTION_CANCEL -> gameView.queueEvent { NativeGameBridge.setSprintHeld(false) }
            }
            true
        }
        val attack = actionButton("ATTACK") { audio.playEffect("attack"); gameView.queueEvent { NativeGameBridge.attack() } }
        val heavy = actionButton("HEAVY") { audio.playEffect("attack"); gameView.queueEvent { NativeGameBridge.heavyAttack() } }
        val jump = actionButton("JUMP") { audio.playEffect("ui"); gameView.queueEvent { NativeGameBridge.jump() } }
        val dodge = actionButton("DODGE") { audio.playEffect("slide"); gameView.queueEvent { NativeGameBridge.dodge() } }
        val gather = actionButton("GATHER") { audio.playEffect("gather"); gameView.queueEvent { NativeGameBridge.gather() } }
        val craft = actionButton("CRAFT") { audio.playEffect("craft"); gameView.queueEvent { NativeGameBridge.craft() } }
        val settings = actionButton("GRAPHICS / FPS") { showGraphicsSettings() }
        actions.addView(sprintSlide, LinearLayout.LayoutParams(dp(150), dp(50)).apply { bottomMargin = dp(6) })
        actions.addView(attack, LinearLayout.LayoutParams(dp(150), dp(50)).apply { bottomMargin = dp(6) })
        actions.addView(heavy, LinearLayout.LayoutParams(dp(150), dp(50)).apply { bottomMargin = dp(6) })
        actions.addView(jump, LinearLayout.LayoutParams(dp(150), dp(50)).apply { bottomMargin = dp(6) })
        actions.addView(dodge, LinearLayout.LayoutParams(dp(150), dp(50)).apply { bottomMargin = dp(6) })
        actions.addView(gather, LinearLayout.LayoutParams(dp(150), dp(50)).apply { bottomMargin = dp(6) })
        actions.addView(craft, LinearLayout.LayoutParams(dp(150), dp(50)).apply { bottomMargin = dp(6) })
        actions.addView(settings, LinearLayout.LayoutParams(dp(150), dp(50)))
        overlay.addView(actions, FrameLayout.LayoutParams(-1, -1))
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
        val water = values.getOrNull(16)?.ifBlank { "DRY" } ?: "DRY"
        val locomotion = values.getOrNull(17)?.ifBlank { "IDLE" } ?: "IDLE"
        val weather = values.getOrNull(18)?.ifBlank { "CLEAR" } ?: "CLEAR"
        val viewMode = values.getOrNull(19)?.replace('_', ' ')?.ifBlank { "THIRD PERSON" } ?: "THIRD PERSON"
        val mapState = values.getOrNull(20)?.ifBlank { "MAP OFF" } ?: "MAP OFF"
        val towerState = values.getOrNull(21)?.replace('_', ' ')?.ifBlank { "TOWER READY" } ?: "TOWER READY"
        stateLabel.text = "$biome  |  $phase  |  DAY $daysPlayed  |  $weather  |  $viewMode  |  $mapState  |  $towerState  |  HP $health  |  STA $stamina  |  HUN $hunger  |  $water  |  $locomotion  |  LV $level  |  XP $xp/$next  |  W $wood  F $fiber  S $stone"
        stateLabel.setTextColor(
            when {
                levelPulse -> Color.rgb(255, 236, 157)
                weather == "THUNDERSTORM" -> Color.rgb(190, 220, 255)
                weather == "RAIN" -> Color.rgb(170, 220, 236)
                else -> Color.WHITE
            }
        )
        val recoveryNotice = cloudRecoveryNotice
        questLabel.text = recoveryNotice ?: if (warden > 0 && objective.contains("Forest Warden")) "$objective  •  $phase  •  $weather  •  FOREST WARDEN HP $warden/100  •  $locomotion" else "$objective  •  $phase  •  DAY $daysPlayed  •  $biome BIOME  •  $weather  •  $water"
        questLabel.setTextColor(if (recoveryNotice != null) Color.rgb(255, 180, 150) else if (questPulse) Color.rgb(255, 236, 157) else Color.rgb(255, 226, 164))
    }

    private fun showAssetPatchOverlay() {
        if (assetPatchOverlay != null) return
        val overlay = FrameLayout(this).apply { setBackgroundColor(Color.rgb(4, 10, 16)) }
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
        val title = TextView(this).apply {
            text = "PREPARING AETHELGARD  •  COOKED 3D ASSET PACK"
            textSize = 18f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(244, 218, 155))
        }
        val status = TextView(this).apply {
            text = "Checking asset manifest…"
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
            text = "Resolving the selected device graphics tier…"
            textSize = 12f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(161, 190, 187))
            setPadding(0, dp(12), 0, 0)
        }
        val note = TextView(this).apply {
            text = "Runtime bundles are pre-cooked offline. Android verifies, unpacks, and mounts them; it does not compile the authoring project on-device."
            textSize = 11f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(146, 168, 171))
            setPadding(0, dp(18), 0, 0)
        }
        val retry = actionButton("RETRY ASSET PREPARATION") { }
        retry.visibility = View.GONE
        panel.addView(title, LinearLayout.LayoutParams(-1, dp(34)))
        panel.addView(status, LinearLayout.LayoutParams(-1, dp(46)))
        panel.addView(progress, LinearLayout.LayoutParams(dp(430), dp(28)))
        panel.addView(details, LinearLayout.LayoutParams(-1, dp(48)))
        panel.addView(note, LinearLayout.LayoutParams(-1, dp(60)))
        panel.addView(retry, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(12) })
        overlay.addView(panel, FrameLayout.LayoutParams(dp(520), -2, Gravity.CENTER))
        rootContainer.addView(overlay)
        assetPatchOverlay = overlay

        lateinit var startPreparation: () -> Unit
        startPreparation = {
            retry.visibility = View.GONE
            progress.progress = 0
            assetDelivery.prepareForTier(selectedGraphicsTier) { event ->
                status.text = event.title
                details.text = event.detail
                progress.progress = event.percent
                if (event.state == AssetDeliveryManager.State.READY) {
                    hudHandler.postDelayed({
                        rootContainer.removeView(overlay)
                        assetPatchOverlay = null
                    }, 450L)
                } else if (event.state == AssetDeliveryManager.State.FAILED) {
                    note.text = event.error ?: "The bundle was rejected. Check the connection or manifest."
                    note.setTextColor(Color.rgb(255, 180, 150))
                    retry.visibility = View.VISIBLE
                }
            }
        }
        retry.setOnClickListener { startPreparation() }
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
            .setNegativeButton("CANCEL", null)
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
            .setTitle("AETHELGARD AUDIO")
            .setView(panel)
            .setNegativeButton("Close", null)
            .show()
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
        setOnClickListener { onClick() }
        background = GradientDrawable().apply {
            cornerRadius = 18f
            setColor(Color.rgb(238, 194, 112))
            setStroke(2, Color.rgb(255, 230, 168))
        }
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

private class GameSurfaceView(context: Context) : GLSurfaceView(context) {
    private val renderer = GameRenderer()
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
                            queueEvent { NativeGameBridge.orbitCamera(dx * 0.0048f, dy * 0.0032f) }
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

private class JoystickView(context: Context, private val onMove: (Float, Float) -> Unit) : View(context) {
    private var centerX = 0f
    private var centerY = 0f
    private var radius = 1f
    private var activePointerId = MotionEvent.INVALID_POINTER_ID
    private val density = context.resources.displayMetrics.density
    private val deadZone = 0.12f

    private fun dp(value: Int): Int = (value * density).roundToInt()

    init {
        setWillNotDraw(false)
        alpha = 0.9f
        layoutParams = FrameLayout.LayoutParams(dp(230), dp(230), Gravity.BOTTOM or Gravity.START).apply {
            leftMargin = dp(28)
            bottomMargin = dp(24)
        }
        isClickable = true
    }

    override fun onSizeChanged(width: Int, height: Int, oldWidth: Int, oldHeight: Int) {
        centerX = width / 2f
        centerY = height / 2f
        radius = width * 0.38f
    }

    override fun onDraw(canvas: android.graphics.Canvas) {
        super.onDraw(canvas)
        centerX = width / 2f
        centerY = height / 2f
        radius = width * 0.38f
        val paint = android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG)
        paint.color = Color.argb(65, 225, 244, 220)
        canvas.drawCircle(centerX, centerY, width * 0.46f, paint)
        paint.color = Color.argb(125, 239, 194, 112)
        canvas.drawCircle(centerX, centerY, radius, paint)
        paint.color = Color.argb(180, 255, 226, 164)
        canvas.drawCircle(centerX, centerY, radius * 0.45f, paint)
    }

    private fun emitMove(event: MotionEvent, index: Int) {
        val dx = event.getX(index) - centerX
        val dy = event.getY(index) - centerY
        val distance = hypot(dx.toDouble(), dy.toDouble()).toFloat()
        if (distance <= radius * deadZone) {
            onMove(0f, 0f)
            return
        }
        val clampedDistance = distance.coerceAtMost(radius)
        val normalizedDistance = ((clampedDistance / radius) - deadZone) / (1f - deadZone)
        val directionX = dx / distance
        val directionY = dy / distance
        onMove(
            (directionX * normalizedDistance).coerceIn(-1f, 1f),
            (directionY * normalizedDistance).coerceIn(-1f, 1f)
        )
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                activePointerId = event.getPointerId(event.actionIndex)
                emitMove(event, event.actionIndex)
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                val index = event.findPointerIndex(activePointerId)
                if (index >= 0) emitMove(event, index)
                return true
            }
            MotionEvent.ACTION_POINTER_UP -> {
                if (event.getPointerId(event.actionIndex) == activePointerId) {
                    activePointerId = MotionEvent.INVALID_POINTER_ID
                    onMove(0f, 0f)
                }
                return true
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                activePointerId = MotionEvent.INVALID_POINTER_ID
                onMove(0f, 0f)
                performClick()
                return true
            }
        }
        return true
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }
}

private class GameRenderer : GLSurfaceView.Renderer {
    var targetFps: Int = 60
    var graphicsTier: Int = 2
    private var lastFrameNanos = 0L

    fun resetFrameClock() {
        lastFrameNanos = 0L
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        NativeGameBridge.init(1, 1)
        NativeGameBridge.setGraphicsQuality(graphicsTier)
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
