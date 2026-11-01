#include "VideoServer.h"
#include <ws2tcpip.h>
#include <stdio.h>

VideoServer::VideoServer(int port)
    : m_port(port)
    , m_listenSocket(INVALID_SOCKET)
    , m_clientSocket(INVALID_SOCKET)
    , m_hListenThread(NULL)
    , m_running(false)
    , m_clientConnected(false)
    , m_framesSent(0)
    , m_bytesSent(0)
{
    InitializeCriticalSection(&m_sendLock);
}

VideoServer::~VideoServer() {
    Stop();
    DeleteCriticalSection(&m_sendLock);
}

bool VideoServer::Start() {
    if (m_running) return true;

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) {
        printf("[VideoServer] Erro ao criar socket de video (WSAGetLastError: %d)\n", WSAGetLastError());
        return false;
    }

    BOOL reuse = TRUE;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons((u_short)m_port);

    if (bind(m_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("[VideoServer] Erro no bind na porta %d (WSAGetLastError: %d)\n", m_port, WSAGetLastError());
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    if (listen(m_listenSocket, 1) == SOCKET_ERROR) {
        printf("[VideoServer] Erro no listen da porta de video (WSAGetLastError: %d)\n", WSAGetLastError());
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    m_running = true;
    m_hListenThread = CreateThread(NULL, 0, ListenThreadProc, this, 0, NULL);
    if (!m_hListenThread) {
        m_running = false;
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    printf("[VideoServer] Streamer H.264 escutando em 127.0.0.1:%d (Aguardando MediaCodec do tablet)...\n", m_port);
    return true;
}

void VideoServer::Stop() {
    if (!m_running) return;

    m_running = false;

    EnterCriticalSection(&m_sendLock);
    if (m_clientSocket != INVALID_SOCKET) {
        shutdown(m_clientSocket, SD_BOTH);
        closesocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET;
    }
    LeaveCriticalSection(&m_sendLock);

    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_hListenThread) {
        WaitForSingleObject(m_hListenThread, 1500);
        CloseHandle(m_hListenThread);
        m_hListenThread = NULL;
    }

    m_clientConnected = false;
    printf("[VideoServer] Servidor de video finalizado.\n");
}

DWORD WINAPI VideoServer::ListenThreadProc(LPVOID lpParam) {
    VideoServer* server = static_cast<VideoServer*>(lpParam);
    server->RunListener();
    return 0;
}

void VideoServer::RunListener() {
    while (m_running) {
        if (!m_clientConnected) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(m_listenSocket, &readSet);

            timeval timeout = { 0, 300000 }; // 300 ms
            int sel = select(0, &readSet, NULL, NULL, &timeout);

            if (sel > 0 && FD_ISSET(m_listenSocket, &readSet)) {
                sockaddr_in clientAddr = {};
                int addrLen = sizeof(clientAddr);
                SOCKET sock = accept(m_listenSocket, (sockaddr*)&clientAddr, &addrLen);

                if (sock != INVALID_SOCKET) {
                    BOOL noDelay = TRUE;
                    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

                    int sndBuf = 256 * 1024; // Buffer generoso de 256KB
                    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&sndBuf, sizeof(sndBuf));

                    EnterCriticalSection(&m_sendLock);
                    m_clientSocket = sock;
                    m_clientConnected = true;
                    LeaveCriticalSection(&m_sendLock);

                    printf("[VideoServer] >>> Receptor de video do Galaxy Tab S9 FE+ CONECTADO! <<<\n");
                }
            }
            continue;
        }

        // Monitora o estado de conexão do cliente
        fd_set errSet;
        FD_ZERO(&errSet);
        FD_SET(m_clientSocket, &errSet);

        timeval timeout = { 0, 200000 };
        int sel = select(0, NULL, NULL, &errSet, &timeout);

        if (sel > 0 && FD_ISSET(m_clientSocket, &errSet)) {
            EnterCriticalSection(&m_sendLock);
            closesocket(m_clientSocket);
            m_clientSocket = INVALID_SOCKET;
            m_clientConnected = false;
            LeaveCriticalSection(&m_sendLock);
            printf("[VideoServer] Cliente de video desconectado.\n");
        }
    }
}

void VideoServer::SendNalPacket(const uint8_t* data, size_t size, bool isKeyFrame) {
    (void)isKeyFrame;
    if (!m_clientConnected || m_clientSocket == INVALID_SOCKET || !data || size == 0) {
        return;
    }

    EnterCriticalSection(&m_sendLock);
    if (m_clientSocket != INVALID_SOCKET) {
        // Prefixo de 4 bytes com o comprimento exato do pacote NAL
        uint32_t packetLen = (uint32_t)size;
        int sentHead = send(m_clientSocket, (const char*)&packetLen, 4, 0);
        int sentBody = send(m_clientSocket, (const char*)data, (int)size, 0);

        if (sentHead <= 0 || sentBody <= 0) {
            closesocket(m_clientSocket);
            m_clientSocket = INVALID_SOCKET;
            m_clientConnected = false;
            printf("[VideoServer] Falha de transmissao de frame (Desconexao detectada).\n");
        } else {
            m_framesSent++;
            m_bytesSent += (size + 4);
        }
    }
    LeaveCriticalSection(&m_sendLock);
}
