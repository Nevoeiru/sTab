package com.stab.sculptbridge.video

import android.os.Process
import android.util.Log
import java.io.InputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Receptor de pacotes de vídeo H.264 via TCP (Porta 27184).
 * Conecta no túnel ADB reverso (127.0.0.1:27184) com TCP_NODELAY e consome o fluxo NAL contínuo.
 */
class VideoPacketReceiver(
    private val host: String = "127.0.0.1",
    private val port: Int = 27184,
    private val onPacketReceived: (ByteArray, Int) -> Unit
) {
    companion object {
        private const val TAG = "sTab_VideoReceiver"
        private const val HEADER_SIZE = 4
        private const val MAX_PACKET_SIZE = 4 * 1024 * 1024 // 4 MB safety limit
    }

    private val isRunning = AtomicBoolean(false)
    private var workerThread: Thread? = null
    private var activeSocket: Socket? = null

    var onConnectionStateChanged: ((Boolean) -> Unit)? = null

    fun start() {
        if (isRunning.getAndSet(true)) return

        workerThread = Thread({
            Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_DISPLAY)
            runReceiverLoop()
        }, "sTab_VideoReceiverThread").apply {
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
        val headerBuf = ByteArray(HEADER_SIZE)
        var payloadBuf = ByteArray(64 * 1024)

        while (isRunning.get()) {
            var socket: Socket? = null
            try {
                Log.i(TAG, "Conectando ao stream de video em $host:$port...")
                socket = Socket()
                socket.tcpNoDelay = true
                socket.receiveBufferSize = 256 * 1024
                socket.connect(InetSocketAddress(host, port), 3000)

                activeSocket = socket
                onConnectionStateChanged?.invoke(true)
                Log.i(TAG, "Stream de video H.264 conectado com sucesso!")

                val inputStream = socket.getInputStream()

                while (isRunning.get()) {
                    // 1. Lê o cabeçalho de 4 bytes (tamanho do pacote Little-Endian)
                    readExact(inputStream, headerBuf, 0, HEADER_SIZE)

                    val packetSize = (headerBuf[0].toInt() and 0xFF) or
                            ((headerBuf[1].toInt() and 0xFF) shl 8) or
                            ((headerBuf[2].toInt() and 0xFF) shl 16) or
                            ((headerBuf[3].toInt() and 0xFF) shl 24)

                    if (packetSize <= 0 || packetSize > MAX_PACKET_SIZE) {
                        Log.e(TAG, "Tamanho de pacote NAL invalido recebido: $packetSize bytes")
                        break
                    }

                    // Redimensiona o buffer de recepção se necessário
                    if (payloadBuf.size < packetSize) {
                        payloadBuf = ByteArray(packetSize)
                    }

                    // 2. Lê exatamente packetSize bytes do pacote NAL
                    readExact(inputStream, payloadBuf, 0, packetSize)

                    // 3. Despacha imediatamente para o decodificador MediaCodec
                    onPacketReceived(payloadBuf, packetSize)
                }
            } catch (e: Exception) {
                if (isRunning.get()) {
                    Log.w(TAG, "Conexao de video encerrada ou indisponivel: ${e.message}")
                }
            } finally {
                try {
                    socket?.close()
                } catch (_: Exception) {}
                activeSocket = null
                onConnectionStateChanged?.invoke(false)
            }

            if (isRunning.get()) {
                try {
                    Thread.sleep(1000)
                } catch (_: InterruptedException) {
                    break
                }
            }
        }
    }

    private fun readExact(stream: InputStream, buffer: ByteArray, offset: Int, length: Int) {
        var bytesRead = 0
        while (bytesRead < length) {
            val count = stream.read(buffer, offset + bytesRead, length - bytesRead)
            if (count < 0) {
                throw java.io.EOFException("Fim de fluxo prematuro ao ler pacote NAL")
            }
            bytesRead += count
        }
    }
}

