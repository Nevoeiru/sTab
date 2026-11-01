package com.stab.sculptbridge

import android.view.MotionEvent

/**
 * Despachante inteligente de eventos de entrada na tela.
 * Separa hardware de S-Pen de dedos e aplica Palm Rejection instantâneo
 * quando a ponta da caneta se aproxima do vidro.
 */
class SculptTouchDispatcher(
    private val client: UsbSocketClient
) {
    val stylusProcessor = StylusMotionProcessor { packet ->
        client.sendPacket(packet)
    }

    val gestureDetector = TouchGestureDetector { packet ->
        client.sendPacket(packet)
    }

    private var isStylusInProximity = false
    var onHoverStateChanged: ((Boolean) -> Unit)? = null

    fun updateDimensions(width: Int, height: Int) {
        stylusProcessor.viewWidth = width
        stylusProcessor.viewHeight = height
    }

    /**
     * Trata eventos de toque com contato (dedos ou caneta).
     */
    fun onTouchEvent(event: MotionEvent): Boolean {
        val toolType = event.getToolType(event.actionIndex)

        if (toolType == MotionEvent.TOOL_TYPE_STYLUS || toolType == MotionEvent.TOOL_TYPE_ERASER) {
            isStylusInProximity = true
            gestureDetector.reset() // Cancela imediatamente qualquer gesto de dedo ativo
            return stylusProcessor.processTouchEvent(event)
        } else if (toolType == MotionEvent.TOOL_TYPE_FINGER) {
            // Se a caneta estiver próxima da tela, bloqueia 100% da palma da mão!
            if (isStylusInProximity) {
                return true // Consome o evento e protege a malha 3D
            }
            return gestureDetector.onTouchEvent(event)
        }
        return false
    }

    /**
     * Trata eventos de aproximação sem contato (Hover da S-Pen).
     */
    fun onHoverEvent(event: MotionEvent): Boolean {
        val toolType = event.getToolType(event.actionIndex)

        if (toolType == MotionEvent.TOOL_TYPE_STYLUS || toolType == MotionEvent.TOOL_TYPE_ERASER) {
            when (event.actionMasked) {
                MotionEvent.ACTION_HOVER_ENTER,
                MotionEvent.ACTION_HOVER_MOVE -> {
                    if (!isStylusInProximity) {
                        onHoverStateChanged?.invoke(true)
                    }
                    isStylusInProximity = true
                    gestureDetector.reset()
                }
                MotionEvent.ACTION_HOVER_EXIT -> {
                    isStylusInProximity = false
                    onHoverStateChanged?.invoke(false)
                }
            }
            return stylusProcessor.processHoverEvent(event)
        }
        return false
    }
}

