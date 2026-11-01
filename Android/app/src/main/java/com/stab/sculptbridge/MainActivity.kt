package com.stab.sculptbridge

import android.app.Activity
import android.os.Build
import android.os.Bundle
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.widget.FrameLayout
import android.widget.Toast

/**
 * Activity Principal do sTab (SculptBridge) no Galaxy Tab S9 FE+.
 * Configura taxa de 90Hz nativa, tela cheia imersiva, blindagem One UI e despacho USB.
 */
class MainActivity : Activity() {

    private lateinit var usbClient: UsbSocketClient
    private lateinit var oneUiShield: OneUiShield
    private lateinit var dispatcher: SculptTouchDispatcher
    private lateinit var canvasView: SculptCanvasView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // 1. Configura a blindagem da One UI e modo imersivo
        oneUiShield = OneUiShield(window)

        // 2. Trava a taxa de atualização do display em 90Hz no Galaxy Tab S9 FE+
        configure90HzDisplay()

        // 3. Inicializa o cliente USB local (porta 27183)
        usbClient = UsbSocketClient()
        dispatcher = SculptTouchDispatcher(usbClient)

        // 4. Cria a View de captura em tela cheia
        canvasView = SculptCanvasView(this).apply {
            this.dispatcher = this@MainActivity.dispatcher
            layoutParams = FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            )
        }

        val rootLayout = FrameLayout(this).apply {
            addView(canvasView)
            setBackgroundColor(android.graphics.Color.parseColor("#121214"))
        }

        setContentView(rootLayout)

        // Aplica exclusão de gestos de borda da tela
        oneUiShield.setupImmersiveMode(rootLayout)
        oneUiShield.applyGestureExclusion(rootLayout)

        usbClient.onConnectionStateChanged = { connected ->
            runOnUiThread {
                val msg = if (connected) "sTab: Conectado ao PC via USB!" else "sTab: Aguardando cabo USB / Servidor..."
                Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            }
        }
    }

    override fun onResume() {
        super.onResume()
        oneUiShield.setupImmersiveMode(window.decorView)
        usbClient.start()
    }

    override fun onPause() {
        super.onPause()
        usbClient.stop()
    }

    /**
     * Intercepta o botão físico da S-Pen para evitar que o menu Air Command da Samsung abra.
     */
    override fun dispatchGenericMotionEvent(event: MotionEvent): Boolean {
        if (oneUiShield.isBarrelButtonPressed(event)) {
            // Repassa para o despachante e consome o evento
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

