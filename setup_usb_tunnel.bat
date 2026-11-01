@echo off
setlocal

set "ROOT_DIR=%~dp0"
set "ADB_PATH=%ROOT_DIR%dependencias\platform-tools\adb.exe"

echo ========================================================
echo   [sTab] Configurando Tunel USB Reverso via ADB...
echo ========================================================

if not exist "%ADB_PATH%" (
    echo [ERRO] adb.exe nao encontrado em: %ADB_PATH%
    pause
    exit /b 1
)

echo [1/4] Verificando conexao com o Galaxy Tab S9 FE+...
"%ADB_PATH%" devices

echo.
echo [2/4] Configurando tunel reverso da porta de telemetria S-Pen (27183)...
"%ADB_PATH%" reverse tcp:27183 tcp:27183

echo.
echo [3/4] Configurando tunel reverso da porta de video H.264 (27184)...
"%ADB_PATH%" reverse tcp:27184 tcp:27184

echo.
echo [4/4] Configurando tunel reverso de telemetria bidirecional Blender (27185)...
"%ADB_PATH%" reverse tcp:27185 tcp:27185

echo.
echo ========================================================
echo [SUCESSO] Tuneis USB ativos com sucesso!
echo Qualquer pacote enviado para 127.0.0.1 no tablet chegara
echo instantaneamente ao servidor no PC via cabo USB.
echo ========================================================

endlocal

