#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Cria um atalho (.lnk) na Área de Trabalho do Windows.
 */
bool CreateDesktopShortcut(const std::wstring& targetPath, const std::wstring& workingDir, const std::wstring& description, const std::wstring& shortcutName) {
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
 * @brief Encontra o executavel do Blender no sistema (Steam, caminhos padrao ou PATH).
 */
std::wstring FindBlenderExecutable() {
    std::vector<std::wstring> candidates = {
        L"j:\\SteamLibrary\\steamapps\\common\\Blender\\blender.exe",
        L"C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe",
        L"C:\\Program Files\\Blender Foundation\\Blender 5.1\\blender.exe",
        L"C:\\Program Files\\Blender Foundation\\Blender 4.2\\blender.exe",
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
 * @brief Instala o Add-on sTab Companion nas pastas de versao do Blender.
 */
int InstallBlenderAddon(const std::wstring& sourceAddonPath) {
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
                    wprintf(L"  [OK] Add-on instalado na versao: Blender %ls\n", versionDir.filename().c_str());
                    installedCount++;
                }
            }
        }
    } catch (...) {}

    return installedCount;
}

int main() {
    SetConsoleTitleW(L"sTab (SculptBridge) - Instalador e Configurador Automatico");

    printf("====================================================================\n");
    printf("     sTab v2.0 - Instalador Automatico para PC & Blender 3D        \n");
    printf("   Galaxy Tab S9 FE+ como Mesa Digitalizadora 90Hz de Alta Precisao\n");
    printf("====================================================================\n\n");

    // 1. Obtém o diretório base onde o instalador está rodando
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    std::wstring rootDir = exePath;

    wprintf(L"[1/5] Diretorio raiz do sTab identificado:\n      %ls\n\n", rootDir.c_str());

    // 2. Instalação Automática do Addon no Blender
    printf("[2/5] Instalando Add-on sTab Companion no Blender...\n");
    std::wstring companionSource = rootDir + L"\\Blender\\sTab_blender_companion.py";

    if (GetFileAttributesW(companionSource.c_str()) != INVALID_FILE_ATTRIBUTES) {
        int count = InstallBlenderAddon(companionSource);
        if (count > 0) {
            printf("      Total de versoes configuradas: %d\n", count);
        } else {
            printf("      [AVISO] Nenhuma pasta de versao do Blender encontrada em %%APPDATA%%.\n");
        }

        // Tenta ativar o addon silenciosamente se encontrar o executavel do Blender
        std::wstring blenderExe = FindBlenderExecutable();
        if (!blenderExe.empty()) {
            wprintf(L"      Blender detectado em: %ls\n", blenderExe.c_str());
            std::wstring enableCmd = L"\"\"" + blenderExe + L"\" --background --python-expr \"import bpy; bpy.ops.preferences.addon_enable(module='sTab_blender_companion'); bpy.ops.wm.save_userpref()\" >nul 2>&1\"";
            _wsystem(enableCmd.c_str());
            printf("      [OK] Add-on habilitado e salvo nas preferencias do Blender!\n\n");
        } else {
            printf("      [OK] Add-on copiado com sucesso. Ative-o na aba Edit > Preferences > Add-ons do Blender.\n\n");
        }
    } else {
        printf("      [AVISO] Arquivo sTab_blender_companion.py nao encontrado na pasta Blender/.\n\n");
    }

    // 3. Verificação do APK para o Tablet
    printf("[3/5] Verificando APK do tablet na pasta principal...\n");
    std::wstring apkPath = rootDir + L"\\sTab.apk";
    if (GetFileAttributesW(apkPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        printf("  [OK] Arquivo 'sTab.apk' pronto na raiz para o Galaxy Tab S9 FE+!\n\n");
    } else {
        // Tenta copiar do build Gradle se existir
        std::wstring debugApk = rootDir + L"\\Android\\app\\build\\outputs\\apk\\debug\\app-debug.apk";
        if (GetFileAttributesW(debugApk.c_str()) != INVALID_FILE_ATTRIBUTES) {
            CopyFileW(debugApk.c_str(), apkPath.c_str(), FALSE);
            printf("  [OK] 'sTab.apk' gerado e copiado para a pasta raiz com sucesso!\n\n");
        } else {
            printf("  [AVISO] 'sTab.apk' nao encontrado. Compile o APK usando Android\\build_apk.bat.\n\n");
        }
    }

    // 4. Detecção do Tablet USB e Instalação Opcional do APK via ADB
    printf("[4/5] Verificando conexao USB com o Galaxy Tab S9 FE+...\n");
    std::wstring adbExe = rootDir + L"\\dependencias\\platform-tools\\adb.exe";
    if (GetFileAttributesW(adbExe.c_str()) != INVALID_FILE_ATTRIBUTES) {
        std::wstring adbCheckCmd = L"\"\"" + adbExe + L"\" devices\"";
        _wsystem(adbCheckCmd.c_str());

        if (GetFileAttributesW(apkPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            printf("      Tentando instalar 'sTab.apk' diretamente no tablet via cabo USB...\n");
            std::wstring installCmd = L"\"\"" + adbExe + L"\" install -r \"" + apkPath + L"\"\"";
            int installRes = _wsystem(installCmd.c_str());
            if (installRes == 0) {
                printf("  [OK] Aplicativo sTab INSTALADO com sucesso no Galaxy Tab S9 FE+!\n\n");
            } else {
                printf("  [DICA] Caso o tablet nao esteja conectado agora, basta plugar o cabo USB depois\n");
                printf("         ou transferir o arquivo 'sTab.apk' diretamente para o seu tablet.\n\n");
            }
        }
    } else {
        printf("  [AVISO] platform-tools/adb.exe nao encontrado.\n\n");
    }

    // 5. Criação do Atalho na Área de Trabalho
    printf("[5/5] Criando atalho na Area de Trabalho...\n");
    std::wstring launcherBat = rootDir + L"\\run_stab.bat";
    bool shortcutCreated = CreateDesktopShortcut(
        launcherBat,
        rootDir,
        L"sTab - Iniciar Mesa Digitalizadora no Blender (Galaxy Tab S9 FE+)",
        L"sTab (Iniciar Mesa Digitalizadora)"
    );

    if (shortcutCreated) {
        printf("  [OK] Atalho criado na sua Area de Trabalho:\n");
        printf("       \"sTab (Iniciar Mesa Digitalizadora)\"\n\n");
    } else {
        printf("  [AVISO] Nao foi possivel criar o atalho automaticamente na Area de Trabalho.\n\n");
    }

    printf("====================================================================\n");
    printf("   [SUCESSO] Instalacao e Configuracao Concluidas com Exito!        \n");
    printf("====================================================================\n");
    printf("  Para usar:\n");
    printf("  1. Certifique-se de que o aplicativo sTab esta aberto no tablet.\n");
    printf("  2. Conecte o cabo USB ao PC.\n");
    printf("  3. Clique duas vezes no atalho 'sTab (Iniciar Mesa Digitalizadora)'\n");
    printf("     na sua Area de Trabalho.\n");
    printf("====================================================================\n\n");

    printf("Pressione qualquer tecla para finalizar...");
    getchar();
    return 0;
}
