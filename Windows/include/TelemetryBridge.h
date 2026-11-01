#ifndef STAB_TELEMETRY_BRIDGE_H
#define STAB_TELEMETRY_BRIDGE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <cstdint>
#include <functional>

/**
 * @brief Callback para dados de pincel do Blender.
 */
typedef std::function<void(const char* brush, float size, float strength, bool symX)> BlenderDataCallback;

/**
 * @brief Ponte de telemetria bidirecional do Blender para o Galaxy Tab S9 FE+.
 * Escuta os dados de pincel emitidos pelo companion Python via UDP (127.0.0.1:27185)
 * e os despacha imediatamente via TCP (127.0.0.1:27185) para o tablet via tunel ADB.
 */
class TelemetryBridge {
public:
    TelemetryBridge(int port = 27185);
    ~TelemetryBridge();

    bool Start();
    void Stop();

    void SetBlenderDataCallback(BlenderDataCallback cb) { m_blenderCb = cb; }

    bool IsTabletConnected() const { return m_tabletConnected; }

private:
    static DWORD WINAPI UdpListenerThreadProc(LPVOID lpParam);
    static DWORD WINAPI TcpListenerThreadProc(LPVOID lpParam);

    void RunUdpListener();
    void RunTcpListener();

    int             m_port;
    SOCKET          m_udpSocket;
    SOCKET          m_tcpListenSocket;
    SOCKET          m_tcpClientSocket;
    HANDLE          m_hUdpThread;
    HANDLE          m_hTcpThread;
    CRITICAL_SECTION m_lock;

    volatile bool   m_running;
    volatile bool   m_tabletConnected;
    BlenderDataCallback m_blenderCb;
};

#endif // STAB_TELEMETRY_BRIDGE_H

