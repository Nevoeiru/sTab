#include "TelemetryBridge.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>

TelemetryBridge::TelemetryBridge(int port)
    : m_port(port)
    , m_udpSocket(INVALID_SOCKET)
    , m_tcpListenSocket(INVALID_SOCKET)
    , m_tcpClientSocket(INVALID_SOCKET)
    , m_hUdpThread(NULL)
    , m_hTcpThread(NULL)
    , m_running(false)
    , m_tabletConnected(false)
{
    InitializeCriticalSection(&m_lock);
}

TelemetryBridge::~TelemetryBridge() {
    Stop();
    DeleteCriticalSection(&m_lock);
}

bool TelemetryBridge::Start() {
    if (m_running) return true;

    // 1. Socket UDP para escutar a telemetria do Blender Companion
    m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_udpSocket == INVALID_SOCKET) {
        printf("[TelemetryBridge] Erro ao criar socket UDP (Erro: %d)\n", WSAGetLastError());
        return false;
    }

    BOOL reuse = TRUE;
    setsockopt(m_udpSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    sockaddr_in udpAddr = {};
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    udpAddr.sin_port = htons((u_short)m_port);

    if (bind(m_udpSocket, (sockaddr*)&udpAddr, sizeof(udpAddr)) == SOCKET_ERROR) {
        printf("[TelemetryBridge] Erro no bind UDP na porta %d (Erro: %d)\n", m_port, WSAGetLastError());
        closesocket(m_udpSocket);
        m_udpSocket = INVALID_SOCKET;
        return false;
    }

    // 2. Socket TCP para aceitar conexao do tablet via ADB tunnel
    m_tcpListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_tcpListenSocket == INVALID_SOCKET) {
        printf("[TelemetryBridge] Erro ao criar socket TCP (Erro: %d)\n", WSAGetLastError());
        closesocket(m_udpSocket);
        m_udpSocket = INVALID_SOCKET;
        return false;
    }

    setsockopt(m_tcpListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    sockaddr_in tcpAddr = {};
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tcpAddr.sin_port = htons((u_short)m_port);

    if (bind(m_tcpListenSocket, (sockaddr*)&tcpAddr, sizeof(tcpAddr)) == SOCKET_ERROR) {
        printf("[TelemetryBridge] Erro no bind TCP na porta %d (Erro: %d)\n", m_port, WSAGetLastError());
        closesocket(m_tcpListenSocket);
        closesocket(m_udpSocket);
        m_tcpListenSocket = INVALID_SOCKET;
        m_udpSocket = INVALID_SOCKET;
        return false;
    }

    if (listen(m_tcpListenSocket, 1) == SOCKET_ERROR) {
        printf("[TelemetryBridge] Erro no listen TCP (Erro: %d)\n", WSAGetLastError());
        closesocket(m_tcpListenSocket);
        closesocket(m_udpSocket);
        m_tcpListenSocket = INVALID_SOCKET;
        m_udpSocket = INVALID_SOCKET;
        return false;
    }

    m_running = true;

    m_hUdpThread = CreateThread(NULL, 0, UdpListenerThreadProc, this, 0, NULL);
    m_hTcpThread = CreateThread(NULL, 0, TcpListenerThreadProc, this, 0, NULL);

    printf("[TelemetryBridge] Ponte de telemetria do Blender ativa em 127.0.0.1:%d (UDP Blender <-> TCP Tablet).\n", m_port);
    return true;
}

void TelemetryBridge::Stop() {
    if (!m_running) return;
    m_running = false;

    EnterCriticalSection(&m_lock);
    if (m_tcpClientSocket != INVALID_SOCKET) {
        shutdown(m_tcpClientSocket, SD_BOTH);
        closesocket(m_tcpClientSocket);
        m_tcpClientSocket = INVALID_SOCKET;
    }
    LeaveCriticalSection(&m_lock);

    if (m_tcpListenSocket != INVALID_SOCKET) {
        closesocket(m_tcpListenSocket);
        m_tcpListenSocket = INVALID_SOCKET;
    }

    if (m_udpSocket != INVALID_SOCKET) {
        closesocket(m_udpSocket);
        m_udpSocket = INVALID_SOCKET;
    }

    if (m_hUdpThread) {
        WaitForSingleObject(m_hUdpThread, 1000);
        CloseHandle(m_hUdpThread);
        m_hUdpThread = NULL;
    }

    if (m_hTcpThread) {
        WaitForSingleObject(m_hTcpThread, 1000);
        CloseHandle(m_hTcpThread);
        m_hTcpThread = NULL;
    }

    m_tabletConnected = false;
    printf("[TelemetryBridge] Ponte de telemetria finalizada.\n");
}

DWORD WINAPI TelemetryBridge::UdpListenerThreadProc(LPVOID lpParam) {
    TelemetryBridge* bridge = static_cast<TelemetryBridge*>(lpParam);
    bridge->RunUdpListener();
    return 0;
}

DWORD WINAPI TelemetryBridge::TcpListenerThreadProc(LPVOID lpParam) {
    TelemetryBridge* bridge = static_cast<TelemetryBridge*>(lpParam);
    bridge->RunTcpListener();
    return 0;
}

void TelemetryBridge::RunUdpListener() {
    char buffer[2048];

    while (m_running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_udpSocket, &readSet);

        timeval timeout = { 0, 100000 }; // 100 ms
        int sel = select(0, &readSet, NULL, NULL, &timeout);

        if (sel > 0 && FD_ISSET(m_udpSocket, &readSet)) {
            sockaddr_in senderAddr = {};
            int addrLen = sizeof(senderAddr);
            int bytes = recvfrom(m_udpSocket, buffer, sizeof(buffer) - 2, 0, (sockaddr*)&senderAddr, &addrLen);

            if (bytes > 0) {
                // Adiciona delimitador newline para facilitar a leitura no Android
                if (buffer[bytes - 1] != '\n') {
                    buffer[bytes] = '\n';
                    bytes++;
                }

                EnterCriticalSection(&m_lock);
                if (m_tabletConnected && m_tcpClientSocket != INVALID_SOCKET) {
                    int sent = send(m_tcpClientSocket, buffer, bytes, 0);
                    if (sent <= 0) {
                        closesocket(m_tcpClientSocket);
                        m_tcpClientSocket = INVALID_SOCKET;
                        m_tabletConnected = false;
                    }
                }
                LeaveCriticalSection(&m_lock);
            }
        }
    }
}

void TelemetryBridge::RunTcpListener() {
    while (m_running) {
        if (!m_tabletConnected) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(m_tcpListenSocket, &readSet);

            timeval timeout = { 0, 300000 }; // 300 ms
            int sel = select(0, &readSet, NULL, NULL, &timeout);

            if (sel > 0 && FD_ISSET(m_tcpListenSocket, &readSet)) {
                sockaddr_in clientAddr = {};
                int addrLen = sizeof(clientAddr);
                SOCKET sock = accept(m_tcpListenSocket, (sockaddr*)&clientAddr, &addrLen);

                if (sock != INVALID_SOCKET) {
                    BOOL noDelay = TRUE;
                    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

                    EnterCriticalSection(&m_lock);
                    m_tcpClientSocket = sock;
                    m_tabletConnected = true;
                    LeaveCriticalSection(&m_lock);

                    printf("[TelemetryBridge] >>> Tablet conectado ao canal de Telemetria do Blender (27185)! <<<\n");
                }
            }
        } else {
            Sleep(250);
        }
    }
}

