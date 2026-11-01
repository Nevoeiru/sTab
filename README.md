# Guia de Uso, Calibração e Operação — sTab v2.0 (SculptBridge)
**Transforme o Samsung Galaxy Tab S9 FE+ em uma Mesa Digitalizadora Profissional para o Blender 3D**

---

## 1. Visão Geral da Arquitetura (v2.0)

O **sTab** é uma solução completa de ultra-baixa latência que conecta o hardware do Galaxy Tab S9 FE+ diretamente ao Windows 11 e ao Blender, sem depender de drivers proprietários em modo kernel:
* **Digitalizador Wacom EMR a 240Hz:** Amostragem sub-frame nativa com 4096 níveis de pressão e inclinação cartesiana calibrada analiticamente ($tiltX$ e $tiltY$).
* **Injeção Nativa via Windows Ink (Zero-Delay):** Utiliza a API `CreateSyntheticPointerDevice(PT_PEN)` com supressão imediata de *Press-and-Hold* (`TABLET_DISABLE_PRESSANDHOLD` via `SetPropW`), eliminando o atraso de 150–250 ms no ataque de pinceladas.
* **Per-Monitor DPI Awareness v2:** Alinhamento 1:1 de coordenadas físicas absolutas sem desvio espacial em telas High-DPI (125%, 150%, 200%).
* **Trava Modal de Navegação 3D:** Exclusão mútua no servidor entre navegação de câmera (`MMB`, `Shift + MMB`) e o ponteiro sintético, impedindo que o `Shift` de Pan acione acidentalmente o *Smooth Brush*.
* **Streaming de Vídeo GPU a 90 FPS com Cadência Estável:** Captura direta na VRAM via DXGI Desktop Duplication com retenção de textura em cenas estáticas (evita *stall* do `MediaCodec`), codificação H.264 (CBR 35 Mbps, zero B-frames) e canal reverso `REQUEST_IDR`.
* **Suporte a Monitor Virtual 16:10 Nativo (2560 × 1600 @ 90Hz):** Integração com o Virtual Display Driver (VDD) comunitário de código aberto pré-assinado para proporção exata sem faixas pretas.
* **Ergonomia de Duas Mãos & Blindagem One UI:** Viewport de desenho para a mão direita e painel tátil **SculptPad** para a mão esquerda com margem física de 16 dp e quota estrita de 200 dp na exclusão de gestos do Android.

---

## 2. Preparação do Tablet (Executada Apenas uma Vez)

1. **Ativar Opções do Desenvolvedor no Tablet:**
   * No Galaxy Tab S9 FE+, vá em **Configurações > Sobre o tablet > Informações do software**.
   * Toque 7 vezes consecutivas em **Número da versão** até surgir a mensagem *"O modo de desenvolvedor foi ativado"*.
2. **Ativar a Depuração USB:**
   * Retorne para **Configurações > Opções do desenvolvedor**.
   * Ative a opção **Depuração USB**.
3. **Conectar o Cabo USB:**
   * Conecte o tablet ao PC usando um cabo USB-C de qualidade.
   * Na primeira conexão, o tablet exibirá uma caixa de diálogo: marque a opção *"Sempre permitir a partir deste computador"* e clique em **Permitir**.
4. **Instalar o Aplicativo no Tablet:**
   * Dê dois cliques em `Android\install_apk.bat` (ou execute `run_stab.bat`, que realiza a checagem e instalação automática se necessário).

---

## 3. Monitor Virtual 16:10 Nativo (Opcional, mas Recomendado)

Para usar o Galaxy Tab S9 FE+ como tela secundária estendida em resolução nativa de **2560 × 1600 a 90 Hz** sem faixas pretas:
* Siga o guia passo a passo em [`VirtualDisplay/INSTALACAO_VDD.md`](file:///j:/sTab/VirtualDisplay/INSTALACAO_VDD.md).
* O arquivo de perfil calibrado já se encontra pronto em [`VirtualDisplay/options.txt`](file:///j:/sTab/VirtualDisplay/options.txt).
* O capturador do sTab detectará o monitor 16:10 automaticamente na inicialização.

---

## 4. Inicialização Rápida no Windows (1 Clique)

Na pasta raiz do projeto:
* Dê dois cliques em **`run_stab.bat`** (ou execute `Windows\bin\sTab_launcher.exe`).

O inicializador executará os seguintes passos automaticamente:
1. Detecta o Galaxy Tab S9 FE+ conectado via cabo USB.
2. Abre os túneis USB reversos de ultra-baixa latência:
   - **Porta 27183:** Canal de telemetria da S-Pen 240Hz, gestos touch e comandos do SculptPad.
   - **Porta 27184:** Canal de transmissão de vídeo H.264 a 90 FPS.
3. Detecta se o monitor virtual 2560x1600 está ativo no sistema.
4. Inicia o aplicativo `sTab` no tablet via ADB Intent.
5. Inicializa o servidor `sTab_server.exe` no Windows.

---

## 5. Add-on Opcional para o Blender (`sTab Companion`)

Embora o sistema funcione 100% de forma autônoma (driverless), o add-on fornece configuração ideal em um clique e telemetria:

1. No Blender, vá em **Edit > Preferences > Add-ons**.
2. Clique no botão **Install...** (ou na seta ao lado no Blender 4.2+) e selecione o arquivo:
   `j:\sTab\Blender\sTab_blender_companion.py`
3. Ative a caixa de seleção ao lado de **3D View: sTab Sculpt Companion**.
4. Na **3D Viewport**, abra a barra lateral pressionando a tecla **`N`** e clique na aba **`sTab`**:
   - **"Otimizar Blender para sTab":** Ajusta a *Tablet API* para Windows Ink, navegação suave e órbita *Turntable*.
   - **"Visualização Argila 3D":** Ativa Matcap de argila vermelha e relevo de cavidades para escultura.
   - **"Alinhar Formato 16:10":** Configura a viewport e render na proporção nativa de 2560×1600 do tablet.
   - **Paleta de Pincéis e Telemetria UDP:** Acesso instantâneo a ferramentas de escultura e transmissão de dados em tempo real.

---

## 6. Mapeamento de Ergonomia e Controles

### 6.1. Mão Direita: S-Pen
* **Ponta da Caneta:** Desenho com sensibilidade contínua de pressão (0..4096) e inclinação polar-cartesiana calibrada X/Y.
* **Ataque Imediato:** Supressão Win32 de *Press-and-Hold* ativa — traços rápidos de escultura sem atraso inicial.
* **Botão Físico da S-Pen:** Mapeado diretamente como **Botão Direito do Mouse (`RMB`)**:
  - Pressionar o botão durante o hover abre instantaneamente o menu de contexto do pincel (*Brush Quick Menu*) sob a ponta da caneta.
  - O menu circular *Air Command* da Samsung é interceptado e neutralizado pela blindagem de baixo nível do sTab.

### 6.2. Mão Direita / Dedos na Viewport: Navegação 3D Direta
* **1 Dedo arrastando na Viewport:** Orbitar Câmera 3D (`MMB`).
* **2 Dedos paralelos arrastando:** Mover Câmera lateralmente (`Shift + MMB` / Pan).
* **Pinça com 2 Dedos (Pinch-to-Zoom):** Zoom suave e contínuo.
* **Dois toques rápidos (2 Taps):** Desfazer última pincelada (`Ctrl + Z`).
* **Três toques rápidos (3 Taps):** Refazer (`Ctrl + Shift + Z`).
* **Hardware Palm Rejection & Trava Modal:** Enquanto a caneta estiver no raio de ~15 mm da tela (`Hover`), os toques na viewport são suspensos. Enquanto houver navegação de câmera por toque, a injeção da S-Pen é travada para evitar deformações acidentais na malha.

### 6.3. Mão Esquerda: Barra Lateral "SculptPad"
* **Margem Física de 16 dp:** Proteção física contra disparo acidental do gesto "Voltar" do Android nas bordas da tela.
* **Exclusão de Gestos Focada (200 dp):** Quota estrita do Android 10+ concentrada nos sliders verticais.
* **Botão `SHIFT` (Smooth):** Mantenha pressionado com o polegar esquerdo para suavizar a malha instantaneamente.
* **Botão `CTRL` (Invert / Sub):** Inverte a ação do pincel (escava relevo em vez de acumular).
* **Zona de Contato Central `SHIFT + CTRL`:** Pressione a área intermediária para comandos combinados de seleção de máscara e corte.
* **Sliders Verticais Táteis:**
  - **Raio (`F`):** Deslize o polegar para cima/baixo para alterar o diâmetro do pincel com anel visual dinâmico.
  - **Força (`Shift + F`):** Deslize para regular a intensidade da deformação.
* **Botões de Ação Imediata:**
  - `Remesh (Ctrl + R)`: Executa o Voxel Remesh da malha ativa.
  - `Transfer (Alt + Q)`: Alterna a escultura entre diferentes objetos ou subtools na cena.
  - `Canhoto / Destro`: Inverte a posição do SculptPad entre a borda esquerda e a borda direita da tela, realinhando a margem de segurança de 16 dp.

---

## 7. Resolução de Dúvidas e Diagnósticos

* **O launcher informa "Nenhum tablet detectado via cabo USB":**
  - Desconecte e reconecte o cabo USB-C.
  - Certifique-se de que a tela do tablet esteja desbloqueada e que a caixa de diálogo de depuração USB tenha sido aceita.
  - Teste a porta USB executando `dependencias\platform-tools\adb.exe devices`.
* **A imagem de vídeo não aparece ou está desatualizada:**
  - O aplicativo do tablet possui recuperação de perda via pacote `REQUEST_IDR`. Ao reconectar ou iniciar o app, ele solicita automaticamente um quadro-chave limpo para o servidor.
  - Verifique se a porta `27184` está ativa executando `setup_usb_tunnel.bat`.
* **Como reconstruir os binários no futuro?**
  - Para o servidor Windows: execute `Windows\build.bat`.
  - Para o aplicativo Android: execute `Android\build_apk.bat`.

