package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.view.Gravity
import android.view.View
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import com.stab.sculptbridge.Protocol
import com.stab.sculptbridge.R
import com.stab.sculptbridge.TelemetryData

/**
 * Barra lateral compacta de alta densidade tátil (SculptPad Rail ~54dp).
 * Organiza de maneira inteligente os atalhos essenciais do Blender Sculpt:
 * - Janela e Configurações (com submodal dedicado para canhoto/destro e tela)
 * - Modificadores de alternância com trava confiável (Shift, Ctrl, Alt)
 * - Sliders flutuantes de Raio e Força
 * - Combinações frequentes (Ctrl+S, Shift+Z, Shift+F)
 * - Teclas de acesso rápido simples (A, F, Z, N, Tab, <, >, ')
 * - Histórico com Desfazer (Ctrl+Z) e Refazer (Ctrl+Shift+Z)
 */
class SculptPadLayout(
    context: Context,
    private val onActionEmitted: (Byte) -> Unit,
    private val onRadiusChanged: (Float, Boolean) -> Unit,
    private val onHandSwapped: () -> Unit
) : FrameLayout(context) {

    var onMinimizeClicked: (() -> Unit)? = null

    private var isLeftEdge = true
    private var configSubmodal: ConfigSubmodal? = null
    private var radiusSubmodal: SliderSubmodal? = null
    private var strengthSubmodal: SliderSubmodal? = null
    private val tooltipView = RailTooltipView(context)

    private val contentLayout = LinearLayout(context)
    private val scrollView = ScrollView(context)

    init {
        setBackgroundColor(Color.parseColor("#151518"))

        scrollView.isVerticalScrollBarEnabled = false
        scrollView.overScrollMode = OVER_SCROLL_IF_CONTENT_SCROLLS

        contentLayout.orientation = LinearLayout.VERTICAL
        contentLayout.gravity = Gravity.CENTER_HORIZONTAL
        val density = resources.displayMetrics.density
        val padPx = (5 * density).toInt()
        contentLayout.setPadding(padPx, padPx, padPx, padPx)

        scrollView.addView(
            contentLayout,
            LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT)
        )
        addView(
            scrollView,
            LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT)
        )

        setupSubmodals()
        setupButtons()
    }

    fun updateEdgeMargin(isLeftEdge: Boolean) {
        this.isLeftEdge = isLeftEdge
    }

    fun getSliderViews(): List<View> = emptyList()

    private fun setupSubmodals() {
        configSubmodal = ConfigSubmodal(
            context = context,
            onHandSwapped = { onHandSwapped() },
            onDisplayModeToggle = { onActionEmitted(Protocol.SCULPTPAD_TOGGLE_DISPLAY_MODE) }
        )

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
        val btnMargin = (4 * density).toInt()

        fun createLayoutParams(): LinearLayout.LayoutParams {
            return LinearLayout.LayoutParams(btnSize, btnSize).apply {
                bottomMargin = btnMargin
            }
        }

        fun attachTooltip(btn: ActionRailButton, text: String) {
            btn.tooltipText = text
            btn.onHoverChanged = { isHovering, b ->
                if (isHovering) tooltipView.showTooltip(b, text, isLeftEdge)
                else tooltipView.hideTooltip()
            }
        }

        fun attachModifierTooltip(btn: ModifierToggleButton, text: String) {
            btn.tooltipText = text
            btn.onHoverChanged = { isHovering, b ->
                if (isHovering) tooltipView.showTooltip(b, text, isLeftEdge)
                else tooltipView.hideTooltip()
            }
        }

        // ==========================================
        // 1. SISTEMA / JANELA E CONFIGURAÇÕES
        // ==========================================
        val minimizeBtn = ActionRailButton(context, activeColorHex = "#00E5FF").apply {
            layoutParams = createLayoutParams()
            setText("«", "#00E5FF")
            onActionClick = { onMinimizeClicked?.invoke() }
        }
        attachTooltip(minimizeBtn, "Minimizar Barra (Maximizar Tela)")
        contentLayout.addView(minimizeBtn)

        val settingsBtn = ActionRailButton(context, activeColorHex = "#00E5FF").apply {
            layoutParams = createLayoutParams()
            setIconResource(R.drawable.ic_settings)
            onActionClick = { configSubmodal?.show(this, isLeftEdge) }
        }
        attachTooltip(settingsBtn, "Configurações (Lado / Tela)")
        contentLayout.addView(settingsBtn)

        contentLayout.addView(createDivider())

        // ==========================================
        // 2. MODIFICADORES DE ALTERNÂNCIA (LIGA / DESLIGA)
        // ==========================================
        val shiftBtn = ModifierToggleButton(context, title = "SH", activeColorHex = "#00E5FF").apply {
            layoutParams = createLayoutParams()
            onToggleChanged = { active ->
                val action = if (active) Protocol.SCULPTPAD_HOLD_SHIFT_DOWN else Protocol.SCULPTPAD_HOLD_SHIFT_UP
                onActionEmitted(action)
            }
        }
        attachModifierTooltip(shiftBtn, "Suavizar / Smooth (Shift - Alternar Liga/Desliga)")
        contentLayout.addView(shiftBtn)

        val ctrlBtn = ModifierToggleButton(context, title = "CT", activeColorHex = "#FF9100").apply {
            layoutParams = createLayoutParams()
            onToggleChanged = { active ->
                val action = if (active) Protocol.SCULPTPAD_HOLD_CTRL_DOWN else Protocol.SCULPTPAD_HOLD_CTRL_UP
                onActionEmitted(action)
            }
        }
        attachModifierTooltip(ctrlBtn, "Inverter / Subtrair (Ctrl - Alternar Liga/Desliga)")
        contentLayout.addView(ctrlBtn)

        val altBtn = ModifierToggleButton(context, title = "AL", activeColorHex = "#E040FB").apply {
            layoutParams = createLayoutParams()
            onToggleChanged = { active ->
                val action = if (active) Protocol.SCULPTPAD_HOLD_ALT_DOWN else Protocol.SCULPTPAD_HOLD_ALT_UP
                onActionEmitted(action)
            }
        }
        attachModifierTooltip(altBtn, "Máscara / Contexto (Alt - Alternar Liga/Desliga)")
        contentLayout.addView(altBtn)

        contentLayout.addView(createDivider())

        // ==========================================
        // 3. SLIDERS INTERATIVOS (SUBMODAIS)
        // ==========================================
        val radiusBtn = ActionRailButton(context, activeColorHex = "#00E5FF").apply {
            layoutParams = createLayoutParams()
            setIconResource(R.drawable.ic_radius)
            onActionClick = { radiusSubmodal?.show(this, isLeftEdge) }
        }
        attachTooltip(radiusBtn, "Raio do Pincel (Slider)")
        contentLayout.addView(radiusBtn)

        val strengthBtn = ActionRailButton(context, activeColorHex = "#FF9100").apply {
            layoutParams = createLayoutParams()
            setIconResource(R.drawable.ic_strength)
            onActionClick = { strengthSubmodal?.show(this, isLeftEdge) }
        }
        attachTooltip(strengthBtn, "Força do Pincel (Slider)")
        contentLayout.addView(strengthBtn)

        contentLayout.addView(createDivider())

        // ==========================================
        // 4. COMBOS RÁPIDOS SIMPLES
        // ==========================================
        val saveBtn = ActionRailButton(context, activeColorHex = "#00E676").apply {
            layoutParams = createLayoutParams()
            setText("Ct+S", "#00E676")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_COMBO_CTRL_S) }
        }
        attachTooltip(saveBtn, "Salvar Projeto (Ctrl+S)")
        contentLayout.addView(saveBtn)

        val wireframeBtn = ActionRailButton(context, activeColorHex = "#40C4FF").apply {
            layoutParams = createLayoutParams()
            setText("Sh+Z", "#40C4FF")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_COMBO_SHIFT_Z) }
        }
        attachTooltip(wireframeBtn, "Ver Arestas / Wireframe (Shift+Z)")
        contentLayout.addView(wireframeBtn)

        val strengthComboBtn = ActionRailButton(context, activeColorHex = "#FF9100").apply {
            layoutParams = createLayoutParams()
            setText("Sh+F", "#FF9100")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_COMBO_SHIFT_F) }
        }
        attachTooltip(strengthComboBtn, "Controle de Força do Pincel (Shift+F)")
        contentLayout.addView(strengthComboBtn)

        contentLayout.addView(createDivider())

        // ==========================================
        // 5. ATALHOS SIMPLES DE LETRAS / TECLAS
        // ==========================================
        val keyABtn = ActionRailButton(context).apply {
            layoutParams = createLayoutParams()
            setText("A")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_KEY_A) }
        }
        attachTooltip(keyABtn, "Editar Máscara / Selecionar (A)")
        contentLayout.addView(keyABtn)

        val keyFBtn = ActionRailButton(context).apply {
            layoutParams = createLayoutParams()
            setText("F")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_KEY_F) }
        }
        attachTooltip(keyFBtn, "Tamanho do Pincel (F)")
        contentLayout.addView(keyFBtn)

        val keyZBtn = ActionRailButton(context).apply {
            layoutParams = createLayoutParams()
            setText("Z")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_KEY_Z) }
        }
        attachTooltip(keyZBtn, "Modo de Render / Shading Pie (Z)")
        contentLayout.addView(keyZBtn)

        val keyNBtn = ActionRailButton(context).apply {
            layoutParams = createLayoutParams()
            setText("N")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_KEY_N) }
        }
        attachTooltip(keyNBtn, "Transformações / Painel Lateral (N)")
        contentLayout.addView(keyNBtn)

        val tabBtn = ActionRailButton(context).apply {
            layoutParams = createLayoutParams()
            setText("Tab")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_KEY_TAB) }
        }
        attachTooltip(tabBtn, "Alternar Sculpt / Edit Mode (Tab)")
        contentLayout.addView(tabBtn)

        val commaBtn = ActionRailButton(context).apply {
            layoutParams = createLayoutParams()
            setText("<")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_KEY_COMMA) }
        }
        attachTooltip(commaBtn, "Orientação de Transformação (< / ,)")
        contentLayout.addView(commaBtn)

        val periodBtn = ActionRailButton(context).apply {
            layoutParams = createLayoutParams()
            setText(">")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_KEY_PERIOD) }
        }
        attachTooltip(periodBtn, "Pontos de Pivô (> / .)")
        contentLayout.addView(periodBtn)

        val quoteBtn = ActionRailButton(context).apply {
            layoutParams = createLayoutParams()
            setText("'")
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_KEY_QUOTE) }
        }
        attachTooltip(quoteBtn, "Menu de Visualizações (')")
        contentLayout.addView(quoteBtn)

        contentLayout.addView(createDivider())

        // ==========================================
        // 6. HISTÓRICO: DESFAZER E REFAZER
        // ==========================================
        val undoBtn = ActionRailButton(context, activeColorHex = "#40C4FF").apply {
            layoutParams = createLayoutParams()
            setIconResource(R.drawable.ic_undo)
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_UNDO) }
        }
        attachTooltip(undoBtn, "Desfazer (Ctrl+Z)")
        contentLayout.addView(undoBtn)

        val redoBtn = ActionRailButton(context, activeColorHex = "#40C4FF").apply {
            layoutParams = createLayoutParams()
            setIconResource(R.drawable.ic_redo)
            onActionClick = { onActionEmitted(Protocol.SCULPTPAD_REDO) }
        }
        attachTooltip(redoBtn, "Refazer (Ctrl+Shift+Z)")
        contentLayout.addView(redoBtn)
    }

    private fun createDivider(): View {
        val density = resources.displayMetrics.density
        return View(context).apply {
            val divWidth = (32 * density).toInt()
            val divHeight = (1 * density).toInt()
            layoutParams = LinearLayout.LayoutParams(divWidth, divHeight).apply {
                val marginV = (4 * density).toInt()
                topMargin = marginV
                bottomMargin = marginV
            }
            setBackgroundColor(Color.parseColor("#2A2C38"))
        }
    }

    fun updateBrushTelemetry(data: TelemetryData) {
        radiusSubmodal?.updateValueQuietly(data.size.toFloat())
        strengthSubmodal?.updateValueQuietly(data.strength)
    }
}
