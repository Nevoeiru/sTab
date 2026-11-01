#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "Protocol.h"
#include "PointerInjector.h"
#include "GestureTranslator.h"
#include "TcpServer.h"
#include "DxgiCapture.h"
#include "H264Encoder.h"
#include "VideoServer.h"

static volatile bool g_running = true;

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
    printf("[1/7] Verificando alinhamento de struct InputPacket... ");
    if (sizeof(InputPacket) == 16) {
        printf("OK (Exatamente 16 bytes)\n");
    } else {
        printf("FALHA! Tamanho detectado: %zu bytes (Esperado: 16)\n", sizeof(InputPacket));
        return 1;
    }

    // 2. Validacao do PointerInjector (Windows Ink)
    printf("[2/7] Inicializando PointerInjector (user32 Synthetic Pointer)... ");
    PointerInjector injector;
    if (injector.Initialize()) {
        printf("OK\n");
    } else {
        printf("FALHA ao carregar SyntheticPointer APIs!\n");
        return 2;
    }

    // 3. Validacao do GestureTranslator
    printf("[3/7] Inicializando GestureTranslator... ");
    GestureTranslator translator;
    translator.ResetAllInputStates();
    printf("OK\n");

    // 4. Teste de bind do socket TCP de entrada (27183)
    printf("[4/7] Testando socket TCP de telemetria em 127.0.0.1:27183... ");
    TcpServer server(27183);
    if (server.Start(&injector, &translator)) {
        printf("OK (Socket escutando)\n");
        server.Stop();
    } else {
        printf("FALHA ao abrir socket de telemetria!\n");
        return 4;
    }

    // 5. Teste do capturador de tela DXGI GPU
    printf("[5/7] Testando capturador DXGI Desktop Duplication... ");
    DxgiCapture capture;
    if (capture.Initialize()) {
        printf("OK (Resolucao detectada: %dx%d)\n", capture.GetWidth(), capture.GetHeight());
    } else {
        printf("AVISO (Desktop Duplication requer display ativo na GPU)\n");
    }

    // 6. Teste do codificador H.264 por hardware (Media Foundation)
    printf("[6/7] Testando codificador Media Foundation H.264 Low-Latency... ");
    H264Encoder encoder;
    if (encoder.Initialize(1920, 1080, 90)) {
        encoder.ForceKeyFrame();
        printf("OK (90 FPS Low-Latency inicializado)\n");
        encoder.Shutdown();
    } else {
        printf("AVISO (Hardware H.264 MFT depende de suporte do driver grafico)\n");
    }

    // 7. Teste do servidor de streaming de video (27184)
    printf("[7/7] Testando socket TCP de streaming de video em 127.0.0.1:27184... ");
    VideoServer videoServer(27184);
    if (videoServer.Start()) {
        printf("OK (Socket de video escutando)\n");
        videoServer.Stop();
    } else {
        printf("FALHA ao abrir socket de video!\n");
        return 7;
    }

    capture.Shutdown();
    injector.Shutdown();

    printf("==================================================\n");
    printf("   [sTab Server] TODOS OS TESTES PASSARAM COM SUCESSO!\n");
    printf("==================================================\n");
    return 0;
}

int main(int argc, char* argv[]) {
    // Modo de autoteste automatizado
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        return RunSelfTest();
    }

    SetConsoleTitleW(L"sTab Server - Galaxy Tab S9 FE+ (Blender Sculpt & 90Hz Stream)");
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    printf("===============================================================\n");
    printf("  sTab (SculptBridge) - Servidor Windows Ink & Video 90Hz v1.0 \n");
    printf("===============================================================\n");
    printf("  Dispositivo alvo: Galaxy Tab S9 FE+ (12.4\" 2560x1600 @ 90Hz)\n");
    printf("  Canal de Entrada: Porta 27183 (Windows Ink 4096 niveis + RMB) \n");
    printf("  Canal de Video:   Porta 27184 (H.264 NVENC/MF 90 FPS Baixa Lat)\n");
    printf("  Recuperacao:      Canal Reverso REQUEST_IDR Ativo             \n");
    printf("===============================================================\n\n");

    // 1. Inicializa o motor de entrada S-Pen e Gestos
    PointerInjector injector;
    if (!injector.Initialize()) {
        printf("[ERRO FATAL] Nao foi possivel inicializar o Windows Ink Synthetic Pointer.\n");
        return 1;
    }

    GestureTranslator translator;
    TcpServer tcpServer(27183);

    // 2. Inicializa o pipeline de captura e streaming de vídeo
    DxgiCapture capture;
    bool hasCapture = capture.Initialize();

    int vidWidth = hasCapture ? capture.GetWidth() : 1920;
    int vidHeight = hasCapture ? capture.GetHeight() : 1080;

    H264Encoder encoder;
    bool hasEncoder = encoder.Initialize(vidWidth, vidHeight, 90, 35000000);
    if (!hasEncoder) {
        printf("[Main] Aviso: Falha ao inicializar o codificador H.264 por hardware Media Foundation.\n");
    }

    VideoServer videoServer(27184);
    videoServer.Start();

    // Vincula a saída do encoder ao servidor de vídeo TCP
    encoder.SetPacketCallback([&videoServer](const uint8_t* data, size_t size, bool isKeyFrame) {
        videoServer.SendNalPacket(data, size, isKeyFrame);
    });

    // Vincula a solicitacao REQUEST_IDR do tablet para forçar quadro chave no encoder
    tcpServer.SetVideoControlCallback([&encoder](uint8_t cmd) {
        if (cmd == VIDEO_CMD_REQUEST_IDR) {
            encoder.ForceKeyFrame();
        }
    });

    if (!tcpServer.Start(&injector, &translator)) {
        printf("[ERRO FATAL] Nao foi possivel iniciar o servidor TCP de telemetria na porta 27183.\n");
        injector.Shutdown();
        return 1;
    }

    printf("\n[sTab] Servidor pronto e em execucao. Pressione Ctrl+C para encerrar.\n\n");

    // Thread de captura de vídeo contínua a 90 FPS
    HANDLE hCaptureThread = CreateThread(NULL, 0, [](LPVOID param) -> DWORD {
        auto* cap = (DxgiCapture*)((void**)param)[0];
        auto* enc = (H264Encoder*)((void**)param)[1];
        auto* vServ = (VideoServer*)((void**)param)[2];

        while (g_running) {
            if (vServ->IsClientConnected() && cap->IsInitialized() && enc->IsInitialized()) {
                ID3D11Texture2D* pTex = NULL;
                if (cap->AcquireNextFrame(&pTex, 11)) { // ~11ms para sincronizar com 90 FPS
                    enc->EncodeFrame(cap->GetDevice(), cap->GetContext(), pTex);
                    cap->ReleaseFrame();
                }
            } else {
                Sleep(20);
            }
        }
        return 0;
    }, new void*[3]{ &capture, &encoder, &videoServer }, 0, NULL);

    uint64_t lastTelemetryPackets = 0;
    uint64_t lastVideoFrames = 0;

    while (g_running) {
        Sleep(2000);

        if (tcpServer.IsClientConnected() || videoServer.IsClientConnected()) {
            uint64_t currentTel = tcpServer.GetTotalPacketsReceived();
            uint64_t currentVid = videoServer.GetTotalFramesSent();

            uint64_t telRate = (currentTel - lastTelemetryPackets) / 2;
            uint64_t vidFps  = (currentVid - lastVideoFrames) / 2;

            lastTelemetryPackets = currentTel;
            lastVideoFrames = currentVid;

            printf("[Status] Telemetria: ~%llu p/s | Video Stream: ~%llu FPS | Transmitido: %.2f MB\n",
                   telRate, vidFps, (double)videoServer.GetTotalBytesSent() / (1024.0 * 1024.0));
        }
    }

    if (hCaptureThread) {
        WaitForSingleObject(hCaptureThread, 1000);
        CloseHandle(hCaptureThread);
    }

    videoServer.Stop();
    encoder.Shutdown();
    capture.Shutdown();

    tcpServer.Stop();
    injector.Shutdown();
    translator.ResetAllInputStates();

    printf("[sTab] Finalizado com sucesso.\n");
    return 0;
}
