package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.view.MotionEvent
import android.view.View

/**
 * Controle deslizante vertical tátil (Touch Slider) para Raio ('F') e Força ('Shift+F').
 * Permite que o polegar esquerdo deslize verticalmente para ajuste contínuo e preciso.
 */
class TouchSliderView(
    context: Context,
    val label: String,
    val triggerAction: Byte,
    val minValue: Float = 10f,
    val maxValue: Float = 500f,
    initialValue: Float = 120f,
    val activeColorHex: String = "#00E5FF"
) : View(context) {

    private val trackPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 6f
        strokeCap = Paint.Cap.ROUND
        color = Color.parseColor("#32323A")
    }

    private val activeTrackPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 6f
        strokeCap = Paint.Cap.ROUND
        color = Color.parseColor(activeColorHex)
    }

    private val thumbPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.parseColor(activeColorHex)
    }

    private val labelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.parseColor("#A0A0B0")
        textSize = 18f
        isFakeBoldText = true
        textAlign = Paint.Align.CENTER
    }

    private val valuePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.WHITE
        textSize = 22f
        isFakeBoldText = true
        textAlign = Paint.Align.CENTER
    }

    var currentValue: Float = initialValue
        private set

    var onValueChanged: ((Float, Boolean) -> Unit)? = null
    var onActionEmitted: ((Byte) -> Unit)? = null

    private var lastTouchY = 0f
    private var isDragging = false
    private val thumbRect = RectF()

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                isDragging = true
                lastTouchY = event.y
                onActionEmitted?.invoke(triggerAction)
                updateValueFromTouch(event.y)
                return true
            }

            MotionEvent.ACTION_MOVE -> {
                if (isDragging) {
                    val dy = lastTouchY - event.y
                    lastTouchY = event.y
                    // Sensibilidade de arrasto suave
                    val deltaRatio = dy / (height * 0.7f)
                    val valueRange = maxValue - minValue
                    currentValue = (currentValue + (deltaRatio * valueRange)).coerceIn(minValue, maxValue)

                    onValueChanged?.invoke(currentValue, true)
                    invalidate()
                }
                return true
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                if (isDragging) {
                    isDragging = false
                    onValueChanged?.invoke(currentValue, false)
                    invalidate()
                }
                return true
            }
        }
        return super.onTouchEvent(event)
    }

    private fun updateValueFromTouch(touchY: Float) {
        val trackTop = height * 0.25f
        val trackBottom = height * 0.85f
        val trackHeight = trackBottom - trackTop

        val clampedY = touchY.coerceIn(trackTop, trackBottom)
        val progress = 1f - ((clampedY - trackTop) / trackHeight)
        currentValue = minValue + (progress * (maxValue - minValue))
        onValueChanged?.invoke(currentValue, true)
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        val centerX = width / 2f
        val trackTop = height * 0.25f
        val trackBottom = height * 0.85f
        val trackHeight = trackBottom - trackTop

        // Rótulo superior
        canvas.drawText(label, centerX, trackTop - 36f, labelPaint)
        // Valor numérico atual
        val displayVal = if (maxValue <= 1f) String.format("%.2f", currentValue) else "${currentValue.toInt()}px"
        canvas.drawText(displayVal, centerX, trackTop - 12f, valuePaint)

        // Trilha inativa de fundo
        canvas.drawLine(centerX, trackTop, centerX, trackBottom, trackPaint)

        // Posição do thumb proporcional ao valor
        val progress = ((currentValue - minValue) / (maxValue - minValue)).coerceIn(0f, 1f)
        val thumbY = trackBottom - (progress * trackHeight)

        // Trilha ativa destacada
        canvas.drawLine(centerX, thumbY, centerX, trackBottom, activeTrackPaint)

        // Thumb circular com anel
        canvas.drawCircle(centerX, thumbY, 14f, thumbPaint)
        if (isDragging) {
            val haloPaint = Paint(thumbPaint).apply { alpha = 80 }
            canvas.drawCircle(centerX, thumbY, 22f, haloPaint)
        }
    }
}

