package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.DashPathEffect
import android.graphics.Paint
import android.view.View

/**
 * Camada de sobreposição transparente que desenha a pré-visualização gráfica do raio do pincel
 * em tempo real enquanto o polegar esquerdo ajusta o slider de raio.
 */
class RadiusOverlayView(context: Context) : View(context) {

    private val circlePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 3f
        color = Color.parseColor("#00E5FF")
    }

    private val dashedPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 2f
        color = Color.parseColor("#8000E5FF")
        pathEffect = DashPathEffect(floatArrayOf(10f, 10f), 0f)
    }

    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.WHITE
        textSize = 28f
        isFakeBoldText = true
        textAlign = Paint.Align.CENTER
    }

    private var currentRadius = 0f
    private var isVisibleState = false

    fun showRadius(radiusPx: Float) {
        currentRadius = radiusPx
        isVisibleState = true
        visibility = VISIBLE
        invalidate()
    }

    fun hideRadius() {
        isVisibleState = false
        visibility = GONE
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if (!isVisibleState || currentRadius <= 0f) return

        val centerX = width / 2f
        val centerY = height / 2f

        // Círculo principal do raio
        canvas.drawCircle(centerX, centerY, currentRadius, circlePaint)
        // Linha de diâmetro tracejada
        canvas.drawLine(centerX - currentRadius, centerY, centerX + currentRadius, centerY, dashedPaint)

        // Rótulo numérico
        val textY = centerY - currentRadius - 18f
        canvas.drawText("Raio: ${currentRadius.toInt()} px", centerX, textY, textPaint)
    }
}

