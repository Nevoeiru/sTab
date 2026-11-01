#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>
#include <atomic>
#include <string>

/**
 * @brief Estado de telemetria compartilhado para a HUD do sTab Server.
 */
struct HudTelemetryState {
    // Status de Conectividade
    bool serverRunning = true;
    bool clientConnected = false;
    bool videoStreaming = false;
    bool blenderConnected = false;

    // Métricas de S-Pen (Entrada 240Hz)
    int penX = 0;
    int penY = 0;
    int penPressure = 0;        // 0 .. 4096
    float penTiltX = 0.0f;      // -90.0 .. +90.0
    float penTiltY = 0.0f;
    bool isContact = false;
    bool isHover = false;
    bool barrelButton = false;

    // Métricas de Vídeo (DXGI 90Hz)
    int displayWidth = 1920;
    int displayHeight = 1080;
    bool is1610Native = false;
    float currentFps = 0.0f;
    float bitrateMbps = 0.0f;
    uint64_t totalBytesSent = 0;

    // Métricas do Blender Companion
    char activeBrush[64] = "Aguardando Blender...";
    float brushRadius = 50.0f;
    float brushStrength = 0.5f;
    bool symmetryX = false;
    bool symmetryY = false;
    bool symmetryZ = false;
};

/**
 * @brief Janela Win32 de HUD gráfica em tempo real do Servidor sTab.
 */
class ServerHud {
public:
    ServerHud();
    ~ServerHud();

    bool Start(HINSTANCE hInstance = NULL);
    void UpdateTelemetry(const HudTelemetryState& state);
    void Stop();
    bool IsOpen() const;

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI WindowThreadStatic(LPVOID lpParam);

    void WindowThread();
    void Render(HDC hdc);
    void DrawLineartStylus(HDC hdc, int x, int y);
    void DrawLineartDisplay(HDC hdc, int x, int y);
    void DrawLineartBlender(HDC hdc, int x, int y);
    void DrawOmnitomeIcon(HDC hdc, int x, int y);

    HWND m_hWnd;
    HANDLE m_hThread;
    HANDLE m_hReadyEvent;
    DWORD m_threadId;
    std::atomic<bool> m_running;

    HudTelemetryState m_state;
    CRITICAL_SECTION m_lock;

    HFONT m_fontTitle;
    HFONT m_fontSection;
    HFONT m_fontRegular;
    HFONT m_fontBold;
    HFONT m_fontMono;
};

