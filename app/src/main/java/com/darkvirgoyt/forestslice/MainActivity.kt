package com.darkvirgoyt.forestslice

import android.app.Activity
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.Bundle
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.Window
import android.view.WindowManager
import android.opengl.GLES30
import android.opengl.GLSurfaceView
import android.content.Context
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
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
    external fun orbitCamera(deltaYaw: Float, deltaPitch: Float)
    external fun setGyroEnabled(enabled: Boolean)
    external fun setGyro(rotationX: Float, rotationY: Float, sensitivity: Float)
    external fun attack()
    external fun jump()
    external fun dodge()
    external fun gather()
    external fun craft()
}

class MainActivity : Activity(), SensorEventListener {
    private lateinit var gameView: GameSurfaceView
    private lateinit var gyroButton: Button
    private var sensorManager: SensorManager? = null
    private var gyroSensor: Sensor? = null
    private var gyroEnabled = false
    private val gyroSensitivity = 1.0f

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        enterImmersiveMode()

        sensorManager = getSystemService(SENSOR_SERVICE) as SensorManager
        gyroSensor = sensorManager?.getDefaultSensor(Sensor.TYPE_GYROSCOPE)

        val root = FrameLayout(this).apply { setBackgroundColor(Color.rgb(7, 16, 20)) }
        gameView = GameSurfaceView(this)
        root.addView(gameView, FrameLayout.LayoutParams(-1, -1))
        root.addView(buildHud())
        root.addView(JoystickView(this) { x, y ->
            gameView.queueEvent { NativeGameBridge.setMove(x, y) }
        })
        setContentView(root)
        updateGyroButton()
        registerGyro()
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
        gyroEnabled = false
        sensorManager?.unregisterListener(this)
        gameView.queueEvent { NativeGameBridge.setGyroEnabled(false) }
        gameView.onPause()
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
        gameView.onResume()
        registerGyro()
        updateGyroButton()
    }

    private fun buildHud(): View {
        val overlay = FrameLayout(this)
        val top = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(28, 16, 28, 0)
        }
        val title = TextView(this).apply {
            text = "FOREST SLICE  •  DAY 01"
            textSize = 15f
            setTextColor(Color.rgb(244, 218, 155))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
        }
        val state = TextView(this).apply {
            text = "     HP  100     HUNGER  82     WOOD  12     FIBER  08"
            textSize = 13f
            setTextColor(Color.WHITE)
        }
        gyroButton = actionButton("GYRO: OFF") {
            gyroEnabled = gyroSensor != null && !gyroEnabled
            gameView.queueEvent { NativeGameBridge.setGyroEnabled(gyroEnabled) }
            updateGyroButton()
        }
        top.addView(title)
        top.addView(state)
        top.addView(gyroButton, LinearLayout.LayoutParams(142, 44).apply { leftMargin = 18 })
        overlay.addView(top, FrameLayout.LayoutParams(-1, 54, Gravity.TOP))

        val actions = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.BOTTOM or Gravity.END
            setPadding(0, 0, 24, 24)
        }
        val attack = actionButton("ATTACK") { gameView.queueEvent { NativeGameBridge.attack() } }
        val jump = actionButton("JUMP") { gameView.queueEvent { NativeGameBridge.jump() } }
        val dodge = actionButton("DODGE") { gameView.queueEvent { NativeGameBridge.dodge() } }
        val gather = actionButton("GATHER") { gameView.queueEvent { NativeGameBridge.gather() } }
        val craft = actionButton("CRAFT") { gameView.queueEvent { NativeGameBridge.craft() } }
        actions.addView(attack, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(jump, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(dodge, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(gather, LinearLayout.LayoutParams(150, 50).apply { bottomMargin = 6 })
        actions.addView(craft, LinearLayout.LayoutParams(150, 50))
        overlay.addView(actions, FrameLayout.LayoutParams(-1, -1))
        return overlay
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
