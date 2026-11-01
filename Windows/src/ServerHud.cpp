#include "ServerHud.h"
#include <dwmapi.h>
#include <stdio.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

extern volatile bool g_running;

ServerHud::ServerHud()
    : m_hWnd(NULL)
    , m_hThread(NULL)
    , m_threadId(0)
    , m_running(false)
    , m_fontTitle(NULL)
    , m_fontSection(NULL)
    , m_fontRegular(NULL)
    , m_fontBold(NULL)
    , m_fontMono(NULL)
{
    InitializeCriticalSection(&m_lock);
}

ServerHud::~ServerHud() {
    Stop();
    DeleteCriticalSection(&m_lock);
}

bool ServerHud::Start(HINSTANCE hInstance) {
    if (m_running) return true;
    m_running = true;

    m_hThread = CreateThread(NULL, 0, WindowThreadStatic, this, 0, &m_threadId);
    return (m_hThread != NULL);
}

void ServerHud::Stop() {
    if (!m_running) return;
    m_running = false;

    if (m_hWnd) {
        PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
    }
    if (m_hThread) {
        WaitForSingleObject(m_hThread, 1000);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}

bool ServerHud::IsOpen() const {
    return m_running && (m_hWnd != NULL);
}

void ServerHud::UpdateTelemetry(const HudTelemetryState& state) {
    EnterCriticalSection(&m_lock);
    m_state = state;
    LeaveCriticalSection(&m_lock);
}

DWORD WINAPI ServerHud::WindowThreadStatic(LPVOID lpParam) {
    auto* self = (ServerHud*)lpParam;
    self->WindowThread();
    return 0;
}

LRESULT CALLBACK ServerHud::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = (ServerHud*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CREATE: {
            SetTimer(hWnd, 1, 33, NULL); // ~30 FPS para atualizacao visual suave
            return 0;
        }
        case WM_TIMER: {
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            if (self) self->Render(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1; // Double buffering previne cintilacao
        case WM_CLOSE: {
            g_running = false;
            DestroyWindow(hWnd);
            return 0;
        }
        case WM_DESTROY: {
            KillTimer(hWnd, 1);
            if (self) self->m_hWnd = NULL;
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void ServerHud::WindowThread() {
    HINSTANCE hInst = GetModuleHandleW(NULL);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"sTab_Server_HUD_Class";

    RegisterClassExW(&wc);

    int winWidth = 760;
    int winHeight = 540;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winWidth) / 2;
    int posY = (screenH - winHeight) / 2;

    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        L"sTab v2.0 - SculptBridge Server (Galaxy Tab S9 FE+ no Blender)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winWidth, winHeight,
        NULL, NULL, hInst, NULL
    );

    if (!m_hWnd) return;

    SetWindowLongPtrW(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

    // Ativa barra de título escura no Windows 10/11
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

    // Inicializa fontes tipográficas
    m_fontTitle   = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_fontSection = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_fontRegular = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_fontBold    = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_fontMono    = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");

    ShowWindow(m_hWnd, SW_SHOWNORMAL);
    UpdateWindow(m_hWnd);

    MSG msg;
    while (m_running && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (m_fontTitle)   DeleteObject(m_fontTitle);
    if (m_fontSection) DeleteObject(m_fontSection);
    if (m_fontRegular) DeleteObject(m_fontRegular);
    if (m_fontBold)    DeleteObject(m_fontBold);
    if (m_fontMono)    DeleteObject(m_fontMono);
}

void ServerHud::DrawLineartStylus(HDC hdc, int x, int y) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 229, 255));
    HGDIOBJ old = SelectObject(hdc, pen);
    // Corpo da caneta e ponta
    POINT pts[5] = { {x, y + 16}, {x + 12, y + 4}, {x + 16, y + 8}, {x + 4, y + 20}, {x, y + 16} };
    Polyline(hdc, pts, 5);
    MoveToEx(hdc, x, y + 16, NULL); LineTo(hdc, x - 3, y + 19);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

void ServerHud::DrawLineartDisplay(HDC hdc, int x, int y) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 230, 118));
    HGDIOBJ old = SelectObject(hdc, pen);
    // Monitor e base
    Rectangle(hdc, x, y, x + 20, y + 14);
    MoveToEx(hdc, x + 10, y + 14, NULL); LineTo(hdc, x + 10, y + 18);
    MoveToEx(hdc, x + 5, y + 18, NULL);  LineTo(hdc, x + 15, y + 18);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

void ServerHud::DrawLineartBlender(HDC hdc, int x, int y) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 179, 0));
    HGDIOBJ old = SelectObject(hdc, pen);
    // Cubo isometrico
    MoveToEx(hdc, x + 10, y, NULL);      LineTo(hdc, x + 20, y + 5);
    LineTo(hdc, x + 10, y + 10);         LineTo(hdc, x, y + 5);
    LineTo(hdc, x + 10, y);
    MoveToEx(hdc, x, y + 5, NULL);       LineTo(hdc, x, y + 15);
    LineTo(hdc, x + 10, y + 20);         LineTo(hdc, x + 20, y + 15);
    LineTo(hdc, x + 20, y + 5);
    MoveToEx(hdc, x + 10, y + 10, NULL); LineTo(hdc, x + 10, y + 20);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

void ServerHud::DrawOmnitomeIcon(HDC hdc, int x, int y) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 229, 255));
    HGDIOBJ old = SelectObject(hdc, pen);
    // Emblema geometrico Omnitome (hexagono com prisma central, sem estrelas)
    POINT hex[7] = {
        {x + 10, y}, {x + 20, y + 5}, {x + 20, y + 15},
        {x + 10, y + 20}, {x, y + 15}, {x, y + 5}, {x + 10, y}
    };
    Polyline(hdc, hex, 7);
    MoveToEx(hdc, x + 10, y, NULL);      LineTo(hdc, x + 10, y + 20);
    MoveToEx(hdc, x, y + 10, NULL);      LineTo(hdc, x + 20, y + 10);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

void ServerHud::Render(HDC hdc) {
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    // Buffer secundário
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

    // 1. Fundo dark principal
    HBRUSH bgBrush = CreateSolidBrush(RGB(18, 19, 23));
    FillRect(memDC, &rc, bgBrush);
    DeleteObject(bgBrush);

    HudTelemetryState s;
    EnterCriticalSection(&m_lock);
    s = m_state;
    LeaveCriticalSection(&m_lock);

    SetBkMode(memDC, TRANSPARENT);

    // 2. Cabeçalho
    DrawOmnitomeIcon(memDC, 24, 20);

    SelectObject(memDC, m_fontTitle);
    SetTextColor(memDC, RGB(236, 239, 244));
    TextOutW(memDC, 52, 18, L"sTab v2.0 - SculptBridge Server", 31);

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, 54, 44, L"Galaxy Tab S9 FE+ como Mesa Digitalizadora 90Hz no Blender 3D", 61);

    // Badge de Conexão Geral
    RECT badgeRc = { width - 260, 20, width - 24, 48 };
    COLORREF badgeColor = s.clientConnected ? RGB(0, 230, 118) : RGB(255, 179, 0);
    HBRUSH badgeBg = CreateSolidBrush(s.clientConnected ? RGB(10, 40, 25) : RGB(45, 35, 15));
    HPEN badgePen = CreatePen(PS_SOLID, 1, badgeColor);
    HGDIOBJ oldPen = SelectObject(memDC, badgePen);
    HGDIOBJ oldBr = SelectObject(memDC, badgeBg);
    RoundRect(memDC, badgeRc.left, badgeRc.top, badgeRc.right, badgeRc.bottom, 12, 12);
    SelectObject(memDC, oldPen);
    SelectObject(memDC, oldBr);
    DeleteObject(badgePen);
    DeleteObject(badgeBg);

    SelectObject(memDC, m_fontBold);
    SetTextColor(memDC, badgeColor);
    if (s.clientConnected) {
        TextOutW(memDC, badgeRc.left + 22, badgeRc.top + 5, L"ONLINE - CONECTADO", 18);
    } else {
        TextOutW(memDC, badgeRc.left + 14, badgeRc.top + 5, L"AGUARDANDO TABLET (USB)", 23);
    }

    // Linha divisória
    HPEN divPen = CreatePen(PS_SOLID, 1, RGB(46, 52, 64));
    oldPen = SelectObject(memDC, divPen);
    MoveToEx(memDC, 24, 75, NULL); LineTo(memDC, width - 24, 75);
    SelectObject(memDC, oldPen);
    DeleteObject(divPen);

    // 3. Grid de Cartões de Telemetria (3 colunas/áreas)
    int cardY = 90;
    int cardH = 260;
    int cardW = (width - 48 - 32) / 3;

    // Helper para desenhar cartão com borda lineart
    auto DrawCardBase = [&](int x, int y, int w, int h, const wchar_t* title) {
        HBRUSH cardBg = CreateSolidBrush(RGB(26, 29, 36));
        HPEN cardBorder = CreatePen(PS_SOLID, 1, RGB(46, 52, 64));
        HGDIOBJ oP = SelectObject(memDC, cardBorder);
        HGDIOBJ oB = SelectObject(memDC, cardBg);
        RoundRect(memDC, x, y, x + w, y + h, 10, 10);
        SelectObject(memDC, oP);
        SelectObject(memDC, oB);
        DeleteObject(cardBorder);
        DeleteObject(cardBg);

        SelectObject(memDC, m_fontSection);
        SetTextColor(memDC, RGB(236, 239, 244));
        TextOutW(memDC, x + 38, y + 14, title, (int)wcslen(title));
    };

    // --- CARTÃO 1: S-Pen 240Hz & Windows Ink ---
    int c1X = 24;
    DrawCardBase(c1X, cardY, cardW, cardH, L"S-Pen 240Hz");
    DrawLineartStylus(memDC, c1X + 14, cardY + 12);

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, c1X + 16, cardY + 50, L"Estado da Caneta:", 17);

    SelectObject(memDC, m_fontBold);
    if (s.isContact) {
        SetTextColor(memDC, RGB(0, 230, 118));
        TextOutW(memDC, c1X + 16, cardY + 70, L"DESENHANDO (CONTACT)", 20);
    } else if (s.isHover) {
        SetTextColor(memDC, RGB(0, 229, 255));
        TextOutW(memDC, c1X + 16, cardY + 70, L"PROXIMIDADE (HOVER)", 19);
    } else {
        SetTextColor(memDC, RGB(144, 155, 172));
        TextOutW(memDC, c1X + 16, cardY + 70, L"FORA DO ALCANCE", 15);
    }

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, c1X + 16, cardY + 100, L"Pressão (4096 níveis):", 22);

    // Barra de Pressão
    int barX = c1X + 16;
    int barY = cardY + 122;
    int barW = cardW - 32;
    int barH = 14;
    RECT pBarBg = { barX, barY, barX + barW, barY + barH };
    HBRUSH pbBrush = CreateSolidBrush(RGB(36, 40, 50));
    FillRect(memDC, &pBarBg, pbBrush);
    DeleteObject(pbBrush);

    int fillW = (s.penPressure * barW) / 4096;
    if (fillW > 0) {
        RECT pBarFill = { barX, barY, barX + fillW, barY + barH };
        HBRUSH pfBrush = CreateSolidBrush(RGB(0, 229, 255));
        FillRect(memDC, &pBarFill, pfBrush);
        DeleteObject(pfBrush);
    }

    wchar_t pressBuf[64];
    swprintf_s(pressBuf, L"%d / 4096 (%.1f%%)", s.penPressure, (s.penPressure * 100.0f) / 4096.0f);
    SelectObject(memDC, m_fontMono);
    SetTextColor(memDC, RGB(236, 239, 244));
    TextOutW(memDC, barX, barY + 18, pressBuf, (int)wcslen(pressBuf));

    wchar_t coordBuf[64];
    swprintf_s(coordBuf, L"Pos: (%d, %d)", s.penX, s.penY);
    TextOutW(memDC, c1X + 16, cardY + 170, coordBuf, (int)wcslen(coordBuf));

    wchar_t tiltBuf[64];
    swprintf_s(tiltBuf, L"Tilt: X:%.0f° | Y:%.0f°", s.penTiltX, s.penTiltY);
    TextOutW(memDC, c1X + 16, cardY + 195, tiltBuf, (int)wcslen(tiltBuf));

    wchar_t barrelBuf[64];
    swprintf_s(barrelBuf, L"Botão S-Pen: %ls", s.barrelButton ? L"ATIVO (RMB)" : L"Normal");
    TextOutW(memDC, c1X + 16, cardY + 220, barrelBuf, (int)wcslen(barrelBuf));

    // --- CARTÃO 2: Vídeo 90Hz & Display ---
    int c2X = c1X + cardW + 16;
    DrawCardBase(c2X, cardY, cardW, cardH, L"Vídeo DXGI 90Hz");
    DrawLineartDisplay(memDC, c2X + 14, cardY + 12);

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, c2X + 16, cardY + 50, L"Modo de Tela:", 13);

    SelectObject(memDC, m_fontBold);
    if (s.is1610Native) {
        SetTextColor(memDC, RGB(0, 230, 118));
        TextOutW(memDC, c2X + 16, cardY + 70, L"16:10 Nativo (2560x1600)", 24);
    } else {
        SetTextColor(memDC, RGB(0, 229, 255));
        wchar_t resBuf[64];
        swprintf_s(resBuf, L"%dx%d", s.displayWidth, s.displayHeight);
        TextOutW(memDC, c2X + 16, cardY + 70, resBuf, (int)wcslen(resBuf));
    }

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, c2X + 16, cardY + 100, L"Desempenho de Streaming:", 25);

    SelectObject(memDC, m_fontBold);
    wchar_t fpsBuf[64];
    swprintf_s(fpsBuf, L"Taxa: ~%.0f FPS", s.currentFps > 0 ? s.currentFps : 90.0f);
    SetTextColor(memDC, RGB(236, 239, 244));
    TextOutW(memDC, c2X + 16, cardY + 122, fpsBuf, (int)wcslen(fpsBuf));

    SelectObject(memDC, m_fontMono);
    wchar_t brBuf[64];
    swprintf_s(brBuf, L"Banda: ~%.1f Mbps", s.bitrateMbps > 0 ? s.bitrateMbps : 35.0f);
    TextOutW(memDC, c2X + 16, cardY + 146, brBuf, (int)wcslen(brBuf));

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, c2X + 16, cardY + 175, L"Túneis USB Reversos:", 20);

    SelectObject(memDC, m_fontMono);
    SetTextColor(memDC, RGB(0, 230, 118));
    TextOutW(memDC, c2X + 16, cardY + 195, L"● 27183 [Input S-Pen]", 21);
    TextOutW(memDC, c2X + 16, cardY + 215, L"● 27184 [Vídeo 90 FPS]", 22);
    TextOutW(memDC, c2X + 16, cardY + 235, L"● 27185 [Companion]", 19);

    // --- CARTÃO 3: Blender Companion ---
    int c3X = c2X + cardW + 16;
    DrawCardBase(c3X, cardY, cardW, cardH, L"Blender Sculpt");
    DrawLineartBlender(memDC, c3X + 14, cardY + 12);

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, c3X + 16, cardY + 50, L"Pincel Ativo:", 13);

    SelectObject(memDC, m_fontBold);
    SetTextColor(memDC, RGB(255, 179, 0));
    wchar_t wBrush[64];
    MultiByteToWideChar(CP_UTF8, 0, s.activeBrush, -1, wBrush, 64);
    TextOutW(memDC, c3X + 16, cardY + 70, wBrush, (int)wcslen(wBrush));

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, c3X + 16, cardY + 100, L"Parâmetros da Ponta:", 20);

    SelectObject(memDC, m_fontMono);
    SetTextColor(memDC, RGB(236, 239, 244));
    wchar_t radBuf[64];
    swprintf_s(radBuf, L"Raio: %.1f px", s.brushRadius);
    TextOutW(memDC, c3X + 16, cardY + 122, radBuf, (int)wcslen(radBuf));

    wchar_t strBuf[64];
    swprintf_s(strBuf, L"Força: %.2f", s.brushStrength);
    TextOutW(memDC, c3X + 16, cardY + 146, strBuf, (int)wcslen(strBuf));

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, c3X + 16, cardY + 175, L"Simetria Sculpt:", 16);

    SelectObject(memDC, m_fontMono);
    wchar_t symBuf[64];
    swprintf_s(symBuf, L"[%ls] X  [%ls] Y  [%ls] Z",
               s.symmetryX ? L"X" : L" ", s.symmetryY ? L"X" : L" ", s.symmetryZ ? L"X" : L" ");
    SetTextColor(memDC, RGB(0, 229, 255));
    TextOutW(memDC, c3X + 16, cardY + 195, symBuf, (int)wcslen(symBuf));

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, c3X + 16, cardY + 225, L"Atalho: Alt+Z (Remesh)", 22);

    // 4. Rodapé Informativo
    int footY = cardY + cardH + 18;
    HBRUSH footBg = CreateSolidBrush(RGB(22, 24, 30));
    RECT footRc = { 24, footY, width - 24, height - 20 };
    FillRect(memDC, &footRc, footBg);
    DeleteObject(footBg);

    SelectObject(memDC, m_fontRegular);
    SetTextColor(memDC, RGB(144, 155, 172));
    TextOutW(memDC, 36, footY + 10,
             L"Dica: Para mover o Blender para a tela do tablet, pressione: Windows + Shift + Seta Direita.", 92);
    TextOutW(memDC, 36, footY + 30,
             L"O servidor sTab pode permanecer aberto ou minimizado enquanto você esculpe no Blender.", 87);

    // Copia para tela
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

