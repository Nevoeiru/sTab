# Guia de Configuração: Monitor Virtual 16:10 Nativo (VDD / IddSampleDriver)
**Resolução Exata de 2560 × 1600 a 90 Hz para o Samsung Galaxy Tab S9 FE+**

---

## 1. Por que usar um Monitor Virtual 16:10?

A grande maioria dos monitores de computador opera na proporção **16:9** (1920×1080 ou 2560×1440), enquanto a tela do Galaxy Tab S9 FE+ possui formato panorâmico estendido **16:10** (2560×1600).
* Se você apenas espelhar a tela 16:9, haverá faixas pretas nas laterais ou achatamento anamórfico de esferas e malhas 3D.
* Com o **Virtual Display Driver (VDD)**, o Windows cria uma segunda tela independente na proporção nativa de **2560 × 1600 a 90 Hz**. O Blender passa a rodar em tela cheia na área de trabalho estendida com correspondência pixel-a-pixel perfeita, nitidez máxima e sem distorções!

---

## 2. O que é o Virtual Display Driver?

O VDD é um driver de exibição indireta em modo de usuário (*IddCx - Indirect Display Driver Class eXtension*) de código aberto, baseado na implementação oficial de referência da Microsoft (`IddSampleDriver`).
* **100% Gratuito e Seguro:** Opera inteiramente em User-Mode, sem riscos de tela azul (BSOD).
* **Pré-assinado:** Possui certificados comunitários aceitos nativamente pelo Windows 10 e Windows 11 sem necessidade de desativar o Secure Boot.

---

## 3. Passo a Passo de Instalação (Feito Uma Única Vez)

1. **Baixar o Driver Pré-assinado:**
   * Acesse o repositório oficial de lançamentos:
     `https://github.com/itsmikethetech/Virtual-Display-Driver/releases`
   * Baixe o pacote `.zip` da versão mais recente (ex: `VDD-v*.zip`).
2. **Copiar o Perfil de Configuração sTab:**
   * Extraia o `.zip` baixado em uma pasta (por exemplo: `C:\VirtualDisplayDriver\`).
   * Copie o arquivo [`options.txt`](file:///j:/sTab/VirtualDisplay/options.txt) localizado nesta pasta `j:\sTab\VirtualDisplay\options.txt` e cole-o dentro da pasta onde o driver foi extraído, substituindo o existente.
3. **Instalar o Driver no Windows:**
   * Abra a pasta do driver, clique com o botão direito sobre o arquivo de instalação executável ou sobre o arquivo `.inf` e selecione **Instalar** (ou execute o instalador como Administrador).
   * O Windows emitirá o som característico de conexão de um novo monitor.
4. **Verificar nas Configurações de Vídeo do Windows:**
   * Clique com o botão direito na Área de Trabalho > **Configurações de exibição**.
   * Você verá o **Monitor 2**.
   * Certifique-se de que a resolução esteja em **2560 × 1600** e a taxa de atualização em **90 Hz** (*Configurações avançadas de tela*).

---

## 4. Como o sTab Opera Automaticamente

* O capturador GPU do sTab ([`DxgiCapture.cpp`](file:///j:/sTab/Windows/src/DxgiCapture.cpp)) detecta automaticamente qualquer tela ativa com a resolução `2560x1600` e a seleciona com prioridade máxima.
* Ao abrir o Blender, basta movê-lo para a tela do tablet pressionando o atalho do teclado:
  **`Windows + Shift + Seta para a Direita`** (ou Seta para a Esquerda).
* O Blender ocupará a totalidade dos 12,4 polegadas do Galaxy Tab S9 FE+ a 90 FPS sem barras pretas!

