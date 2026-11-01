package com.stab.sculptbridge.video

import android.content.Context
import android.util.AttributeSet
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView

/**
 * View dedicada com aceleração direta por hardware para renderização de vídeo H.264 do Blender.
 * Posicionada no nível inferior da Viewport para servir de tela primária da escultura 3D.
 */
class VideoSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : SurfaceView(context, attrs, defStyleAttr), SurfaceHolder.Callback {

    var onSurfaceReady: ((Surface, Int, Int) -> Unit)? = null
    var onSurfaceDestroyed: (() -> Unit)? = null

    init {
        holder.addCallback(this)
        // Garante que a surface de vídeo fique atrás das sobreposições de S-Pen e UI
        setZOrderMediaOverlay(false)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        // Surface criada; aguarda dimensions em surfaceChanged
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (holder.surface.isValid) {
            onSurfaceReady?.invoke(holder.surface, width, height)
        }
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        onSurfaceDestroyed?.invoke()
    }
}

