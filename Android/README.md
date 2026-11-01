# Módulo Android do sTab (`Android`)

Esta pasta contém o projeto do aplicativo nativo em Kotlin/Android que roda diretamente no **Galaxy Tab S9 FE+**.

> **Objetivo:** Gerar o pacote `sTab.apk` que:
> 1. Faz a leitura do digitalizador Wacom EMR a 240 Hz com detecção de pressão (0-4096), inclinação (*Tilt X/Y*), aproximação (*Hover*) e botão físico lateral.
> 2. Separa com precisão os eventos de caneta (`TOOL_TYPE_STYLUS`) dos gestos de toque de dedo na tela (`TOOL_TYPE_FINGER`).
> 3. Bloqueia ativamente o menu *Air Command* da Samsung e os gestos de borda da One UI via `setSystemGestureExclusionRects`.
> 4. Renderiza a barra lateral ergonômica **SculptPad** para a mão esquerda (`Shift`, `Ctrl`, Sliders de Tamanho/Força do Pincel) aproveitando a margem da proporção 16:10.
> 5. Decodifica o vídeo H.264 acelerado por hardware a 90 FPS em uma `SurfaceView` de baixa latência (`KEY_LOW_LATENCY = 1`).
> 6. Envia o sinal reverso `REQUEST_IDR` para restauração de quadros em caso de pico de processamento no PC.

---

## Estrutura de Pastas

* **`app/src/main/java/com/stab/sculptbridge/`:**
  * `StylusMotionProcessor.kt`: Leitura e calibração da S-Pen.
  * `TouchGestureDetector.kt`: Detector de gestos de dedo (Orbit, Pan, Zoom, Undo, Redo).
  * `OneUiShield.kt`: Neutralização do Air Command e gestos do sistema.
  * `UsbSocketClient.kt`: Envio dos pacotes de 16 bytes via socket local USB `127.0.0.1:27183`.
  * `SculptPadLayout.kt`: Interface tátil lateral para a mão esquerda.
  * `VideoSurfaceView.kt`: Decodificador H.264 `MediaCodec` 90Hz.
* **`app/src/main/res/`:** Recursos visuais, ícones lineart e layouts.
* **`app/src/main/AndroidManifest.xml`:** Manifesto com trava em tela cheia paisagem 90Hz e modo imersivo.

