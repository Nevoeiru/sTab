package com.stab.sculptbridge

import android.graphics.Rect
import android.os.Build
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.Window
import android.view.WindowManager
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat

/**
 * Blindagem de Sistema contra interferências da Samsung One UI e do Android.
 * Neutraliza o menu Air Command da S-Pen e desativa gestos de borda da tela.
 */
class OneUiShield(private val window: Window) {

    /**
     * Configura a tela cheia imersiva absoluta sem barras de status ou navegação.
     */
    fun setupImmersiveMode(rootView: View) {
        WindowCompat.setDecorFitsSystemWindows(window, false)

        val controller = WindowInsetsControllerCompat(window, rootView)
        controller.hide(WindowInsetsCompat.Type.systemBars())
        controller.systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            window.attributes.layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        }

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
    }

    /**
     * Aplica exclusão de gestos de borda (API 29+) para evitar que arrastos
     * perto das laterais acionem o comando de "Voltar" do Android.
     */
    fun applyGestureExclusion(view: View) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            view.addOnLayoutChangeListener { v, left, top, right, bottom, _, _, _, _ ->
                val width = right - left
                val height = bottom - top
                if (width > 0 && height > 0) {
                    // Protege toda a área do canvas e das bordas laterais
                    val rects = listOf(Rect(0, 0, width, height))
                    v.systemGestureExclusionRects = rects
                }
            }
        }
    }

    /**
     * Intercepta o botão físico da S-Pen antes que o serviço da Samsung abra o Air Command.
     * Retorna true se for um evento do botão primário da caneta.
     */
    fun isBarrelButtonPressed(event: MotionEvent): Boolean {
        val buttonState = event.buttonState
        val isBarrel = (buttonState and MotionEvent.BUTTON_STYLUS_PRIMARY) != 0 ||
                       (buttonState and MotionEvent.BUTTON_SECONDARY) != 0
        return isBarrel
    }

    /**
     * Intercepta eventos de tecla de hardware da S-Pen.
     */
    fun shouldConsumeKeyEvent(event: KeyEvent): Boolean {
        return when (event.keyCode) {
            KeyEvent.KEYCODE_STYLUS_BUTTON_PRIMARY,
            KeyEvent.KEYCODE_STYLUS_BUTTON_TAIL,
            KeyEvent.KEYCODE_BUTTON_1,
            KeyEvent.KEYCODE_BUTTON_2 -> true
            else -> false
        }
    }
}

