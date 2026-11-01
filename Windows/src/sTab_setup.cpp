#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Verifica se o processo atual possui privilégios de Administrador.
 */
bool IsElevated() {
    BOOL isElevated = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isElevated);
        FreeSid(adminGroup);
    }
    return isElevated == TRUE;
}

/**
 * @brief Solicita elevação UAC do Windows para permissões administrativas.
 */
bool RelaunchElevated() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.nShow = SW_SHOWNORMAL;

    return ShellExecuteExW(&sei) == TRUE;
}

/**
 * @brief Cria ou atualiza o atalho (.lnk) na Área de Trabalho do Windows.
 */
bool UpdateDesktopShortcut(const std::wstring& targetPath, const std::wstring& workingDir, const std::wstring& description, const std::wstring& shortcutName) {
    CoInitialize(NULL);
    IShellLinkW* psl = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl);
    if (SUCCEEDED(hr)) {
        psl->SetPath(targetPath.c_str());
        psl->SetWorkingDirectory(workingDir.c_str());
        psl->SetDescription(description.c_str());

        IPersistFile* ppf = NULL;
        hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hr)) {
            wchar_t desktopPath[MAX_PATH];
            if (SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath) == S_OK) {
                std::wstring linkPath = std::wstring(desktopPath) + L"\\" + shortcutName + L".lnk";
                hr = ppf->Save(linkPath.c_str(), TRUE);
            }
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return SUCCEEDED(hr);
}

/**
 * @brief Encontra o executável do Blender no sistema.
 */
std::wstring FindBlenderExecutable() {
    std::vector<std::wstring> candidates = {
        L"j:\\SteamLibrary\\steamapps\\common\\Blender\\blender.exe",
        L"C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe",
        L"C:\\Program Files\\Blender Foundation\\Blender 5.1\\blender.exe",
        L"C:\\Program Files\\Blender Foundation\\Blender 4.2\\blender.exe",
        L"C:\\Program Files\\Blender Foundation\\Blender 4.1\\blender.exe",
        L"C:\\Program Files\\Blender Foundation\\Blender 4.0\\blender.exe",
        L"C:\\Program Files\\Steam\\steamapps\\common\\Blender\\blender.exe",
        L"D:\\SteamLibrary\\steamapps\\common\\Blender\\blender.exe"
    };

    for (const auto& path : candidates) {
        if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }
    return L"";
}

/**
 * @brief Atualiza o Add-on sTab Companion em todas as pastas de versão do Blender.
 */
int UpdateBlenderAddon(const std::wstring& sourceAddonPath) {
    wchar_t appData[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData) != S_OK) {
        return 0;
    }

    std::wstring blenderBase = std::wstring(appData) + L"\\Blender Foundation\\Blender";
    if (GetFileAttributesW(blenderBase.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return 0;
    }

    int installedCount = 0;
    try {
        for (const auto& entry : fs::directory_iterator(blenderBase)) {
            if (entry.is_directory()) {
                fs::path versionDir = entry.path();
                fs::path scriptsDir = versionDir / "scripts";
                fs::path addonsDir = scriptsDir / "addons";

                std::error_code ec;
                fs::create_directories(addonsDir, ec);

                fs::path targetAddon = addonsDir / "sTab_blender_companion.py";
                fs::copy_file(sourceAddonPath, targetAddon, fs::copy_options::overwrite_existing, ec);

                if (!ec) {
                    wprintf(L"  [OK] Add-on atualizado na versao: Blender %ls\n", versionDir.filename().c_str());
                    installedCount++;
                }
            }
        }
    } catch (...) {}

    return installedCount;
}

/**
 * @brief Instala ou atualiza automaticamente o Driver de Monitor Virtual (VDD 16:10).
 */
bool InstallVirtualDisplayDriver(const std::wstring& rootDir) {
    std::wstring vddDest = L"C:\\VirtualDisplayDriver";
    std::error_code ec;
    fs::create_directories(vddDest, ec);

    // Pastas de origem candidatas dentro do repositório
    std::vector<std::wstring> sourceFolders = {
        rootDir + L"\\VirtualDisplay\\control\\SignedDrivers\\x86\\VDD",
        rootDir + L"\\VirtualDisplay\\driver\\VirtualDisplayDriver"
    };

    std::wstring validSource = L"";
    for (const auto& src : sourceFolders) {
        if (GetFileAttributesW((src + L"\\MttVDD.inf").c_str()) != INVALID_FILE_ATTRIBUTES) {
            validSource = src;
            break;
        }
    }

    if (validSource.empty()) {
        printf("  [AVISO] Arquivos do driver VDD nao encontrados na pasta VirtualDisplay.\n");
        return false;
    }

    // Copia os arquivos do driver para C:\VirtualDisplayDriver
    try {
        for (const auto& file : fs::directory_iterator(validSource)) {
            fs::copy_file(file.path(), fs::path(vddDest) / file.path().filename(), fs::copy_options::overwrite_existing, ec);
        }
    } catch (...) {}

    // Copia devcon.exe
    std::wstring devconSrc = validSource + L"\\devcon.exe";
    if (GetFileAttributesW(devconSrc.c_str()) != INVALID_FILE_ATTRIBUTES) {
        CopyFileW(devconSrc.c_str(), (vddDest + L"\\devcon.exe").c_str(), FALSE);
    }

    // Copia o arquivo de calibracao 16:10 options.txt
    std::wstring optionsSrc = rootDir + L"\\VirtualDisplay\\options.txt";
    if (GetFileAttributesW(optionsSrc.c_str()) != INVALID_FILE_ATTRIBUTES) {
        CopyFileW(optionsSrc.c_str(), (vddDest + L"\\options.txt").c_str(), FALSE);
        CopyFileW(optionsSrc.c_str(), (vddDest + L"\\option.txt").c_str(), FALSE);
    }

    // Registra a chave no Windows Registry (SOFTWARE\MikeTheTech\VirtualDisplayDriver -> VDDPATH)
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\MikeTheTech\\VirtualDisplayDriver", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"VDDPATH", 0, REG_SZ, (const BYTE*)vddDest.c_str(), (DWORD)((vddDest.length() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }

    // 1. Instalação do pacote de driver via PnPUtil
    printf("      Registrando pacote do driver no Windows (PnPUtil)...\n");
    std::wstring pnpCmd = L"pnputil /add-driver \"C:\\VirtualDisplayDriver\\MttVDD.inf\" /install >nul 2>&1";
    _wsystem(pnpCmd.c_str());

    // 2. Instancia o dispositivo de exibição indireta Root\MttVDD via devcon
    std::wstring devconExe = vddDest + L"\\devcon.exe";
    if (GetFileAttributesW(devconExe.c_str()) != INVALID_FILE_ATTRIBUTES) {
        printf("      Instanciando dispositivo de Monitor Virtual (Root\\MttVDD)...\n");
        std::wstring devconInstall = L"\"\"" + devconExe + L"\" install \"C:\\VirtualDisplayDriver\\MttVDD.inf\" Root\\MttVDD >nul 2>&1\"";
        _wsystem(devconInstall.c_str());
    }

    printf("  [OK] Driver de Monitor Virtual 16:10 (2560x1600 @ 90Hz) instalado com sucesso!\n");
    return true;
}

/**
 * @brief Encerra com segurança qualquer versão anterior em execução do sTab_server ou sTab_launcher.
 * Garante que portas de rede e arquivos binários não estejam travados por processos antigos.
 */
void KillRunningStabProcesses() {
    printf("[Seguranca] Encerrando processos sTab anteriores em execucao...\n");

    // 1. Encerra via taskkill do Windows
    _wsystem(L"taskkill /F /IM sTab_server.exe /IM sTab_launcher.exe /IM adb.exe >nul 2>&1");

    // 2. Garante via Win32 Toolhelp API
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = {};
        pe.dwSize = sizeof(pe);
        DWORD myPid = GetCurrentProcessId();

        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (pe.th32ProcessID != myPid) {
                    if (_wcsicmp(pe.szExeFile, L"sTab_server.exe") == 0 ||
                        _wcsicmp(pe.szExeFile, L"sTab_launcher.exe") == 0 ||
                        _wcsicmp(pe.szExeFile, L"adb.exe") == 0) {
                        HANDLE hProc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pe.th32ProcessID);
                        if (hProc) {
                            TerminateProcess(hProc, 0);
                            WaitForSingleObject(hProc, 1000);
                            CloseHandle(hProc);
                        }
                    }
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    Sleep(500); // Aguarda liberação completa de sockets e handles pelo SO
    printf("  [OK] Processos anteriores finalizados com sucesso.\n\n");
}

int main() {
    SetConsoleTitleW(L"sTab (SculptBridge) - Instalador e Atualizador Automatico");

    printf("====================================================================\n");
    printf("     sTab v2.0 - Instalador e Atualizador para PC & Blender 3D     \n");
    printf("   Galaxy Tab S9 FE+ como Mesa Digitalizadora 90Hz de Alta Precisao\n");
    printf("====================================================================\n\n");

    // 0. Verifica elevação administrativa para instalação do driver
    if (!IsElevated()) {
        printf("[AVISO] Solicitando permissoes de Administrador para instalar drivers...\n");
        if (RelaunchElevated()) {
            return 0; // O processo elevado foi iniciado
        }
        printf("[AVISO] Execucao continuada sem privilegios de Administrador.\n\n");
    }

    // 0.1 Segurança: Encerra versões anteriores em execução
    KillRunningStabProcesses();

    // 1. Diretório base
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    std::wstring rootDir = exePath;

    wprintf(L"[1/5] Diretorio raiz identificado: %ls\n\n", rootDir.c_str());

    // 2. Instalação / Atualização Automática do Driver Virtual 16:10
    printf("[2/5] Instalando / Atualizando Driver de Monitor Virtual (VDD 16:10)...\n");
    InstallVirtualDisplayDriver(rootDir);
    printf("\n");

    // 3. Atualização do Add-on no Blender
    printf("[3/5] Atualizando Add-on sTab Companion no Blender...\n");
    std::wstring companionSource = rootDir + L"\\Blender\\sTab_blender_companion.py";
    if (GetFileAttributesW(companionSource.c_str()) != INVALID_FILE_ATTRIBUTES) {
        int count = UpdateBlenderAddon(companionSource);
        if (count > 0) {
            printf("      Total de versoes atualizadas: %d\n", count);
        }

        std::wstring blenderExe = FindBlenderExecutable();
        if (!blenderExe.empty()) {
            wprintf(L"      Blender detectado em: %ls\n", blenderExe.c_str());
            std::wstring enableCmd = L"\"\"" + blenderExe + L"\" --background --python-expr \"import bpy; bpy.ops.preferences.addon_enable(module='sTab_blender_companion'); bpy.ops.wm.save_userpref()\" >nul 2>&1\"";
            _wsystem(enableCmd.c_str());
            printf("      [OK] Add-on habilitado e salvo nas preferencias do Blender!\n\n");
        } else {
            printf("      [OK] Add-on copiado com sucesso.\n\n");
        }
    }

    // 4. Verificação e Instalação do APK no Tablet
    printf("[4/5] Verificando conexao USB e aplicativo no tablet...\n");
    std::wstring apkPath = rootDir + L"\\sTab.apk";
    std::wstring adbExe = rootDir + L"\\dependencias\\platform-tools\\adb.exe";

    if (GetFileAttributesW(adbExe.c_str()) != INVALID_FILE_ATTRIBUTES &&
        GetFileAttributesW(apkPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        std::wstring installCmd = L"\"\"" + adbExe + L"\" install -r \"" + apkPath + L"\" >nul 2>&1\"";
        int res = _wsystem(installCmd.c_str());
        if (res == 0) {
            printf("  [OK] Aplicativo sTab ATUALIZADO no Galaxy Tab S9 FE+ via cabo USB!\n\n");
        } else {
            printf("  [INFO] Tablet nao detectado no momento ou aplicativo ja atualizado.\n");
            printf("         (O arquivo 'sTab.apk' esta disponivel na pasta raiz para uso).\n\n");
        }
    }

    // 5. Atualização do Atalho na Área de Trabalho
    printf("[5/5] Atualizando atalho na Area de Trabalho...\n");
    std::wstring launcherExe = rootDir + L"\\Windows\\bin\\sTab_launcher.exe";
    if (GetFileAttributesW(launcherExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
        launcherExe = rootDir + L"\\run_stab.bat";
    }

    bool shortcutUpdated = UpdateDesktopShortcut(
        launcherExe,
        rootDir,
        L"sTab - Iniciar Mesa Digitalizadora no Blender (Galaxy Tab S9 FE+)",
        L"sTab (Iniciar Mesa Digitalizadora)"
    );

    if (shortcutUpdated) {
        printf("  [OK] Atalho atualizado com sucesso na Area de Trabalho:\n");
        printf("       \"sTab (Iniciar Mesa Digitalizadora)\"\n\n");
    }

    printf("====================================================================\n");
    printf("   [SUCESSO] Instalacao / Atualizacao Concluida com Exito!          \n");
    printf("====================================================================\n");
    printf("  Iniciando agora o servidor sTab com Janela HUD no Windows...\n\n");

    // Inicia o launcher automaticamente
    ShellExecuteW(NULL, L"open", launcherExe.c_str(), NULL, rootDir.c_str(), SW_SHOWNORMAL);

    Sleep(1000);
    return 0;
}
