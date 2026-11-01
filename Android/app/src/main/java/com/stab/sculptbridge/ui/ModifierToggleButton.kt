package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.MotionEvent
import android.widget.FrameLayout
import android.widget.TextView

/**
 * Componente modular especializado para modificadores táteis de alternância (Liga / Desliga).
 * Projetado para os botões Shift (Suavizar), Ctrl (Inverter) e Alt (Máscara).
 *
 * Imune ao cancelamento de toque (ACTION_CANCEL) disparado pelo hardware Wacom/S-Pen
 * quando a ponta da caneta entra em contato com o vidro.
 */
class ModifierToggleButton @JvmOverloads constructor(
    context: Context,
    val title: String,
    val activeColorHex: String = "#00E5FF"
) : FrameLayout(context) {

    private val textView = TextView(context)
    private var isActiveState = false

    var tooltipText: String = ""
    var onToggleChanged: ((isActive: Boolean) -> Unit)? = null
    var onHoverChanged: ((isHovering: Boolean, button: ModifierToggleButton) -> Unit)? = null

    init {
        isClickable = true
        isFocusable = true

        textView.layoutParams = LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT, Gravity.CENTER)
        textView.textSize = 12f
        textView.setTextColor(Color.parseColor("#E0E0E6"))
        textView.typeface = android.graphics.Typeface.DEFAULT_BOLD
        addView(textView)

        updateVisualState(false)
    }

    fun isModifierActive(): Boolean = isActiveState

    fun setModifierActive(active: Boolean, triggerCallback: Boolean = true) {
        if (isActiveState != active) {
            isActiveState = active
            updateVisualState(isActiveState)
            if (triggerCallback) {
                onToggleChanged?.invoke(isActiveState)
            }
        }
    }

    private fun updateVisualState(isActive: Boolean) {
        val density = resources.displayMetrics.density
        val cornerPx = 8f * density

        val bg = GradientDrawable().apply {
            cornerRadius = cornerPx
            if (isActive) {
                setColor(Color.parseColor("#282A36"))
                setStroke((2.5f * density).toInt(), Color.parseColor(activeColorHex))
            } else {
                setColor(Color.parseColor("#1B1C22"))
                setStroke((1.0f * density).toInt(), Color.parseColor("#2D2E38"))
            }
        }
        background = bg

        val color = if (isActive) Color.parseColor(activeColorHex) else Color.parseColor("#C0C0D0")
        textView.setTextColor(color)
        textView.text = if (isActive) "\u25CF $title" else title
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                isActiveState = !isActiveState
                updateVisualState(isActiveState)
                performHapticFeedback(android.view.HapticFeedbackConstants.VIRTUAL_KEY)
                onToggleChanged?.invoke(isActiveState)
                return true
            }

            MotionEvent.ACTION_UP -> {
                // Toque finalizado com sucesso; estado permanece como alternado
                return true
            }

            MotionEvent.ACTION_CANCEL -> {
                // Quando a S-Pen toca o vidro, o palm rejection do hardware cancela o toque de dedo.
                // O estado alternado é estritamente preservado para o artista continuar esculpindo!
                return true
            }
        }
        return super.onTouchEvent(event)
    }

    override fun onHoverEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_HOVER_ENTER -> {
                onHoverChanged?.invoke(true, this)
                return true
            }
            MotionEvent.ACTION_HOVER_EXIT -> {
                onHoverChanged?.invoke(false, this)
                return true
            }
        }
        return super.onHoverEvent(event)
    }
}

