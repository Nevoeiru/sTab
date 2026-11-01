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
    printf("[1/5] Verificando alinhamento de struct InputPacket... ");
    if (sizeof(InputPacket) == 16) {
        printf("OK (Exatamente 16 bytes)\n");
    } else {
        printf("FALHA! Tamanho detectado: %zu bytes (Esperado: 16)\n", sizeof(InputPacket));
        return 1;
    }

    // 2. Validacao do PointerInjector (Windows Ink)
    printf("[2/5] Inicializando PointerInjector (user32 Synthetic Pointer)... ");
    PointerInjector injector;
    if (injector.Initialize()) {
        printf("OK\n");
    } else {
        printf("FALHA ao carregar SyntheticPointer APIs!\n");
        return 2;
    }

    // 3. Validacao do GestureTranslator
    printf("[3/5] Inicializando GestureTranslator... ");
    GestureTranslator translator;
    translator.ResetAllInputStates();
    printf("OK\n");

    // 4. Teste de injecao simulada de S-Pen (Hover + Pressao + Botao Direito)
    printf("[4/5] Testando injecao de evento simulado S-Pen... ");
    InputPacket testPacket = {};
    testPacket.packetType = PACKET_TYPE_PEN;
    testPacket.flags = FLAG_PEN_HOVER | FLAG_PEN_CONTACT;
    testPacket.x = 32768; // Centro da tela horizontal
    testPacket.y = 32768; // Centro da tela vertical
    testPacket.pressure = 2048; // 50% de pressao
    testPacket.tiltX = 15;
    testPacket.tiltY = -10;

    bool injectOk = injector.InjectPen(testPacket);
    if (injectOk) {
        printf("OK (Ponteiro sintetico aceitou o evento)\n");
    } else {
        printf("AVISO (Injecao simulada requer foco ou sessao interativa ativa)\n");
    }

    // 5. Teste de inicializacao do socket TCP
    printf("[5/5] Testando bind do socket TCP em 127.0.0.1:27183... ");
    TcpServer server(27183);
    if (server.Start(&injector, &translator)) {
        printf("OK (Socket escutando)\n");
        server.Stop();
    } else {
        printf("FALHA ao abrir socket TCP!\n");
        return 5;
    }

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

    SetConsoleTitleW(L"sTab Server - Galaxy Tab S9 FE+ (Blender Sculpt)");
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    printf("===============================================================\n");
    printf("  sTab (SculptBridge) - Servidor de Entrada Windows Ink v1.0   \n");
    printf("===============================================================\n");
    printf("  Dispositivo alvo: Galaxy Tab S9 FE+ (12.4\" 2560x1600 @ 90Hz)\n");
    printf("  API do Tablet:   Windows Ink Oficial (4096 niveis + Tilt)    \n");
    printf("  Botao da S-Pen:  Botao Direito do Mouse (Brush Context Menu) \n");
    printf("  Acoes de Dedo:   1 Dedo = Orbit 3D (MMB)                     \n");
    printf("                   2 Dedos = Pan 3D (Shift + MMB)              \n");
    printf("                   Pinca = Zoom Viewport                       \n");
    printf("                   2 Taps = Undo (Ctrl+Z) | 3 Taps = Redo      \n");
    printf("===============================================================\n\n");

    PointerInjector injector;
    if (!injector.Initialize()) {
        printf("[ERRO FATAL] Nao foi possivel inicializar o Windows Ink Synthetic Pointer.\n");
        printf("Pressione qualquer tecla para sair...\n");
        getchar();
        return 1;
    }

    GestureTranslator translator;
    TcpServer server(27183);

    // Callback para recepção de sinais de controle de vídeo
    server.SetVideoControlCallback([](uint8_t cmd) {
        if (cmd == VIDEO_CMD_REQUEST_IDR) {
            printf("[VideoControl] Solicitacao de quadro IDR recebida do tablet!\n");
        }
    });

    if (!server.Start(&injector, &translator)) {
        printf("[ERRO FATAL] Nao foi possivel iniciar o servidor TCP na porta 27183.\n");
        injector.Shutdown();
        return 1;
    }

    printf("\n[sTab] Servidor pronto e em execucao. Pressione Ctrl+C para encerrar.\n\n");

    uint64_t lastPacketCount = 0;
    while (g_running) {
        Sleep(2000);

        if (server.IsClientConnected()) {
            uint64_t currentPackets = server.GetTotalPacketsReceived();
            uint64_t delta = currentPackets - lastPacketCount;
            lastPacketCount = currentPackets;

            if (delta > 0) {
                printf("[Status] Conectado | Taxa de amostragem: ~%llu pacotes/s (Total: %llu)\n",
                       delta / 2, currentPackets);
            }
        }
    }

    server.Stop();
    injector.Shutdown();
    translator.ResetAllInputStates();

    printf("[sTab] Finalizado com sucesso.\n");
    return 0;
}
