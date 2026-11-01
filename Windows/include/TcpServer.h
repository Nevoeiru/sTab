#ifndef STAB_TCP_SERVER_H
#define STAB_TCP_SERVER_H

#include <winsock2.h>
#include <windows.h>
#include <atomic>
#include <functional>
#include "Protocol.h"
#include "PointerInjector.h"
#include "GestureTranslator.h"

/**
 * @brief Callback para eventos de controle de vídeo (ex.: REQUEST_IDR).
 */
typedef std::function<void(uint8_t command)> VideoControlCallback;

/**
 * @brief Callback para telemetria de pacotes de entrada em tempo real.
 */
typedef std::function<void(const InputPacket& packet)> InputPacketCallback;

/**
 * @brief Servidor TCP Winsock de alta performance para recepção de telemetria USB.
 * Configurado com TCP_NODELAY e enquadramento exato de pacotes de 16 bytes.
 */
class TcpServer {
public:
    TcpServer(int port = 27183);
    ~TcpServer();

    /**
     * @brief Inicia o socket do servidor na porta especificada e abre a thread de escuta.
     * @return true se o socket estiver escutando com sucesso.
     */
    bool Start(PointerInjector* injector, GestureTranslator* translator);

    /**
     * @brief Para o servidor e encerra as conexões ativas.
     */
    void Stop();

    /**
     * @brief Registra um callback para comandos de controle de vídeo.
     */
    void SetVideoControlCallback(VideoControlCallback cb);

    /**
     * @brief Registra um callback para telemetria de entrada (HUD).
     */
    void SetInputPacketCallback(InputPacketCallback cb);

    bool IsRunning() const { return m_running; }
    bool IsClientConnected() const { return m_clientConnected; }
    uint64_t GetTotalPacketsReceived() const { return m_totalPackets; }

private:
    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    void RunWorker();

    void HandleIncomingData(const uint8_t* buffer, int bytesReceived);
    void DispatchPacket(const InputPacket& packet);

    int m_port;
    SOCKET m_listenSocket;
    SOCKET m_clientSocket;
    HANDLE m_hThread;

    std::atomic<bool> m_running;
    std::atomic<bool> m_clientConnected;
    std::atomic<uint64_t> m_totalPackets;

    PointerInjector*   m_injector;
    GestureTranslator* m_translator;
    VideoControlCallback m_videoControlCb;
    InputPacketCallback  m_inputPacketCb;

    // Buffer de enquadramento para garantir o consumo exato de 16 bytes
    uint8_t m_packetBuffer[sizeof(InputPacket)];
    int m_bufferBytesAccumulated;
};

#endif // STAB_TCP_SERVER_H

