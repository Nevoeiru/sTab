package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.graphics.drawable.GradientDrawable
import android.os.Build
import android.view.Gravity
import android.view.View
import android.widget.LinearLayout
import android.widget.PopupWindow
import android.widget.SeekBar
import android.widget.TextView

/**
 * Submodal flutuante e modular para ajuste fino de parâmetros de escultura (Raio e Força).
 * Abre imediatamente adjacente ao botão acionado na barra compacta e economiza 100% do espaço de tela.
 */
class SliderSubmodal(
    private val context: Context,
    private val title: String,
    private val isIntegerUnits: Boolean,
    private val unitSuffix: String,
    private val minValue: Float,
    private val maxValue: Float,
    private var currentValue: Float,
    private val activeColorHex: String,
    private val presets: List<Float>,
    private val onValueChanged: (Float, Boolean) -> Unit
) {

    private val popupWindow: PopupWindow
    private val valueTextView: TextView
    private val seekBar: SeekBar

    init {
        val density = context.resources.displayMetrics.density
        val rootLayout = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            val padPx = (14 * density).toInt()
            setPadding(padPx, padPx, padPx, padPx)

            val bg = GradientDrawable().apply {
                cornerRadius = 14f * density
                setColor(Color.parseColor("#1C1D24"))
                setStroke((1.5f * density).toInt(), Color.parseColor("#353746"))
            }
            background = bg
        }

        // 1. Cabeçalho: Título + Valor numérico em tempo real
        val headerLayout = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            ).apply { bottomMargin = (10 * density).toInt() }
        }

        val titleTextView = TextView(context).apply {
            text = title.uppercase()
            textSize = 12f
            setTextColor(Color.parseColor("#9EA2B0"))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
        }

        valueTextView = TextView(context).apply {
            textSize = 14f
            setTextColor(Color.parseColor(activeColorHex))
            typeface = android.graphics.Typeface.DEFAULT_BOLD
            text = formatValue(currentValue)
        }

        headerLayout.addView(titleTextView)
        headerLayout.addView(valueTextView)
        rootLayout.addView(headerLayout)

        // 2. Slider SeekBar de alta resolução (1000 passos)
        val maxSteps = 1000
        seekBar = SeekBar(context).apply {
            max = maxSteps
            val initialProgress = (((currentValue - minValue) / (maxValue - minValue)) * maxSteps).toInt()
            progress = initialProgress.coerceIn(0, maxSteps)

            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(sb: SeekBar?, prog: Int, fromUser: Boolean) {
                    if (fromUser) {
                        val fraction = prog.toFloat() / maxSteps.toFloat()
                        currentValue = minValue + fraction * (maxValue - minValue)
                        valueTextView.text = formatValue(currentValue)
                        onValueChanged(currentValue, true)
                    }
                }

                override fun onStartTrackingTouch(sb: SeekBar?) {
                    onValueChanged(currentValue, true)
                }

                override fun onStopTrackingTouch(sb: SeekBar?) {
                    onValueChanged(currentValue, false)
                }
            })

            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            ).apply { bottomMargin = (12 * density).toInt() }
        }
        rootLayout.addView(seekBar)

        // 3. Linha de chips de presets rápidos
        val presetsLayout = LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )
        }

        for (preset in presets) {
            val chip = TextView(context).apply {
                val chipText = if (isIntegerUnits) "${preset.toInt()}" else String.format("%.1f", preset)
                text = chipText
                textSize = 11f
                gravity = Gravity.CENTER
                setTextColor(Color.parseColor("#C8C9D4"))

                val chipPadH = (10 * density).toInt()
                val chipPadV = (5 * density).toInt()
                setPadding(chipPadH, chipPadV, chipPadH, chipPadV)

                val chipBg = GradientDrawable().apply {
                    cornerRadius = 6f * density
                    setColor(Color.parseColor("#252730"))
                    setStroke((1f * density).toInt(), Color.parseColor("#3A3D4C"))
                }
                background = chipBg

                layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f).apply {
                    marginEnd = (4 * density).toInt()
                }

                setOnClickListener {
                    currentValue = preset
                    val prog = (((currentValue - minValue) / (maxValue - minValue)) * maxSteps).toInt()
                    seekBar.progress = prog.coerceIn(0, maxSteps)
                    valueTextView.text = formatValue(currentValue)
                    onValueChanged(currentValue, false)
                }
            }
            presetsLayout.addView(chip)
        }
        rootLayout.addView(presetsLayout)

        val modalWidthPx = (280 * density).toInt()
        popupWindow = PopupWindow(rootLayout, modalWidthPx, LinearLayout.LayoutParams.WRAP_CONTENT, true).apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            isOutsideTouchable = true
            isFocusable = true
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                elevation = 16f * density
            }
        }
    }

    private fun formatValue(value: Float): String {
        return if (isIntegerUnits) {
            "${value.toInt()} $unitSuffix"
        } else {
            String.format("%.2f %s", value, unitSuffix).trim()
        }
    }

    fun show(anchorView: View, isLeftEdge: Boolean) {
        val density = context.resources.displayMetrics.density
        val xOffset = if (isLeftEdge) (58 * density).toInt() else (-290 * density).toInt()
        val yOffset = (-30 * density).toInt()

        popupWindow.showAsDropDown(anchorView, xOffset, yOffset)
    }

    fun updateValueQuietly(value: Float) {
        currentValue = value
        valueTextView.text = formatValue(currentValue)
        val maxSteps = 1000
        val prog = (((currentValue - minValue) / (maxValue - minValue)) * maxSteps).toInt()
        seekBar.progress = prog.coerceIn(0, maxSteps)
    }

    fun dismiss() {
        if (popupWindow.isShowing) {
            popupWindow.dismiss()
        }
    }
}

