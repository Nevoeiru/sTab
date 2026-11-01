#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string>

/**
 * @brief Launcher integrado sTab (SculptBridge).
 * Configura os túneis ADB USB reversos e inicializa o servidor de baixa latência.
 */
int main() {
    SetConsoleTitleA("sTab (SculptBridge) - Launcher Galaxy Tab S9 FE+");

    printf("====================================================================\n");
    printf("   sTab (SculptBridge) - Launcher Integrado para Blender 3D        \n");
    printf("   Galaxy Tab S9 FE+ como Mesa Digitalizadora 90Hz de Alta Precisao\n");
    printf("====================================================================\n\n");

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecA(exePath);

    std::string binDir = exePath;
    std::string rootDir = binDir;

    // Se estiver em Windows\bin, a raiz fica dois níveis acima
    char rootBuf[MAX_PATH];
    strcpy_s(rootBuf, exePath);
    PathRemoveFileSpecA(rootBuf); // Sai de bin para Windows
    PathRemoveFileSpecA(rootBuf); // Sai de Windows para raiz do projeto
    rootDir = rootBuf;

    std::string adbPath = "\"" + rootDir + "\\dependencias\\platform-tools\\adb.exe\"";
    std::string serverPath = "\"" + binDir + "\\sTab_server.exe\"";

    DWORD serverAttr = GetFileAttributesA((binDir + "\\sTab_server.exe").c_str());
    if (serverAttr == INVALID_FILE_ATTRIBUTES) {
        serverPath = "\"" + rootDir + "\\Windows\\bin\\sTab_server.exe\"";
    }

    // 0. Garante que qualquer instancia orfa ou antiga seja encerrada para liberar portas
    system("taskkill /F /IM sTab_server.exe >nul 2>&1");
    Sleep(300);

    // 1. Configura os tuneis reversos via ADB
    printf("[1/3] Configurando tuneis USB reversos de ultra-baixa latencia...\n");
    auto RunTimedCmd = [](const std::string& cmd) -> bool {
        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {};
        std::string modCmd = cmd;
        if (CreateProcessA(NULL, &modCmd[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            DWORD waitRes = WaitForSingleObject(pi.hProcess, 1500);
            if (waitRes == WAIT_TIMEOUT) {
                TerminateProcess(pi.hProcess, 1);
            }
            DWORD exitCode = 1;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return (waitRes == WAIT_OBJECT_0 && exitCode == 0);
        }
        return false;
    };

    bool ok1 = RunTimedCmd(adbPath + " reverse tcp:27183 tcp:27183");
    bool ok2 = RunTimedCmd(adbPath + " reverse tcp:27184 tcp:27184");
    bool ok3 = RunTimedCmd(adbPath + " reverse tcp:27185 tcp:27185");

    if (ok1 && ok2 && ok3) {
        printf("  [OK] Porta 27183 (Input S-Pen 240Hz, Touch e Modificadores) ATIVA.\n");
        printf("  [OK] Porta 27184 (Video H.264 90Hz DXGI GPU) ATIVA.\n");
        printf("  [OK] Porta 27185 (Telemetria Bidirecional Blender) ATIVA.\n\n");
    } else {
        printf("  [AVISO] Nao foi possivel configurar o tunel reverso automaticamente.\n");
        printf("  Certifique-se de que o tablet esta conectado via cabo USB com Depuracao USB ativa.\n\n");
    }

    // Garante que o Windows esteja em modo de Area de Trabalho Estendida (/extend)
    system("displayswitch.exe /extend >nul 2>&1");

    // Checa se ha monitor secundario estendido ativo
    bool hasExtendedDisplay = false;
    DISPLAY_DEVICEA dd = {};
    dd.cb = sizeof(dd);
    for (DWORD devNum = 0; EnumDisplayDevicesA(NULL, devNum, &dd, 0); devNum++) {
        if (!(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) && (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
            DEVMODEA dm = {};
            dm.dmSize = sizeof(dm);
            if (EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
                hasExtendedDisplay = true;
                printf("  [DISPLAY] Monitor Estendido ativo em %s (%lux%lu @ %luHz)!\n",
                       dd.DeviceName, (unsigned long)dm.dmPelsWidth, (unsigned long)dm.dmPelsHeight, (unsigned long)dm.dmDisplayFrequency);
            }
        }
    }
    if (!hasExtendedDisplay) {
        printf("  [DISPLAY] Monitor estendido nao encontrado. Ativando modo espelhado padrao.\n\n");
    } else {
        printf("\n");
    }

    // 2. Inicia o app no tablet via ADB Intent
    printf("[2/3] Enviando sinal para abrir o app sTab no tablet...\n");
    std::string launchAppCmd = adbPath + " shell am start -n com.stab.sculptbridge/.MainActivity >nul 2>&1";
    system(launchAppCmd.c_str());

    // 3. Executa o servidor principal do sTab
    printf("[3/3] Iniciando o servidor Windows sTab_server.exe com Janela HUD...\n");
    printf("====================================================================\n");
    printf("   A Janela Grafica HUD sTab esta aberta.\n");
    printf("   Feche a janela HUD ou pressione Ctrl+C para encerrar.\n");
    printf("====================================================================\n\n");

    int serverRes = system(serverPath.c_str());

    printf("\n[sTab Launcher] Servidor finalizado com codigo: %d\n", serverRes);
    if (serverRes != 0) {
        system("pause");
    }
    return serverRes;
}

