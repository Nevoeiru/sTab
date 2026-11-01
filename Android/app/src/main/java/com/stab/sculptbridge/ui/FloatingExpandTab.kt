package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.drawable.GradientDrawable
import android.util.AttributeSet
import android.view.Gravity
import android.view.View
import android.widget.FrameLayout

/**
 * Aba flutuante retrátil ultra-discreta na borda da tela.
 * Permite restaurar a barra lateral SculptPad quando ela estiver minimizada,
 * maximizando 100% da viewport para o Blender 3D.
 */
class FloatingExpandTab @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    var isLeftEdge: Boolean = true
        set(value) {
            field = value
            updateShape()
            invalidate()
        }

    var onExpandClicked: (() -> Unit)? = null

    private val chevronPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 3.0f
        strokeCap = Paint.Cap.ROUND
        strokeJoin = Paint.Join.ROUND
        color = Color.parseColor("#00E5FF")
    }

    private val chevronPath = Path()

    init {
        isClickable = true
        isFocusable = true
        updateShape()

        setOnClickListener {
            performHapticFeedback(android.view.HapticFeedbackConstants.KEYBOARD_TAP)
            onExpandClicked?.invoke()
        }
    }

    private fun updateShape() {
        val density = resources.displayMetrics.density
        val r = 10f * density

        val bg = GradientDrawable().apply {
            if (isLeftEdge) {
                // Cantos arredondados na direita
                cornerRadii = floatArrayOf(0f, 0f, r, r, r, r, 0f, 0f)
            } else {
                // Cantos arredondados na esquerda
                cornerRadii = floatArrayOf(r, r, 0f, 0f, 0f, 0f, r, r)
            }
            setColor(Color.parseColor("#E61E2028"))
            setStroke((1.5f * density).toInt(), Color.parseColor("#00E5FF"))
        }
        background = bg
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()

        chevronPath.reset()
        if (isLeftEdge) {
            // Seta apontando para a direita '>' (Expandir)
            val cx = w * 0.42f
            val cy = h * 0.5f
            val size = 9f
            chevronPath.moveTo(cx - size * 0.5f, cy - size)
            chevronPath.lineTo(cx + size * 0.5f, cy)
            chevronPath.lineTo(cx - size * 0.5f, cy + size)
        } else {
            // Seta apontando para a esquerda '<' (Expandir)
            val cx = w * 0.58f
            val cy = h * 0.5f
            val size = 9f
            chevronPath.moveTo(cx + size * 0.5f, cy - size)
            chevronPath.lineTo(cx - size * 0.5f, cy)
            chevronPath.lineTo(cx + size * 0.5f, cy + size)
        }

        canvas.drawPath(chevronPath, chevronPaint)
    }
}

