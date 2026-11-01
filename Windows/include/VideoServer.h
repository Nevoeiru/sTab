#ifndef STAB_VIDEO_SERVER_H
#define STAB_VIDEO_SERVER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <atomic>
#include <vector>

/**
 * @brief Servidor TCP de streaming de vídeo H.264 em tempo real (Porta 27184 via túnel USB).
 * Envia pacotes NAL prefixados por 4 bytes de tamanho com descarte automático de frames
 * em caso de congestionamento para assegurar latência zero permanente.
 */
class VideoServer {
public:
    VideoServer(int port = 27184);
    ~VideoServer();

    /**
     * @brief Inicia o socket de escuta de vídeo na porta 27184.
     */
    bool Start();

    /**
     * @brief Encerra o servidor de vídeo e libera os sockets.
     */
    void Stop();

    /**
     * @brief Envia um pacote NAL H.264 para o tablet conectado.
     * @param data Ponteiro para os dados do NAL.
     * @param size Tamanho do pacote em bytes.
     * @param isKeyFrame Indica se o pacote contém um frame IDR / SPS / PPS.
     */
    void SendNalPacket(const uint8_t* data, size_t size, bool isKeyFrame);

    bool IsRunning() const { return m_running; }
    bool IsClientConnected() const { return m_clientConnected; }
    uint64_t GetTotalFramesSent() const { return m_framesSent; }
    uint64_t GetTotalBytesSent() const { return m_bytesSent; }

private:
    static DWORD WINAPI ListenThreadProc(LPVOID lpParam);
    void RunListener();

    int m_port;
    SOCKET m_listenSocket;
    SOCKET m_clientSocket;
    HANDLE m_hListenThread;

    std::atomic<bool> m_running;
    std::atomic<bool> m_clientConnected;
    std::atomic<uint64_t> m_framesSent;
    std::atomic<uint64_t> m_bytesSent;

    CRITICAL_SECTION m_sendLock;
};

#endif // STAB_VIDEO_SERVER_H

