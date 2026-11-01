@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."
set "ADB_PATH=%ROOT_DIR%\dependencias\platform-tools\adb.exe"
set "APK_PATH=%SCRIPT_DIR%app\build\outputs\apk\debug\app-debug.apk"

echo ========================================================
echo   [sTab] Instalando sTab.apk no Galaxy Tab S9 FE+...
echo ========================================================

if not exist "%APK_PATH%" (
    echo [ERRO] APK nao encontrado. Execute build_apk.bat primeiro!
    exit /b 1
)

echo Verificando dispositivos USB conectados:
"%ADB_PATH%" devices

echo.
echo Instalando APK via cabo USB...
"%ADB_PATH%" install -r "%APK_PATH%"

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCESSO] Aplicativo instalado no tablet com exito!
    echo Abrindo o sTab no tablet...
    "%ADB_PATH%" shell am start -n com.stab.sculptbridge/.MainActivity
) else (
    echo.
    echo [ERRO] Falha ao instalar no tablet. Verifique se a Depuracao USB esta ativa.
)

endlocal

