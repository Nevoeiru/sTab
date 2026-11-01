# Guia de Uso, Calibração e Operação — sTab (SculptBridge)
**Transforme o Samsung Galaxy Tab S9 FE+ em uma Mesa Digitalizadora Profissional para o Blender 3D**

---

## 1. Visão Geral da Arquitetura

O **sTab** é uma solução completa de baixíssima latência que conecta o hardware do Galaxy Tab S9 FE+ diretamente ao Windows e ao Blender, sem depender de drivers de terceiros pagos:
* **Digitalizador Wacom EMR a 240Hz:** Amostragem sub-frame nativa com 4096 níveis de pressão e inclinação cartesiana.
* **Injeção Nativa via Windows Ink:** Utiliza a API `CreateSyntheticPointerDevice(PT_PEN)` do Windows, interpretada pelo Blender com a mesma fidelidade de uma Wacom Cintiq oficial.
* **Streaming de Vídeo GPU a 90 FPS:** Captura de tela direta na VRAM via DXGI Desktop Duplication e codificação por hardware H.264 (Zero B-Frames, CBR 35 Mbps) com decodificação `MediaCodec` por hardware na tela do tablet.
* **Recuperação Instantânea `REQUEST_IDR`:** Canal de feedback que recupera qualquer quadro corrompido ou atrasado em menos de 1 ms.
* **Ergonomia de Duas Mãos:** Viewport de desenho para a mão direita e painel tátil **SculptPad** para o polegar esquerdo.

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

## 3. Inicialização Rápida no Windows (1 Clique)

Na pasta raiz do projeto:
* Dê dois cliques em **`run_stab.bat`** (ou execute `Windows\bin\sTab_launcher.exe`).

O inicializador executará os seguintes passos automaticamente:
1. Detecta o Galaxy Tab S9 FE+ conectado via cabo USB.
2. Abre os túneis USB reversos de ultra-baixa latência:
   - **Porta 27183:** Canal de telemetria da S-Pen 240Hz, gestos touch e comandos do SculptPad.
   - **Porta 27184:** Canal de transmissão de vídeo H.264 a 90 FPS.
3. Inicia o aplicativo `sTab` no tablet via ADB Intent.
4. Inicializa o servidor `sTab_server.exe` no Windows.

---

## 4. Add-on Opcional para o Blender (`sTab Companion`)

Embora o sistema funcione 100% de forma autônoma (driverless), o add-on fornece configuração ideal em um clique e telemetria:

1. No Blender, vá em **Edit > Preferences > Add-ons**.
2. Clique no botão **Install...** (ou na seta ao lado no Blender 4.2+) e selecione o arquivo:
   `j:\sTab\Blender\sTab_blender_companion.py`
3. Ative a caixa de seleção ao lado de **3D View: sTab Sculpt Companion**.
4. Na **3D Viewport**, abra a barra lateral pressionando a tecla **`N`** e clique na aba **`sTab`**.
5. Clique no botão **"Otimizar Blender para sTab"**:
   - Ajusta a *Tablet API* para Windows Ink.
   - Calibra a navegação 3D para rotação suave com os dedos.
   - Configura zoom vertical e rotação ao redor do ponto ativo.
6. Clique em **"Visualização Argila 3D"** para ativar o Matcap de escultura com relevo de cavidades.

---

## 5. Mapeamento de Ergonomia e Controles

### 5.1. Mão Direita: S-Pen
* **Ponta da Caneta:** Desenho com sensibilidade contínua de pressão (0..4096) e inclinação X/Y.
* **Botão Físico da S-Pen:** Mapeado diretamente como **Botão Direito do Mouse (`RMB`)**:
  - Pressionar o botão durante o hover abre instantaneamente o menu de contexto do pincel (*Brush Quick Menu*) sob a ponta da caneta.
  - O menu circular *Air Command* da Samsung é interceptado e neutralizado pela blindagem de baixo nível do sTab.

### 5.2. Mão Direita / Dedos na Viewport: Navegação 3D Direta
* **1 Dedo arrastando na Viewport:** Orbitar Câmera 3D (`MMB`).
* **2 Dedos paralelos arrastando:** Mover Câmera lateralmente (`Shift + MMB` / Pan).
* **Pinça com 2 Dedos (Pinch-to-Zoom):** Zoom suave e contínuo.
* **Dois toques rápidos (2 Taps):** Desfazer última pincelada (`Ctrl + Z`).
* **Três toques rápidos (3 Taps):** Refazer (`Ctrl + Shift + Z`).
* **Hardware Palm Rejection:** No instante em que a ponta da caneta entra no raio de ~15 mm da tela (`Hover`), os toques de dedos são suspensos na viewport para evitar marcas acidentais.

### 5.3. Mão Esquerda: Barra Lateral "SculptPad"
* **Botão `SHIFT` (Smooth):** Mantenha pressionado com o polegar esquerdo para suavizar a malha instantaneamente sem precisar alternar ferramentas.
* **Botão `CTRL` (Invert / Sub):** Inverte a ação do pincel (ex.: escava relevo em vez de acumular).
* **Zona de Contato Central `SHIFT + CTRL`:** Pressione a área intermediária para comandos combinados de seleção de máscara e corte.
* **Sliders Verticais Táteis:**
  - **Raio (`F`):** Deslize o polegar para cima/baixo para alterar o diâmetro do pincel com pré-visualização circular dinâmica na tela.
  - **Força (`Shift + F`):** Deslize para regular a intensidade da deformação.
* **Botões de Ação Imediata:**
  - `Remesh (Ctrl + R)`: Executa o Voxel Remesh da malha ativa.
  - `Transfer (Alt + Q)`: Alterna a escultura entre diferentes objetos ou subtools na cena.
  - `Canhoto / Destro`: Inverte a posição do SculptPad entre a borda esquerda e a borda direita da tela.

---

## 6. Resolução de Dúvidas e Diagnósticos

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

