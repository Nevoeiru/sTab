package com.stab.sculptbridge.video

import android.media.MediaCodec
import android.media.MediaFormat
import android.os.Build
import android.util.Log
import android.view.Surface
import java.nio.ByteBuffer

/**
 * Decodificador H.264 de Ultra-Baixa Latência para o Galaxy Tab S9 FE+.
 * Utiliza aceleração por hardware MediaCodec configurado com KEY_LOW_LATENCY e KEY_OPERATING_RATE = 90Hz.
 * Renderiza os quadros diretamente na Surface sem buffering intermediário.
 */
class VideoDecoder(
    private val surface: Surface,
    private val width: Int = 1920,
    private val height: Int = 1080,
    private val onIdrNeeded: () -> Unit
) {
    companion object {
        private const val TAG = "sTab_VideoDecoder"
        private const val MIME_TYPE = MediaFormat.MIMETYPE_VIDEO_AVC
    }

    private var codec: MediaCodec? = null
    private var isConfigured = false
    private var consecutiveErrors = 0
    var onResolutionChanged: ((Int, Int) -> Unit)? = null

    fun start() {
        if (isConfigured) return

        try {
            codec = MediaCodec.createDecoderByType(MIME_TYPE)
            val format = MediaFormat.createVideoFormat(MIME_TYPE, width, height).apply {
                // Modo de baixa latência em tempo real (Android 11+)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    setInteger(MediaFormat.KEY_LOW_LATENCY, 1)
                }
                // Prioridade máxima em tempo real (0 = Realtime, 1 = Best-effort)
                setInteger(MediaFormat.KEY_PRIORITY, 0)
                try {
                    // Configura taxa de operacao do decodificador para 60 FPS de baixa latencia
                    setFloat(MediaFormat.KEY_OPERATING_RATE, 60.0f)
                } catch (_: Exception) {}
            }

            codec?.configure(format, surface, null, 0)
            codec?.start()
            isConfigured = true
            consecutiveErrors = 0
            Log.i(TAG, "MediaCodec H.264 Low-Latency iniciado com sucesso ($width x $height @ 60 FPS).")
        } catch (e: Exception) {
            Log.e(TAG, "Falha ao inicializar o MediaCodec H.264: ${e.message}", e)
            codec = null
            isConfigured = false
        }
    }

    /**
     * Alimenta um pacote NAL H.264 recebido da rede diretamente na fila de entrada do hardware.
     */
    fun decodeNal(data: ByteArray, offset: Int, length: Int) {
        val activeCodec = codec ?: return
        if (!isConfigured) return

        try {
            val inIndex = activeCodec.dequeueInputBuffer(2000L) // 2ms timeout
            if (inIndex >= 0) {
                val inputBuffer = activeCodec.getInputBuffer(inIndex)
                if (inputBuffer != null) {
                    inputBuffer.clear()
                    inputBuffer.put(data, offset, length)
                    val ptsUs = System.nanoTime() / 1000L
                    activeCodec.queueInputBuffer(inIndex, 0, length, ptsUs, 0)
                }
                consecutiveErrors = 0
            } else {
                consecutiveErrors++
                if (consecutiveErrors > 15) {
                    // Buffer de entrada saturado ou quadro quebrado - solicita IDR
                    Log.w(TAG, "Timeout excessivo de entrada ($consecutiveErrors). Solicitando IDR Frame...")
                    onIdrNeeded()
                    consecutiveErrors = 0
                }
            }

            drainOutput()
        } catch (e: Exception) {
            Log.e(TAG, "Erro durante alimentacao do decodificador: ${e.message}")
            onIdrNeeded()
        }
    }

    /**
     * Drena os quadros decodificados imediatamente para a Surface (Zero-Copy).
     */
    private fun drainOutput() {
        val activeCodec = codec ?: return
        val bufferInfo = MediaCodec.BufferInfo()

        while (true) {
            val outIndex = activeCodec.dequeueOutputBuffer(bufferInfo, 0L)
            when {
                outIndex >= 0 -> {
                    // true = renderiza imediatamente na Surface do display
                    activeCodec.releaseOutputBuffer(outIndex, true)
                }
                outIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED -> {
                    val newFormat = activeCodec.outputFormat
                    val w = try { newFormat.getInteger(MediaFormat.KEY_WIDTH) } catch (_: Exception) { width }
                    val h = try { newFormat.getInteger(MediaFormat.KEY_HEIGHT) } catch (_: Exception) { height }
                    Log.i(TAG, "Formato de saida alterado: ${w}x${h}")
                    onResolutionChanged?.invoke(w, h)
                }
                else -> {
                    // Sem quadros pendentes prontos neste instante
                    break
                }
            }
        }
    }

    fun stop() {
        try {
            codec?.stop()
        } catch (_: Exception) {}
        try {
            codec?.release()
        } catch (_: Exception) {}
        codec = null
        isConfigured = false
        Log.i(TAG, "MediaCodec liberado.")
    }
}

