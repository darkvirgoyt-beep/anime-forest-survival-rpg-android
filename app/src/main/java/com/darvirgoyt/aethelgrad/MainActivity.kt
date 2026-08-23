package com.darvirgoyt.aethelgrad

import android.app.Activity
import android.app.AlertDialog
import android.content.pm.ApplicationInfo
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
import android.widget.CheckBox
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.LinearLayout
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
        accountSession.shutdown()
        audio.release()
        super.onDestroy()
    }

    private fun buildOnboardingOverlay(): View {
        val overlay = FrameLayout(this)
        overlay.addView(CinematicLoginBackdropView(this), FrameLayout.LayoutParams(-1, -1))

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
            text = "WELCOME, WAYFARER"
            textSize = 20f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(236, 193, 108))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            setPadding(0, dp(8), 0, dp(2))
        }
        val instruction = TextView(this).apply {
            text = "Securely sign in to access your cloud worlds and co-op expeditions."
            textSize = 12f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(210, 214, 218))
            setPadding(0, 0, 0, dp(6))
        }
        onboardingStatus = TextView(this).apply {
            text = "ONLINE ONLY  •  Checking your Google Play account…"
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
        val google = cinematicButton("✦  CONTINUE WITH GOOGLE PLAY", true) {
            accountSession.requestGooglePlaySignIn()
        }.apply {
            isEnabled = false
            alpha = 0.5f
        }
        consent.setOnCheckedChangeListener { _, checked ->
            google.isEnabled = checked
            google.alpha = if (checked) 1f else 0.5f
        }
        val characterStage = cinematicButton("PREPARE CHARACTER  ›", false) {
            val authenticated = accountSession.snapshot.state == SessionState.AUTHENTICATED
            if (!authenticated) {
                onboardingStatus.text = "Google Play sign-in is required before character creation."
                onboardingStatus.setTextColor(Color.rgb(255, 180, 150))
            } else {
                onboardingStatus.text = "Account verified. Character customization is the next online world stage."
                onboardingStatus.setTextColor(Color.rgb(164, 231, 190))
            }
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
        panel.addView(characterStage, LinearLayout.LayoutParams(-1, dp(44)).apply { topMargin = dp(8) })
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
        if (!::onboardingStatus.isInitialized) return
        onboardingStatus.text = snapshot.message
        onboardingStatus.setTextColor(
            when (snapshot.state) {
                SessionState.AUTHENTICATED, SessionState.GUEST -> Color.rgb(164, 231, 190)
                SessionState.SIGNING_IN -> Color.rgb(255, 205, 145)
                SessionState.CONFIGURATION_ERROR, SessionState.NETWORK_ERROR -> Color.rgb(255, 205, 145)
                SessionState.DENIED, SessionState.EXPIRED, SessionState.ERROR -> Color.rgb(255, 180, 150)
                SessionState.SIGNED_OUT -> Color.WHITE
            }
        )
    }

    private fun buildHud(): View {
        val overlay = FrameLayout(this)
        val top = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(28), dp(16), dp(28), 0)
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
        val jump = actionButton("JUMP") { audio.playEffect("ui"); gameView.queueEvent { NativeGameBridge.jump() } }
        val dodge = actionButton("DODGE") { audio.playEffect("slide"); gameView.queueEvent { NativeGameBridge.dodge() } }
        val gather = actionButton("GATHER") { audio.playEffect("gather"); gameView.queueEvent { NativeGameBridge.gather() } }
        val craft = actionButton("CRAFT") { audio.playEffect("craft"); gameView.queueEvent { NativeGameBridge.craft() } }
        val settings = actionButton("AUDIO SETTINGS") { showAudioSettings() }
        actions.addView(sprintSlide, LinearLayout.LayoutParams(dp(150), dp(50)).apply { bottomMargin = dp(6) })
        actions.addView(attack, LinearLayout.LayoutParams(dp(150), dp(50)).apply { bottomMargin = dp(6) })
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
