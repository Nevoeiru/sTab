@echo off
set "STAB_ROOT=%~dp0.."

set "JAVA_HOME=%STAB_ROOT%\dependencias\jdk-17"
set "ANDROID_HOME=%STAB_ROOT%\dependencias\android-sdk"
set "ANDROID_SDK_ROOT=%STAB_ROOT%\dependencias\android-sdk"

set "PATH=%STAB_ROOT%\dependencias\platform-tools;%STAB_ROOT%\dependencias\w64devkit\bin;%STAB_ROOT%\dependencias\jdk-17\bin;%STAB_ROOT%\dependencias\android-sdk\cmdline-tools\latest\bin;%PATH%"

echo ========================================================
echo   [sTab] Ambiente Portatil Ativado!
echo   - ADB:         %STAB_ROOT%\dependencias\platform-tools\adb.exe
echo   - C++ (g++):   %STAB_ROOT%\dependencias\w64devkit\bin\g++.exe
echo   - Java (JDK):  %STAB_ROOT%\dependencias\jdk-17\bin\javac.exe
echo   - Android SDK: %ANDROID_HOME%
echo ========================================================

