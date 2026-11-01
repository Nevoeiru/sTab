# Planejamento Arquitetural e de Implementação: Galaxy Tab S9 FE+ como Pen Display para Blender Sculpt Mode

---

## 1. Análise de Hardware, Panorama e Arquitetura Zero-Driver

### 1.1. Especificações Críticas do Galaxy Tab S9 FE+ (`SM-X610` / `SM-X616`)
Para desenhar uma arquitetura estável e de baixíssima latência, o projeto respeita os pontos fortes e os limites físicos do hardware:
* **Tela:** 12,4 polegadas, resolução nativa de **2560 × 1600 pixels** (proporção **16:10**) a **90 Hz**.
* **S-Pen (Tecnologia Wacom EMR):** 4.096 níveis de pressão, detecção de inclinação (*Tilt X/Y*), aproximação sem bateria (*Hover* a ~15 mm do vidro) e botão lateral físico (*Barrel Button*). Digitalizador opera a taxa de amostragem de até **240 Hz**.
* **Processador e Decodificação:** Exynos 1380 com decodificador de vídeo por hardware (`MediaCodec` H.264 / HEVC de baixa latência).
* **Gargalo Físico Importante — Barramento USB-C 2.0:** Diferente da linha S9 padrão (com USB 3.2 e DisplayPort Alt Mode), a linha **S9 FE+ possui barramento USB 2.0**. Não há suporte a saída DisplayPort direta por hardware via cabo. A banda real de dados via cabo USB (túnel ADB / TCP) fica entre **280 e 340 Mbps** (~35–42 MB/s).
  * *Impacto Arquitetural:* Não podemos enviar frames sem compressão, mas **300 Mbps é mais de 6 vezes superior** ao que um fluxo H.264/HEVC acelerado por GPU (NVENC/AMF/QSV) precisa em 2560×1600 @ 90fps (que consome entre 25 e 40 Mbps com fidelidade visual perfeita para malhas 3D) somado à telemetria da S-Pen (< 150 Kbps).

---

### 1.2. Comparativo Direto: Nosso Sistema Customizado vs. SuperDisplay vs. Alternativas

| Critério | SuperDisplay | Sunshine + Moonlight | Nossa Arquitetura Customizada ("SculptBridge") | Vantagem no Blender Sculpt |
| :--- | :--- | :--- | :--- | :--- |
| **Instalação no Windows** | Exige driver de kernel (`.sys`) assinado e reinicialização. | Exige drivers de controle virtual / áudio. | **100% Modo de Usuário (User-Mode)**. Executável portátil (`.exe`), sem arquivos `.sys`, sem modo de teste do Windows. | **Nosso App** (Zero risco ao SO, sem poluição de drivers) |
| **Pressão e Inclinação da S-Pen** | Nativo via driver Wintab/Ink proprietário. | Básico (apenas pressão, tilt instável via RDP/GameStream). | Nativo via Windows Ink oficial (`CreateSyntheticPointerDevice` com 4096 níveis e Tilt X/Y). | **Empate com SuperDisplay** (Mesma fidelidade profissional) |
| **Ação do Botão da S-Pen** | Customizável via app, mas sujeito a atraso de driver. | Conflita com comandos de gamepad/mouse. | **Mapeado nativamente como Botão Direito do Mouse (RMB)**. | **Nosso App** (Abre o Brush Context Menu sob a ponta da caneta) |
| **Navegação 3D na Viewport** | Depende de atalhos de teclado ou emulação genérica de mouse. | Focado em gamepads; touch emula mouse comum. | **Gestos Touch Nativos de Dedo na Tela** (1 dedo = Orbit, 2 dedos = Pan, Pinça = Zoom) separados da S-Pen. | **Nosso App** (Experiência estilo Nomad Sculpt / ZBrush) |
| **Interface de Escultura na Tela** | Botões flutuantes genéricos sobrepostos na imagem 3D. | Sem interface auxiliar de atalhos. | **SculptPad fixo lateral** aproveitando a proporção da tela sem cobrir a viewport. | **Nosso App** (Polegar esquerdo controla Shift/Ctrl/F sem teclado) |
| **Tolerância a Falhas USB 2.0** | Tenta resoluções excessivas e pode sofrer buffering. | Requer configuração manual de bitrate. | **Canal de recuperação instantânea `REQUEST_IDR`** em caso de quadros perdidos. | **Nosso App** (Recupera frames corrompidos em 1 ms) |

---

### 1.3. A Filosofia Zero-Driver: Como Operamos 100% em User-Mode

Ao contrário de soluções que instalam drivers de sistema de baixo nível, a nossa arquitetura opera integralmente no espaço do usuário:

1. **Na Caneta (Zero Driver de Kernel):**
   * O SuperDisplay foi criado na era do Windows 7/8, quando o Windows não possuía uma API moderna para canetas de terceiros, exigindo um driver de kernel para emular uma mesa Wintab.
   * Nosso servidor utiliza a API nativa da Microsoft introduzida no Windows 10/11: `CreateSyntheticPointerDevice` configurada com `PT_PEN`. O Windows enxerga os pacotes do cabo USB como uma mesa digitalizadora legítima de alta precisão com suporte completo a pressão (0-4096), inclinação (-90° a +90°), hover e contato, que o Blender consome diretamente através de *Preferences > System > Tablet API = Windows Ink*.
2. **Na Tela (Captura Direta de VRAM via DXGI):**
   * O servidor lê os frames diretamente da memória de vídeo através da `Desktop Duplication API` (DirectX/DXGI) e comprime em tempo real via Media Foundation (NVENC / QuickSync / AMF), enviando os pacotes H.264 para o decodificador de hardware do tablet (`MediaCodec`).

---

### 1.4. Geometria de Exibição: Proporção 16:9 vs. 16:10 e o Uso Inteligente do "SculptPad"

O monitor físico do PC normalmente possui proporção **16:9** (ex.: 1920×1080 ou 2560×1440), enquanto a tela do Galaxy Tab S9 FE+ é **16:10** (2560×1600).
* Se a imagem do PC for esticada para preencher os 16:10 do tablet, uma esfera esculpida no Blender parecerá achatada como uma elipse.
* **Nossa Solução de Geometria:**
  1. **Modo Espelhamento do Monitor Principal (Padrão - Sem Drivers Adicionais):**
     * A viewport de vídeo do Blender no tablet é mantida estritamente na proporção nativa **16:9** (ex.: 2240×1260 ou 2304×1296).
     * O espaço restante de **160 a 256 pixels** na lateral esquerda (ou direita) da tela é preenchido pela barra lateral nativa de botões táteis (**SculptPad**).
     * Dessa forma, a malha 3D fica matematicamente proporcional (1:1), sem barras pretas desperdiçadas e sem sobrepor a interface de escultura sobre o modelo 3D.
  2. **Modo Segunda Tela Independente (Opcional):**
     * Caso você deseje utilizar o tablet como um monitor estendido independente (com resolução nativa de 2560×1600 a 90Hz) para deixar o monitor do PC livre para referências (PureRef) ou tutoriais, basta habilitar o driver de monitor virtual open-source gratuito (*Virtual-Display-Driver* via `IddSampleDriver`). Nosso software se conecta a ele sem precisar alterar nenhuma linha de código.

---

## 2. Arquitetura Técnica Detalhada

```
+-----------------------------------------------------------------------------------+
|                        GALAXY TAB S9 FE+ (2560x1600 @ 90Hz)                       |
|                                                                                   |
|  +-----------------------------+   +-------------------------------------------+  |
|  |   SculptPad Tátil Lateral   |   |   Área de Viewport 3D (Proporção 16:9)    |  |
|  |   (Sobra 16:10 ~ 180px)     |   |                                           |  |
|  | - Hold: Shift (Smooth)      |   | 1. SurfaceView (MediaCodec H.264 90fps)   |  |
|  | - Hold: Ctrl (Invert/Sub)   |   | 2. Detecção Touch Nativa:                 |  |
|  | - Slider Tátil: Raio (F)    |   |    - DEDOS NA TELA (TOOL_TYPE_FINGER):    |  |
|  | - Slider Tátil: Força (Sh+F)|   |      * 1 Dedo Drag -> Orbit 3D (MMB)       |  |
|  | - Atalhos: Remesh, Alt+Q    |   |      * 2 Dedos Drag -> Pan 3D (Shift+MMB)  |  |
|  +--------------+--------------+   |      * Pinça 2 Dedos -> Zoom (Ctrl+MMB)    |  |
|                 |                  |      * Toque 2 Dedos -> Undo (Ctrl+Z)      |  |
|                 |                  |      * Toque 3 Dedos -> Redo (Ctrl+Sh+Z)   |  |
|                 |                  |    - CANETA S-PEN (TOOL_TYPE_STYLUS):     |  |
|                 |                  |      * Ponta -> Esculpe com Pressão e Tilt|  |
|                 |                  |      * Botão Lateral -> Right Click (RMB) |  |
|                 |                  |        (Abre Brush Context Menu Blender)  |  |
|                 |                  +---------------------+---------------------+  |
+-----------------|----------------------------------------|------------------------+
                  |                                        |
                  +-------------------+--------------------+
                                      |
                        [ Cabo USB-C 2.0 via ADB Tunnel ]
                  +-------------------+--------------------+
                  | Canal 1: Input Binário (16 bytes/ev)   | Canal 2: Vídeo H.264
                  | + Canal Reverso: REQUEST_IDR (1 byte)  | 90 FPS Baixa Latência
                  | Latência de Entrada < 1.5 ms           | Taxa de Bits: 30-40 Mbps
                  v                                        ^
+-----------------------------------------------------------------------------------+
|                                  HOST WINDOWS 10/11                               |
|                                                                                   |
|  +-----------------------------------------+   +-------------------------------+  |
|  |     Receptor de Input (C++ Nativo)      |   | Captura & Encoder (DXGI/MF)   |  |
|  |                                         |   |                               |  |
|  | - Pacote 0x01 (S-Pen):                  |   | - IDXGIOutputDuplication      |  |
|  |   * Ponta -> InjectSyntheticPointer     |   |   (Zero-Copy da GPU)          |  |
|  |     (Windows Ink: Pressão, Tilt, Hover) |   | - Media Foundation H.264      |  |
|  |   * Botão Lateral -> Injeta RMB Click   |   |   (CBR, Low-Latency, Zero B)  |  |
|  | - Pacote 0x02 (Gestos Touch de Dedo):   |   | - Listener de REQUEST_IDR:    |  |
|  |   * SendInput MMB, Shift+MMB, Ctrl+MMB  |   |   Força chave IDR na hora     |  |
|  | - Pacote 0x03 (Dock SculptPad):         |   +-------------------------------+  |
|  |   * SendInput Modificadores e Atalhos   |                                      |
|  +--------------------+--------------------+                                      |
|                       |                                                           |
|                       v                                                           |
|  +-----------------------------------------------------------------------------+  |
|  |                     BLENDER 4.x / 5.x (SCULPT MODE)                         |  |
|  |        (Configurado com Tablet API = Windows Ink + Emulate 3 Button)        |  |
|  +-----------------------------------------------------------------------------+  |
+-----------------------------------------------------------------------------------+
```

---

### 2.1. O Protocolo Binário de Entrada (16 Bytes) e Canal de Controle

Cada evento de entrada gerado no tablet é transmitido em uma `struct` binária de **16 bytes fixos** (`#pragma pack(push, 1)`), mantendo o consumo de banda abaixo de 0,1% da capacidade do USB 2.0:

* `uint8_t packetType`:
  * `0x01` = Evento S-Pen (`PT_PEN` / Windows Ink).
  * `0x02` = Evento de Navegação Touch por Dedos (`TOOL_TYPE_FINGER`: Orbit, Pan, Zoom, Undo, Redo).
  * `0x03` = Modificador da barra lateral SculptPad (`Shift`, `Ctrl`, `Alt`, `F`, etc.).
  * `0x04` = Comando de Controle de Vídeo (enviado do tablet ao PC: `REQUEST_IDR`).
* `uint8_t flags`:
  * `0x01` = S-Pen em Hover (`POINTER_FLAG_INRANGE`).
  * `0x02` = S-Pen em Contato (`POINTER_FLAG_INCONTACT`).
  * `0x04` = **Botão Lateral da S-Pen Pressionado (`RMB_ACTIVE`)**.
  * `0x08` = Ponta Invertida / Borracha.
  * `0x10` = Gesto de Dedo Ativo na Viewport.
* `uint16_t x`: Posição X normalizada (`0` a `65535`) mapeada na viewport do Blender.
* `uint16_t y`: Posição Y normalizada (`0` a `65535`).
* `uint16_t pressure`: Pressão normalizada da S-Pen (`0` a `4096`).
* `int16_t tiltX`: Inclinação horizontal (-90° a +90°).
* `int16_t tiltY`: Inclinação vertical (-90° a +90°).
* `uint32_t payload`: Código de tecla Virtual (`VK_SHIFT`, `VK_CONTROL`), delta de deslocamento do gesto `(dx: int16, dy: int16)` ou identificador de comando.

---

### 2.2. Blindagem do Sistema Android: Bloqueio do Air Command e Gestos da One UI

No Galaxy Tab S9 FE+, dois comportamentos nativos da Samsung interferem na escultura profissional se não forem tratados explicitamente:

1. **Bloqueio do Menu Suspenso *Air Command* (Botão da S-Pen):**
   * Por padrão, pressionar o botão da S-Pen com ela próxima à tela faz a interface One UI da Samsung abrir o menu circular *Air Command*, roubando o foco do app.
   * **Tratamento:** O app intercepta os eventos em nível baixo nos métodos `dispatchKeyEvent`, `onKeyDown` e `dispatchGenericMotionEvent` com o código `KeyEvent.KEYCODE_STYLUS_BUTTON_TAIL` / `BUTTON_STYLUS_PRIMARY` e retorna `true` imediatamente (consumindo o evento antes que o serviço da Samsung o receba). O estado do botão é enviado diretamente para o servidor PC como **Botão Direito do Mouse (RMB)**.
2. **Neutralização dos Gestos de Borda do Android (Gesto "Voltar" da One UI):**
   * Ao esculpir perto das extremidades ou usar a barra lateral SculptPad, deslizar os dedos pode acionar o gesto do Android de voltar para a tela inicial.
   * **Tratamento:** Aplicação da API oficial do Android 10+ `View.setSystemGestureExclusionRects(rects)` em toda a área de tela do app, além de rodar em modo imersivo persistente (`WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE`).

---

### 2.3. Resiliência de Vídeo: Canal de Recuperação Instantânea `REQUEST_IDR`

Em transmissões de vídeo via USB 2.0, operações de alto custo de CPU no PC (como executar um *Voxel Remesh* pesado ou aplicar um modificador multirresolução) podem atrasar momentaneamente o envio de pacotes. Em fluxos H.264 sem frames B, a perda de um único frame diferencial (P-Frame) provocaria artefatos visuais ou congelamento até o próximo frame-chave (GOP).
* **Solução:** O decodificador `MediaCodec` do Android monitora a continuidade dos timestamps de exibição. Se detectar pacote corrompido ou descarte de frames, envia imediatamente um pacote de 1 byte `REQUEST_IDR` pelo canal de socket reverso.
* O codificador no PC recebe o sinal e força a placa de vídeo a emitir um frame I (IDR Frame) completo no milissegundo seguinte, restaurando a nitidez sem que o usuário perceba qualquer travamento.

---

## 3. Design de UX e Ergonomia de Escultura

### 3.1. Layout Ergonômico de Duas Mãos

A disposição foi concebida para que o usuário trabalhe exatamente como em uma mesa profissional com teclado, porém de forma 100% tátil e portátil:
* **Mão Direita (S-Pen):** Dedicada exclusivamente à precisão do traço, pressão, inclinação e acionamento do menu de contexto de pincel via botão lateral (RMB).
* **Mão Esquerda (Polegar no SculptPad):** Descansa na lateral da tela operando atalhos contínuos de modificação (`Shift` para suavizar, `Ctrl` para inverter/escavar, sliders de tamanho/força) sem que você precise olhar para a mão.
* **Dedos na Viewport (Navegação Espacial Direta):** Movimentam a câmera 3D de maneira intuitiva e sem atritos, exatamente como no Nomad Sculpt ou Procreate.

---

### 3.2. Mapeamento do Botão da S-Pen: Botão Direito do Mouse (RMB)

No Blender Sculpt Mode, o **Botão Direito do Mouse (RMB)** é um dos comandos mais poderosos: ele aciona instantaneamente o menu de contexto do pincel ativo (*Brush Context Menu* / *Sculpt Quick Menu*), permitindo alterar raio, força, autosmooth e formato da curva diretamente sob a ponta da caneta.

* **Comportamento Mapeado:**
  * Pressionar o botão físico da S-Pen durante o *Hover* ou contato gera no Windows um evento de **Right Click (RMB)** na posição exata da caneta.
  * O menu de contexto do Blender se abre instantaneamente no local desejado.
  * Soltar o botão ou tocar na tela com a caneta escolhe o parâmetro e fecha o menu com fluidez máxima.

---

### 3.3. Ações de Dedo Nativas na Tela (Navegação Touch 3D)

O digitalizador do Galaxy Tab S9 FE+ distingue com precisão de hardware o contato da caneta (`TOOL_TYPE_STYLUS`) do contato dos dedos (`TOOL_TYPE_FINGER`). Aproveitamos essa distinção para criar uma navegação de câmera 3D natural:

1. **Quando a S-Pen não está em contato nem em hover imediato:**
   * **Arrastar com 1 Dedo na Viewport:** O servidor traduz o arrasto para **Botão do Meio do Mouse (`MMB` + Drag)** -> **Orbitar Câmera 3D**.
   * **Arrastar com 2 Dedos paralelos:** O servidor traduz para **`Shift` + `MMB` + Drag** -> **Pan (Mover Câmera 3D lateralmente/verticalmente)**.
   * **Gesto de Pinça com 2 Dedos (Pinch-to-Zoom):** A aproximação ou afastamento dos dedos é calculada como delta de distância e traduzida para **`Ctrl` + `MMB` + Drag** (ou `Wheel Zoom`) -> **Zoom Fluido e Contínuo na Viewport**.
   * **Toque Rápido com 2 Dedos (Tap):** Dispara `Ctrl + Z` -> **Desfazer (Undo)** imediato da última pincelada.
   * **Toque Rápido com 3 Dedos (Tap):** Dispara `Ctrl + Shift + Z` -> **Refazer (Redo)**.
2. **Proteção Total contra Toques Acidentais (Hardware Palm Rejection):**
   * No milissegundo em que a ponta da S-Pen se aproxima a ~15 mm da tela (`ACTION_HOVER_ENTER`), a viewport 3D **bloqueia instantaneamente qualquer evento de dedo ou palma da mão**, evitando marcas ou rotações acidentais na malha durante o desenho.
   * Apenas a barra lateral **SculptPad** permanece responsiva aos toques da mão esquerda enquanto a S-Pen esculpe.

---

### 3.4. Anatomia da Barra Lateral "SculptPad" (Mão Esquerda)

Localizada na faixa de folga de resolução (160–180 px de largura):
1. **Botões de Retenção Momentânea (*Hold-to-Modify*):**
   * **Botão Gigante `SHIFT` (Smooth):** Mantido pressionado com o polegar esquerdo, ativa o modo de suavização da malha enquanto a S-Pen risca. Ao soltar o polegar, o pincel volta ao original sem nenhuma troca de ferramenta necessária.
   * **Botão Gigante `CTRL` (Invert / Sub):** Inverte a ação da ferramenta (ex.: escava com o *Clay Strips* em vez de acumular).
   * **Zona Combinada `SHIFT + CTRL`:** Localizada exatamente na interseção entre os dois botões, permitindo pressionar ambos com o mesmo polegar para ferramentas de recorte e máscara (*Lasso Trim*, *Box Mask*).
2. **Sliders Verticais Táteis:**
   * **Slider A (Tamanho do Raio - `F`):** Arrastar o polegar para cima/baixo ajusta o raio do pincel interativamente, exibindo um anel visual proporcional no tablet.
   * **Slider B (Força - `Shift + F`):** Ajusta a intensidade da pincelada.
3. **Gatilhos Rápidos de Produção:**
   * `Voxel Remesh (Ctrl + R)` e `Tamanho do Grid (R)`.
   * `Alt + Q` (Modo de Transferência para alternar instantaneamente entre objetos/subtools no modo de escultura).
   * `Alt + M` (Limpar Máscara) / `Ctrl + I` (Inverter Máscara).
   * Botão de alternância instantânea entre **Modo Destro e Canhoto** (move o SculptPad para a borda direita).

---

## 4. Análise Técnica: É Necessário um Add-on para o Blender?

Uma dúvida central de arquitetura: **Precisamos desenvolver um add-on em Python para o Blender rodar esse ecossistema, ou do jeito que está já funciona perfeitamente?**

### 4.1. Diagnóstico: O Sistema Funciona 100% de Forma Autônoma (Sem Add-on)
A resposta direta é: **Para o funcionamento de escultura, sensibilidade de pressão, inclinação e navegação de câmera, NENHUM add-on é estritamente obrigatório.**

* **Por que funciona sem add-on?**
  * O Blender possui suporte nativo e maduro à API **Windows Ink** desde as versões 2.8x. Ao injetarmos eventos através de `CreateSyntheticPointerDevice` (`PT_PEN`), o Blender interpreta a S-Pen exatamente como uma mesa Wacom ou Huion oficial conectada à porta USB do Windows.
  * Os atalhos de escultura (`Shift`, `Ctrl`, `F`, `Shift+F`, `Ctrl+R`, `Alt+Q`, `MMB` para orbit, `Shift+MMB` para pan e `RMB` para brush menu) são comandos universais nativos do *keymap* padrão do Blender. O `SendInput` no Windows entrega esses comandos diretamente para a janela ativa.

---

### 4.2. Onde um Add-on Leve Agrega Valor Real (Acelerador Opcional de Workflow)
Embora não seja obrigatório para o core do sistema, um **Add-on leve em Python (`sTab Companion`)** traz vantagens práticas consideráveis se implementado como um utilitário complementar:

1. **Configuração de 1 Clique (*Zero Friction Setup*):**
   * Em vez de o usuário precisar navegar manualmente pelas preferências do Blender (*Edit > Preferences > System > Tablet API > Windows Ink*, configurar *Emulate 3 Button Mouse*, ajustar limites de zoom e curvas de sensibilidade), o add-on adiciona um painel na barra lateral (`N-Panel`) com um botão único: **"Otimizar Blender para Galaxy Tab S9 FE+"**. Ele aplica todos os ajustes ideais automaticamente.
2. **Telemetria Bidirecional em Tempo Real (Feedback Visual Rico):**
   * Sem o add-on, o fluxo é estritamente unidirecional: o tablet envia comandos e o PC executa. O tablet não sabe qual pincel está selecionado nem o valor numérico exato do raio.
   * Com o add-on rodando um servidor UDP local minúsculo em segundo plano (consumo irrisório de CPU), o Blender pode avisar ao app do tablet qual pincel está ativo (*Clay Strips*, *Draw*, *Smooth*), o valor exato do raio em pixels e se a simetria está ligada, permitindo que a interface do tablet mostre o ícone do brush ativo e o raio numérico exato.
3. **Imunidade de Pincelada Durante Navegação Rápida:**
   * Em situações extremas de pintura rápida onde o usuário toca o dedo para girar a câmera uma fração de segundo antes de levantar a caneta, o add-on pode forçar a trava momentânea do operador de *Stroke*, garantindo 100% de certeza de que nenhum micro-risco ocorra na geometria 3D.

---

### 4.3. Veredito Arquitetural: Estratégia Híbrida Desacoplada
Adotaremos a **estratégia híbrida desacoplada**:
* **O Núcleo (Etapas 1 a 4) é 100% autônomo:** Você pode abrir qualquer versão do Blender (4.0, 4.2, 5.0), ou até outros softwares como ZBrush, Substance 3D Painter ou Photoshop, e a S-Pen com pressão, botão direito (RMB), SculptPad e navegação por toque funcionarão imediatamente sem depender de nenhum plugin instalado no software hospedeiro.
* **O Add-on (Etapa 5) entra como um módulo facilitador opcional:** Um arquivo Python simples (`sTab_blender_companion.py`) que configura o ambiente em 1 clique e habilita a telemetria opcional para quem deseja a experiência mais refinada possível.

---

## 5. Planejamento de Implementação em 5 Etapas Modulares (Sprints)

O projeto é estruturado em **5 etapas modulares**, garantindo que cada fase gere código conciso, testável individualmente e dentro dos padrões de engenharia:

---

### Etapa 1: Servidor Windows — Motor de Injeção S-Pen (Windows Ink) + Navegação e Câmera
* **Objetivo:** Criar o servidor em C++ nativo (Win32) que recebe os pacotes de 16 bytes via túnel USB (ADB), injeta a S-Pen via Windows Ink com pressão/tilt, injeta o botão da caneta como RMB e traduz os gestos touch de dedo em navegação 3D (`MMB`, `Shift+MMB`, `Ctrl+MMB`).
* **Módulos a Desenvolver:**
  * `Protocol.h`: Definição binária compacta dos pacotes de entrada (`0x01` a `0x04`) e flags.
  * `PointerInjector.h / .cpp`: Inicialização da API nativa `CreateSyntheticPointerDevice(PT_PEN)` com suporte a pressão (0-4096), inclinação X/Y, hover, contato e injeção do botão como **Right Mouse Button (`RMB`)**.
  * `GestureTranslator.h / .cpp`: Mapeamento de gestos touch em `SendInput` para rotação de câmera (`MMB`), pan e zoom suave.
  * `TcpServer.h / .cpp`: Servidor Winsock não-bloqueante escutando na porta `27183` com `TCP_NODELAY`.
  * `setup_usb_tunnel.bat`: Script de inicialização automática do túnel `adb reverse tcp:27183 tcp:27183`.
* **Resultado:** Servidor compilável com MSVC ou MinGW sem bibliotecas externas pesadas.

---

### Etapa 2: Cliente Android — Digitalizador S-Pen 240Hz, Gestos Touch e Blindagem One UI
* **Objetivo:** Desenvolver o núcleo Android em Kotlin com amostragem de alta frequência da S-Pen, gestos de dedo na tela para controle de câmera, bloqueio do Air Command e neutralização dos gestos de borda da Samsung.
* **Módulos a Desenvolver:**
  * `AndroidManifest.xml`: Configurações de tela cheia imersiva a 90Hz e landscape travado.
  * `UsbSocketClient.kt`: Gerenciador de conexão persistente com o PC via socket local `127.0.0.1:27183` com fila de alta prioridade.
  * `OneUiShield.kt`: Módulo de proteção do sistema que aplica `setSystemGestureExclusionRects` e consome os eventos físicos do botão da S-Pen (`BUTTON_STYLUS_PRIMARY`), impedindo a abertura do menu Air Command da Samsung.
  * `TouchGestureDetector.kt`: Processador de gestos de dedo na viewport (`TOOL_TYPE_FINGER`), identificando 1 dedo (Orbit), 2 dedos (Pan), pinça (Zoom) e toques de 2/3 dedos (Undo/Redo).
  * `StylusMotionProcessor.kt`: Leitor de sub-frames do digitalizador Wacom (`event.historySize`), conversão de coordenadas e cálculo de `tiltX`/`tiltY` a partir dos ângulos de orientação do Android.
* **Resultado:** Conexão funcional entre tablet e PC, permitindo esculpir no Blender com pressão, tilt, botão direito e girar a câmera usando os dedos na tela.

---

### Etapa 3: Interface Android — Barra Lateral "SculptPad" e Modificadores
* **Objetivo:** Construir a interface visual tátil para o polegar esquerdo, eliminando a dependência do teclado físico.
* **Módulos a Desenvolver:**
  * `SculptPadLayout.kt`: Painel lateral tátil posicionado na margem excedente de proporção da tela.
  * `HoldModifierButton.kt`: Componente modular para botões momentâneos (`Shift`, `Ctrl`, `Shift+Ctrl`, `Alt`) com multitouch independente.
  * `TouchSliderView.kt`: Controle deslizante vertical para tamanho de pincel (`F`) e força (`Shift+F`), desenhando visualização dinâmica do diâmetro na tela.
  * `QuickActionButton.kt`: Botões de disparo instantâneo para *Voxel Remesh*, seleção de subtool (`Alt+Q`), Undo e alternância Destro/Canhoto.
* **Resultado:** Interface de controle completa e ergonômica rodando a 90 FPS no tablet.

---

### Etapa 4: Pipeline de Vídeo de Baixa Latência (DXGI -> H.264 -> MediaCodec 90Hz) com Recuperação IDR
* **Objetivo:** Transmitir a tela do Blender da memória de vídeo do PC para o display do Galaxy Tab S9 FE+ a 90 FPS via cabo USB 2.0 com latência imperceptível e recuperação instantânea contra perdas.
* **Módulos a Desenvolver:**
  * `DxgiCapture.h / .cpp`: Captura direta de frames da VRAM via `IDXGIOutputDuplication`.
  * `H264Encoder.h / .cpp`: Codificador Media Foundation por hardware (NVENC/AMF/QSV) em modo de ultrabaixa latência (CBR 35 Mbps, zero B-frames) e suporte a injeção forçada de frame IDR sob demanda.
  * `VideoServer.h / .cpp`: Transmissão de pacotes de vídeo NAL na porta `27184` e receptor do sinal de 1 byte `REQUEST_IDR`.
  * `VideoSurfaceView.kt`: Visualizador Android utilizando `MediaCodec` configurado com `KEY_LOW_LATENCY = 1` conectado diretamente à `SurfaceView` do tablet.
* **Resultado:** Display de vídeo nítido e fluido a 90 FPS no tablet com recuperação de quadros em 1 ms.

---

### Etapa 5: Launcher Integrado e Add-on Opcional de Calibração para o Blender
* **Objetivo:** Fornecer facilidade de uso em um clique com launcher para Windows e o plugin companion opcional para o Blender.
* **Módulos a Desenvolver:**
  * `sTab_launcher.exe` / `run_stab.bat`: Utilitário que detecta a conexão do tablet, configura os túneis ADB e inicia os servidores em segundo plano.
  * `blender_sTab_companion.py` (Add-on Opcional):
    * Botão de 1 clique no N-Panel do Blender para configurar a *Tablet API* para Windows Ink, sensibilidade e atalhos otimizados.
    * Servidor local leve de telemetria UDP (envia nome do brush ativo, raio e estado da simetria para exibição na UI do tablet).
  * Guia de uso e verificação de calibração para o usuário.
* **Resultado:** Ecossistema finalizado, pronto para uso profissional no Blender.
