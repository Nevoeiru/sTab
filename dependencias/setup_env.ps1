# Script de carregamento de variáveis de ambiente do sTab
$Root = Split-Path -Parent $PSScriptRoot

$env:JAVA_HOME = "$Root\dependencias\jdk-17"
$env:ANDROID_HOME = "$Root\dependencias\android-sdk"
$env:ANDROID_SDK_ROOT = "$Root\dependencias\android-sdk"

$toolPaths = @(
    "$Root\dependencias\platform-tools",
    "$Root\dependencias\w64devkit\bin",
    "$Root\dependencias\jdk-17\bin",
    "$Root\dependencias\gradle-8.5\bin",
    "$Root\dependencias\android-sdk\cmdline-tools\latest\bin"
)

foreach ($p in $toolPaths) {
    if (Test-Path $p) {
        if ($env:PATH -notlike "*$p*") {
            $env:PATH = "$p;$env:PATH"
        }
    }
}

Write-Host "[sTab] Ambiente Portatil Ativado com Sucesso!" -ForegroundColor Green
Write-Host "  -> ADB:        $Root\dependencias\platform-tools\adb.exe"
Write-Host "  -> C++ (g++):  $Root\dependencias\w64devkit\bin\g++.exe"
Write-Host "  -> Java (JDK): $Root\dependencias\jdk-17\bin\javac.exe"
Write-Host "  -> Android SDK: $env:ANDROID_HOME"

