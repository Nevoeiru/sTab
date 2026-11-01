package com.stab.sculptbridge

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View

/**
 * View de captura de alta performance a 90Hz para a viewport de escultura 3D.
 * Conecta o hardware Wacom EMR e os gestos de multitoque ao despachante do sTab.
 */
class SculptCanvasView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    var dispatcher: SculptTouchDispatcher? = null

    // Feedback visual leve sob a ponta da S-Pen (Anel Lineart)
    private val penHoverPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 2.5f
        color = Color.parseColor("#4FC3F7") // Azul ciano sutil
    }

    private var cursorX = -100f
    private var cursorY = -100f
    private var cursorRadius = 16f
    private var isCursorVisible = false

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        dispatcher?.updateDimensions(w, h)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        updateCursorPosition(event)
        val handled = dispatcher?.onTouchEvent(event) ?: false
        return handled || super.onTouchEvent(event)
    }

    override fun onHoverEvent(event: MotionEvent): Boolean {
        updateCursorPosition(event)
        val handled = dispatcher?.onHoverEvent(event) ?: false
        return handled || super.onHoverEvent(event)
    }

    override fun onGenericMotionEvent(event: MotionEvent): Boolean {
        updateCursorPosition(event)
        val handled = dispatcher?.onHoverEvent(event) ?: false
        return handled || super.onGenericMotionEvent(event)
    }

    private fun updateCursorPosition(event: MotionEvent) {
        val tool = event.getToolType(event.actionIndex)
        if (tool == MotionEvent.TOOL_TYPE_STYLUS || tool == MotionEvent.TOOL_TYPE_ERASER) {
            cursorX = event.x
            cursorY = event.y
            val pressure = event.pressure
            cursorRadius = 14f + (pressure * 20f)
            isCursorVisible = (event.actionMasked != MotionEvent.ACTION_HOVER_EXIT &&
                               event.actionMasked != MotionEvent.ACTION_UP)
            invalidate()
        }
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if (isCursorVisible) {
            // Desenha o retículo/anel lineart indicativo do cursor da S-Pen
            canvas.drawCircle(cursorX, cursorY, cursorRadius, penHoverPaint)
        }
    }
}

