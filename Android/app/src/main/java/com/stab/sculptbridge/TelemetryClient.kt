package com.stab.sculptbridge

import android.os.Handler
import android.os.Looper
import android.util.Log
import org.json.JSONObject
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Cliente de telemetria bidirecional do Blender via túnel ADB USB (127.0.0.1:27185).
 * Recebe atualizações em tempo real do pincel ativo, tamanho, força e simetria do Blender.
 */
class TelemetryClient(
    private val host: String = "127.0.0.1",
    private val port: Int = 27185
) {
    companion object {
        private const val TAG = "sTab_TelemetryClient"
        private const val RECONNECT_DELAY_MS = 2000L
    }

    private val isRunning = AtomicBoolean(false)
    private var workerThread: Thread? = null
    private var activeSocket: Socket? = null
    private val mainHandler = Handler(Looper.getMainLooper())

    var onTelemetryReceived: ((TelemetryData) -> Unit)? = null

    fun start() {
        if (isRunning.getAndSet(true)) return

        workerThread = Thread({ runReceiverLoop() }, "sTab_TelemetryThread").apply {
            isDaemon = true
            start()
        }
    }

    fun stop() {
        isRunning.set(false)
        try {
            activeSocket?.close()
        } catch (_: Exception) {}
        activeSocket = null

        workerThread?.interrupt()
        workerThread = null
    }

    private fun runReceiverLoop() {
        while (isRunning.get()) {
            var socket: Socket? = null
            try {
                socket = Socket()
                socket.tcpNoDelay = true
                socket.connect(InetSocketAddress(host, port), 2000)
                activeSocket = socket

                val reader = BufferedReader(InputStreamReader(socket.getInputStream()))

                while (isRunning.get()) {
                    val line = reader.readLine() ?: break
                    if (line.isBlank()) continue

                    try {
                        val json = JSONObject(line)
                        val brushName = json.optString("brush", "Sculpt")
                        val size = json.optInt("size", 50)
                        val strength = json.optDouble("strength", 0.5).toFloat()
                        val symmetryX = json.optBoolean("symmetry_x", false)

                        val data = TelemetryData(
                            brushName = brushName,
                            size = size,
                            strength = strength,
                            symmetryX = symmetryX
                        )

                        mainHandler.post {
                            onTelemetryReceived?.invoke(data)
                        }
                    } catch (e: Exception) {
                        Log.w(TAG, "Falha ao decodificar JSON de telemetria: ${e.message}")
                    }
                }
            } catch (_: Exception) {
                // Conexão encerrada ou Blender offline
            } finally {
                try {
                    socket?.close()
                } catch (_: Exception) {}
                activeSocket = null
            }

            if (isRunning.get()) {
                try {
                    Thread.sleep(RECONNECT_DELAY_MS)
                } catch (_: InterruptedException) {
                    break
                }
            }
        }
    }
}

