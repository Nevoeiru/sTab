package com.stab.sculptbridge

import android.app.Activity
import android.graphics.Color
import android.graphics.Rect
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.Surface
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.Toast
import com.stab.sculptbridge.ui.AspectViewportLayout
import com.stab.sculptbridge.ui.FloatingExpandTab
import com.stab.sculptbridge.ui.RadiusOverlayView
import com.stab.sculptbridge.ui.SculptPadLayout
import com.stab.sculptbridge.ui.ToolHoverBadge
import com.stab.sculptbridge.video.VideoDecoder
import com.stab.sculptbridge.video.VideoPacketReceiver
import com.stab.sculptbridge.video.VideoSurfaceView

/**
 * Activity Principal do sTab (SculptBridge) no Galaxy Tab S9 FE+.
 * Integra a View de captura 3D a 90Hz, streaming de vídeo H.264 direto da GPU e o painel SculptPad.
 */
class MainActivity : Activity() {

    private lateinit var usbClient: UsbSocketClient
    private lateinit var oneUiShield: OneUiShield
    private lateinit var dispatcher: SculptTouchDispatcher
    private lateinit var videoSurfaceView: VideoSurfaceView
    private lateinit var canvasView: SculptCanvasView
    private lateinit var radiusOverlay: RadiusOverlayView
    private lateinit var sculptPad: SculptPadLayout
    private lateinit var telemetryClient: TelemetryClient

    private var videoDecoder: VideoDecoder? = null
    private var videoReceiver: VideoPacketReceiver? = null

    private lateinit var rootContainer: LinearLayout
    private lateinit var viewportContainer: AspectViewportLayout
    private lateinit var expandTab: FloatingExpandTab
    private var isLeftHandMode = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // 1. Configura blindagem do sistema contra One UI e gestos de borda
        oneUiShield = OneUiShield(window)

        // 2. Trava a taxa de atualização do display em 90Hz no Galaxy Tab S9 FE+
        configure90HzDisplay()

        // 3. Inicializa cliente USB e despachante
        usbClient = UsbSocketClient()
        dispatcher = SculptTouchDispatcher(usbClient)

        // 3.1. Inicializa o cliente de telemetria bidirecional do Blender
        telemetryClient = TelemetryClient().apply {
            onTelemetryReceived = { data ->
                runOnUiThread {
                    sculptPad.updateBrushTelemetry(data)
                }
            }
        }

        // 4. Cria a Viewport: Superfície de Vídeo (Fundo) + Canvas de Captura + Overlay de Raio + HUD Tool Badge
        videoSurfaceView = VideoSurfaceView(this).apply {
            layoutParams = FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            )
            onSurfaceReady = { surface, width, height ->
                setupVideoPipeline(surface, width, height)
            }
            onSurfaceDestroyed = {
                tearDownVideoPipeline()
            }
        }

        canvasView = SculptCanvasView(this).apply {
            this.dispatcher = this@MainActivity.dispatcher
            layoutParams = FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            )
        }

        radiusOverlay = RadiusOverlayView(this).apply {
            layoutParams = FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            )
            visibility = android.view.View.GONE
        }

        viewportContainer = AspectViewportLayout(this).apply {
            layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1f)
            addView(videoSurfaceView)
            addView(canvasView)
            addView(radiusOverlay)
        }

        // 5. Cria a barra lateral tátil compacta SculptPad (54dp de largura)
        val density = resources.displayMetrics.density
        val padWidthPx = (54 * density).toInt()

        sculptPad = SculptPadLayout(
            context = this,
            onActionEmitted = { action ->
                emitSculptPadAction(action)
            },
            onRadiusChanged = { radius, isAdjusting ->
                if (isAdjusting) {
                    radiusOverlay.showRadius(radius)
                } else {
                    radiusOverlay.hideRadius()
                }
            },
            onHandSwapped = {
                swapHandOrientation()
            }
        ).apply {
            layoutParams = LinearLayout.LayoutParams(padWidthPx, LinearLayout.LayoutParams.MATCH_PARENT)
            onMinimizeClicked = {
                visibility = android.view.View.GONE
                expandTab.visibility = android.view.View.VISIBLE
            }
        }

        // 5.1 Aba flutuante de restauração quando a barra estiver minimizada
        expandTab = FloatingExpandTab(this).apply {
            val tabWidth = (24 * density).toInt()
            val tabHeight = (54 * density).toInt()
            layoutParams = LinearLayout.LayoutParams(tabWidth, tabHeight).apply {
                gravity = Gravity.CENTER_VERTICAL
            }
            isLeftEdge = !isLeftHandMode
            visibility = android.view.View.GONE
            onExpandClicked = {
                visibility = android.view.View.GONE
                sculptPad.visibility = android.view.View.VISIBLE
            }
        }

        // 6. Monta o container horizontal
        rootContainer = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            setBackgroundColor(Color.parseColor("#121214"))
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
            )
        }

        // Garante multitoque simultâneo entre botões e área de desenho
        (window.decorView as? ViewGroup)?.isMotionEventSplittingEnabled = true
        rootContainer.isMotionEventSplittingEnabled = true
        sculptPad.isMotionEventSplittingEnabled = true

        assembleLayout()
        setContentView(rootContainer)

        // Aplica modo imersivo e exclusão de gestos direcionada (quota de 200 dp da One UI)
        oneUiShield.setupImmersiveMode(rootContainer)
        oneUiShield.applyTargetedGestureExclusion(rootContainer, sculptPad.getSliderViews())

        usbClient.onConnectionStateChanged = { connected ->
            runOnUiThread {
                val msg = if (connected) "sTab: Conectado ao PC via USB!" else "sTab: Aguardando cabo USB / Servidor..."
                Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun assembleLayout() {
        rootContainer.removeAllViews()
        expandTab.isLeftEdge = !isLeftHandMode
        if (isLeftHandMode) {
            // Modo Canhoto: Viewport na esquerda, SculptPad na borda direita
            rootContainer.addView(viewportContainer)
            rootContainer.addView(sculptPad)
            rootContainer.addView(expandTab)
        } else {
            // Modo Destro (Padrão): SculptPad na borda esquerda, Viewport na direita
            rootContainer.addView(expandTab)
            rootContainer.addView(sculptPad)
            rootContainer.addView(viewportContainer)
        }
    }

    private fun swapHandOrientation() {
        isLeftHandMode = !isLeftHandMode
        sculptPad.updateEdgeMargin(isLeftEdge = !isLeftHandMode)
        expandTab.isLeftEdge = !isLeftHandMode
        assembleLayout()
        val side = if (isLeftHandMode) "Canhoto (Direita)" else "Destro (Esquerda)"
        Toast.makeText(this, "SculptPad alternado para modo $side", Toast.LENGTH_SHORT).show()
    }

    private fun emitSculptPadAction(action: Byte) {
        if (action == Protocol.SCULPTPAD_TOGGLE_DISPLAY_MODE) {
            Toast.makeText(this, "Alternando modo de tela (Estendido / Espelhado)...", Toast.LENGTH_SHORT).show()
        }
        if (action == Protocol.GESTURE_TAP_TWO_FINGERS || action == Protocol.GESTURE_TAP_THREE_FINGERS) {
            val packet = InputPacket(
                packetType = Protocol.PACKET_TYPE_TOUCH_GESTURE,
                flags = Protocol.FLAG_TOUCH_GESTURE_ACTIVE,
                x = 0, y = 0, pressure = 0, tiltX = 0, tiltY = 0,
                payload = (action.toInt() and 0xFF)
            )
            usbClient.sendPacket(packet)
        } else {
            val packet = InputPacket(
                packetType = Protocol.PACKET_TYPE_SCULPTPAD,
                flags = Protocol.FLAG_NONE,
                x = 0, y = 0, pressure = 0, tiltX = 0, tiltY = 0,
                payload = (action.toInt() and 0xFF)
            )
            usbClient.sendPacket(packet)
        }
    }

    private fun setupVideoPipeline(surface: Surface, width: Int, height: Int) {
        tearDownVideoPipeline()

        val decoder = VideoDecoder(
            surface = surface,
            width = width,
            height = height,
            onIdrNeeded = {
                requestKeyFrameFromHost()
            }
        ).apply {
            onResolutionChanged = { w, h ->
                runOnUiThread {
                    if (w > 0 && h > 0) {
                        viewportContainer.targetAspectRatio = w.toFloat() / h.toFloat()
                    }
                }
            }
        }
        decoder.start()
        videoDecoder = decoder

        val receiver = VideoPacketReceiver(
            onPacketReceived = { nalData, size ->
                videoDecoder?.decodeNal(nalData, 0, size)
            }
        )
        receiver.start()
        videoReceiver = receiver

        // Solicita imediatamente um quadro-chave inicial para sincronizar o visor
        requestKeyFrameFromHost()
    }

    private fun tearDownVideoPipeline() {
        videoReceiver?.stop()
        videoReceiver = null
        videoDecoder?.stop()
        videoDecoder = null
    }

    private fun requestKeyFrameFromHost() {
        usbClient.sendPacket(Protocol.createVideoControlPacket(Protocol.VIDEO_CMD_REQUEST_IDR))
    }

    override fun onResume() {
        super.onResume()
        oneUiShield.setupImmersiveMode(window.decorView)
        usbClient.start()
        telemetryClient.start()
        videoReceiver?.start()
    }

    override fun onPause() {
        super.onPause()
        videoReceiver?.stop()
        telemetryClient.stop()
        usbClient.stop()
    }

    override fun dispatchGenericMotionEvent(event: MotionEvent): Boolean {
        if (oneUiShield.isBarrelButtonPressed(event)) {
            canvasView.dispatchGenericMotionEvent(event)
            return true
        }
        return super.dispatchGenericMotionEvent(event)
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        if (oneUiShield.shouldConsumeKeyEvent(event)) {
            return true
        }
        return super.dispatchKeyEvent(event)
    }

    private fun configure90HzDisplay() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            val display = windowManager.defaultDisplay
            val modes = display.supportedModes
            val mode90Hz = modes.firstOrNull { it.refreshRate >= 89.0f }
            if (mode90Hz != null) {
                val params = window.attributes
                params.preferredDisplayModeId = mode90Hz.modeId
                window.attributes = params
            }
        }
    }
}
