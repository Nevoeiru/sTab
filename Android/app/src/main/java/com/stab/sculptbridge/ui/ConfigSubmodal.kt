package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.graphics.drawable.GradientDrawable
import android.os.Build
import android.view.Gravity
import android.view.View
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.PopupWindow
import android.widget.TextView
import com.stab.sculptbridge.R

/**
 * Submodal modular de configurações rápidas do sTab.
 * Contém opções como alternar lado (destro/canhoto) e modo de tela (estendido/espelhado),
 * isolando essas ações para evitar cliques acidentais durante a escultura 3D.
 */
class ConfigSubmodal(
    private val context: Context,
    private val onHandSwapped: () -> Unit,
    private val onDisplayModeToggle: () -> Unit
) {

    private val popupWindow: PopupWindow

    init {
        val density = context.resources.displayMetrics.density
        val padPx = (14 * density).toInt()

        val rootLayout = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(padPx, padPx, padPx, padPx)

            val bg = GradientDrawable().apply {
                cornerRadius = 14f * density
                setColor(Color.parseColor("#1C1D24"))
                setStroke((1.5f * density).toInt(), Color.parseColor("#353746"))
            }
            background = bg
        }

        // Cabeçalho
        val headerLayout = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            ).apply { bottomMargin = (12 * density).toInt() }
        }

        val titleTextView = TextView(context).apply {
            text = "CONFIGURAÇÕES"
            textSize = 12f
            setTextColor(Color.parseColor("#00E5FF"))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
        }

        val closeButton = TextView(context).apply {
            text = "✕"
            textSize = 14f
            setTextColor(Color.parseColor("#8E92A4"))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            val closePad = (4 * density).toInt()
            setPadding(closePad, closePad, closePad, closePad)
            setOnClickListener { dismiss() }
        }

        headerLayout.addView(titleTextView)
        headerLayout.addView(closeButton)
        rootLayout.addView(headerLayout)

        // Botão 1: Alternar Lado (Canhoto / Destro)
        val swapHandCard = createConfigItem(
            iconRes = R.drawable.ic_hand_switch,
            title = "Alternar Lado da Barra",
            description = "Destro (Esquerda) ⇄ Canhoto (Direita)"
        ) {
            onHandSwapped()
            dismiss()
        }
        rootLayout.addView(swapHandCard)

        // Divisor fino
        rootLayout.addView(createDivider())

        // Botão 2: Alternar Modo de Tela (Estendido / Espelhado)
        val displayModeCard = createConfigItem(
            iconRes = R.drawable.ic_display_mode,
            title = "Modo de Tela",
            description = "Estendido (2º Monitor) ⇄ Espelhado"
        ) {
            onDisplayModeToggle()
            dismiss()
        }
        rootLayout.addView(displayModeCard)

        val modalWidthPx = (270 * density).toInt()
        popupWindow = PopupWindow(rootLayout, modalWidthPx, LinearLayout.LayoutParams.WRAP_CONTENT, true).apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            isOutsideTouchable = true
            isFocusable = true
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                elevation = 16f * density
            }
        }
    }

    private fun createConfigItem(
        iconRes: Int,
        title: String,
        description: String,
        onClick: () -> Unit
    ): LinearLayout {
        val density = context.resources.displayMetrics.density

        val container = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            val padH = (10 * density).toInt()
            val padV = (8 * density).toInt()
            setPadding(padH, padV, padH, padV)

            val bg = GradientDrawable().apply {
                cornerRadius = 8f * density
                setColor(Color.parseColor("#232530"))
                setStroke((1f * density).toInt(), Color.parseColor("#343746"))
            }
            background = bg

            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )

            isClickable = true
            isFocusable = true
            setOnClickListener {
                performHapticFeedback(android.view.HapticFeedbackConstants.KEYBOARD_TAP)
                onClick()
            }
        }

        val iconView = ImageView(context).apply {
            val iconSize = (24 * density).toInt()
            layoutParams = LinearLayout.LayoutParams(iconSize, iconSize).apply {
                marginEnd = (10 * density).toInt()
            }
            setImageResource(iconRes)
            scaleType = ImageView.ScaleType.FIT_CENTER
        }

        val textLayout = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
        }

        val titleTv = TextView(context).apply {
            text = title
            textSize = 12f
            setTextColor(Color.parseColor("#E0E0E6"))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
        }

        val descTv = TextView(context).apply {
            text = description
            textSize = 10f
            setTextColor(Color.parseColor("#8E92A4"))
        }

        textLayout.addView(titleTv)
        textLayout.addView(descTv)

        container.addView(iconView)
        container.addView(textLayout)

        return container
    }

    private fun createDivider(): View {
        val density = context.resources.displayMetrics.density
        return View(context).apply {
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                (1 * density).toInt()
            ).apply {
                topMargin = (6 * density).toInt()
                bottomMargin = (6 * density).toInt()
            }
            setBackgroundColor(Color.parseColor("#2A2C38"))
        }
    }

    fun show(anchorView: View, isLeftEdge: Boolean) {
        val density = context.resources.displayMetrics.density
        val xOffset = if (isLeftEdge) (58 * density).toInt() else (-280 * density).toInt()
        val yOffset = (-20 * density).toInt()

        popupWindow.showAsDropDown(anchorView, xOffset, yOffset)
    }

    fun dismiss() {
        if (popupWindow.isShowing) {
            popupWindow.dismiss()
        }
    }
}

