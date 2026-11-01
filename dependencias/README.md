# Pasta de Dependências e Ferramentas Portáteis (`dependencias`)

Esta pasta contém o ecossistema portátil de ferramentas de compilação, empacotamento e depuração para o desenvolvimento do **sTab (SculptBridge)** diretamente no Antigravity, sem necessidade de instalar dependências globais no Windows e sem exigir privilégios de administrador.

> **Aviso:** Esta pasta está listada no `.gitignore`. Nada aqui dentro será incluído no `.exe` ou no `.apk` final distribuído aos usuários.

---

## Estrutura de Ferramentas Portáteis

1. **`platform-tools/` (Google Android Platform-Tools):**
   * Contém o `adb.exe` (versão 37.0.1) para estabelecer o túnel reverso de rede via cabo USB (`adb reverse tcp:27183 tcp:27183`), instalar o APK no tablet e inspecionar logs em tempo real (`adb logcat`).

2. **`w64devkit/` (MinGW-w64 C++ & Build Tools):**
   * Contém `g++.exe` (GCC 16.2.0), `gcc.exe`, `make.exe` (GNU Make 4.4.1), `gdb.exe` e todas as bibliotecas de desenvolvimento Win32 necessárias (`windows.h`, `winsock2.h`, etc.).
   * Permite compilar os arquivos C++ da pasta `Windows/` gerando o executável final em segundos através de um simples comando `make` ou script `.bat`.

3. **`jdk-17/` (Eclipse Temurin OpenJDK 17):**
   * Contém `javac.exe`, `java.exe` e `jar.exe` portáteis, necessários para compilar o código do aplicativo Android e executar os utilitários do Android SDK.

4. **`android-sdk/` (Android Command-Line Tools & SDK):**
   * Contém `sdkmanager.bat` e ferramentas do Google para baixar e gerenciar o Android SDK (`platforms;android-34`, `build-tools`) e gerar o APK.

---

## Como Carregar as Ferramentas no Terminal

Para carregar todas essas ferramentas temporariamente no PATH do seu terminal:

* **No PowerShell:**
  ```powershell
  . .\dependencias\setup_env.ps1
  ```
* **No Prompt de Comando (CMD):**
  ```cmd
  call .\dependencias\setup_env.bat
  ```

