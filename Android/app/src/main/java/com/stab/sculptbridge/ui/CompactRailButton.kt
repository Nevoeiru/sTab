package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.util.AttributeSet
import android.view.Gravity
import android.view.MotionEvent
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.TextView

/**
 * Botão modular de alta densidade para a barra lateral compacta (SculptPad Rail).
 * Suporta ícones lineart vetoriais e texto curto para modificadores de teclado (Shift, Ctrl, Alt).
 */
class CompactRailButton @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private val iconView = ImageView(context)
    private val textView = TextView(context)

    var activeColorHex: String = "#00E5FF" // Ciano por padrão
    var isHoldModifier: Boolean = false
    var onHoldDown: (() -> Unit)? = null
    var onHoldUp: (() -> Unit)? = null
    var onActionClick: (() -> Unit)? = null

    private var isPressedState = false

    init {
        isClickable = true
        isFocusable = true

        val density = resources.displayMetrics.density
        val iconSize = (22 * density).toInt()

        iconView.layoutParams = LayoutParams(iconSize, iconSize, Gravity.CENTER)
        iconView.scaleType = ImageView.ScaleType.FIT_CENTER
        addView(iconView)

        textView.layoutParams = LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT, Gravity.CENTER)
        textView.textSize = 11f
        textView.setTextColor(Color.parseColor("#E0E0E6"))
        textView.typeface = android.graphics.Typeface.DEFAULT_BOLD
        textView.visibility = GONE
        addView(textView)

        updateVisualState(isActive = false)
    }

    fun setIconResource(resId: Int) {
        iconView.setImageResource(resId)
        iconView.visibility = VISIBLE
        textView.visibility = GONE
    }

    fun setText(text: String, colorHex: String = "#E0E0E6") {
        textView.text = text
        textView.setTextColor(Color.parseColor(colorHex))
        textView.visibility = VISIBLE
        iconView.visibility = GONE
    }

    fun updateVisualState(isActive: Boolean) {
        val density = resources.displayMetrics.density
        val cornerPx = 8f * density

        val bg = GradientDrawable().apply {
            cornerRadius = cornerPx
            if (isActive) {
                setColor(Color.parseColor("#282A36"))
                setStroke((1.8f * density).toInt(), Color.parseColor(activeColorHex))
            } else {
                setColor(Color.parseColor("#1B1C22"))
                setStroke((1.0f * density).toInt(), Color.parseColor("#2D2E38"))
            }
        }
        background = bg

        if (iconView.visibility == VISIBLE) {
            val tint = if (isActive) Color.parseColor(activeColorHex) else Color.parseColor("#C8C9D4")
            iconView.setColorFilter(tint)
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (isHoldModifier) {
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    isPressedState = true
                    updateVisualState(isActive = true)
                    performHapticFeedback(android.view.HapticFeedbackConstants.VIRTUAL_KEY)
                    onHoldDown?.invoke()
                    return true
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    isPressedState = false
                    updateVisualState(isActive = false)
                    onHoldUp?.invoke()
                    return true
                }
            }
            return true
        } else {
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    isPressedState = true
                    updateVisualState(isActive = true)
                    performHapticFeedback(android.view.HapticFeedbackConstants.KEYBOARD_TAP)
                }
                MotionEvent.ACTION_UP -> {
                    isPressedState = false
                    updateVisualState(isActive = false)
                    onActionClick?.invoke()
                    performClick()
                }
                MotionEvent.ACTION_CANCEL -> {
                    isPressedState = false
                    updateVisualState(isActive = false)
                }
            }
            return true
        }
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }
}

