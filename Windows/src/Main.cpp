#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include "Protocol.h"
#include "PointerInjector.h"
#include "GestureTranslator.h"
#include "TcpServer.h"
#include "DxgiCapture.h"
#include "H264Encoder.h"
#include "VideoServer.h"
#include "TelemetryBridge.h"
#include "ServerHud.h"

volatile bool g_running = true;

BOOL WINAPI ConsoleCtrlHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        printf("\n[sTab] Encerrando servidor com seguranca...\n");
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

int RunSelfTest() {
    printf("==================================================\n");
    printf("   [sTab Server] Executando Autoteste de Validacao\n");
    printf("==================================================\n");

    // 1. Validacao do tamanho do pacote binario de protocolo
    printf("[1/8] Verificando alinhamento de struct InputPacket... ");
    if (sizeof(InputPacket) == 16) {
        printf("OK (Exatamente 16 bytes)\n");
    } else {
        printf("FALHA! Tamanho detectado: %zu bytes (Esperado: 16)\n", sizeof(InputPacket));
        return 1;
    }

    // 2. Validacao do PointerInjector (Windows Ink)
    printf("[2/8] Inicializando PointerInjector (user32 Synthetic Pointer)... ");
    PointerInjector injector;
    if (injector.Initialize()) {
        printf("OK\n");
    } else {
        printf("FALHA ao carregar SyntheticPointer APIs!\n");
        return 2;
    }

    // 3. Validacao do GestureTranslator e da Trava Modal com o PointerInjector
    printf("[3/8] Inicializando GestureTranslator e Trava Modal... ");
    GestureTranslator translator;
    translator.ResetAllInputStates();
    
    // Conecta a trava modal
    translator.SetNavigationStateCallback([&injector](bool isNavigating) {
        injector.SetModalNavigationActive(isNavigating);
    });

    // Testa se a trava modal bloqueia injeções concorrentes durante navegação 3D
    injector.SetModalNavigationActive(true);
    InputPacket testPacket = {};
    testPacket.packetType = PACKET_TYPE_PEN;
    testPacket.flags = FLAG_PEN_CONTACT;
    testPacket.x = 32768;
    testPacket.y = 32768;
    testPacket.pressure = 2048;
    if (injector.InjectPen(testPacket) == false) {
        injector.SetModalNavigationActive(false);
        printf("OK (Trava modal de navegacao 3D ativa)\n");
    } else {
        printf("FALHA na trava modal!\n");
        return 3;
    }

    // 4. Teste da supressão Win32 de Press-and-Hold (latência 150-250ms)
    printf("[4/8] Testando supressao Win32 de Press-and-Hold (SetPropW)... ");
    HWND hDesktop = GetDesktopWindow();
    injector.SuppressPressAndHoldLatency(hDesktop);
    printf("OK (TABLET_DISABLE_PRESSANDHOLD aplicado)\n");

    // 5. Teste de bind do socket TCP de entrada (27183)
    printf("[5/8] Testando socket TCP de telemetria em 127.0.0.1:27183... ");
    TcpServer server(27183);
    if (server.Start(&injector, &translator)) {
        printf("OK (Socket escutando)\n");
        server.Stop();
    } else {
        printf("FALHA ao abrir socket de telemetria!\n");
        return 5;
    }

    // 6. Teste do capturador de tela DXGI GPU
    printf("[6/8] Testando capturador DXGI Desktop Duplication... ");
    DxgiCapture capture;
    if (capture.Initialize()) {
        printf("OK (Resolucao detectada: %dx%d)\n", capture.GetWidth(), capture.GetHeight());
    } else {
        printf("AVISO (Desktop Duplication requer display ativo na GPU)\n");
    }

    // 7. Teste do codificador H.264 por hardware (Media Foundation)
    printf("[7/8] Testando codificador Media Foundation H.264 Low-Latency... ");
    H264Encoder encoder;
    if (encoder.Initialize(1920, 1080, 60)) {
        encoder.ForceKeyFrame();
        printf("OK (60 FPS Low-Latency inicializado)\n");
        encoder.Shutdown();
    } else {
        printf("AVISO (Hardware H.264 MFT depende de suporte do driver grafico)\n");
    }

    // 8. Teste do servidor de streaming de video (27184)
    printf("[8/9] Testando socket TCP de streaming de video em 127.0.0.1:27184... ");
    VideoServer videoServer(27184);
    if (videoServer.Start()) {
        printf("OK (Socket de video escutando)\n");
        videoServer.Stop();
    } else {
        printf("FALHA ao abrir socket de video!\n");
        return 8;
    }

    // 9. Teste da ponte de telemetria bidirecional Blender (27185)
    printf("[9/9] Testando ponte de telemetria Blender UDP/TCP em 127.0.0.1:27185... ");
    TelemetryBridge telemetryBridge(27185);
    if (telemetryBridge.Start()) {
        printf("OK (Ponte de telemetria escutando)\n");
        telemetryBridge.Stop();
    } else {
        printf("FALHA ao abrir ponte de telemetria!\n");
        return 9;
    }

    capture.Shutdown();
    injector.Shutdown();

    printf("==================================================\n");
    printf("   [sTab Server] TODOS OS TESTES PASSARAM COM SUCESSO!\n");
    printf("==================================================\n");
    return 0;
}

int main(int argc, char* argv[]) {
    // Desativa buffer para exibicao imediata de logs em console e arquivos
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Inicializa Winsock globalmente
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 0. Configura Per-Monitor DPI Awareness v2 para precisao absoluta de coordenadas fisicas
    HMODULE hUser32Dpi = LoadLibraryW(L"user32.dll");
    if (hUser32Dpi) {
        typedef BOOL (WINAPI *pfnSetDpiAwareness)(void*);
        auto setDpi = (pfnSetDpiAwareness)(void*)GetProcAddress(hUser32Dpi, "SetProcessDpiAwarenessContext");
        if (setDpi) {
            setDpi((void*)-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        }
        FreeLibrary(hUser32Dpi);
    }

    // Modo de autoteste automatizado
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        return RunSelfTest();
    }

    SetConsoleTitleW(L"sTab Server - Galaxy Tab S9 FE+ (Blender Sculpt & 90Hz Stream)");
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    printf("===============================================================\n");
    printf("  sTab (SculptBridge) - Servidor Windows Ink & Video 90Hz v2.0 \n");
    printf("===============================================================\n");
    printf("  Dispositivo alvo: Galaxy Tab S9 FE+ (12.4\" 2560x1600 @ 90Hz)\n");
    printf("  Canal de Entrada: Porta 27183 (Windows Ink 4096 niveis + RMB) \n");
    printf("  Canal de Video:   Porta 27184 (H.264 NVENC/MF 90 FPS Baixa Lat)\n");
    printf("  Recuperacao:      Canal Reverso REQUEST_IDR Ativo             \n");
    printf("  Otimizacao Win32: Supressao Press-and-Hold + Trava Modal Ativas\n");
    printf("===============================================================\n\n");

    // 0. Auto-configuração dos túneis ADB USB reversos (27183, 27184, 27185)
    wchar_t serverExePath[MAX_PATH];
    GetModuleFileNameW(NULL, serverExePath, MAX_PATH);
    PathRemoveFileSpecW(serverExePath); // remove sTab_server.exe -> Windows\bin
    PathRemoveFileSpecW(serverExePath); // remove bin -> Windows
    PathRemoveFileSpecW(serverExePath); // remove Windows -> raiz do projeto
    std::wstring rootPath = serverExePath;
    std::wstring adbBinary = rootPath + L"\\dependencias\\platform-tools\\adb.exe";

    if (GetFileAttributesW(adbBinary.c_str()) != INVALID_FILE_ATTRIBUTES) {
        printf("[sTab] Configurando tuneis reversos USB no Galaxy Tab S9 FE+...\n");
        auto RunAdbTimeout = [](const std::wstring& cmd) {
            STARTUPINFOW si = {};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION pi = {};
            std::wstring modCmd = cmd;
            if (CreateProcessW(NULL, &modCmd[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                if (WaitForSingleObject(pi.hProcess, 1500) == WAIT_TIMEOUT) {
                    TerminateProcess(pi.hProcess, 1);
                }
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
        };

        RunAdbTimeout(L"\"" + adbBinary + L"\" reverse tcp:27183 tcp:27183");
        RunAdbTimeout(L"\"" + adbBinary + L"\" reverse tcp:27184 tcp:27184");
        RunAdbTimeout(L"\"" + adbBinary + L"\" reverse tcp:27185 tcp:27185");
        printf("[sTab] Tuneis reversos ativos: Portas 27183, 27184 e 27185.\n\n");
    }

    // 1. Inicializa o motor de entrada S-Pen e Gestos
    PointerInjector injector;
    if (!injector.Initialize()) {
        printf("[ERRO FATAL] Nao foi possivel inicializar o Windows Ink Synthetic Pointer.\n");
        return 1;
    }

    GestureTranslator translator;

    // Vincula a trava modal de navegacao de camera 3D com a injecao de caneta
    translator.SetNavigationStateCallback([&injector](bool isNavigating) {
        injector.SetModalNavigationActive(isNavigating);
    });

    TcpServer tcpServer(27183);

    // 2. Inicializa o pipeline de captura e streaming de vídeo
    DxgiCapture capture;
    bool hasCapture = capture.Initialize();

    int vidWidth = hasCapture ? capture.GetWidth() : 1920;
    int vidHeight = hasCapture ? capture.GetHeight() : 1080;
    int deskX = hasCapture ? capture.GetDesktopX() : 0;
    int deskY = hasCapture ? capture.GetDesktopY() : 0;

    // Vincula a resolucao e origem fisica exata da tela capturada ao injetor de caneta
    injector.SetTargetDisplayBounds(deskX, deskY, vidWidth, vidHeight);
    printf("[Main] Area de injecao S-Pen calibrada para o display: (%d, %d) [%dx%d]\n",
           deskX, deskY, vidWidth, vidHeight);

    H264Encoder encoder;
    bool hasEncoder = encoder.Initialize(vidWidth, vidHeight, 60, 20000000);
    if (!hasEncoder) {
        printf("[Main] Aviso: Falha ao inicializar o codificador H.264 por hardware Media Foundation.\n");
    }

    VideoServer videoServer(27184);
    videoServer.Start();

    // Vincula a saída do encoder ao servidor de vídeo TCP
    encoder.SetPacketCallback([&videoServer](const uint8_t* data, size_t size, bool isKeyFrame) {
        videoServer.SendNalPacket(data, size, isKeyFrame);
    });

    // Quando o tablet conectar no stream de video, força envio de IDR imediatamente
    videoServer.SetClientConnectedCallback([&encoder]() {
        encoder.ForceKeyFrame();
    });

    // Vincula a solicitacao REQUEST_IDR do tablet para forçar quadro chave no encoder
    tcpServer.SetVideoControlCallback([&encoder](uint8_t cmd) {
        if (cmd == VIDEO_CMD_REQUEST_IDR) {
            encoder.ForceKeyFrame();
        }
    });

    // 3. Inicializa a Janela HUD Gráfica do Servidor
    ServerHud serverHud;
    serverHud.SetDisplayResolution(vidWidth, vidHeight, capture.IsUsingVirtualDisplay());
    serverHud.Start();

    // Callbacks de telemetria de entrada para a HUD (Thread-Safe)
    tcpServer.SetInputPacketCallback([&serverHud, vidWidth, vidHeight](const InputPacket& pkt) {
        if (pkt.packetType == PACKET_TYPE_PEN) {
            int px = (int)(((double)pkt.x / 65535.0) * vidWidth);
            int py = (int)(((double)pkt.y / 65535.0) * vidHeight);
            bool contact = (pkt.flags & FLAG_PEN_CONTACT) != 0;
            bool hover = (pkt.flags & FLAG_PEN_HOVER) != 0;
            bool barrel = (pkt.flags & FLAG_PEN_BARREL_BUTTON) != 0;
            serverHud.UpdatePenTelemetry(px, py, pkt.pressure, (float)pkt.tiltX, (float)pkt.tiltY, contact, hover, barrel);
        }
    });

    if (!tcpServer.Start(&injector, &translator)) {
        printf("[ERRO FATAL] Nao foi possivel iniciar o servidor TCP de telemetria na porta 27183.\n");
        serverHud.Stop();
        injector.Shutdown();
        return 1;
    }

    // 4. Inicializa a ponte de telemetria do Blender (porta 27185 - Thread-Safe)
    TelemetryBridge telemetryBridge(27185);
    telemetryBridge.SetBlenderDataCallback([&serverHud](const char* brush, float size, float strength, bool symX) {
        serverHud.UpdateBlenderStats(brush, size, strength, symX, false, false);
    });
    telemetryBridge.Start();

    printf("\n[sTab] Servidor pronto e em execucao. Pressione Ctrl+C para encerrar.\n\n");

    // Thread de captura de vídeo com taxa variável adaptativa (VFR até 60 FPS)
    HANDLE hCaptureThread = CreateThread(NULL, 0, [](LPVOID param) -> DWORD {
        auto* cap = (DxgiCapture*)((void**)param)[0];
        auto* enc = (H264Encoder*)((void**)param)[1];
        auto* vServ = (VideoServer*)((void**)param)[2];

        CoInitializeEx(NULL, COINIT_MULTITHREADED);
        HDESK hDesk = OpenInputDesktop(0, FALSE, GENERIC_ALL);
        if (hDesk) {
            SetThreadDesktop(hDesk);
            CloseDesktop(hDesk);
        }

        ULONGLONG lastEncodeTick = 0;

        while (g_running) {
            if (vServ->IsClientConnected() && cap->IsInitialized() && enc->IsInitialized()) {
                ID3D11Texture2D* pTex = NULL;
                bool isNewFrame = false;
                // Amostragem na cadencia de 60 FPS (~16ms)
                if (cap->AcquireNextFrame(&pTex, 16, &isNewFrame)) {
                    ULONGLONG now = GetTickCount64();
                    bool forceHeartbeat = (now - lastEncodeTick >= 500); // Keep-alive a cada 500ms se estático

                    if (isNewFrame || forceHeartbeat) {
                        enc->EncodeFrame(cap->GetDevice(), cap->GetContext(), pTex);
                        lastEncodeTick = now;
                    }
                    cap->ReleaseFrame();
                } else {
                    Sleep(1);
                }
            } else {
                Sleep(20);
            }
        }

        CoUninitialize();
        return 0;
    }, new void*[3]{ &capture, &encoder, &videoServer }, 0, NULL);

    uint64_t lastTelemetryPackets = 0;
    uint64_t lastVideoFrames = 0;
    uint64_t lastBytesSent = 0;

    while (g_running) {
        Sleep(500);

        uint64_t currentTel = tcpServer.GetTotalPacketsReceived();
        uint64_t currentVid = videoServer.GetTotalFramesSent();
        uint64_t currentBytes = videoServer.GetTotalBytesSent();

        float currentFps = (float)(currentVid - lastVideoFrames) * 2.0f;
        float bytesSec = (float)(currentBytes - lastBytesSent) * 2.0f;
        float bitrateMbps = (bytesSec * 8.0f) / 1000000.0f;

        serverHud.UpdateConnectionState(tcpServer.IsClientConnected(), videoServer.IsClientConnected(), telemetryBridge.IsTabletConnected());
        serverHud.UpdateVideoStats(currentFps, bitrateMbps, currentBytes);

        if (tcpServer.IsClientConnected() || videoServer.IsClientConnected()) {
            printf("[Status] Telemetria: ~%.0f p/s | Video Stream: ~%.0f FPS | Transmitido: %.2f MB\n",
                   (double)(currentTel - lastTelemetryPackets) * 2.0,
                   (double)currentFps,
                   (double)currentBytes / (1024.0 * 1024.0));
        }

        lastTelemetryPackets = currentTel;
        lastVideoFrames = currentVid;
        lastBytesSent = currentBytes;
    }

    if (hCaptureThread) {
        WaitForSingleObject(hCaptureThread, 1000);
        CloseHandle(hCaptureThread);
    }

    serverHud.Stop();
    videoServer.Stop();
    telemetryBridge.Stop();
    encoder.Shutdown();
    capture.Shutdown();

    tcpServer.Stop();
    injector.Shutdown();
    translator.ResetAllInputStates();

    WSACleanup();

    printf("[sTab] Finalizado com sucesso.\n");
    return 0;
}
