#include <windows.h>
#include <stdio.h>
#include <string>

/**
 * @brief Launcher integrado sTab (SculptBridge).
 * Configura os túneis ADB USB reversos e inicializa o servidor de baixa latência.
 */
int main() {
    SetConsoleTitleA("sTab (SculptBridge) - Launcher Galaxy Tab S9 FE+");

    printf("====================================================================\n");
    printf("   sTab (SculptBridge) - Launcher Integrado para Blender 3D\n");
    printf("   Galaxy Tab S9 FE+ como Mesa Digitalizadora 90Hz de Alta Precisao\n");
    printf("====================================================================\n\n");

    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);

    std::string adbPath = "..\\dependencias\\platform-tools\\adb.exe";
    std::string serverPath = "bin\\sTab_server.exe";

    // Checa se o servidor existe
    DWORD serverAttr = GetFileAttributesA(serverPath.c_str());
    if (serverAttr == INVALID_FILE_ATTRIBUTES) {
        printf("[AVISO] Servidor 'bin\\sTab_server.exe' nao encontrado no diretorio local.\n");
        serverPath = "sTab_server.exe";
    }

    // 1. Configura os tuneis reversos via ADB
    printf("[1/3] Configurando tuneis USB reversos de ultra-baixa latencia...\n");
    std::string cmd1 = adbPath + " reverse tcp:27183 tcp:27183";
    std::string cmd2 = adbPath + " reverse tcp:27184 tcp:27184";
    std::string cmd3 = adbPath + " reverse tcp:27185 tcp:27185";

    int res1 = system(cmd1.c_str());
    int res2 = system(cmd2.c_str());
    int res3 = system(cmd3.c_str());

    if (res1 == 0 && res2 == 0 && res3 == 0) {
        printf("  [OK] Porta 27183 (Input S-Pen 240Hz, Touch e Modificadores) ATIVA.\n");
        printf("  [OK] Porta 27184 (Video H.264 90Hz DXGI GPU) ATIVA.\n");
        printf("  [OK] Porta 27185 (Telemetria Bidirecional Blender) ATIVA.\n\n");
    } else {
        printf("  [AVISO] Nao foi possivel configurar o tunel reverso automaticamente.\n");
        printf("  Certifique-se de que o tablet esta conectado via cabo USB com Depuracao USB ativa.\n\n");
    }

    // Checa se ha monitor 16:10 / 2560x1600 conectado
    bool has1610 = false;
    DISPLAY_DEVICEA dd = {};
    dd.cb = sizeof(dd);
    for (DWORD devNum = 0; EnumDisplayDevicesA(NULL, devNum, &dd, 0); devNum++) {
        DEVMODEA dm = {};
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
            if (dm.dmPelsWidth == 2560 && dm.dmPelsHeight == 1600) {
                has1610 = true;
                break;
            }
        }
    }
    if (has1610) {
        printf("  [DISPLAY] Monitor 2560x1600 @ 90Hz (VDD 16:10 Nativo) DETECTADO!\n\n");
    } else {
        printf("  [DISPLAY] Modo espelhamento / display padrao ativo.\n");
        printf("  (Dica: Para resolucao 16:10 nativa sem faixas pretas, consulte VirtualDisplay/INSTALACAO_VDD.md)\n\n");
    }

    // 2. Inicia o app no tablet via ADB Intent
    printf("[2/3] Enviando sinal para abrir o app sTab no tablet...\n");
    std::string launchAppCmd = adbPath + " shell am start -n com.stab.sculptbridge/.MainActivity >nul 2>&1";
    system(launchAppCmd.c_str());

    // 3. Executa o servidor principal do sTab
    printf("[3/3] Iniciando o servidor Windows sTab_server.exe...\n");
    printf("====================================================================\n");
    printf("   Pressione Ctrl+C para encerrar o servidor a qualquer momento.\n");
    printf("====================================================================\n\n");

    int serverRes = system(serverPath.c_str());

    printf("\n[sTab Launcher] Servidor finalizado com codigo: %d\n", serverRes);
    return serverRes;
}

