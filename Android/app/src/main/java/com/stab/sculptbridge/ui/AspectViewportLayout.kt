package com.stab.sculptbridge.ui

import android.content.Context
import android.graphics.Color
import android.util.AttributeSet
import android.view.View
import android.widget.FrameLayout

/**
 * Container ergonômico que preserva a proporção nativa exata 16:9 (1920x1080)
 * da tela do PC no display WQXGA (16:10) do tablet Galaxy Tab S9 FE+.
 * 
 * Centraliza a superfície de vídeo (SurfaceView) e a camada de captura S-Pen (SculptCanvasView)
 * garantindo alinhamento pixel-a-pixel sem compressão horizontal ou distorções.
 */
class AspectViewportLayout @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    var targetAspectRatio: Float = 16.0f / 9.0f // 1920x1080 padrão
        set(value) {
            if (field != value && value > 0f) {
                field = value
                requestLayout()
            }
        }

    private var viewportWidth = 0
    private var viewportHeight = 0

    init {
        setBackgroundColor(Color.parseColor("#121214"))
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val availWidth = MeasureSpec.getSize(widthMeasureSpec)
        val availHeight = MeasureSpec.getSize(heightMeasureSpec)

        if (availWidth <= 0 || availHeight <= 0) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec)
            return
        }

        val availableRatio = availWidth.toFloat() / availHeight.toFloat()

        if (availableRatio > targetAspectRatio) {
            // Mais largo que 16:9 -> ajusta largura para manter proporção (Pillarbox nas laterais)
            viewportHeight = availHeight
            viewportWidth = (availHeight * targetAspectRatio).toInt()
        } else {
            // Mais alto que 16:9 -> ajusta altura para manter proporção (Letterbox no topo/base)
            viewportWidth = availWidth
            viewportHeight = (availWidth / targetAspectRatio).toInt()
        }

        val childWidthSpec = MeasureSpec.makeMeasureSpec(viewportWidth, MeasureSpec.EXACTLY)
        val childHeightSpec = MeasureSpec.makeMeasureSpec(viewportHeight, MeasureSpec.EXACTLY)

        for (i in 0 until childCount) {
            val child = getChildAt(i)
            if (child.visibility != View.GONE) {
                child.measure(childWidthSpec, childHeightSpec)
            }
        }

        setMeasuredDimension(availWidth, availHeight)
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        val containerWidth = right - left
        val containerHeight = bottom - top

        val childLeft = (containerWidth - viewportWidth) / 2
        val childTop = (containerHeight - viewportHeight) / 2
        val childRight = childLeft + viewportWidth
        val childBottom = childTop + viewportHeight

        for (i in 0 until childCount) {
            val child = getChildAt(i)
            if (child.visibility != View.GONE) {
                child.layout(childLeft, childTop, childRight, childBottom)
            }
        }
    }
}

