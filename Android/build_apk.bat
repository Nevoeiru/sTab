@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."

set "JAVA_HOME=%ROOT_DIR%\dependencias\jdk-17"
set "ANDROID_HOME=%ROOT_DIR%\dependencias\android-sdk"
set "ANDROID_SDK_ROOT=%ROOT_DIR%\dependencias\android-sdk"
set "PATH=%ROOT_DIR%\dependencias\platform-tools;%ROOT_DIR%\dependencias\jdk-17\bin;%ROOT_DIR%\dependencias\gradle-8.5\bin;%PATH%"

echo ========================================================
echo   [sTab] Compilando APK Android no Antigravity...
echo ========================================================

cd /d "%SCRIPT_DIR%"
call gradle.bat assembleDebug --no-daemon

if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================================
    echo [SUCESSO] APK compilado com sucesso!
    echo Local: %SCRIPT_DIR%app\build\outputs\apk\debug\app-debug.apk
    echo ========================================================
) else (
    echo.
    echo [ERRO] Falha na compilacao do APK.
    exit /b 1
)

endlocal

