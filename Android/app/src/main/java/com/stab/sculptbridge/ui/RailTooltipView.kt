package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.graphics.drawable.GradientDrawable
import android.os.Build
import android.view.Gravity
import android.view.View
import android.widget.PopupWindow
import android.widget.TextView

/**
 * Tooltip flutuante exibido quando a ponta da S-Pen passa em Hover sobre os botões do SculptPad.
 */
class RailTooltipView(private val context: Context) {

    private val popup: PopupWindow
    private val textView: TextView

    init {
        val density = context.resources.displayMetrics.density

        textView = TextView(context).apply {
            textSize = 11.5f
            setTextColor(Color.parseColor("#FFFFFF"))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            gravity = Gravity.CENTER

            val padH = (10 * density).toInt()
            val padV = (5 * density).toInt()
            setPadding(padH, padV, padH, padV)

            val bg = GradientDrawable().apply {
                cornerRadius = 8f * density
                setColor(Color.parseColor("#EE1F2028"))
                setStroke((1f * density).toInt(), Color.parseColor("#00E5FF"))
            }
            background = bg
        }

        popup = PopupWindow(
            textView,
            android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
            android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
            false
        ).apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            isOutsideTouchable = true
            isTouchable = false
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                elevation = 10f * density
            }
        }
    }

    fun showTooltip(anchor: View, text: String, isLeftEdge: Boolean) {
        textView.text = text
        val density = context.resources.displayMetrics.density
        val xOffset = if (isLeftEdge) (52 * density).toInt() else (-160 * density).toInt()
        val yOffset = (-10 * density).toInt()

        if (popup.isShowing) {
            popup.update(anchor, xOffset, yOffset, -1, -1)
        } else {
            popup.showAsDropDown(anchor, xOffset, yOffset)
        }
    }

    fun hideTooltip() {
        if (popup.isShowing) {
            popup.dismiss()
        }
    }
}

