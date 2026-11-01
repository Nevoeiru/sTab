package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.widget.LinearLayout
import android.widget.TextView
import com.stab.sculptbridge.TelemetryData

/**
 * Cartão de visualização da telemetria do pincel ativo no Blender 3D.
 * Exibe o nome do pincel, tamanho em pixels, força percentual e status de simetria X.
 */
class BrushTelemetryCard(context: Context) : LinearLayout(context) {

    private val brushNameText: TextView
    private val radiusBadge: TextView
    private val strengthBadge: TextView
    private val symmetryBadge: TextView

    init {
        orientation = VERTICAL
        gravity = Gravity.CENTER
        val density = resources.displayMetrics.density
        val pad = (6 * density).toInt()
        setPadding(pad, pad, pad, pad)

        background = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = 8 * density
            setColor(Color.parseColor("#1C1C22"))
            setStroke((1 * density).toInt(), Color.parseColor("#2E2E38"))
        }

        brushNameText = TextView(context).apply {
            textSize = 13f
            setTextColor(Color.parseColor("#00E5FF"))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            gravity = Gravity.CENTER
            text = "BLENDER CONECTADO"
        }
        addView(brushNameText)

        val badgeRow = LinearLayout(context).apply {
            orientation = HORIZONTAL
            gravity = Gravity.CENTER
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT).apply {
                topMargin = (4 * density).toInt()
            }
        }

        radiusBadge = createBadge("R: 50px", "#E0E0E8")
        strengthBadge = createBadge("F: 50%", "#FF9100")
        symmetryBadge = createBadge("X-SYM", "#76FF03").apply {
            visibility = GONE
        }

        badgeRow.addView(radiusBadge)
        badgeRow.addView(strengthBadge)
        badgeRow.addView(symmetryBadge)
        addView(badgeRow)
    }

    private fun createBadge(initialText: String, textColorHex: String): TextView {
        val density = resources.displayMetrics.density
        return TextView(context).apply {
            text = initialText
            textSize = 10f
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            setTextColor(Color.parseColor(textColorHex))
            gravity = Gravity.CENTER
            val hPad = (4 * density).toInt()
            val vPad = (2 * density).toInt()
            setPadding(hPad, vPad, hPad, vPad)
            layoutParams = LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT).apply {
                setMargins((2 * density).toInt(), 0, (2 * density).toInt(), 0)
            }
            background = GradientDrawable().apply {
                shape = GradientDrawable.RECTANGLE
                cornerRadius = 4 * density
                setColor(Color.parseColor("#262630"))
            }
        }
    }

    fun updateTelemetry(data: TelemetryData) {
        brushNameText.text = data.brushName.uppercase()
        radiusBadge.text = "R: ${data.size}px"
        strengthBadge.text = "F: ${(data.strength * 100).toInt()}%"
        symmetryBadge.visibility = if (data.symmetryX) VISIBLE else GONE
    }
}
