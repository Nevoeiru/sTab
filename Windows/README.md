# Módulo Windows do sTab (`Windows`)

Esta pasta contém o código-fonte em C++ nativo e as definições de compilação do servidor de desktop do **sTab (SculptBridge)**.

> **Objetivo:** Gerar o executável portátil `sTab_server.exe` que roda 100% em modo de usuário (User-Mode), sem instalação de drivers de kernel, responsável por:
> 1. Receber os pacotes binários de entrada via túnel USB (porta TCP `27183`).
> 2. Injetar a caneta S-Pen com 4096 níveis de pressão e inclinação via `CreateSyntheticPointerDevice` (Windows Ink oficial).
> 3. Traduzir o botão físico da S-Pen como clique direito (`RMB` - Brush Context Menu).
> 4. Traduzir os gestos de dedo na tela para controle da câmera 3D (`MMB` Orbit, `Shift+MMB` Pan, `Ctrl+MMB` Zoom, Undo/Redo).
> 5. Capturar a viewport do Blender via DirectX (`DXGI OutputDuplication`) e codificar em H.264 de baixa latência (porta TCP `27184`).

---

## Estrutura de Pastas

* **`include/`:** Cabeçalhos C++ (`.h`), incluindo o protocolo de comunicação binário compartilhado (`Protocol.h`).
* **`src/`:** Implementação modular dos subsistemas:
  * `PointerInjector.cpp`: Injeção de eventos `PT_PEN` (Windows Ink).
  * `GestureTranslator.cpp`: Tradução de gestos touch em eventos `SendInput` do Windows.
  * `TcpServer.cpp`: Servidor Winsock não-bloqueante (`TCP_NODELAY`).
  * `Main.cpp`: Ponto de entrada do executável.
* **`bin/`:** Destino do executável compilado (`sTab_server.exe`).

