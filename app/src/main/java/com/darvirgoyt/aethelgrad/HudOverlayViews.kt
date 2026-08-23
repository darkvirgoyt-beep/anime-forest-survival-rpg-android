package com.darvirgoyt.aethelgrad

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
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
        paint.color = Color.rgb(20, 77, 65)
        canvas.drawRect(cx - radius, cy - radius, cx, cy + radius, paint)
        paint.color = Color.rgb(102, 66, 35)
        canvas.drawRect(cx, cy - radius, cx + radius, cy + radius, paint)
        paint.color = Color.rgb(46, 92, 122)
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
