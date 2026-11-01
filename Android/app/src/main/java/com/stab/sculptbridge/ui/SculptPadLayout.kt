package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.view.Gravity
import android.view.View
import android.widget.LinearLayout
import android.widget.ScrollView
import com.stab.sculptbridge.Protocol
import com.stab.sculptbridge.R
import com.stab.sculptbridge.TelemetryData

/**
 * Barra lateral compacta de ultra-baixo impacto visual (SculptPad Rail ~54dp).
 * Centraliza ações frequentes do Blender Sculpt através de ícones lineart de alta densidade
 * e submodais flutuantes expansíveis sob demanda para ajustes de Raio e Força.
 */
class SculptPadLayout(
    context: Context,
    private val onActionEmitted: (Byte) -> Unit,
    private val onRadiusChanged: (Float, Boolean) -> Unit,
    private val onHandSwapped: () -> Unit
) : LinearLayout(context) {

    var onMinimizeClicked: (() -> Unit)? = null

    private var isLeftEdge = true
    private var radiusSubmodal: SliderSubmodal? = null
    private var strengthSubmodal: SliderSubmodal? = null
    private val tooltipView = RailTooltipView(context)

    private lateinit var radiusButton: CompactRailButton
    private lateinit var strengthButton: CompactRailButton

    init {
        orientation = VERTICAL
        setBackgroundColor(Color.parseColor("#151518"))
        gravity = Gravity.CENTER_HORIZONTAL

        val density = resources.displayMetrics.density
        val padPx = (6 * density).toInt()
        setPadding(padPx, padPx, padPx, padPx)

        setupButtons()
        setupSubmodals()
    }

    fun updateEdgeMargin(isLeftEdge: Boolean) {
        this.isLeftEdge = isLeftEdge
    }

    fun getSliderViews(): List<View> {
        return emptyList()
    }

    private fun setupSubmodals() {
        radiusSubmodal = SliderSubmodal(
            context = context,
            title = "Raio do Pincel",
            isIntegerUnits = true,
            unitSuffix = "px",
            minValue = 5f,
            maxValue = 500f,
            currentValue = 100f,
            activeColorHex = "#00E5FF",
            presets = listOf(25f, 50f, 100f, 200f, 350f, 500f),
            onValueChanged = { radius, isAdjusting ->
                onRadiusChanged(radius, isAdjusting)
                onActionEmitted(Protocol.SCULPTPAD_TRIGGER_RADIUS)
            }
        )

        strengthSubmodal = SliderSubmodal(
            context = context,
            title = "Força do Pincel",
            isIntegerUnits = false,
            unitSuffix = "",
            minValue = 0.05f,
            maxValue = 1.0f,
            currentValue = 0.5f,
            activeColorHex = "#FF9100",
            presets = listOf(0.1f, 0.25f, 0.5f, 0.75f, 1.0f),
            onValueChanged = { _, _ ->
                onActionEmitted(Protocol.SCULPTPAD_TRIGGER_STRENGTH)
            }
        )
    }

    private fun setupButtons() {
        val density = resources.displayMetrics.density
        val btnSize = (44 * density).toInt()
        val btnMargin = (5 * density).toInt()

        fun createLayoutParams(): LayoutParams {
            return LayoutParams(btnSize, btnSize).apply {
                bottomMargin = btnMargin
            }
        }

        fun attachTooltip(btn: CompactRailButton, text: String) {
            btn.tooltipText = text
            btn.onHoverChanged = { isHovering, b ->
                if (isHovering) {
                    tooltipView.showTooltip(b, text, isLeftEdge)
                } else {
                    tooltipView.hideTooltip()
                }
            }
        }

        // 0. Botão Minimizar Barra Lateral (Maximizar Área 3D)
        val minimizeBtn = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#00E5FF"
            setText("\u00AB", "#00E5FF") // Ícone lineart '«'
            onActionClick = { onMinimizeClicked?.invoke() }
        }
        attachTooltip(minimizeBtn, "Minimizar Barra (Maximizar Tela)")
        addView(minimizeBtn)

        // 1. Alternar lado da barra (Esquerda / Direita)
        val swapHandBtn = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            setIconResource(R.drawable.ic_hand_switch)
            onActionClick = { onHandSwapped() }
        }
        attachTooltip(swapHandBtn, "Alternar Lado da Barra")
        addView(swapHandBtn)

        // 1b. Alternar Modo de Tela (Estendido / Espelhado)
        val displayModeBtn = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#00E5FF"
            setIconResource(R.drawable.ic_display_mode)
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_TOGGLE_DISPLAY_MODE) }
        }
        attachTooltip(displayModeBtn, "Alternar Tela (Estendido / Espelhado)")
        addView(displayModeBtn)

        addView(createDivider())

        // 2. Raio do Pincel (Abre submodal)
        radiusButton = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#00E5FF"
            setIconResource(R.drawable.ic_radius)
            onActionClick = {
                radiusSubmodal?.show(this, isLeftEdge)
            }
        }
        attachTooltip(radiusButton, "Raio do Pincel")
        addView(radiusButton)

        // 3. Força do Pincel (Abre submodal)
        strengthButton = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#FF9100"
            setIconResource(R.drawable.ic_strength)
            onActionClick = {
                strengthSubmodal?.show(this, isLeftEdge)
            }
        }
        attachTooltip(strengthButton, "Força do Pincel")
        addView(strengthButton)

        addView(createDivider())

        // 4. Modificador SHIFT (Smooth Brush Hold)
        val shiftBtn = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#00E5FF"
            isHoldModifier = true
            setText("SH", "#00E5FF")
            onHoldDown = { onActionEmitted(Protocol.SCULPTPAD_HOLD_SHIFT_DOWN) }
            onHoldUp = { onActionEmitted(Protocol.SCULPTPAD_HOLD_SHIFT_UP) }
        }
        attachTooltip(shiftBtn, "Suavizar / Smooth (Shift)")
        addView(shiftBtn)

        // 5. Modificador CTRL (Invert Brush Hold)
        val ctrlBtn = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#FF9100"
            isHoldModifier = true
            setText("CT", "#FF9100")
            onHoldDown = { onActionEmitted(Protocol.SCULPTPAD_HOLD_CTRL_DOWN) }
            onHoldUp = { onActionEmitted(Protocol.SCULPTPAD_HOLD_CTRL_UP) }
        }
        attachTooltip(ctrlBtn, "Inverter / Subtrair (Ctrl)")
        addView(ctrlBtn)

        // 6. Modificador ALT (Mask / Navigation Hold)
        val altBtn = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#E040FB"
            isHoldModifier = true
            setText("AL", "#E040FB")
            onHoldDown = { onActionEmitted(Protocol.SCULPTPAD_HOLD_ALT_DOWN) }
            onHoldUp = { onActionEmitted(Protocol.SCULPTPAD_HOLD_ALT_UP) }
        }
        attachTooltip(altBtn, "Máscara / Contexto (Alt)")
        addView(altBtn)

        // 7. Ação Rápida Remesh (Ctrl+R no Blender)
        val remeshBtn = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#76FF03"
            setIconResource(R.drawable.ic_remesh)
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_TRIGGER_REMESH) }
        }
        attachTooltip(remeshBtn, "Remesh Automático (Ctrl+R)")
        addView(remeshBtn)

        addView(createDivider())

        // 8. Desfazer (Undo / Ctrl+Z)
        val undoBtn = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#40C4FF"
            setIconResource(R.drawable.ic_undo)
            onActionClick = { onActionEmitted(Protocol.GESTURE_TAP_TWO_FINGERS) }
        }
        attachTooltip(undoBtn, "Desfazer (Ctrl+Z)")
        addView(undoBtn)

        // 9. Refazer (Redo / Ctrl+Shift+Z)
        val redoBtn = CompactRailButton(context).apply {
            layoutParams = createLayoutParams()
            activeColorHex = "#40C4FF"
            setIconResource(R.drawable.ic_redo)
            onActionClick = { onActionEmitted(Protocol.GESTURE_TAP_THREE_FINGERS) }
        }
        attachTooltip(redoBtn, "Refazer (Ctrl+Shift+Z)")
        addView(redoBtn)
    }

    private fun createDivider(): View {
        val density = resources.displayMetrics.density
        return View(context).apply {
            val divWidth = (32 * density).toInt()
            val divHeight = (1 * density).toInt()
            layoutParams = LayoutParams(divWidth, divHeight).apply {
                val marginV = (4 * density).toInt()
                topMargin = marginV
                bottomMargin = marginV
            }
            setBackgroundColor(Color.parseColor("#2A2C36"))
        }
    }

    /**
     * Atualiza dados de telemetria recebidos do Blender.
     */
    fun updateBrushTelemetry(data: TelemetryData) {
        radiusSubmodal?.updateValueQuietly(data.size.toFloat())
        strengthSubmodal?.updateValueQuietly(data.strength)
    }
}
