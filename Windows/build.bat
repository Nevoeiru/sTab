@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "TOOLCHAIN_BIN=%SCRIPT_DIR%..\dependencias\w64devkit\bin"

if exist "%TOOLCHAIN_BIN%\g++.exe" (
    set "PATH=%TOOLCHAIN_BIN%;%PATH%"
)

echo ========================================================
echo   [sTab] Compilando Servidor Windows em C++ (Input + Video 90Hz)...
echo ========================================================

if not exist "%SCRIPT_DIR%bin" mkdir "%SCRIPT_DIR%bin"
if not exist "%SCRIPT_DIR%obj" mkdir "%SCRIPT_DIR%obj"

g++ -std=c++17 -O3 -Wall -Wextra -I"%SCRIPT_DIR%include" ^
    "%SCRIPT_DIR%src\Main.cpp" ^
    "%SCRIPT_DIR%src\PointerInjector.cpp" ^
    "%SCRIPT_DIR%src\GestureTranslator.cpp" ^
    "%SCRIPT_DIR%src\TcpServer.cpp" ^
    "%SCRIPT_DIR%src\DxgiCapture.cpp" ^
    "%SCRIPT_DIR%src\H264Encoder.cpp" ^
    "%SCRIPT_DIR%src\VideoServer.cpp" ^
    -o "%SCRIPT_DIR%bin\sTab_server.exe" ^
    -lws2_32 -luser32 -ld3d11 -ldxgi -lmfplat -lmfuuid -lole32 -loleaut32 -static -s

g++ -std=c++17 -O3 -Wall -Wextra -I"%SCRIPT_DIR%include" ^
    "%SCRIPT_DIR%src\sTab_launcher.cpp" ^
    -o "%SCRIPT_DIR%bin\sTab_launcher.exe" ^
    -static -s

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCESSO] Compilacao concluida com exito!
    echo Executavel gerado em: %SCRIPT_DIR%bin\sTab_server.exe
    echo.
    echo Executando autoteste de integridade:
    "%SCRIPT_DIR%bin\sTab_server.exe" --test
) else (
    echo.
    echo [ERRO] Falha na compilacao do sTab_server.exe!
    exit /b 1
)

endlocal
