package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.drawable.Drawable
import android.view.MotionEvent
import android.view.View
import androidx.core.content.ContextCompat

/**
 * Botão compacto de disparo rápido (Quick Action) com ícone lineart e feedback visual.
 */
class QuickActionButton(
    context: Context,
    val iconResId: Int,
    val label: String,
    val action: Byte = 0,
    val onClickCallback: (() -> Unit)? = null
) : View(context) {

    private val bgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.parseColor("#1F1F24")
    }

    private val borderPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 2.5f
        color = Color.parseColor("#32323A")
    }

    private val labelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.parseColor("#E0E0E8")
        textSize = 18f
        isFakeBoldText = true
        textAlign = Paint.Align.CENTER
    }

    private val icon: Drawable? = ContextCompat.getDrawable(context, iconResId)?.mutate()
    private val rectF = RectF()
    private var isPressedState = false

    var onActionEmitted: ((Byte) -> Unit)? = null

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        rectF.set(4f, 4f, w - 4f, h - 4f)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                isPressedState = true
                borderPaint.color = Color.parseColor("#00E5FF")
                invalidate()
                return true
            }

            MotionEvent.ACTION_UP -> {
                if (isPressedState) {
                    isPressedState = false
                    borderPaint.color = Color.parseColor("#32323A")
                    invalidate()

                    if (action != 0.toByte()) {
                        onActionEmitted?.invoke(action)
                    }
                    onClickCallback?.invoke()
                }
                return true
            }

            MotionEvent.ACTION_CANCEL -> {
                isPressedState = false
                borderPaint.color = Color.parseColor("#32323A")
                invalidate()
                return true
            }
        }
        return super.onTouchEvent(event)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val corner = 12f

        // Fundo e borda
        canvas.drawRoundRect(rectF, corner, corner, bgPaint)
        canvas.drawRoundRect(rectF, corner, corner, borderPaint)

        // Desenho do ícone lineart
        icon?.let {
            val iconSize = (height * 0.42f).toInt()
            val left = (width - iconSize) / 2
            val top = (height * 0.16f).toInt()
            it.setBounds(left, top, left + iconSize, top + iconSize)
            it.draw(canvas)
        }

        // Rótulo textual
        val textY = height * 0.82f
        canvas.drawText(label, width / 2f, textY, labelPaint)
    }
}

