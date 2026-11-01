package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.view.Gravity
import android.widget.LinearLayout
import com.stab.sculptbridge.Protocol
import com.stab.sculptbridge.R

/**
 * Painel lateral ergonômico "SculptPad" para a mão esquerda (ou direita no modo canhoto).
 * Agrupa botões de retenção (Shift, Ctrl, Alt), sliders táteis e gatilhos rápidos.
 */
class SculptPadLayout(
    context: Context,
    private val onActionEmitted: (Byte) -> Unit,
    private val onRadiusChanged: (Float, Boolean) -> Unit,
    private val onHandSwapped: () -> Unit
) : LinearLayout(context) {

    lateinit var radiusSlider: TouchSliderView
        private set
    lateinit var strengthSlider: TouchSliderView
        private set
    lateinit var telemetryCard: BrushTelemetryCard
        private set

    init {
        orientation = VERTICAL
        setBackgroundColor(Color.parseColor("#151518"))
        updateEdgeMargin(isLeftEdge = true)

        setupTopToolbar()
        setupTelemetryCard()
        setupSliders()
        setupHoldModifiers()
        setupFooterUndoRedo()
    }

    /**
     * Atualiza a margem física de 14-16 dp na borda externa do SculptPad,
     * afastando os botões da zona capacitiva de disparo de gestos do Android.
     */
    fun updateEdgeMargin(isLeftEdge: Boolean) {
        val density = resources.displayMetrics.density
        val outerMarginPx = (16 * density).toInt() // Margem de segurança de 16 dp
        val innerMarginPx = (8 * density).toInt()
        val topBottomPx = (8 * density).toInt()

        if (isLeftEdge) {
            setPadding(outerMarginPx, topBottomPx, innerMarginPx, topBottomPx)
        } else {
            setPadding(innerMarginPx, topBottomPx, outerMarginPx, topBottomPx)
        }
    }

    fun getSliderViews(): List<android.view.View> {
        return if (::radiusSlider.isInitialized && ::strengthSlider.isInitialized) {
            listOf(radiusSlider, strengthSlider)
        } else {
            emptyList()
        }
    }

    private fun setupTopToolbar() {
        val topRow = LinearLayout(context).apply {
            orientation = HORIZONTAL
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, 90).apply {
                bottomMargin = 10
            }
        }

        val remeshBtn = QuickActionButton(
            context,
            R.drawable.ic_remesh,
            "Remesh",
            Protocol.SCULPTPAD_TRIGGER_REMESH
        ).apply {
            layoutParams = LayoutParams(0, LayoutParams.MATCH_PARENT, 1f).apply { marginEnd = 6 }
            onActionEmitted = this@SculptPadLayout.onActionEmitted
        }

        val transferBtn = QuickActionButton(
            context,
            R.drawable.ic_transfer,
            "Trocar",
            Protocol.SCULPTPAD_TRIGGER_TRANSFER
        ).apply {
            layoutParams = LayoutParams(0, LayoutParams.MATCH_PARENT, 1f).apply { marginEnd = 6 }
            onActionEmitted = this@SculptPadLayout.onActionEmitted
        }

        val switchHandBtn = QuickActionButton(
            context,
            R.drawable.ic_hand_switch,
            "Lado",
            0
        ) {
            onHandSwapped()
        }.apply {
            layoutParams = LayoutParams(0, LayoutParams.MATCH_PARENT, 1f)
        }

        topRow.addView(remeshBtn)
        topRow.addView(transferBtn)
        topRow.addView(switchHandBtn)
        addView(topRow)
    }

    private fun setupTelemetryCard() {
        telemetryCard = BrushTelemetryCard(context).apply {
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT).apply {
                bottomMargin = 10
            }
        }
        addView(telemetryCard)
    }

    /**
     * Atualiza os dados de telemetria recebidos do Blender no cartão e nos sliders.
     */
    fun updateBrushTelemetry(data: com.stab.sculptbridge.TelemetryData) {
        if (::telemetryCard.isInitialized) {
            telemetryCard.updateTelemetry(data)
        }
        if (::radiusSlider.isInitialized) {
            radiusSlider.updateValueQuietly(data.size.toFloat())
        }
        if (::strengthSlider.isInitialized) {
            strengthSlider.updateValueQuietly(data.strength)
        }
    }

    private fun setupSliders() {
        val slidersRow = LinearLayout(context).apply {
            orientation = HORIZONTAL
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, 0, 1.1f).apply {
                bottomMargin = 10
            }
        }

        radiusSlider = TouchSliderView(
            context,
            label = "RAIO",
            triggerAction = Protocol.SCULPTPAD_TRIGGER_RADIUS,
            minValue = 5f,
            maxValue = 400f,
            initialValue = 100f,
            activeColorHex = "#00E5FF"
        ).apply {
            layoutParams = LayoutParams(0, LayoutParams.MATCH_PARENT, 1f).apply { marginEnd = 6 }
            onValueChanged = { radius, isAdjusting ->
                onRadiusChanged(radius, isAdjusting)
            }
            onActionEmitted = this@SculptPadLayout.onActionEmitted
        }

        strengthSlider = TouchSliderView(
            context,
            label = "FORÇA",
            triggerAction = Protocol.SCULPTPAD_TRIGGER_STRENGTH,
            minValue = 0.05f,
            maxValue = 1.0f,
            initialValue = 0.5f,
            activeColorHex = "#FF9100"
        ).apply {
            layoutParams = LayoutParams(0, LayoutParams.MATCH_PARENT, 1f)
            onActionEmitted = this@SculptPadLayout.onActionEmitted
        }

        slidersRow.addView(radiusSlider)
        slidersRow.addView(strengthSlider)
        addView(slidersRow)
    }

    private fun setupHoldModifiers() {
        val modifiersContainer = LinearLayout(context).apply {
            orientation = VERTICAL
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, 0, 1.4f).apply {
                bottomMargin = 10
            }
        }

        // Botão Gigante SHIFT (Smooth Brush)
        val shiftBtn = HoldModifierButton(
            context,
            title = "SHIFT",
            subtitle = "Smooth Brush",
            downAction = Protocol.SCULPTPAD_HOLD_SHIFT_DOWN,
            upAction = Protocol.SCULPTPAD_HOLD_SHIFT_UP,
            activeColorHex = "#00E5FF"
        ).apply {
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f).apply { bottomMargin = 6 }
            onActionEmitted = this@SculptPadLayout.onActionEmitted
        }

        // Zona Combinada SHIFT + CTRL (Mask / Trim)
        val combinedBtn = HoldModifierButton(
            context,
            title = "SHIFT + CTRL",
            subtitle = "Mascara / Trim",
            downAction = Protocol.SCULPTPAD_HOLD_SHIFT_DOWN,
            upAction = Protocol.SCULPTPAD_HOLD_SHIFT_UP,
            activeColorHex = "#76FF03"
        ).apply {
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, 0, 0.7f).apply { bottomMargin = 6 }
            onActionEmitted = { action ->
                this@SculptPadLayout.onActionEmitted(action)
                if (action == Protocol.SCULPTPAD_HOLD_SHIFT_DOWN) {
                    this@SculptPadLayout.onActionEmitted(Protocol.SCULPTPAD_HOLD_CTRL_DOWN)
                } else {
                    this@SculptPadLayout.onActionEmitted(Protocol.SCULPTPAD_HOLD_CTRL_UP)
                }
            }
        }

        // Botão Gigante CTRL (Invert / Sub)
        val ctrlBtn = HoldModifierButton(
            context,
            title = "CTRL",
            subtitle = "Inverter / Sub",
            downAction = Protocol.SCULPTPAD_HOLD_CTRL_DOWN,
            upAction = Protocol.SCULPTPAD_HOLD_CTRL_UP,
            activeColorHex = "#FF9100"
        ).apply {
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, 0, 1f).apply { bottomMargin = 6 }
            onActionEmitted = this@SculptPadLayout.onActionEmitted
        }

        // Botão ALT
        val altBtn = HoldModifierButton(
            context,
            title = "ALT",
            subtitle = "Alternativo",
            downAction = Protocol.SCULPTPAD_HOLD_ALT_DOWN,
            upAction = Protocol.SCULPTPAD_HOLD_ALT_UP,
            activeColorHex = "#E040FB"
        ).apply {
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, 0, 0.7f)
            onActionEmitted = this@SculptPadLayout.onActionEmitted
        }

        modifiersContainer.addView(shiftBtn)
        modifiersContainer.addView(combinedBtn)
        modifiersContainer.addView(ctrlBtn)
        modifiersContainer.addView(altBtn)
        addView(modifiersContainer)
    }

    private fun setupFooterUndoRedo() {
        val footerRow = LinearLayout(context).apply {
            orientation = HORIZONTAL
            layoutParams = LayoutParams(LayoutParams.MATCH_PARENT, 80)
        }

        val undoBtn = QuickActionButton(
            context,
            R.drawable.ic_undo,
            "Undo",
            0
        ) {
            // Emite pacote de gesto Undo (Ctrl+Z)
            onActionEmitted(Protocol.GESTURE_TAP_TWO_FINGERS)
        }.apply {
            layoutParams = LayoutParams(0, LayoutParams.MATCH_PARENT, 1f).apply { marginEnd = 6 }
        }

        val redoBtn = QuickActionButton(
            context,
            R.drawable.ic_redo,
            "Redo",
            0
        ) {
            // Emite pacote de gesto Redo (Ctrl+Shift+Z)
            onActionEmitted(Protocol.GESTURE_TAP_THREE_FINGERS)
        }.apply {
            layoutParams = LayoutParams(0, LayoutParams.MATCH_PARENT, 1f)
        }

        footerRow.addView(undoBtn)
        footerRow.addView(redoBtn)
        addView(footerRow)
    }
}

