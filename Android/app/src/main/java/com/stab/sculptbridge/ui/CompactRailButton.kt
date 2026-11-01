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
 * Suporta ícones lineart vetoriais e texto curto para modificadores de teclado (Shift, Ctrl, Alt)
 * com trava de toque inteligente (Sticky Lock) para superar o Palm Rejection de hardware do tablet.
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
    var tooltipText: String = ""
    var onHoldDown: (() -> Unit)? = null
    var onHoldUp: (() -> Unit)? = null
    var onActionClick: (() -> Unit)? = null
    var onHoverChanged: ((isHovering: Boolean, button: CompactRailButton) -> Unit)? = null

    private var isPressedState = false
    private var isLocked = false
    private var downTimeMs: Long = 0L
    private var baseText: String = ""

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
        baseText = text
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
                setStroke((2.2f * density).toInt(), Color.parseColor(activeColorHex))
            } else {
                setColor(Color.parseColor("#1B1C22"))
                setStroke((1.0f * density).toInt(), Color.parseColor("#2D2E38"))
            }
        }
        background = bg

        if (textView.visibility == VISIBLE) {
            val color = if (isActive) Color.parseColor(activeColorHex) else Color.parseColor("#E0E0E6")
            textView.setTextColor(color)
            if (isHoldModifier) {
                textView.text = if (isLocked) "\u25CF$baseText" else baseText
            }
        }

        if (iconView.visibility == VISIBLE) {
            val tint = if (isActive) Color.parseColor(activeColorHex) else Color.parseColor("#C8C9D4")
            iconView.setColorFilter(tint)
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (isHoldModifier) {
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    downTimeMs = System.currentTimeMillis()
                    if (isLocked) {
                        // Se já estava travado, um novo toque desliga o modificador
                        isLocked = false
                        isPressedState = false
                        updateVisualState(isActive = false)
                        performHapticFeedback(android.view.HapticFeedbackConstants.VIRTUAL_KEY)
                        onHoldUp?.invoke()
                    } else {
                        // Ativa o modificador
                        isPressedState = true
                        updateVisualState(isActive = true)
                        performHapticFeedback(android.view.HapticFeedbackConstants.VIRTUAL_KEY)
                        onHoldDown?.invoke()
                    }
                    return true
                }
                MotionEvent.ACTION_UP -> {
                    val duration = System.currentTimeMillis() - downTimeMs
                    if (duration < 350) {
                        // Toque rápido: trava o modificador ATIVO (Sticky Lock)
                        // Permite esculpir com a S-Pen sem que o palm rejection do hardware corte o dedo
                        isLocked = true
                        isPressedState = true
                        updateVisualState(isActive = true)
                    } else {
                        // Segurou por mais de 350ms: desliga ao soltar
                        isLocked = false
                        isPressedState = false
                        updateVisualState(isActive = false)
                        onHoldUp?.invoke()
                    }
                    return true
                }
                MotionEvent.ACTION_CANCEL -> {
                    // Quando a S-Pen toca a tela, o hardware Wacom EMR envia ACTION_CANCEL ao dedo.
                    // Mantém o modificador TRAVADO ATIVO para que o usuário continue esculpindo com Shift!
                    isLocked = true
                    isPressedState = true
                    updateVisualState(isActive = true)
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

    override fun onHoverEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_HOVER_ENTER -> {
                updateVisualState(isActive = true)
                onHoverChanged?.invoke(true, this)
                return true
            }
            MotionEvent.ACTION_HOVER_EXIT -> {
                if (!isPressedState && !isLocked) {
                    updateVisualState(isActive = false)
                }
                onHoverChanged?.invoke(false, this)
                return true
            }
        }
        return super.onHoverEvent(event)
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }
}
