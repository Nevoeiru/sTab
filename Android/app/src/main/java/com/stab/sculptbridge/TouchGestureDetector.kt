package com.stab.sculptbridge

import android.view.MotionEvent
import kotlin.math.abs
import kotlin.math.hypot

/**
 * Detector de gestos multitoque de dedo na tela (TOOL_TYPE_FINGER).
 * Traduz movimentos de 1 dedo (Orbit), 2 dedos (Pan/Zoom) e toques rápidos (Undo/Redo).
 */
class TouchGestureDetector(
    private val onGesturePacket: (InputPacket) -> Unit
) {
    companion object {
        private const val TAP_TIMEOUT_MS = 250L
        private const val TAP_SLOP_PX = 30f
        private const val ZOOM_SENSITIVITY = 4.0f
    }

    private var activeGesture: Byte = Protocol.GESTURE_NONE
    private var lastX = 0f
    private var lastY = 0f
    private var lastSpan = 0f

    private var touchDownTime = 0L
    private var touchDownPointers = 0
    private var maxPointersInGesture = 0
    private var hasMovedSignificantly = false

    /**
     * Processa eventos brutos de toque gerados exclusivamente por dedos.
     */
    fun onTouchEvent(event: MotionEvent): Boolean {
        val pointerCount = event.pointerCount
        val action = event.actionMasked

        when (action) {
            MotionEvent.ACTION_DOWN -> {
                touchDownTime = System.currentTimeMillis()
                touchDownPointers = 1
                maxPointersInGesture = 1
                hasMovedSignificantly = false

                lastX = event.x
                lastY = event.y
                activeGesture = Protocol.GESTURE_NONE
                return true
            }

            MotionEvent.ACTION_POINTER_DOWN -> {
                if (pointerCount > maxPointersInGesture) {
                    maxPointersInGesture = pointerCount
                }
                if (pointerCount == 2) {
                    lastSpan = calculateSpan(event)
                    lastX = (event.getX(0) + event.getX(1)) / 2f
                    lastY = (event.getY(0) + event.getY(1)) / 2f
                }
                return true
            }

            MotionEvent.ACTION_MOVE -> {
                val dx = (event.x - lastX)
                val dy = (event.y - lastY)

                if (!hasMovedSignificantly && (abs(dx) > TAP_SLOP_PX || abs(dy) > TAP_SLOP_PX)) {
                    hasMovedSignificantly = true
                }

                if (pointerCount == 1 && hasMovedSignificantly) {
                    // 1 Dedo = Orbit 3D (MMB no Blender)
                    if (activeGesture != Protocol.GESTURE_ORBIT_DRAG) {
                        emitGesture(Protocol.GESTURE_ORBIT_DRAG, phase = 0, dx = 0, dy = 0)
                        activeGesture = Protocol.GESTURE_ORBIT_DRAG
                    }
                    emitGesture(Protocol.GESTURE_ORBIT_DRAG, phase = 1, dx = dx.toInt(), dy = dy.toInt())
                    lastX = event.x
                    lastY = event.y

                } else if (pointerCount == 2 && hasMovedSignificantly) {
                    val currentSpan = calculateSpan(event)
                    val spanDelta = currentSpan - lastSpan
                    val midX = (event.getX(0) + event.getX(1)) / 2f
                    val midY = (event.getY(0) + event.getY(1)) / 2f
                    val midDx = midX - lastX
                    val midDy = midY - lastY

                    if (abs(spanDelta) > 15f) {
                        // Gesto de Pinça = Zoom na Viewport
                        val wheelDelta = (spanDelta * ZOOM_SENSITIVITY).toInt()
                        emitGesture(Protocol.GESTURE_PINCH_ZOOM, phase = 1, dx = 0, dy = wheelDelta)
                    } else if (abs(midDx) > 2f || abs(midDy) > 2f) {
                        // 2 Dedos Paralelos = Pan 3D (Shift + MMB)
                        if (activeGesture != Protocol.GESTURE_PAN_DRAG) {
                            emitGesture(Protocol.GESTURE_PAN_DRAG, phase = 0, dx = 0, dy = 0)
                            activeGesture = Protocol.GESTURE_PAN_DRAG
                        }
                        emitGesture(Protocol.GESTURE_PAN_DRAG, phase = 1, dx = midDx.toInt(), dy = midDy.toInt())
                    }

                    lastSpan = currentSpan
                    lastX = midX
                    lastY = midY
                }
                return true
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                if (action == MotionEvent.ACTION_UP) {
                    val duration = System.currentTimeMillis() - touchDownTime
                    if (!hasMovedSignificantly && duration < TAP_TIMEOUT_MS) {
                        // Toques rápidos (Taps) para Undo / Redo
                        if (maxPointersInGesture == 2) {
                            emitGesture(Protocol.GESTURE_TAP_TWO_FINGERS, phase = 0, dx = 0, dy = 0)
                        } else if (maxPointersInGesture >= 3) {
                            emitGesture(Protocol.GESTURE_TAP_THREE_FINGERS, phase = 0, dx = 0, dy = 0)
                        }
                    }

                    // Encerra qualquer gesto contínuo ativo
                    if (activeGesture != Protocol.GESTURE_NONE) {
                        emitGesture(activeGesture, phase = 2, dx = 0, dy = 0)
                        activeGesture = Protocol.GESTURE_NONE
                    }
                    emitGesture(Protocol.GESTURE_STATE_END, phase = 0, dx = 0, dy = 0)
                }
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

