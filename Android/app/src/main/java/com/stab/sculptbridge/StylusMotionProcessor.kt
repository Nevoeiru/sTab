package com.stab.sculptbridge

import android.view.MotionEvent
import kotlin.math.asin
import kotlin.math.cos
import kotlin.math.sin

/**
 * Processador de telemetria de alta frequência da S-Pen (Wacom EMR a 240Hz).
 * Lê histórico de sub-frames, normaliza pressão (0..4096) e converte ângulos polares de inclinação
 * em coordenadas cartesianas (tiltX / tiltY) esperadas pelo Windows Ink e pelo Blender.
 */
class StylusMotionProcessor(
    private val onPacketReady: (InputPacket) -> Unit
) {
    var viewWidth: Int = 2560
    var viewHeight: Int = 1600

    /**
     * Processa eventos de contato físico (ACTION_DOWN, ACTION_MOVE, ACTION_UP).
     */
    fun processTouchEvent(event: MotionEvent): Boolean {
        val action = event.actionMasked
        val pointerIndex = event.actionIndex

        if (event.getToolType(pointerIndex) != MotionEvent.TOOL_TYPE_STYLUS &&
            event.getToolType(pointerIndex) != MotionEvent.TOOL_TYPE_ERASER) {
            return false
        }

        when (action) {
            MotionEvent.ACTION_DOWN -> {
                emitSingleSample(event, pointerIndex, isContact = true)
                return true
            }

            MotionEvent.ACTION_MOVE -> {
                // Lê todo o histórico de sub-frames capturados pelo digitalizador Wacom entre frames de tela
                val historySize = event.historySize
                for (h in 0 until historySize) {
                    emitHistoricalSample(event, pointerIndex, h, isContact = true)
                }
                emitSingleSample(event, pointerIndex, isContact = true)
                return true
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                emitSingleSample(event, pointerIndex, isContact = false)
                return true
            }
        }
        return false
    }

    /**
     * Processa eventos de aproximação sem contato físico (Hover a ~15mm do vidro).
     */
    fun processHoverEvent(event: MotionEvent): Boolean {
        val pointerIndex = event.actionIndex
        if (event.getToolType(pointerIndex) != MotionEvent.TOOL_TYPE_STYLUS &&
            event.getToolType(pointerIndex) != MotionEvent.TOOL_TYPE_ERASER) {
            return false
        }

        when (event.actionMasked) {
            MotionEvent.ACTION_HOVER_ENTER,
            MotionEvent.ACTION_HOVER_MOVE -> {
                emitSingleSample(event, pointerIndex, isContact = false)
                return true
            }
            MotionEvent.ACTION_HOVER_EXIT -> {
                emitHoverExitSample(event, pointerIndex)
                return true
            }
        }
        return false
    }

    private fun emitSingleSample(event: MotionEvent, pointerIndex: Int, isContact: Boolean) {
        val rawX = event.getX(pointerIndex)
        val rawY = event.getY(pointerIndex)
        val rawPressure = event.getPressure(pointerIndex)
        val tiltRad = event.getAxisValue(MotionEvent.AXIS_TILT, pointerIndex)
        val orientRad = event.getAxisValue(MotionEvent.AXIS_ORIENTATION, pointerIndex)
        val isEraser = event.getToolType(pointerIndex) == MotionEvent.TOOL_TYPE_ERASER

        val isBarrel = (event.buttonState and MotionEvent.BUTTON_STYLUS_PRIMARY) != 0 ||
                       (event.buttonState and MotionEvent.BUTTON_SECONDARY) != 0

        val (normX, normY) = normalizeCoordinates(rawX, rawY)
        val normPressure = if (isContact) (rawPressure * 4096f).toInt().coerceIn(0, 4096) else 0
        val (tiltX, tiltY) = convertPolarTiltToCartesian(tiltRad, orientRad)

        var flags: Byte = Protocol.FLAG_PEN_HOVER
        if (isContact) flags = (flags.toInt() or Protocol.FLAG_PEN_CONTACT.toInt()).toByte()
        if (isBarrel)  flags = (flags.toInt() or Protocol.FLAG_PEN_BARREL_BUTTON.toInt()).toByte()
        if (isEraser)  flags = (flags.toInt() or Protocol.FLAG_PEN_INVERTED_ERASER.toInt()).toByte()

        val packet = InputPacket(
            packetType = Protocol.PACKET_TYPE_PEN,
            flags = flags,
            x = normX,
            y = normY,
            pressure = normPressure,
            tiltX = tiltX,
            tiltY = tiltY,
            payload = 0
        )
        onPacketReady(packet)
    }

    private fun emitHistoricalSample(event: MotionEvent, pointerIndex: Int, historyIndex: Int, isContact: Boolean) {
        val rawX = event.getHistoricalX(pointerIndex, historyIndex)
        val rawY = event.getHistoricalY(pointerIndex, historyIndex)
        val rawPressure = event.getHistoricalPressure(pointerIndex, historyIndex)
        val tiltRad = event.getHistoricalAxisValue(MotionEvent.AXIS_TILT, pointerIndex, historyIndex)
        val orientRad = event.getHistoricalAxisValue(MotionEvent.AXIS_ORIENTATION, pointerIndex, historyIndex)
        val isEraser = event.getToolType(pointerIndex) == MotionEvent.TOOL_TYPE_ERASER
        val isBarrel = (event.buttonState and MotionEvent.BUTTON_STYLUS_PRIMARY) != 0

        val (normX, normY) = normalizeCoordinates(rawX, rawY)
        val normPressure = if (isContact) (rawPressure * 4096f).toInt().coerceIn(0, 4096) else 0
        val (tiltX, tiltY) = convertPolarTiltToCartesian(tiltRad, orientRad)

        var flags: Byte = Protocol.FLAG_PEN_HOVER
        if (isContact) flags = (flags.toInt() or Protocol.FLAG_PEN_CONTACT.toInt()).toByte()
        if (isBarrel)  flags = (flags.toInt() or Protocol.FLAG_PEN_BARREL_BUTTON.toInt()).toByte()
        if (isEraser)  flags = (flags.toInt() or Protocol.FLAG_PEN_INVERTED_ERASER.toInt()).toByte()

        val packet = InputPacket(
            packetType = Protocol.PACKET_TYPE_PEN,
            flags = flags,
            x = normX,
            y = normY,
            pressure = normPressure,
            tiltX = tiltX,
            tiltY = tiltY,
            payload = 0
        )
        onPacketReady(packet)
    }

    private fun emitHoverExitSample(event: MotionEvent, pointerIndex: Int) {
        val (normX, normY) = normalizeCoordinates(event.getX(pointerIndex), event.getY(pointerIndex))
        val packet = InputPacket(
            packetType = Protocol.PACKET_TYPE_PEN,
            flags = Protocol.FLAG_NONE, // Sem hover nem contato
            x = normX,
            y = normY,
            pressure = 0,
            tiltX = 0,
            tiltY = 0,
            payload = 0
        )
        onPacketReady(packet)
    }

    private fun normalizeCoordinates(rawX: Float, rawY: Float): Pair<Int, Int> {
        val w = if (viewWidth > 0) viewWidth.toFloat() else 2560f
        val h = if (viewHeight > 0) viewHeight.toFloat() else 1600f

        val normX = ((rawX / w) * 65535f).toInt().coerceIn(0, 65535)
        val normY = ((rawY / h) * 65535f).toInt().coerceIn(0, 65535)
        return Pair(normX, normY)
    }

    /**
     * Converte os eixos polares do Android (AXIS_TILT em radianos e AXIS_ORIENTATION)
     * para coordenadas cartesianas de inclinação X e Y (-90° a +90°) conforme padrão Windows Ink:
     * tiltX = asin(sin(tilt) * sin(orientation)) * (180 / PI)
     * tiltY = asin(-sin(tilt) * cos(orientation)) * (180 / PI)
     */
    private fun convertPolarTiltToCartesian(tiltRad: Float, orientRad: Float): Pair<Short, Short> {
        val sinTilt = sin(tiltRad.toDouble())
        val tiltXDeg = Math.toDegrees(asin(sinTilt * sin(orientRad.toDouble()))).toInt().coerceIn(-90, 90)
        val tiltYDeg = Math.toDegrees(asin(-sinTilt * cos(orientRad.toDouble()))).toInt().coerceIn(-90, 90)
        return Pair(tiltXDeg.toShort(), tiltYDeg.toShort())
    }
}

