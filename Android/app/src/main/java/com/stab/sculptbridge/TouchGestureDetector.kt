package com.stab.sculptbridge

import android.view.MotionEvent
import kotlin.math.abs
import kotlin.math.hypot

/**
 * Detector de gestos multitoque de dedo de alto desempenho (TOOL_TYPE_FINGER).
 * Traduz com precisão cirúrgica:
 * - 1 Dedo: Orbit / Rotação 3D (MMB no Blender)
 * - 2 Dedos Movendo Juntos: Pan / Mover 3D (Shift + MMB no Blender)
 * - 2 Dedos Afastando/Aproximando: Zoom suave na Viewport 3D (Mouse Wheel)
 * - Toques rápidos (Taps): 2 Dedos = Undo (Ctrl+Z) | 3 Dedos = Redo (Ctrl+Shift+Z)
 */
class TouchGestureDetector(
    private val onGesturePacket: (InputPacket) -> Unit
) {
    companion object {
        private const val TAP_TIMEOUT_MS = 240L
        private const val TAP_SLOP_PX = 25f
        private const val PINCH_THRESHOLD_PX = 12f // Limiar de notch do mouse wheel
    }

    private var activeGesture: Byte = Protocol.GESTURE_NONE
    private var lastX = 0f
    private var lastY = 0f
    private var lastSpan = 0f
    private var accumulatedPinchDelta = 0f

    private var touchDownTime = 0L
    private var maxPointersInGesture = 0
    private var hasMovedSignificantly = false

    fun onTouchEvent(event: MotionEvent): Boolean {
        val pointerCount = event.pointerCount
        val action = event.actionMasked

        when (action) {
            MotionEvent.ACTION_DOWN -> {
                touchDownTime = System.currentTimeMillis()
                maxPointersInGesture = 1
                hasMovedSignificantly = false
                accumulatedPinchDelta = 0f

                lastX = event.x
                lastY = event.y
                activeGesture = Protocol.GESTURE_NONE
                return true
            }

            MotionEvent.ACTION_POINTER_DOWN -> {
                if (pointerCount > maxPointersInGesture) {
                    maxPointersInGesture = pointerCount
                }

                // Ao apoiar o segundo dedo, encerra imediatamente rotação de 1 dedo anterior
                if (activeGesture == Protocol.GESTURE_ORBIT_DRAG) {
                    emitGesture(Protocol.GESTURE_ORBIT_DRAG, phase = 2, dx = 0, dy = 0)
                    activeGesture = Protocol.GESTURE_NONE
                }

                if (pointerCount == 2) {
                    lastSpan = calculateSpan(event)
                    lastX = (event.getX(0) + event.getX(1)) / 2f
                    lastY = (event.getY(0) + event.getY(1)) / 2f
                    accumulatedPinchDelta = 0f
                }
                return true
            }

            MotionEvent.ACTION_MOVE -> {
                if (pointerCount == 1) {
                    val dx = event.x - lastX
                    val dy = event.y - lastY

                    if (!hasMovedSignificantly && (abs(dx) > TAP_SLOP_PX || abs(dy) > TAP_SLOP_PX)) {
                        hasMovedSignificantly = true
                    }

                    if (hasMovedSignificantly && maxPointersInGesture == 1) {
                        // 1 Dedo = Orbit 3D (MMB)
                        if (activeGesture != Protocol.GESTURE_ORBIT_DRAG) {
                            emitGesture(Protocol.GESTURE_ORBIT_DRAG, phase = 0, dx = 0, dy = 0)
                            activeGesture = Protocol.GESTURE_ORBIT_DRAG
                        }
                        emitGesture(Protocol.GESTURE_ORBIT_DRAG, phase = 1, dx = dx.toInt(), dy = dy.toInt())
                        lastX = event.x
                        lastY = event.y
                    }

                } else if (pointerCount == 2) {
                    val currentSpan = calculateSpan(event)
                    val spanDelta = currentSpan - lastSpan

                    val midX = (event.getX(0) + event.getX(1)) / 2f
                    val midY = (event.getY(0) + event.getY(1)) / 2f
                    val midDx = midX - lastX
                    val midDy = midY - lastY

                    val spanChangeAbs = abs(spanDelta)
                    val moveChangeAbs = hypot(midDx.toDouble(), midDy.toDouble()).toFloat()

                    if (!hasMovedSignificantly && (spanChangeAbs > 10f || moveChangeAbs > TAP_SLOP_PX)) {
                        hasMovedSignificantly = true
                    }

                    if (hasMovedSignificantly) {
                        accumulatedPinchDelta += spanDelta

                        // 1. Zoom na Viewport se a variação de distância for dominante
                        if (abs(accumulatedPinchDelta) >= PINCH_THRESHOLD_PX) {
                            if (activeGesture == Protocol.GESTURE_PAN_DRAG) {
                                emitGesture(Protocol.GESTURE_PAN_DRAG, phase = 2, dx = 0, dy = 0)
                                activeGesture = Protocol.GESTURE_NONE
                            }
                            val wheelNotch = if (accumulatedPinchDelta > 0) 120 else -120
                            emitGesture(Protocol.GESTURE_PINCH_ZOOM, phase = 1, dx = 0, dy = wheelNotch)
                            accumulatedPinchDelta = 0f
                        }

                        // 2. Pan 3D (Shift + MMB) se o deslocamento do ponto médio for dominante
                        if (moveChangeAbs > 2.5f) {
                            if (activeGesture != Protocol.GESTURE_PAN_DRAG) {
                                emitGesture(Protocol.GESTURE_PAN_DRAG, phase = 0, dx = 0, dy = 0)
                                activeGesture = Protocol.GESTURE_PAN_DRAG
                            }
                            emitGesture(Protocol.GESTURE_PAN_DRAG, phase = 1, dx = midDx.toInt(), dy = midDy.toInt())
                        }
                    }

                    lastSpan = currentSpan
                    lastX = midX
                    lastY = midY
                }
                return true
            }

            MotionEvent.ACTION_POINTER_UP -> {
                // Ao levantar um dos dedos do gesto multitoque, finaliza Pan imediatamente
                if (activeGesture == Protocol.GESTURE_PAN_DRAG) {
                    emitGesture(Protocol.GESTURE_PAN_DRAG, phase = 2, dx = 0, dy = 0)
                    activeGesture = Protocol.GESTURE_NONE
                }
                // Previne que o dedo remanescente engate um Orbit indesejado
                maxPointersInGesture = maxOf(maxPointersInGesture, pointerCount)
                hasMovedSignificantly = true
                return true
            }

            MotionEvent.ACTION_UP -> {
                val duration = System.currentTimeMillis() - touchDownTime
                if (!hasMovedSignificantly && duration < TAP_TIMEOUT_MS) {
                    // Toques rápidos (Taps) para Undo / Redo
                    if (maxPointersInGesture == 2) {
                        emitGesture(Protocol.GESTURE_TAP_TWO_FINGERS, phase = 0, dx = 0, dy = 0)
                    } else if (maxPointersInGesture >= 3) {
                        emitGesture(Protocol.GESTURE_TAP_THREE_FINGERS, phase = 0, dx = 0, dy = 0)
                    }
                }

                reset()
                return true
            }

            MotionEvent.ACTION_CANCEL -> {
                reset()
                return true
            }
        }
        return false
    }

    fun reset() {
        if (activeGesture != Protocol.GESTURE_NONE) {
            emitGesture(activeGesture, phase = 2, dx = 0, dy = 0)
            activeGesture = Protocol.GESTURE_NONE
        }
        emitGesture(Protocol.GESTURE_STATE_END, phase = 0, dx = 0, dy = 0)
        hasMovedSignificantly = false
        accumulatedPinchDelta = 0f
    }

    private fun calculateSpan(event: MotionEvent): Float {
        if (event.pointerCount < 2) return 0f
        val dx = event.getX(0) - event.getX(1)
        val dy = event.getY(0) - event.getY(1)
        return hypot(dx.toDouble(), dy.toDouble()).toFloat()
    }

    private fun emitGesture(gestureType: Byte, phase: Int, dx: Int, dy: Int) {
        val payload = (gestureType.toInt() and 0xFF) or ((phase and 0xFF) shl 8)
        val packet = InputPacket(
            packetType = Protocol.PACKET_TYPE_TOUCH_GESTURE,
            flags = Protocol.FLAG_TOUCH_GESTURE_ACTIVE,
            x = 0,
            y = 0,
            pressure = 0,
            tiltX = dx.toShort(),
            tiltY = dy.toShort(),
            payload = payload
        )
        onGesturePacket(packet)
    }
}
