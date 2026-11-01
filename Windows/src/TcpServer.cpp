#include "TcpServer.h"
#include <ws2tcpip.h>
#include <stdio.h>

TcpServer::TcpServer(int port)
    : m_port(port)
    , m_listenSocket(INVALID_SOCKET)
    , m_clientSocket(INVALID_SOCKET)
    , m_hThread(NULL)
    , m_running(false)
    , m_clientConnected(false)
    , m_totalPackets(0)
    , m_injector(NULL)
    , m_translator(NULL)
    , m_bufferBytesAccumulated(0)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

TcpServer::~TcpServer() {
    Stop();
    WSACleanup();
}

bool TcpServer::Start(PointerInjector* injector, GestureTranslator* translator) {
    if (m_running) return true;

    m_injector = injector;
    m_translator = translator;

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) {
        printf("[TcpServer] Erro ao criar socket (WSAGetLastError: %d)\n", WSAGetLastError());
        return false;
    }

    BOOL reuse = TRUE;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Vinculado exclusivamente ao loopback do túnel ADB
    serverAddr.sin_port = htons((u_short)m_port);

    if (bind(m_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("[TcpServer] Erro no bind na porta %d (WSAGetLastError: %d)\n", m_port, WSAGetLastError());
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    if (listen(m_listenSocket, 1) == SOCKET_ERROR) {
        printf("[TcpServer] Erro no listen (WSAGetLastError: %d)\n", WSAGetLastError());
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    m_running = true;
    m_hThread = CreateThread(NULL, 0, WorkerThreadProc, this, 0, NULL);
    if (!m_hThread) {
        printf("[TcpServer] Erro ao criar thread de trabalho.\n");
        m_running = false;
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    printf("[TcpServer] Servidor escutando em 127.0.0.1:%d (Aguardando conexao do tablet via ADB)...\n", m_port);
    return true;
}

void TcpServer::Stop() {
    if (!m_running) return;

    m_running = false;

    if (m_clientSocket != INVALID_SOCKET) {
        shutdown(m_clientSocket, SD_BOTH);
        closesocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET;
    }

    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_hThread) {
        WaitForSingleObject(m_hThread, 1500);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    m_clientConnected = false;
    printf("[TcpServer] Servidor finalizado.\n");
}

void TcpServer::SetVideoControlCallback(VideoControlCallback cb) {
    m_videoControlCb = cb;
}

DWORD WINAPI TcpServer::WorkerThreadProc(LPVOID lpParam) {
    TcpServer* server = static_cast<TcpServer*>(lpParam);
    server->RunWorker();
    return 0;
}

void TcpServer::RunWorker() {
    uint8_t recvBuffer[4096];

    while (m_running) {
        // Aguarda conexão de entrada do cliente
        if (!m_clientConnected) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(m_listenSocket, &readSet);

            timeval timeout = { 0, 300000 }; // 300 ms
            int sel = select(0, &readSet, NULL, NULL, &timeout);

            if (sel > 0 && FD_ISSET(m_listenSocket, &readSet)) {
                sockaddr_in clientAddr = {};
                int addrLen = sizeof(clientAddr);
                m_clientSocket = accept(m_listenSocket, (sockaddr*)&clientAddr, &addrLen);

                if (m_clientSocket != INVALID_SOCKET) {
                    // Configuração de ultra-baixa latência (Desativa Algoritmo de Nagle)
                    BOOL noDelay = TRUE;
                    setsockopt(m_clientSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

                    int rcvBuf = 64 * 1024;
                    setsockopt(m_clientSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvBuf, sizeof(rcvBuf));

                    m_clientConnected = true;
                    m_bufferBytesAccumulated = 0;
                    printf("[TcpServer] >>> Tablet Galaxy Tab S9 FE+ CONECTADO via cabo USB! <<<\n");
                }
            }
            continue;
        }

        // Loop de recepção de telemetria contínua enquanto conectado
        fd_set clientSet;
        FD_ZERO(&clientSet);
        FD_SET(m_clientSocket, &clientSet);

        timeval timeout = { 0, 100000 }; // 100 ms
        int sel = select(0, &clientSet, NULL, NULL, &timeout);

        if (sel > 0 && FD_ISSET(m_clientSocket, &clientSet)) {
            int bytesRead = recv(m_clientSocket, (char*)recvBuffer, sizeof(recvBuffer), 0);
            if (bytesRead <= 0) {
                // Desconexão detectada
                printf("[TcpServer] Tablet desconectado.\n");
                closesocket(m_clientSocket);
                m_clientSocket = INVALID_SOCKET;
                m_clientConnected = false;

                if (m_translator) {
                    m_translator->ResetAllInputStates();
                }
            } else {
                HandleIncomingData(recvBuffer, bytesRead);
            }
        }
    }
}

void TcpServer::HandleIncomingData(const uint8_t* buffer, int bytesReceived) {
    const int PACKET_SIZE = sizeof(InputPacket); // Exatamente 16 bytes

    for (int i = 0; i < bytesReceived; ++i) {
        m_packetBuffer[m_bufferBytesAccumulated++] = buffer[i];

        if (m_bufferBytesAccumulated == PACKET_SIZE) {
            InputPacket packet;
            memcpy(&packet, m_packetBuffer, PACKET_SIZE);
            DispatchPacket(packet);

            m_bufferBytesAccumulated = 0;
            m_totalPackets++;
        }
    }
}

void TcpServer::DispatchPacket(const InputPacket& packet) {
    switch (packet.packetType) {
        case PACKET_TYPE_PEN:
            if (m_injector) {
                m_injector->InjectPen(packet);
            }
            break;

        case PACKET_TYPE_TOUCH_GESTURE:
            if (m_translator) {
                m_translator->ProcessTouchGesture(packet);
            }
            break;

        case PACKET_TYPE_SCULPTPAD:
            if (m_translator) {
                m_translator->ProcessSculptPadAction(packet);
            }
            break;

        case PACKET_TYPE_VIDEO_CONTROL:
            if (m_videoControlCb) {
                uint8_t cmd = (uint8_t)(packet.payload & 0xFF);
                m_videoControlCb(cmd);
            }
            break;

        default:
            break;
    }
}

