package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.util.AttributeSet
import android.view.Gravity
import android.widget.LinearLayout
import android.widget.TextView

/**
 * Indicador visual flutuante (HUD Badge) que exibe o nome da ferramenta ativa do Blender
 * e seus parâmetros sempre que a caneta S-Pen estiver em Hover sobre a tela.
 */
class ToolHoverBadge @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : LinearLayout(context, attrs, defStyleAttr) {

    private val brushNameText = TextView(context)
    private val brushParamsText = TextView(context)

    init {
        orientation = HORIZONTAL
        gravity = Gravity.CENTER_VERTICAL

        val density = resources.displayMetrics.density
        val padH = (14 * density).toInt()
        val padV = (7 * density).toInt()
        setPadding(padH, padV, padH, padV)

        val bg = GradientDrawable().apply {
            cornerRadius = 20f * density
            setColor(Color.parseColor("#E6181920")) // Slate escuro translúcido 90%
            setStroke((1.2f * density).toInt(), Color.parseColor("#383A48"))
        }
        background = bg

        brushNameText.apply {
            textSize = 12.5f
            setTextColor(Color.parseColor("#00E5FF")) // Ciano vibrante
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            text = "Pincel: Esculpir"
        }

        brushParamsText.apply {
            textSize = 11.5f
            setTextColor(Color.parseColor("#A8ABB8"))
            typeface = android.graphics.Typeface.MONOSPACE
            text = " | R: 100px \u2022 F: 0.50"
            val marginL = (4 * density).toInt()
            setPadding(marginL, 0, 0, 0)
        }

        addView(brushNameText)
        addView(brushParamsText)

        visibility = GONE
    }

    fun updateToolInfo(brushName: String, radius: Float, strength: Float) {
        val name = if (brushName.isNotBlank()) brushName else "Pincel Ativo"
        brushNameText.text = "Pincel: $name"
        brushParamsText.text = " | R: ${radius.toInt()}px \u2022 F: ${String.format("%.2f", strength)}"
    }

    fun showBadge() {
        if (visibility != VISIBLE) {
            alpha = 0f
            visibility = VISIBLE
            animate().alpha(1f).setDuration(120).start()
        }
    }

    fun hideBadge() {
        if (visibility == VISIBLE) {
            animate().alpha(0f).setDuration(180).withEndAction {
                visibility = GONE
            }.start()
        }
    }
}

