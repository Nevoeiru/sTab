@echo off
setlocal enabledelayedexpansion

set "ROOT_DIR=%~dp0"
set "ADB_PATH=%ROOT_DIR%dependencias\platform-tools\adb.exe"
set "SERVER_EXE=%ROOT_DIR%Windows\bin\sTab_server.exe"
set "APK_FILE=%ROOT_DIR%Android\app\build\outputs\apk\debug\app-debug.apk"

title sTab (SculptBridge) - Galaxy Tab S9 FE+ no Blender

echo ====================================================================
echo      sTab (SculptBridge) - Sistema Integrado para o Blender 3D
echo         Transformando Galaxy Tab S9 FE+ em Mesa Digitalizadora
echo ====================================================================
echo.

:: 1. Verifica ADB
if not exist "%ADB_PATH%" (
    echo [ERRO] ADB nao encontrado em: %ADB_PATH%
    echo Certifique-se de que a pasta dependencias foi configurada corretamente.
    pause
    exit /b 1
)

:: 2. Verifica se o executável do servidor existe; caso contrário, compila
if not exist "%SERVER_EXE%" (
    echo [AVISO] Servidor Windows nao encontrado. Compilando automaticamente...
    call "%ROOT_DIR%Windows\build.bat"
    if !ERRORLEVEL! neq 0 (
        echo [ERRO] Falha ao compilar sTab_server.exe.
        pause
        exit /b 1
    )
)

:: 3. Verifica conexao com o Galaxy Tab S9 FE+
echo [1/4] Verificando conexao USB com o Galaxy Tab S9 FE+...
for /f "skip=1 tokens=1,2" %%A in ('"%ADB_PATH%" devices') do (
    if "%%B"=="device" (
        set "DEVICE_FOUND=1"
        set "DEVICE_SERIAL=%%A"
    )
)

if "%DEVICE_FOUND%" neq "1" (
    echo.
    echo [AVISO] Nenhum tablet detectado via cabo USB!
    echo   1. Conecte o cabo USB-C entre o PC e o Galaxy Tab S9 FE+.
    echo   2. Verifique se a 'Depuracao USB' esta ativada nas Opcoes do Desenvolvedor.
    echo   3. Confirme a permissao de depuracao na tela do tablet.
    echo.
    echo Aguardando conexao do tablet...
    "%ADB_PATH%" wait-for-device
    echo [OK] Tablet conectado via USB com sucesso!
) else (
    echo [OK] Tablet detectado: %DEVICE_SERIAL%
)

:: 4. Configura os tuneis reversos USB
echo.
echo [2/4] Configurando tuneis USB reversos de ultra-baixa latencia...
"%ADB_PATH%" reverse tcp:27183 tcp:27183
if %ERRORLEVEL% equ 0 (
    echo   - Porta 27183 (Input S-Pen 240Hz, Multitouch e SculptPad): ATIVA
) else (
    echo   [ERRO] Falha ao abrir tunel reverso na porta 27183.
)

"%ADB_PATH%" reverse tcp:27184 tcp:27184
if %ERRORLEVEL% equ 0 (
    echo   - Porta 27184 (Stream de Video H.264 a 90 FPS GPU): ATIVA
) else (
    echo   [ERRO] Falha ao abrir tunel reverso na porta 27184.
)

:: 5. Verifica se o APK esta instalado ou precisa ser atualizado
echo.
echo [3/4] Verificando aplicativo sTab no tablet...
if exist "%APK_FILE%" (
    "%ADB_PATH%" shell pm list packages | findstr "com.stab.sculptbridge" >nul
    if !ERRORLEVEL! neq 0 (
        echo   Aplicativo ainda nao instalado no tablet. Instalando APK...
        "%ADB_PATH%" install -r "%APK_FILE%"
        if !ERRORLEVEL! equ 0 (
            echo   [OK] Aplicativo sTab instalado com sucesso no tablet!
        )
    ) else (
        echo   [OK] Aplicativo sTab ja instalado no tablet.
    )
    
    :: Inicia o app automaticamente no tablet se desejado
    echo   Iniciando app sTab no Galaxy Tab S9 FE+...
    "%ADB_PATH%" shell am start -n com.stab.sculptbridge/.MainActivity >nul 2>&1
)

:: 6. Inicia o Servidor Windows
echo.
echo [4/4] Iniciando Servidor Windows (Input Injector + DXGI H.264 Encoder)...
echo ====================================================================
echo   Pressione Ctrl+C nesta janela a qualquer momento para encerrar.
echo ====================================================================
echo.

"%SERVER_EXE%"

echo.
echo [sTab] Servidor finalizado.
endlocal

