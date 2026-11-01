package com.stab.sculptbridge

import android.util.Log
import java.io.OutputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Cliente Socket TCP de ultra-baixa latência para envio de telemetria USB (ADB Tunnel).
 * Opera em 127.0.0.1:27183 com TCP_NODELAY e fila circular sem bloqueio de thread de UI.
 */
class UsbSocketClient(
    private val host: String = "127.0.0.1",
    private val port: Int = 27183
) {
    companion object {
        private const val TAG = "UsbSocketClient"
        private const val QUEUE_CAPACITY = 256
        private const val RECONNECT_DELAY_MS = 1500L
    }

    private val isRunning = AtomicBoolean(false)
    private val isConnected = AtomicBoolean(false)
    private val sendQueue = LinkedBlockingQueue<ByteArray>(QUEUE_CAPACITY)

    private var workerThread: Thread? = null
    private var socket: Socket? = null
    private var outputStream: OutputStream? = null

    var onConnectionStateChanged: ((Boolean) -> Unit)? = null

    /**
     * Inicia a thread de conexão em segundo plano.
     */
    fun start() {
        if (isRunning.getAndSet(true)) return

        workerThread = Thread({ runClientLoop() }, "UsbSocketSenderThread").apply {
            priority = Thread.MAX_PRIORITY
            start()
        }
    }

    /**
     * Encerra o cliente e fecha sockets.
     */
    fun stop() {
        if (!isRunning.getAndSet(false)) return

        try {
            socket?.close()
        } catch (_: Exception) {}

        workerThread?.interrupt()
        workerThread = null
        isConnected.set(false)
        onConnectionStateChanged?.invoke(false)
    }

    /**
     * Envia um pacote de entrada para a fila de transmissão USB.
     * Se a fila estiver cheia (congestão), descarta o pacote mais antigo para priorizar o mais recente.
     */
    fun sendPacket(packet: InputPacket) {
        if (!isConnected.get()) return

        val bytes = packet.toByteArray()
        if (!sendQueue.offer(bytes)) {
            sendQueue.poll() // Descarta o frame anterior para manter latência zero
            sendQueue.offer(bytes)
        }
    }

    private fun runClientLoop() {
        while (isRunning.get()) {
            try {
                Log.i(TAG, "Tentando conectar ao servidor sTab em $host:$port via cabo USB...")
                val sock = Socket()
                sock.tcpNoDelay = true
                sock.sendBufferSize = 32 * 1024
                sock.connect(InetSocketAddress(host, port), 2000)

                socket = sock
                outputStream = sock.getOutputStream()
                isConnected.set(true)
                Log.i(TAG, ">>> Conectado com sucesso ao Servidor Windows! <<<")
                onConnectionStateChanged?.invoke(true)

                // Loop de transmissão contínua enquanto conectado
                val stream = outputStream!!
                while (isRunning.get() && isConnected.get()) {
                    val data = sendQueue.take()
                    stream.write(data)
                    stream.flush()
                }

            } catch (e: Exception) {
                if (isRunning.get()) {
                    Log.w(TAG, "Conexao perdida ou servidor indisponivel: ${e.message}")
                }
            } finally {
                cleanupSocket()
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

    private fun cleanupSocket() {
        try {
            outputStream?.close()
        } catch (_: Exception) {}
        try {
            socket?.close()
        } catch (_: Exception) {}

        outputStream = null
        socket = null
        sendQueue.clear()

        if (isConnected.getAndSet(false)) {
            onConnectionStateChanged?.invoke(false)
        }
    }
}
