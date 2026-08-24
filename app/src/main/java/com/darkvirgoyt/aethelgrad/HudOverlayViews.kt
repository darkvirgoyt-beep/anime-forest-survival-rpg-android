package com.darkvirgoyt.aethelgrad

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.view.View

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

class AethelgardWorldMapView(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val path = Path()

    override fun onDraw(canvas: Canvas) {
        val left = width * 0.04f
        val right = width * 0.96f
        val top = height * 0.06f
        val bottom = height * 0.90f
        val mapWidth = right - left
        val mapHeight = bottom - top

        paint.style = Paint.Style.FILL
        paint.color = Color.rgb(9, 24, 31)
        canvas.drawRoundRect(RectF(left, top, right, bottom), 18f, 18f, paint)
        canvas.save()
        canvas.clipRect(left, top, right, bottom)
        paint.color = Color.rgb(22, 75, 61)
        canvas.drawRect(left, top, left + mapWidth * 0.34f, bottom, paint)
        paint.color = Color.rgb(100, 64, 34)
        canvas.drawRect(left + mapWidth * 0.34f, top, left + mapWidth * 0.68f, bottom, paint)
        paint.color = Color.rgb(42, 82, 112)
        canvas.drawRect(left + mapWidth * 0.68f, top, right, bottom, paint)

        paint.color = Color.rgb(42, 119, 112)
        path.reset()
        path.moveTo(left + mapWidth * 0.63f, top)
        path.cubicTo(left + mapWidth * 0.57f, top + mapHeight * 0.23f, left + mapWidth * 0.75f, top + mapHeight * 0.42f, left + mapWidth * 0.61f, bottom)
        path.lineTo(left + mapWidth * 0.70f, bottom)
        path.cubicTo(left + mapWidth * 0.84f, top + mapHeight * 0.46f, left + mapWidth * 0.66f, top + mapHeight * 0.26f, left + mapWidth * 0.71f, top)
        path.close()
        canvas.drawPath(path, paint)

        paint.color = Color.rgb(218, 174, 79)
        paint.strokeWidth = 7f
        paint.style = Paint.Style.STROKE
        path.reset()
        path.moveTo(left, top + mapHeight * 0.57f)
        path.cubicTo(left + mapWidth * 0.25f, top + mapHeight * 0.48f, left + mapWidth * 0.64f, top + mapHeight * 0.63f, right, top + mapHeight * 0.46f)
        canvas.drawPath(path, paint)
        paint.style = Paint.Style.FILL

        val landmarks = arrayOf(
            0.10f to 0.18f, 0.18f to 0.42f, 0.31f to 0.76f,
            0.47f to 0.34f, 0.56f to 0.70f, 0.78f to 0.58f,
            0.88f to 0.78f
        )
        paint.color = Color.rgb(255, 220, 113)
        landmarks.forEach { (x, y) -> canvas.drawCircle(left + mapWidth * x, top + mapHeight * y, 5f, paint) }
        paint.color = Color.rgb(255, 236, 170)
        canvas.drawCircle(left + mapWidth * 0.47f, top + mapHeight * 0.52f, 9f, paint)
        paint.color = Color.rgb(11, 25, 31)
        canvas.drawCircle(left + mapWidth * 0.47f, top + mapHeight * 0.52f, 4f, paint)
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = 2f
        paint.color = Color.rgb(231, 195, 111)
        canvas.drawRoundRect(RectF(left, top, right, bottom), 18f, 18f, paint)
        canvas.restore()

        paint.style = Paint.Style.FILL
        paint.textAlign = Paint.Align.CENTER
        paint.textSize = 12f
        paint.color = Color.rgb(255, 235, 172)
        canvas.drawText("N", left + mapWidth * 0.50f, top + 18f, paint)
        paint.textAlign = Paint.Align.LEFT
        paint.textSize = 11f
        paint.color = Color.rgb(231, 215, 174)
        canvas.drawText("FOREST", left + 12f, bottom - 12f, paint)
        paint.textAlign = Paint.Align.CENTER
        canvas.drawText("SAND", left + mapWidth * 0.50f, bottom - 12f, paint)
        paint.textAlign = Paint.Align.RIGHT
        canvas.drawText("SNOW", right - 12f, bottom - 12f, paint)
    }
}

class CircularMiniMapView(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)

    override fun onDraw(canvas: Canvas) {
        val size = width.coerceAtMost(height).toFloat()
        val cx = width / 2f
        val cy = height / 2f
        val radius = size * 0.44f
        paint.style = Paint.Style.FILL
        paint.color = Color.argb(220, 6, 20, 27)
        canvas.drawCircle(cx, cy, radius, paint)
        canvas.save()
        canvas.clipPath(android.graphics.Path().apply { addCircle(cx, cy, radius * 0.90f, android.graphics.Path.Direction.CW) })
        paint.color = Color.rgb(18, 83, 74)
        canvas.drawRect(cx - radius, cy - radius, cx, cy + radius, paint)
        paint.color = Color.rgb(96, 59, 46)
        canvas.drawRect(cx, cy - radius, cx + radius, cy + radius, paint)
        paint.color = Color.rgb(47, 91, 132)
        canvas.drawRect(cx - radius * 0.18f, cy - radius, cx + radius * 0.10f, cy + radius, paint)
        paint.color = Color.rgb(221, 183, 91)
        canvas.drawRect(cx - radius * 0.78f, cy - radius * 0.06f, cx + radius * 0.78f, cy + radius * 0.06f, paint)
        paint.color = Color.rgb(255, 235, 171)
        canvas.drawCircle(cx, cy, radius * 0.095f, paint)
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

    fun updateVitals(nextHealth: Int, nextStamina: Int) {
        health = nextHealth.coerceIn(0, 100)
        stamina = nextStamina.coerceIn(0, 100)
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        val widthF = width.toFloat()
        val pad = widthF * 0.035f
        val labelWidth = widthF * 0.16f
        val trackLeft = labelWidth + pad
        val trackRight = widthF - pad
        val trackWidth = trackRight - trackLeft
        val barHeight = height * 0.22f
        val topHealth = height * 0.16f
        val topStamina = height * 0.59f
        paint.style = Paint.Style.FILL
        paint.color = Color.argb(205, 8, 19, 25)
        canvas.drawRoundRect(RectF(0f, 0f, widthF, height.toFloat()), height * 0.18f, height * 0.18f, paint)
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = height * 0.025f
        paint.color = Color.rgb(106, 193, 181)
        canvas.drawRoundRect(RectF(0f, 0f, widthF, height.toFloat()), height * 0.18f, height * 0.18f, paint)

        drawMeter(canvas, "HP", health, topHealth, barHeight, trackLeft, trackWidth, if (health <= 30) Color.rgb(255, 130, 94) else Color.rgb(218, 70, 72))
        drawMeter(canvas, "STA", stamina, topStamina, barHeight, trackLeft, trackWidth, Color.rgb(92, 218, 190))
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
