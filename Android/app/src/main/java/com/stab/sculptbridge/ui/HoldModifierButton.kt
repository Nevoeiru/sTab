package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.view.MotionEvent
import android.view.View
import com.stab.sculptbridge.Protocol

/**
 * Botão tátil de retenção contínua (Hold-to-Modify) para o polegar esquerdo.
 * Suporta multitouch independente, permitindo segurar Shift/Ctrl enquanto a S-Pen esculpe.
 */
class HoldModifierButton(
    context: Context,
    val title: String,
    val subtitle: String,
    val downAction: Byte,
    val upAction: Byte,
    val activeColorHex: String = "#00E5FF"
) : View(context) {

    private val bgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.parseColor("#1F1F24")
    }

    private val borderPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 3f
        color = Color.parseColor("#32323A")
    }

    private val activeBgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.parseColor(activeColorHex)
        alpha = 50 // Fundo translúcido quando ativo
    }

    private val titlePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.WHITE
        textSize = 34f
        isFakeBoldText = true
        textAlign = Paint.Align.CENTER
    }

    private val subtitlePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.parseColor("#A0A0B0")
        textSize = 20f
        textAlign = Paint.Align.CENTER
    }

    private val rectF = RectF()
    private var isHeld = false
    private var activePointerId = -1

    var onActionEmitted: ((Byte) -> Unit)? = null

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        rectF.set(6f, 6f, w - 6f, h - 6f)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val action = event.actionMasked
        val index = event.actionIndex

        when (action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                if (!isHeld) {
                    activePointerId = event.getPointerId(index)
                    setHeldState(true)
                }
                return true
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                if (isHeld && event.getPointerId(index) == activePointerId) {
                    setHeldState(false)
                    activePointerId = -1
                }
                return true
            }

            MotionEvent.ACTION_CANCEL -> {
                if (isHeld) {
                    setHeldState(false)
                    activePointerId = -1
                }
                return true
            }
        }
        return super.onTouchEvent(event)
    }

    private fun setHeldState(held: Boolean) {
        if (isHeld == held) return
        isHeld = held

        if (isHeld) {
            borderPaint.color = Color.parseColor(activeColorHex)
            titlePaint.color = Color.parseColor(activeColorHex)
            onActionEmitted?.invoke(downAction)
        } else {
            borderPaint.color = Color.parseColor("#32323A")
            titlePaint.color = Color.WHITE
            onActionEmitted?.invoke(upAction)
        }
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val cornerRadius = 16f

        // Fundo do botão
        canvas.drawRoundRect(rectF, cornerRadius, cornerRadius, bgPaint)
        if (isHeld) {
            canvas.drawRoundRect(rectF, cornerRadius, cornerRadius, activeBgPaint)
        }

        // Borda lineart
        canvas.drawRoundRect(rectF, cornerRadius, cornerRadius, borderPaint)

        // Textos centralizados
        val centerY = rectF.centerY()
        canvas.drawText(title, rectF.centerX(), centerY - 6f, titlePaint)
        canvas.drawText(subtitle, rectF.centerX(), centerY + 26f, subtitlePaint)
    }
}

