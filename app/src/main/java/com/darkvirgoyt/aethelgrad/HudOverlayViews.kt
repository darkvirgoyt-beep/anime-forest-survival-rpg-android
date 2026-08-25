package com.darvirgoyt.aethelgrad

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.DashPathEffect
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.graphics.Shader
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.View
import kotlin.math.cos
import kotlin.math.sin

/** Lightweight native-canvas HUD ornaments that do not block gameplay touch input. */
class AimCrosshairView(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { strokeCap = Paint.Cap.ROUND }

    override fun onDraw(canvas: Canvas) {
        val cx = width / 2f
        val cy = height / 2f
        val unit = width.coerceAtMost(height) / 2f
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = unit * 0.055f
        paint.color = Color.argb(210, 255, 232, 171)
        canvas.drawCircle(cx, cy, unit * 0.30f, paint)
        canvas.drawLine(cx - unit * 0.48f, cy, cx - unit * 0.15f, cy, paint)
        canvas.drawLine(cx + unit * 0.15f, cy, cx + unit * 0.48f, cy, paint)
        canvas.drawLine(cx, cy - unit * 0.48f, cx, cy - unit * 0.15f, paint)
        canvas.drawLine(cx, cy + unit * 0.15f, cx, cy + unit * 0.48f, paint)
        paint.style = Paint.Style.FILL
        paint.color = Color.rgb(255, 226, 158)
        canvas.drawCircle(cx, cy, unit * 0.065f, paint)
    }
}

/** Live player location and heading in the authored 100 x 100 km game-map contract. */
data class WorldMapPlayerState(
    val xKm: Float = 10f,
    val yKm: Float = 16f,
    val yawDegrees: Float = 0f,
    val discoveredMask: Int = 1
)

class AethelgardWorldMapView(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { strokeCap = Paint.Cap.ROUND; strokeJoin = Paint.Join.ROUND }
    private val path = Path()
    private var playerState = WorldMapPlayerState()
    private var zoom = 1f
    private var panX = 0f
    private var panY = 0f
    private var lastX = 0f
    private var lastY = 0f
    private var moved = false
    private val scaleDetector = ScaleGestureDetector(context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
        override fun onScale(detector: ScaleGestureDetector): Boolean {
            zoom = (zoom * detector.scaleFactor).coerceIn(1f, 2.6f)
            invalidate()
            return true
        }
    })

    fun setPlayerState(state: WorldMapPlayerState) {
        playerState = state.copy(xKm = state.xKm.coerceIn(0f, 100f), yKm = state.yKm.coerceIn(0f, 100f))
        invalidate()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        scaleDetector.onTouchEvent(event)
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                lastX = event.x
                lastY = event.y
                moved = false
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                if (event.pointerCount == 1) {
                    val dx = event.x - lastX
                    val dy = event.y - lastY
                    if (kotlin.math.abs(dx) + kotlin.math.abs(dy) > 1f) moved = true
                    panX = (panX + dx).coerceIn(-width * 0.42f, width * 0.42f)
                    panY = (panY + dy).coerceIn(-height * 0.42f, height * 0.42f)
                    lastX = event.x
                    lastY = event.y
                    invalidate()
                }
                return true
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                return true
            }
        }
        return true
    }

    private fun mapRect(canvas: Canvas): RectF {
        val left = width * 0.035f
        val right = width * 0.965f
        val top = height * 0.055f
        val bottom = height * 0.90f
        return RectF(left, top, right, bottom)
    }

    private fun mapX(rect: RectF, xKm: Float): Float = rect.centerX() + (rect.left + rect.width() * (xKm / 100f) - rect.centerX()) * zoom + panX
    private fun mapY(rect: RectF, yKm: Float): Float = rect.centerY() + (rect.bottom - rect.height() * (yKm / 100f) - rect.centerY()) * zoom + panY

    override fun onDraw(canvas: Canvas) {
        val rect = mapRect(canvas)
        paint.style = Paint.Style.FILL
        paint.shader = LinearGradient(rect.left, rect.top, rect.right, rect.bottom, Color.rgb(18, 59, 52), Color.rgb(36, 68, 96), Shader.TileMode.CLAMP)
        canvas.drawRoundRect(rect, 22f, 22f, paint)
        paint.shader = null

        canvas.save()
        canvas.clipRect(rect)
        path.reset()
        path.moveTo(rect.left - 40f, rect.top - 40f)
        path.lineTo(mapX(rect, 33f), rect.top - 40f)
        path.cubicTo(mapX(rect, 29f), mapY(rect, 24f), mapX(rect, 39f), mapY(rect, 49f), mapX(rect, 32f), rect.bottom + 40f)
        path.lineTo(rect.left - 40f, rect.bottom + 40f)
        path.close()
        paint.shader = LinearGradient(rect.left, rect.top, mapX(rect, 38f), rect.bottom, Color.rgb(24, 101, 72), Color.rgb(41, 73, 58), Shader.TileMode.CLAMP)
        canvas.drawPath(path, paint)
        path.reset()
        path.moveTo(mapX(rect, 33f), rect.top - 40f)
        path.lineTo(mapX(rect, 69f), rect.top - 40f)
        path.cubicTo(mapX(rect, 64f), mapY(rect, 23f), mapX(rect, 73f), mapY(rect, 59f), mapX(rect, 67f), rect.bottom + 40f)
        path.lineTo(mapX(rect, 32f), rect.bottom + 40f)
        path.cubicTo(mapX(rect, 39f), mapY(rect, 72f), mapX(rect, 28f), mapY(rect, 43f), mapX(rect, 33f), rect.top - 40f)
        path.close()
        paint.shader = LinearGradient(mapX(rect, 34f), rect.top, mapX(rect, 70f), rect.bottom, Color.rgb(114, 76, 44), Color.rgb(74, 91, 74), Shader.TileMode.CLAMP)
        canvas.drawPath(path, paint)
        paint.shader = null

        // Raised ridge silhouettes create an isometric relief read without importing external terrain.
        drawRelief(canvas, rect, 12f, 30f, 0.16f, Color.argb(72, 8, 39, 35))
        drawRelief(canvas, rect, 55f, 74f, 0.22f, Color.argb(78, 54, 37, 28))
        drawRelief(canvas, rect, 79f, 96f, 0.18f, Color.argb(90, 18, 40, 75))

        // Fictional river corridor and primary route from the existing authored map contract.
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = rect.width() * 0.028f
        paint.color = Color.rgb(47, 151, 164)
        path.reset()
        path.moveTo(mapX(rect, 31f), rect.top - 20f)
        path.cubicTo(mapX(rect, 25f), mapY(rect, 24f), mapX(rect, 41f), mapY(rect, 47f), mapX(rect, 31f), mapY(rect, 58f))
        path.cubicTo(mapX(rect, 23f), mapY(rect, 74f), mapX(rect, 29f), rect.bottom + 20f, mapX(rect, 18f), rect.bottom + 30f)
        canvas.drawPath(path, paint)
        paint.strokeWidth = rect.width() * 0.012f
        paint.color = Color.rgb(238, 195, 103)
        paint.pathEffect = DashPathEffect(floatArrayOf(18f, 12f), 0f)
        path.reset()
        path.moveTo(mapX(rect, 10f), mapY(rect, 16f))
        path.cubicTo(mapX(rect, 20f), mapY(rect, 28f), mapX(rect, 30f), mapY(rect, 47f), mapX(rect, 40f), mapY(rect, 48f))
        path.cubicTo(mapX(rect, 58f), mapY(rect, 50f), mapX(rect, 69f), mapY(rect, 55f), mapX(rect, 88f), mapY(rect, 83f))
        canvas.drawPath(path, paint)
        paint.pathEffect = null

        // Topographic contour bands make the map read as a 3D world rather than flat biome blocks.
        paint.strokeWidth = 1.5f
        paint.color = Color.argb(96, 235, 213, 155)
        for (index in 0..5) {
            val y = 12f + index * 15f
            path.reset()
            path.moveTo(rect.left - 10f, mapY(rect, y))
            path.cubicTo(mapX(rect, 20f), mapY(rect, y + 5f), mapX(rect, 38f), mapY(rect, y - 4f), mapX(rect, 52f), mapY(rect, y + 2f))
            path.cubicTo(mapX(rect, 70f), mapY(rect, y + 7f), rect.right + 10f, mapY(rect, y - 2f), rect.right + 20f, mapY(rect, y + 4f))
            canvas.drawPath(path, paint)
        }

        val landmarks = listOf(
            Triple("CAMP", 10f, 16f), Triple("VILLAGE", 14f, 40f), Triple("CAVE", 26f, 78f),
            Triple("GATE", 40f, 48f), Triple("KILN", 47f, 28f), Triple("OASIS", 55f, 69f),
            Triple("FROST", 74f, 57f), Triple("BASIN", 84f, 59f), Triple("ARENA", 88f, 83f)
        )
        landmarks.forEachIndexed { index, landmark ->
            val discovered = index == 0 || (playerState.discoveredMask and (1 shl (index.coerceAtMost(3)))) != 0
            val x = mapX(rect, landmark.second)
            val y = mapY(rect, landmark.third)
            paint.style = Paint.Style.FILL
            paint.color = if (discovered) Color.rgb(255, 219, 117) else Color.argb(85, 211, 217, 200)
            canvas.drawCircle(x, y, if (index == 0) 9f else 6f, paint)
            paint.color = Color.argb(if (discovered) 210 else 80, 15, 29, 30)
            canvas.drawCircle(x, y, 2.5f, paint)
            if (zoom > 1.25f || index < 3) {
                paint.textAlign = Paint.Align.CENTER
                paint.textSize = 9f
                paint.color = if (discovered) Color.rgb(255, 239, 183) else Color.argb(100, 225, 226, 208)
                canvas.drawText(landmark.first, x, y - 11f, paint)
            }
        }

        val playerX = mapX(rect, playerState.xKm)
        val playerY = mapY(rect, playerState.yKm)
        val heading = Math.toRadians(playerState.yawDegrees.toDouble())
        paint.style = Paint.Style.FILL
        paint.color = Color.argb(75, 255, 233, 140)
        path.reset()
        path.moveTo(playerX, playerY)
        path.lineTo(playerX + cos(heading).toFloat() * 26f - sin(heading).toFloat() * 12f, playerY - sin(heading).toFloat() * 26f - cos(heading).toFloat() * 12f)
        path.lineTo(playerX + cos(heading).toFloat() * 26f + sin(heading).toFloat() * 12f, playerY - sin(heading).toFloat() * 26f + cos(heading).toFloat() * 12f)
        path.close()
        canvas.drawPath(path, paint)
        paint.color = Color.rgb(255, 241, 177)
        canvas.drawCircle(playerX, playerY, 10f, paint)
        paint.color = Color.rgb(24, 39, 39)
        canvas.drawCircle(playerX, playerY, 4f, paint)
        canvas.restore()

        paint.style = Paint.Style.STROKE
        paint.strokeWidth = 2.5f
        paint.color = Color.rgb(231, 195, 111)
        canvas.drawRoundRect(rect, 22f, 22f, paint)
        paint.style = Paint.Style.FILL
        paint.textAlign = Paint.Align.CENTER
        paint.textSize = 13f
        paint.color = Color.rgb(255, 235, 172)
        canvas.drawText("N", rect.centerX(), rect.top + 19f, paint)
        paint.textSize = 10f
        paint.color = Color.rgb(225, 215, 184)
        canvas.drawText("DRAG TO PAN  •  PINCH TO ZOOM", rect.centerX(), height * 0.965f, paint)
        paint.textAlign = Paint.Align.LEFT
        canvas.drawText("VERDANT VEIL", rect.left + 14f, rect.bottom - 12f, paint)
        paint.textAlign = Paint.Align.CENTER
        canvas.drawText("EMBER ROAD", rect.centerX(), rect.bottom - 12f, paint)
        paint.textAlign = Paint.Align.RIGHT
        canvas.drawText("FROSTWAKE", rect.right - 14f, rect.bottom - 12f, paint)
    }

    private fun drawRelief(canvas: Canvas, rect: RectF, startX: Float, endX: Float, heightFactor: Float, color: Int) {
        paint.style = Paint.Style.FILL
        paint.color = color
        path.reset()
        path.moveTo(mapX(rect, startX), rect.bottom + 20f)
        path.lineTo(mapX(rect, startX + 5f), mapY(rect, 34f))
        path.lineTo(mapX(rect, startX + 12f), mapY(rect, 53f + heightFactor * 20f))
        path.lineTo(mapX(rect, startX + 20f), mapY(rect, 41f))
        path.lineTo(mapX(rect, endX), rect.bottom + 20f)
        path.close()
        canvas.drawPath(path, paint)
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = 1.2f
        paint.color = Color.argb(90, 245, 220, 164)
        for (line in 0..2) {
            path.reset()
            path.moveTo(mapX(rect, startX + 4f), mapY(rect, 34f + line * 6f))
            path.cubicTo(mapX(rect, startX + 10f), mapY(rect, 28f + line * 7f), mapX(rect, startX + 17f), mapY(rect, 48f + line * 5f), mapX(rect, endX - 3f), mapY(rect, 39f + line * 4f))
            canvas.drawPath(path, paint)
        }
    }
}

class CircularMiniMapView(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val path = Path()
    private var playerXKm = 10f
    private var playerYKm = 16f
    private var yawDegrees = 0f

    fun setPlayerState(state: WorldMapPlayerState) {
        playerXKm = state.xKm.coerceIn(0f, 100f)
        playerYKm = state.yKm.coerceIn(0f, 100f)
        yawDegrees = state.yawDegrees
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        val size = width.coerceAtMost(height).toFloat()
        val cx = width / 2f
        val cy = height / 2f
        val radius = size * 0.44f
        paint.style = Paint.Style.FILL
        paint.color = Color.argb(225, 6, 20, 27)
        canvas.drawCircle(cx, cy, radius, paint)
        canvas.save()
        canvas.clipPath(Path().apply { addCircle(cx, cy, radius * 0.90f, Path.Direction.CW) })
        paint.color = Color.rgb(18, 83, 74)
        canvas.drawRect(cx - radius, cy - radius, cx, cy + radius, paint)
        paint.color = Color.rgb(96, 59, 46)
        canvas.drawRect(cx, cy - radius, cx + radius, cy + radius, paint)
        paint.color = Color.rgb(47, 91, 132)
        canvas.drawRect(cx - radius * 0.18f, cy - radius, cx + radius * 0.10f, cy + radius, paint)
        paint.color = Color.argb(160, 30, 120, 133)
        canvas.drawRect(cx - radius * 0.55f, cy - radius, cx - radius * 0.35f, cy + radius, paint)
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = size * 0.018f
        paint.color = Color.argb(170, 238, 199, 113)
        path.reset()
        path.moveTo(cx - radius, cy + radius * 0.10f)
        path.cubicTo(cx - radius * 0.40f, cy - radius * 0.06f, cx + radius * 0.25f, cy + radius * 0.20f, cx + radius, cy - radius * 0.12f)
        canvas.drawPath(path, paint)
        val px = cx - radius * 0.90f + radius * 1.80f * (playerXKm / 100f)
        val py = cy + radius * 0.90f - radius * 1.80f * (playerYKm / 100f)
        val heading = Math.toRadians(yawDegrees.toDouble())
        paint.style = Paint.Style.FILL
        paint.color = Color.argb(80, 255, 235, 171)
        path.reset()
        path.moveTo(px, py)
        path.lineTo(px + cos(heading).toFloat() * 18f, py - sin(heading).toFloat() * 18f)
        path.lineTo(px + cos(heading + 2.55).toFloat() * 8f, py - sin(heading + 2.55).toFloat() * 8f)
        path.close()
        canvas.drawPath(path, paint)
        paint.color = Color.rgb(255, 235, 171)
        canvas.drawCircle(px, py, radius * 0.095f, paint)
        canvas.restore()
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = size * 0.035f
        paint.color = Color.rgb(229, 193, 110)
        canvas.drawCircle(cx, cy, radius, paint)
        paint.style = Paint.Style.FILL
        paint.color = Color.rgb(255, 228, 163)
        paint.textSize = size * 0.13f
        paint.textAlign = Paint.Align.CENTER
        canvas.drawText("N", cx, cy - radius * 0.67f, paint)
    }
}

class VitalMeterView(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private var health = 100
    private var stamina = 100
    private var hunger = 100

    fun updateVitals(nextHealth: Int, nextStamina: Int, nextHunger: Int = 100) {
        health = nextHealth.coerceIn(0, 100)
        stamina = nextStamina.coerceIn(0, 100)
        hunger = nextHunger.coerceIn(0, 100)
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        val widthF = width.toFloat()
        val pad = widthF * 0.035f
        val labelWidth = widthF * 0.16f
        val trackLeft = labelWidth + pad
        val trackRight = widthF - pad
        val trackWidth = trackRight - trackLeft
        val barHeight = height * 0.17f
        val topHealth = height * 0.08f
        val topStamina = height * 0.41f
        val topHunger = height * 0.74f
        paint.style = Paint.Style.FILL
        paint.color = Color.argb(205, 8, 19, 25)
        canvas.drawRoundRect(RectF(0f, 0f, widthF, height.toFloat()), height * 0.18f, height * 0.18f, paint)
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = height * 0.025f
        paint.color = Color.rgb(106, 193, 181)
        canvas.drawRoundRect(RectF(0f, 0f, widthF, height.toFloat()), height * 0.18f, height * 0.18f, paint)

        drawMeter(canvas, "HP", health, topHealth, barHeight, trackLeft, trackWidth, if (health <= 30) Color.rgb(255, 130, 94) else Color.rgb(218, 70, 72))
        drawMeter(canvas, "STA", stamina, topStamina, barHeight, trackLeft, trackWidth, Color.rgb(92, 218, 190))
        drawMeter(canvas, "HUN", hunger, topHunger, barHeight, trackLeft, trackWidth, Color.rgb(78, 166, 220))
    }

    private fun drawMeter(canvas: Canvas, label: String, value: Int, top: Float, heightF: Float, left: Float, trackWidth: Float, fillColor: Int) {
        val radius = heightF / 2f
        paint.style = Paint.Style.FILL
        paint.color = Color.argb(185, 29, 43, 48)
        canvas.drawRoundRect(RectF(left, top, left + trackWidth, top + heightF), radius, radius, paint)
        if (value > 0) {
            paint.color = fillColor
            canvas.drawRoundRect(RectF(left, top, left + trackWidth * (value / 100f), top + heightF), radius, radius, paint)
        }
        paint.textAlign = Paint.Align.LEFT
        paint.textSize = heightF * 0.78f
        paint.typeface = android.graphics.Typeface.DEFAULT_BOLD
        paint.color = Color.rgb(250, 235, 196)
        canvas.drawText(label, left * 0.18f, top + heightF * 0.77f, paint)
        paint.textAlign = Paint.Align.RIGHT
        paint.textSize = heightF * 0.68f
        paint.color = Color.WHITE
        canvas.drawText("$value", left + trackWidth - heightF * 0.35f, top + heightF * 0.72f, paint)
    }
}
