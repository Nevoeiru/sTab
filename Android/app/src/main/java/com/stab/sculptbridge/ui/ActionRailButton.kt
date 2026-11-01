package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.MotionEvent
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.TextView

/**
 * Componente modular especializado para botões de atalho de clique simples e instantâneo.
 * Suporta ícones lineart vetoriais ou textos curtos para teclas simples do Blender.
 */
class ActionRailButton @JvmOverloads constructor(
    context: Context,
    val activeColorHex: String = "#00E5FF"
) : FrameLayout(context) {

    private val iconView = ImageView(context)
    private val textView = TextView(context)

    var tooltipText: String = ""
    var onActionClick: (() -> Unit)? = null
    var onHoverChanged: ((isHovering: Boolean, button: ActionRailButton) -> Unit)? = null

    init {
        isClickable = true
        isFocusable = true

        val density = resources.displayMetrics.density
        val iconPad = (10 * density).toInt()

        iconView.layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT, Gravity.CENTER)
        iconView.setPadding(iconPad, iconPad, iconPad, iconPad)
        iconView.scaleType = ImageView.ScaleType.FIT_CENTER
        iconView.visibility = GONE
        addView(iconView)

        textView.layoutParams = LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT, Gravity.CENTER)
        textView.textSize = 12f
        textView.setTextColor(Color.parseColor("#E0E0E6"))
        textView.typeface = android.graphics.Typeface.DEFAULT_BOLD
        textView.visibility = GONE
        addView(textView)

        updateVisualState(false)
    }

    fun setIconResource(resId: Int) {
        iconView.setImageResource(resId)
        iconView.visibility = VISIBLE
        textView.visibility = GONE
    }

    fun setText(text: String, colorHex: String? = null) {
        textView.text = text
        if (colorHex != null) {
            textView.setTextColor(Color.parseColor(colorHex))
        }
        textView.visibility = VISIBLE
        iconView.visibility = GONE
    }

    private fun updateVisualState(isPressed: Boolean) {
        val density = resources.displayMetrics.density
        val cornerPx = 8f * density

        val bg = GradientDrawable().apply {
            cornerRadius = cornerPx
            if (isPressed) {
                setColor(Color.parseColor("#282A36"))
                setStroke((2.0f * density).toInt(), Color.parseColor(activeColorHex))
            } else {
                setColor(Color.parseColor("#1B1C22"))
                setStroke((1.0f * density).toInt(), Color.parseColor("#2D2E38"))
            }
        }
        background = bg
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                updateVisualState(true)
                performHapticFeedback(android.view.HapticFeedbackConstants.KEYBOARD_TAP)
                return true
            }

            MotionEvent.ACTION_UP -> {
                updateVisualState(false)
                onActionClick?.invoke()
                performClick()
                return true
            }

            MotionEvent.ACTION_CANCEL -> {
                updateVisualState(false)
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

