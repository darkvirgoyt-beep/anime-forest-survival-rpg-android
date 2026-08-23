package com.darkvirgoyt.forestslice

import android.app.Activity
import android.app.AlertDialog
import android.content.pm.ApplicationInfo
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.Window
import android.view.WindowManager
import android.opengl.GLES30
import android.opengl.GLSurfaceView
import android.content.Context
import android.widget.Button
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.SeekBar
import android.widget.TextView
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10
import kotlin.math.hypot

object NativeGameBridge {
    init { System.loadLibrary("forestgame") }
    external fun init(width: Int, height: Int)
    external fun resize(width: Int, height: Int)
    external fun render(deltaSeconds: Float)
    external fun setMove(x: Float, y: Float)
    external fun setSprintHeld(held: Boolean)
    external fun orbitCamera(deltaYaw: Float, deltaPitch: Float)
    external fun setGyroEnabled(enabled: Boolean)
    external fun setGyro(rotationX: Float, rotationY: Float, sensitivity: Float)
    external fun attack()
    external fun jump()
    external fun dodge()
    external fun slide()
    external fun gather()
    external fun craft()
    external fun getHudState(): String
}

class MainActivity : Activity(), SensorEventListener {
    private lateinit var gameView: GameSurfaceView
    private lateinit var gyroButton: Button
    private var sensorManager: SensorManager? = null
    private var gyroSensor: Sensor? = null
    private var gyroEnabled = false
    private val gyroSensitivity = 1.0f
    private lateinit var audio: GameAudio
    private lateinit var stateLabel: TextView
    private lateinit var questLabel: TextView
    private lateinit var onboardingOverlay: View
    private lateinit var onboardingStatus: TextView
    private lateinit var characterNameInput: EditText
    private val accountSession = AccountSessionManager()
    private val characterCreation = CharacterCreationState()
    private var selectedServer = ServerDirectory.regions.first()
    private val isDeveloperBuild: Boolean
        get() = (applicationInfo.flags and ApplicationInfo.FLAG_DEBUGGABLE) != 0
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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        enterImmersiveMode()

        sensorManager = getSystemService(SENSOR_SERVICE) as SensorManager
        gyroSensor = sensorManager?.getDefaultSensor(Sensor.TYPE_GYROSCOPE)
        audio = GameAudio(this)
        audio.playMusic()

        val root = FrameLayout(this).apply { setBackgroundColor(Color.rgb(7, 16, 20)) }
        gameView = GameSurfaceView(this)
        root.addView(gameView, FrameLayout.LayoutParams(-1, -1))
        root.addView(buildHud())
        root.addView(JoystickView(this) { x, y ->
            gameView.queueEvent { NativeGameBridge.setMove(x, y) }
        })
        onboardingOverlay = buildOnboardingOverlay()
        root.addView(onboardingOverlay)
        setContentView(root)
        updateGyroButton()
        registerGyro()
        accountSession.initialize(this, ::applyAccountSnapshot)
    }

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
    }

    override fun onDestroy() {
        hudHandler.removeCallbacks(hudUpdater)
        audio.release()
        super.onDestroy()
    }

    private fun buildOnboardingOverlay(): View {
        val overlay = FrameLayout(this).apply { setBackgroundColor(Color.argb(238, 7, 16, 20)) }
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
            setPadding(32, 24, 32, 24)
            background = GradientDrawable().apply {
                cornerRadius = 24f
                setColor(Color.rgb(18, 35, 39))
                setStroke(2, Color.rgb(111, 180, 158))
            }
        }
        val title = TextView(this).apply {
            text = "AETHELGARD: WILD HORIZONS – CRAFTING"
            textSize = 22f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(244, 218, 155))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
        }
        val subtitle = TextView(this).apply {
            text = "FOUNDATION BUILD 01  •  ACCOUNT  →  SERVER  →  CHARACTER  →  WORLD"
            textSize = 11f
            gravity = Gravity.CENTER
            setTextColor(Color.LTGRAY)
            setPadding(0, 6, 0, 12)
        }
        onboardingStatus = TextView(this).apply {
            text = "ONLINE ONLY  •  Sign in with Google Play to continue to server selection and cloud world access."
            textSize = 12f
            gravity = Gravity.CENTER
            setTextColor(Color.WHITE)
            setPadding(0, 4, 0, 10)
        }
        val accountRow = LinearLayout(this).apply { gravity = Gravity.CENTER }
        val google = actionButton("GOOGLE PLAY SIGN-IN") {
            accountSession.requestGooglePlaySignIn()
        }
        if (isDeveloperBuild) {
            val guest = actionButton("DEV GUEST") {
                val snapshot = accountSession.startGuest()
                onboardingStatus.text = "DEV ONLY  •  ${snapshot.message}"
                onboardingStatus.setTextColor(Color.rgb(164, 231, 190))
            }
            accountRow.addView(guest, LinearLayout.LayoutParams(150, 46).apply { rightMargin = 10 })
            accountRow.addView(google, LinearLayout.LayoutParams(230, 46))
        } else {
            accountRow.addView(google, LinearLayout.LayoutParams(390, 46))
        }

        val serverTitle = TextView(this).apply {
            text = "SERVER / REGION"
            textSize = 12f
            setTextColor(Color.rgb(244, 218, 155))
            setPadding(0, 14, 0, 4)
        }
        val serverRow = LinearLayout(this).apply { gravity = Gravity.CENTER }
        ServerDirectory.regions.forEach { region ->
            val button = actionButton(region.name.removePrefix("Aethelgard ").uppercase()) {
                selectedServer = region
                onboardingStatus.text = "${region.name} selected  •  PING: ${region.pingMs?.let { "$it ms" } ?: "—"}  •  ${region.status}"
            }
            serverRow.addView(button, LinearLayout.LayoutParams(190, 42).apply { rightMargin = 8 })
        }

        val characterTitle = TextView(this).apply {
            text = "CHARACTER CREATION"
            textSize = 12f
            setTextColor(Color.rgb(244, 218, 155))
            setPadding(0, 12, 0, 4)
        }
        characterNameInput = EditText(this).apply {
            hint = "Character name (3–16 letters/numbers)"
            textSize = 14f
            setSingleLine(true)
            setTextColor(Color.WHITE)
            setHintTextColor(Color.GRAY)
            setPadding(16, 0, 16, 0)
        }
        val styleRow = LinearLayout(this).apply { gravity = Gravity.CENTER }
        fun styleButton(label: String, value: () -> Int, update: (Int) -> Unit): Button {
            val button = actionButton("$label: ${value() + 1}") { }
            button.setOnClickListener {
                val next = (value() + 1) % 4
                update(next)
                button.text = "$label: ${next + 1}"
            }
            return button
        }
        styleRow.addView(styleButton("EYEBROW", { characterCreation.eyebrowStyle }, { characterCreation.eyebrowStyle = it }), LinearLayout.LayoutParams(155, 42).apply { rightMargin = 8 })
        styleRow.addView(styleButton("CLOTHES", { characterCreation.outfitStyle }, { characterCreation.outfitStyle = it }), LinearLayout.LayoutParams(155, 42).apply { rightMargin = 8 })
        styleRow.addView(styleButton("HAIR", { characterCreation.hairStyle }, { characterCreation.hairStyle = it }), LinearLayout.LayoutParams(155, 42))

        val enter = actionButton("CREATE PROFILE / ENTER WORLD") {
            characterCreation.name = characterNameInput.text.toString()
            val error = characterCreation.validate()
            val authenticated = accountSession.snapshot.state == SessionState.AUTHENTICATED
            val developerGuest = isDeveloperBuild && accountSession.snapshot.state == SessionState.GUEST
            val serverReady = selectedServer.status == "ONLINE" && selectedServer.pingMs != null
            if (!authenticated && !developerGuest) {
                onboardingStatus.text = "Google Play sign-in is required before entering the online world."
                onboardingStatus.setTextColor(Color.rgb(255, 180, 150))
            } else if (!serverReady && !developerGuest) {
                onboardingStatus.text = "Server health/ping is not ready. Online world entry is blocked."
                onboardingStatus.setTextColor(Color.rgb(255, 180, 150))
            } else if (error != null) {
                onboardingStatus.text = error
                onboardingStatus.setTextColor(Color.rgb(255, 180, 150))
            } else {
                onboardingStatus.text = "${characterCreation.name} ready on ${selectedServer.name}. World bootstrap begins next."
                onboardingStatus.setTextColor(Color.rgb(164, 231, 190))
                overlay.postDelayed({ overlay.visibility = View.GONE }, 650L)
            }
        }

        panel.addView(title, LinearLayout.LayoutParams(-1, 38))
        panel.addView(subtitle, LinearLayout.LayoutParams(-1, 34))
        panel.addView(onboardingStatus, LinearLayout.LayoutParams(-1, 42))
        panel.addView(accountRow, LinearLayout.LayoutParams(-1, 48))
        panel.addView(serverTitle, LinearLayout.LayoutParams(-1, 30))
        panel.addView(serverRow, LinearLayout.LayoutParams(-1, 44))
        panel.addView(characterTitle, LinearLayout.LayoutParams(-1, 28))
        panel.addView(characterNameInput, LinearLayout.LayoutParams(-1, 48).apply { bottomMargin = 8 })
        panel.addView(styleRow, LinearLayout.LayoutParams(-1, 44))
        panel.addView(enter, LinearLayout.LayoutParams(-1, 50).apply { topMargin = 14 })
        overlay.addView(panel, FrameLayout.LayoutParams(-1, -2, Gravity.CENTER).apply {
            leftMargin = 40
            rightMargin = 40
        })
        return overlay
    }

    private fun applyAccountSnapshot(snapshot: SessionSnapshot) {
        if (!::onboardingStatus.isInitialized) return
        onboardingStatus.text = snapshot.message
        onboardingStatus.setTextColor(
            when (snapshot.state) {
                SessionState.AUTHENTICATED, SessionState.GUEST -> Color.rgb(164, 231, 190)
                SessionState.SIGNING_IN -> Color.rgb(255, 205, 145)
                SessionState.ERROR -> Color.rgb(255, 180, 150)
                SessionState.SIGNED_OUT -> Color.WHITE
            }
        )
    }

    private fun buildHud(): View {
        val overlay = FrameLayout(this)
        val top = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(28, 16, 28, 0)
        }
        val title = TextView(this).apply {
            text = "AETHELGARD  •  DAY 01"
            textSize = 15f
            setTextColor(Color.rgb(244, 218, 155))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
        }
        stateLabel = TextView(this).apply {
            text = "HP 100  |  STA 100  |  HUN 82  |  LV 1  |  XP 0/100  |  W 12  F 08  S 04"
            textSize = 13f
            setTextColor(Color.WHITE)
            setShadowLayer(4f, 1f, 1f, Color.BLACK)
        }
        questLabel = TextView(this).apply {
            text = "THE FIRST EMBER  •  Gather 3 resource caches"
            textSize = 13f
            setTextColor(Color.rgb(255, 226, 164))
            setShadowLayer(4f, 1f, 1f, Color.BLACK)
            setPadding(28, 0, 28, 0)
        }
        gyroButton = actionButton("GYRO: OFF") {
            audio.playEffect("ui")
            gyroEnabled = gyroSensor != null && !gyroEnabled
            gameView.queueEvent { NativeGameBridge.setGyroEnabled(gyroEnabled) }
            updateGyroButton()
        }
        top.addView(title)
        top.addView(stateLabel)
        top.addView(gyroButton, LinearLayout.LayoutParams(142, 44).apply { leftMargin = 18 })
        overlay.addView(top, FrameLayout.LayoutParams(-1, 54, Gravity.TOP))
        overlay.addView(questLabel, FrameLayout.LayoutParams(-1, 42, Gravity.TOP).apply { topMargin = 54 })

        val actions = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.BOTTOM or Gravity.END
            setPadding(0, 0, 24, 24)
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
        val jump = actionButton("JUMP") { audio.playEffect("ui"); gameView.queueEvent { NativeGameBridge.jump() } }
        val dodge = actionButton("DODGE") { audio.playEffect("slide"); gameView.queueEvent { NativeGameBridge.dodge() } }
        val gather = actionButton("GATHER") { audio.playEffect("gather"); gameView.queueEvent { NativeGameBridge.gather() } }
        val craft = actionButton("CRAFT") { audio.playEffect("craft"); gameView.queueEvent { NativeGameBridge.craft() } }
        val settings = actionButton("AUDIO SETTINGS") { showAudioSettings() }
        actions.addView(sprintSlide, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(attack, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(jump, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(dodge, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(gather, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(craft, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(settings, LinearLayout.LayoutParams(150, 50))
        overlay.addView(actions, FrameLayout.LayoutParams(-1, -1))
        return overlay
    }

    private fun applyHudSnapshot(snapshot: String) {
        val values = snapshot.split('|')
        if (values.size < 13 || !::stateLabel.isInitialized || !::questLabel.isInitialized) return
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
        val objective = values.drop(12).joinToString("|")
        stateLabel.text = "HP $health  |  STA $stamina  |  HUN $hunger  |  LV $level  |  XP $xp/$next  |  W $wood  F $fiber  S $stone"
        stateLabel.setTextColor(if (levelPulse) Color.rgb(255, 236, 157) else Color.WHITE)
        questLabel.text = if (warden in 1..99) "$objective  •  WARDEN HP $warden%" else objective
        questLabel.setTextColor(if (questPulse) Color.rgb(255, 236, 157) else Color.rgb(255, 226, 164))
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
        textSize = 13f
        setTextColor(Color.rgb(14, 26, 27))
        setOnClickListener { onClick() }
        background = GradientDrawable().apply {
            cornerRadius = 18f
            setColor(Color.rgb(238, 194, 112))
            setStroke(2, Color.rgb(255, 230, 168))
        }
    }
}

private class GameSurfaceView(context: Context) : GLSurfaceView(context) {
    private val renderer = GameRenderer()
    private var lastLookX = 0f
    private var lastLookY = 0f

    init {
        setEGLContextClientVersion(3)
        setRenderer(renderer)
        setPreserveEGLContextOnPause(true)
        renderMode = RENDERMODE_CONTINUOUSLY
        isFocusable = true
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                lastLookX = event.x
                lastLookY = event.y
            }
            MotionEvent.ACTION_MOVE -> {
                if (event.x > width * 0.42f) {
                    val dx = event.x - lastLookX
                    val dy = event.y - lastLookY
                    queueEvent { NativeGameBridge.orbitCamera(dx * 0.006f, dy * 0.004f) }
                }
                lastLookX = event.x
                lastLookY = event.y
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                queueEvent { NativeGameBridge.setMove(0f, 0f) }
            }
        }
        return true
    }
}

private class JoystickView(context: Context, private val onMove: (Float, Float) -> Unit) : View(context) {
    private var centerX = 0f
    private var centerY = 0f
    private var radius = 1f

    init {
        setWillNotDraw(false)
        alpha = 0.9f
        layoutParams = FrameLayout.LayoutParams(230, 230, Gravity.BOTTOM or Gravity.START).apply {
            leftMargin = 28
            bottomMargin = 24
        }
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

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val dx = event.x - centerX
        val dy = event.y - centerY
        val length = hypot(dx.toDouble(), dy.toDouble()).toFloat().coerceAtLeast(1f)
        val scale = (radius / length).coerceAtMost(1f)
        when (event.action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                onMove((dx / radius * scale).coerceIn(-1f, 1f), (dy / radius * scale).coerceIn(-1f, 1f))
                return true
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                onMove(0f, 0f)
                return true
            }
        }
        return true
    }
}

private class GameRenderer : GLSurfaceView.Renderer {
    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        NativeGameBridge.init(1, 1)
        GLES30.glDisable(GLES30.GL_DEPTH_TEST)
        GLES30.glEnable(GLES30.GL_BLEND)
        GLES30.glBlendFunc(GLES30.GL_SRC_ALPHA, GLES30.GL_ONE_MINUS_SRC_ALPHA)
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        GLES30.glViewport(0, 0, width, height)
        NativeGameBridge.resize(width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        NativeGameBridge.render(1f / 60f)
    }
}
