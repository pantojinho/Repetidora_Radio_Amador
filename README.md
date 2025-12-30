# Repetidora de Rádio Amador - ESP32-2432S028 (CYD)

<div align="center">

![ESP32](https://img.shields.io/badge/ESP32-CYD-blue)
![Arduino](https://img.shields.io/badge/Arduino-IDE-orange)
![License](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Status-Stable-brightgreen)
[![zread](https://img.shields.io/badge/Ask_Zread-_.svg?style=flat&color=00b0aa&labelColor=000000&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTQuOTYxNTYgMS42MDAxSDIuMjQxNTZDMS44ODgxIDEuNjAwMSAxLjYwMTU2IDEuODg2NjQgMS42MDE1NiAyLjI0MDFWNC45NjAxQzEuNjAxNTYgNS4zMTM1NiAxLjg4ODEgNS42MDAxIDIuMjQxNTYgNS42MDAxSDQuOTYxNTZDNS4zMTUwMiA1LjYwMDEgNS42MDE1NiA1LjMxMzU2IDUuNjAxNTYgNC45NjAxVjIuMjQwMUM1LjYwMTU2IDEuODg2NjQgNS4zMTUwMiAxLjYwMDEgNC45NjE1NiAxLjYwMDFaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00Ljk2MTU2IDEwLjM5OTlIMi4yNDE1NkMxLjg4ODEgMTAuMzk5OSAxLjYwMTU2IDEwLjY4NjQgMS42MDE1NiAxMS4wMzk5VjEzLjc1OTlDMS42MDE1NiAxNC4xMTM0IDEuODg4MSAxNC4zOTk5IDIuMjQxNTYgMTQuMzk5OUg0Ljk2MTU2QzUuMzE1MDIgMTQuMzk5OSA1LjYwMTU2IDE0LjExMzQgNS42MDE1NiAxMy43NTk5VjExLjAzOTlDNS42MDE1NiAxMC42ODY0IDUuMzE1MDIgMTAuMzk5OSA0Ljk2MTU2IDEwLjM5OTlaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik0xMy43NTg0IDEuNjAwMUgxMS4wMzg0QzEwLjY4NSAxLjYwMDEgMTAuMzk4NCAxLjg4NjY0IDEwLjM5ODQgMi4yNDAxVjQuOTYwMUMxMC4zOTg0IDUuMzEzNTYgMTAuNjg1IDUuNjAwMSAxMS4wMzg0IDUuNjAwMUgxMy43NTg0QzE0LjExMTkgNS42MDAxIDE0LjM5ODQgNS4zMTM1NiAxNC4zOTg0IDQuOTYwMVYyLjI0MDFDMTQuMzk4NCAxLjg4NjY0IDE0LjExMTkgMS42MDAxIDEzLjc1ODQgMS42MDAxWiIgZmlsbD0iI2ZmZiIvPgo8cGF0aCBkPSJNNCAxMkwxMiA0TDQgMTJaIiBmaWxsPSIjZmZmIi8%2BCjxwYXRoIGQ9Ik00IDEyTDEyIDQiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPgo8L3N2Zz4K&logoColor=ffffff)](https://zread.ai/pantojinho/Repetidora_Radio_Amador)

**Sistema completo de repetidora de rádio amador com interface gráfica**

</div>

<div align="center">

![Display da Repetidora em Funcionamento](RPT2ESP32-com33beep/ESP_32.jpg)

*Interface visual da repetidora mostrando status "EM ESCUTA", callsign "PY2KEP SP", courtesy tone "Boop" (01/33), estatísticas de QSOs, uptime e informações do CT*

</div>

---

## 📖 Sobre

Este projeto implementa uma repetidora de rádio amador moderna baseada no microcontrolador **ESP32-WROOM-32** com o display **ESP32-2432S028R** (conhecido como "Cheap Yellow Display" ou CYD). A repetidora possui uma interface visual TFT colorida com touchscreen, sistema de courtesy tones audíveis e indicador de status por LED RGB.

### ✅ Validação e Compatibilidade

O código foi **completamente validado** e comparado com o código original primitivo (sem display). A lógica principal foi **100% preservada**:

- ✅ **Tempos e intervalos**: Todos os valores mantidos (HANG_TIME=600ms, VOICE_INTERVAL=11min, CW_INTERVAL=16min, QSO_CT_CHANGE=5)
- ✅ **Lógica de debounce COR**: Idêntica ao original (350ms)
- ✅ **Controle de PTT**: Mesma lógica de ativação/desativação
- ✅ **Funções de áudio**: Adaptadas para CYD mas mantendo a mesma lógica
- ✅ **Troca automática de CT**: Funcionando corretamente (corrigido na v2.2)
- ✅ **IDs automáticos**: Intervalos e sequência conforme original

**Adaptações necessárias para o CYD**:
- I2S direto (GPIO26) em vez de DAC built-in
- LittleFS em vez de SPIFFS (mais moderno e confiável)
- Pinos COR/PTT movidos para Extended IO (GPIO22/27) para evitar conflito com LED RGB

Todas as funcionalidades originais foram preservadas e melhoradas com interface visual e sistema de debug.

### Principais Características

- 🖥️ **Display TFT 2.8"** ILI9341 (320x240 pixels) com orientação paisagem
- 👆 **Touchscreen resistivo** para seleção de courtesy tones
- 🎵 **33 Courtesy Tones** diferentes selecionáveis
- 🎨 **LED RGB** com indicador visual de status em tempo real:
  - 🟢 **Verde fixo**: Em espera (Idle)
  - 🟡 **Amarelo fixo**: Recebendo sinal (RX)
  - 🔴 **Vermelho fixo**: Transmitindo (TX)
- 📊 **Display informativo** com estatísticas em tempo real
- 🔊 **Áudio I2S** para reproduction de courtesy tones no speaker onboard
- ⚡ **Otimizações de performance**: Display sem flicker, atualizações parciais
- 📝 **Sistema de logging** em NDJSON para análise offline
- 🐛 **Sistema de debug configurável** com níveis (NONE/MINIMAL/NORMAL/VERBOSE)
- 🔄 **Identificação automática** em Voz e CW (Morse) com intervalos configuráveis
- ✅ **Lógica 100% compatível** com código original (validada e testada)

---

## 🎯 Funcionalidades

### Controle de Repetidora
- Detecção automática de sinal (COR - Squelch Detection)
- Ativação/desativação de PTT (Push-to-Talk)
- Hang time configurável (600ms)
- Contador de QSOs

### Interface Visual
- Header com callsign customizável
- Status em tempo real (EM ESCUTA / RX ATIVO / TX ATIVO)
- Seleção visual de courtesy tone
- Estatísticas: QSOs, Uptime, Índice do CT
- Barra de progresso durante transmissão
- Touchscreen com debounce para evitar trocas acidentais

### Indicador LED RGB
- Sistema completo de feedback visual
- Transições suaves via PWM (5kHz, 8 bits)
- Efeitos: breathing e rainbow cíclico

---

## 🔧 Hardware Necessário

### Obrigatório
- Placa **ESP32-2432S028R** (Cheap Yellow Display)
- Rádio transceptor com acesso a:
  - Saída de squelch/COR
  - Entrada de PTT
- Speaker 8Ω 1-3W (opcional, para áudio onboard)

### Opcional (Recomendado)
- **Level shifter** ou optocoupler (se rádio usar 5V)
- Cabos JST 2-pin para conexão do speaker

---

## 📦 Configuração de Pinagem

### Display TFT (SPI)
| Função | GPIO | Descrição |
|--------|------|-----------|
| TFT_MISO | 12 | Master In Slave Out |
| TFT_MOSI | 13 | Master Out Slave In |
| TFT_SCLK | 14 | Serial Clock |
| TFT_CS | 15 | Chip Select |
| TFT_DC | 2 | Data/Command |
| TFT_RST | -1 | Reset (ligado ao EN) |
| TFT_BL | 21 | Backlight |

### Repetidora (Extended IO)
| Função | GPIO | Conector | Descrição |
|--------|------|----------|-----------|
| PIN_COR | 22 | P3/CN1 | Entrada COR (squelch detection) |
| PIN_PTT | 27 | CN1 | Saída PTT (push-to-talk) |
| SPEAKER | 26 | JST 2-pin | Speaker onboard (I2S) |

### LED RGB
| Função | GPIO | Descrição |
|--------|------|-----------|
| LED_R | 4 | LED Vermelho |
| LED_G | 16 | LED Verde |
| LED_B | 17 | LED Azul |

> ⚠️ **Importante**: GPIO16/17 são do LED RGB - NÃO usar para COR/PTT para evitar conflitos

---

## 🚀 Instalação

### 1. Instalar o Arduino IDE
Baixe e instale a versão mais recente do [Arduino IDE](https://www.arduino.cc/en/software).

### 2. Adicionar suporte ESP32
1. Abra Arduino IDE
2. Vá em `File > Preferences`
3. Adicione a URL no campo "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Vá em `Tools > Board > Boards Manager`
5. Pesquise por "esp32" e instale o pacote

### 3. Instalar bibliotecas necessárias
Instale as seguintes bibliotecas via `Sketch > Include Library > Manage Libraries`:

- **TFT_eSPI** (por Bodmer)
- **XPT2046_Touchscreen**

### 4. Instalar Plugin de Upload de Dados (SPIFFS/LittleFS)
Para fazer upload de arquivos de áudio (WAV), você precisa instalar o plugin:

#### Instalação via Arduino IDE 2.x (Recomendado)
1. Abra o **Arduino IDE 2.x**
2. Vá em `Tools > Manage Plugins...`
3. Pesquise por "ESP32 Sketch Data Upload" ou "LittleFS Upload"
4. Clique em **Install** e aguarde a instalação

#### Instalação Manual (se necessário)
Se você baixou o arquivo `.vsix` manualmente:

1. **NÃO execute o arquivo `.vsix`** (não clique duas vezes nele)
2. Copie o arquivo para a pasta de plugins do Arduino IDE:
   ```
   C:\Users\[SeuUsuario]\.arduinoIDE\plugins\
   ```
3. **Feche completamente** o Arduino IDE 2.x
4. **Reabra** o Arduino IDE 2.x

#### Como Usar o Plugin
Diferente da versão antiga (1.8), na versão 2.x o plugin funciona como extensão de código (estilo VS Code):

1. **Feche o Monitor Serial** (obrigatório - o upload falha se estiver aberto)
2. Pressione **Ctrl + Shift + P** (abre a Paleta de Comandos)
3. Digite: `Upload LittleFS` ou `Upload SPIFFS`
4. Selecione o comando na lista
5. Aguarde o upload completar

> ⚠️ **Importante**: 
> - O arquivo `.vsix` deve estar diretamente na pasta `plugins`, não em uma subpasta
> - Sempre feche o Monitor Serial antes de fazer upload
> - Certifique-se de que os arquivos estão na pasta `data` dentro do projeto

Este plugin permite fazer upload de arquivos da pasta `/data` para a memória SPIFFS do ESP32.

### 5. Preparar Arquivos de Áudio
Os arquivos de áudio devem ser colocados na pasta `/data` do projeto:

```
Repetidora_Radio_Amador/
├── data/
│   └── id_voz_8k16.wav    # Arquivo de identificação em voz (já existe!)
└── RPT2ESP32-com33beep/
    └── RPT2ESP32-com33beep.ino
```

**Formato esperado do arquivo WAV:**
- **Sample Rate**: 8000 Hz (conforme nome: 8k16)
- **Bit Depth**: 16-bit PCM
- **Canais**: Mono (1 canal)
- **Formato**: WAV não-comprimido (PCM)

### 6. ⚠️ IMPORTANTE: Ordem de Upload

**⚠️ ATENÇÃO: É CRÍTICO seguir esta ordem!**

1. **PRIMEIRO: Compile e faça upload do código** (sem o áudio ainda)
2. **DEPOIS: Faça upload dos arquivos de áudio**

**Por quê?**
- O código precisa ser compilado primeiro para criar a estrutura do LittleFS no ESP32
- Se você tentar fazer upload do áudio antes de compilar, pode ocorrer erro
- Após compilar e fazer upload do código uma vez, o sistema LittleFS estará pronto para receber os arquivos

### 7. Upload do Código Principal (PRIMEIRO)

1. Selecione a placa: `ESP32 Dev Module` ou `ESP32-2432S028`
2. Conecte o ESP32 via USB
3. **Compile o código** (`Sketch > Verify/Compile`) - verifique se não há erros
4. **Carregue o código** (`Sketch > Upload`)
5. Aguarde o upload completar e o ESP32 reiniciar

> ✅ **Agora o código está no ESP32 e o sistema LittleFS está inicializado**

### 8. Upload dos Arquivos de Áudio para o ESP32 (DEPOIS)

1. **Feche o Monitor Serial** (obrigatório - o upload sempre falha se estiver aberto)
2. Mantenha o ESP32 conectado via USB
3. No Arduino IDE 2.x, com o projeto aberto (`RPT2ESP32-com33beep.ino`)
4. Pressione **Ctrl + Shift + P** para abrir a Paleta de Comandos
5. Digite: `Upload LittleFS` ou `Upload SPIFFS`
6. Selecione o comando na lista
7. Aguarde o upload completar (você verá "Data uploaded successfully" no console)
8. O arquivo `id_voz_8k16.wav` será gravado na memória LittleFS do ESP32

**Nota**: Se você receber um erro "SPIFFS image not found" ou o comando não aparecer:
- Certifique-se de que a pasta `/data` está no mesmo nível do arquivo `.ino`
- Verifique se você instalou o plugin corretamente (veja seção 4)
- Se instalou manualmente, verifique se o arquivo `.vsix` está diretamente em `plugins`, não em uma subpasta
- Reinicie o Arduino IDE 2.x após instalar o plugin
- **Certifique-se de ter compilado e feito upload do código primeiro!**

### 8. Configurar TFT_eSPI
O arquivo `User_Setup.h` da biblioteca TFT_eSPI deve ser configurado assim:

```cpp
#define ILI9341_2_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1
#define TFT_BL   21
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000
#define SPI_USE_HW_SPI
```

> **Localização**: `Arduino/libraries/TFT_eSPI/User_Setup.h`

### 10. Verificar Funcionamento

Após fazer upload do código e dos arquivos de áudio:

1. Abra o **Serial Monitor** (115200 baud)
2. Você deve ver mensagens do sistema:
   ```
   === INICIALIZACAO REPETIDORA ===
   LittleFS inicializado com sucesso
   Display: W=320, H=240
   TEXTO 'EM ESCUTA' DESENHADO: x=79, y=100, w=162, bg=0x07E0
   === INICIALIZACAO CONCLUIDA ===
   ```
3. O display deve mostrar:
   - Header azul com o callsign
   - Status "EM ESCUTA" em verde
   - Courtesy tone selecionado
   - Estatísticas (QSOs, Uptime, CT)
4. O LED RGB deve estar verde (modo idle)

---

## 🔌 Conexões com o Rádio

### ⚠️ Avisos Importantes
- **Level Shifter OBRIGATÓRIO**: Se o rádio usar 5V, SEMPRE use level shifter ou optocoupler
- **GND Comum**: Conecte GND comum entre CYD, rádio RX e TX
- **Teste com Multímetro**: Verifique todas as conexões antes de ligar

### Diagrama de Conexão
```
Rádio RX (COR)  ──[Level Shifter]── GPIO22 (P3/CN1) ── ESP32
Rádio TX (PTT)  ──[Level Shifter]── GPIO27 (CN1)    ── ESP32
GND Comum       ──────────────────── GND (P3/CN1)    ── ESP32
Speaker 8Ω      ──────────────────── JST 2-pin      ── GPIO26
```

### 1. COR (Squelch Detection)
- Conecte a saída de squelch do rádio ao GPIO22
- Configuração: `pinMode(PIN_COR, INPUT_PULLUP)`
- Funcionamento: LOW quando há sinal detectado

### 2. PTT (Push-to-Talk)
- Conecte a entrada de PTT do rádio ao GPIO27
- Configuração: `pinMode(PIN_PTT, OUTPUT)`
- Funcionamento: HIGH ativa transmissão

### 3. Speaker (Opcional)
- Conecte speaker 8Ω 1-3W ao conector JST 2-pin
- Controlado via GPIO26 (I2S)
- Reproduz courtesy tones após cada QSO

---

## 🎮 Como Usar

### Operação Básica

1. **Ligar a repetidora**: Conecte via USB, o display mostrará o status inicial
2. **EM ESCUTA**: A repetidora está em espera (LED rainbow)
3. **Quando alguém transmitir**:
   - O COR detecta o sinal
   - PTT é ativado automaticamente
   - LED muda para amarelo pulsante (RX)
   - Display mostra "RX ATIVO"
4. **Após o término da transmissão**:
   - Hang time de 600ms
   - Courtesy tone é tocado
   - LED volta para rainbow
5. **Trocar Courtesy Tone**:
   - Toque em qualquer lugar da tela
   - O CT avança para o próximo (circular 1-33)
   - Display atualiza mostrando o novo CT

### Lista de Courtesy Tones (33)

1. Boop
2. Beep
3. Apollo
4. Moonbounce
5. Tumbleweed
6. Bee-Boo
7. Bumble Bee
8. YellowJacket
9. ShootingStar
10. Comet
11. Stardust
12. Duncecap
13. Piano Chord
14. NBC
15. 3up
16. TelRing
17. BlastOff
18. Water Drop
19. Whippoorwhill
20. Sat Pass
21. OverHere
22. Nextel
23-31. RC210-1 a RC210-9
32. XP Error
33. XP OK

---

## 🌐 Configuração via WiFi

A repetidora possui um sistema completo de configuração via interface web, permitindo ajustar todos os parâmetros sem precisar recompilar o código.

### 📻 Credenciais de Acesso WiFi

O dispositivo cria automaticamente um Access Point WiFi no boot:

| Credencial | Valor | Descrição |
|-----------|-------|------------|
| **SSID** | `REPETIDORA_SETUP` | Nome da rede WiFi para configuração |
| **Senha** | `repetidora123` | Senha para acessar o AP |
| **IP** | `192.168.4.1` | Endereço IP padrão do ESP32 em modo AP |

### 🔧 Como Conectar e Configurar

#### Passo 1: Conectar no WiFi AP
1. Ative o WiFi no seu dispositivo (celular, tablet ou laptop)
2. Procure pela rede `REPETIDORA_SETUP`
3. Digite a senha: `repetidora123`
4. Aguarde conectar

#### Passo 2: Acessar a Interface Web
1. Abra o navegador web (Chrome, Firefox, Safari, Edge, etc.)
2. Digite o endereço IP: `http://192.168.4.1`
3. Pressione Enter

> **💡 Dica:** Se o IP for diferente (ex: 192.168.4.2), pressione o botão BOOT na placa para ver as informações do WiFi no display.

#### Passo 3: Configurar Parâmetros
1. Todas as configurações são organizadas em seções
2. Faça as alterações desejadas
3. Clique em "💾 Salvar e Reiniciar" no final da página
4. O ESP32 reiniciará automaticamente com as novas configurações

### 🎮 Controle via BOOT Button (GPIO 0)

O botão BOOT integrado na placa ESP32-2432S028R fornece controle sobre a visualização:

#### 1. Toque Rápido - Toggle de Tela
- **Pressionar e soltar** o BOOT button alterna entre **tela normal** (repetidora) e **tela do WiFi** (informações de acesso)
- **Cada vez que você pressiona o botão**, a tela alterna entre os dois modos
- Não afeta a operação da repetidora - continua funcionando normalmente
- **⚠️ IMPORTANTE:** O botão **NÃO funciona durante transmissão (TX)** - aguarde o término da transmissão para alternar a tela

#### 2. Toque Prolongado (> 5 segundos) - Reset de Fábrica
- **Segurar BOOT button por 5+ segundos** restaura todas as configurações para os valores de fábrica padrão
- Display mostra fundo vermelho com alerta "ATENÇÃO!" durante o reset
- Ao soltar, o ESP32 reinicia com configurações limpas

> **⚠️ AVISO:** O reset de fábrica apaga TODAS as configurações personalizadas. Use apenas se realmente precisar restaurar os valores padrão.

> **💡 Dica:** Se o botão não responder, verifique se a repetidora não está em modo TX (transmitindo). O botão só funciona quando a repetidora está em modo Idle ou RX.

### 🌐 Interface Web de Configuração

A interface web é uma página HTML responsiva com design moderno, organizada em seções:

#### Seções Disponíveis

1. **📻 Informações Básicas**
   - Indicativo (Callsign)
   - Frequência (MHz)

2. **🔊 Configurações Morse (CW)**
   - Mensagem Morse (ID)
   - Velocidade (WPM): 5-40
   - Frequência do Tom (Hz): 300-1200

3. **⏱️ Configurações de Tempos**
   - Hang Time (ms): 100-2000
   - PTT Timeout (s): 60-600
   - Intervalo ID Voz (min): 5-30
   - Intervalo ID CW (min): 5-30
   - Troca CT (QSOs): 1-20

4. **🎵 Configurações de Áudio**
   - Volume: 0-100%
   - Sample Rate (Hz): 8000/11025/16000/22050/44100

5. **🔔 Courtesy Tone (CT)**
   - Seletor dos 33 courtesy tones diferentes

6. **🐛 Configurações de Debug**
   - Nível de Debug: 0 (None), 1 (Minimal), 2 (Normal), 3 (Verbose)
   - Console Debug: Visualização de logs em tempo real

7. **⚙️ Botões de Ação**
   - **💾 Salvar e Reiniciar:** Salva todas as configurações e reinicia o ESP32
   - **🔄 Reiniciar Dispositivo:** Reinicia o ESP32 sem salvar
   - **📋 Ver Console Debug:** Abre/fecha console de logs
   - **⚠️ Reset de Fábrica:** Restaura configurações padrão

### 📊 Tela de Informações WiFi

Quando o BOOT button é pressionado, o display mostra:

**Cabeçalho:**
- Callsign: PY2KEP SP
- Frequência: 439.450 MHz

**Status Principal:**
- Fundo: Ciano
- Texto: "WIFI AP ATIVO"
- Credenciais (3 linhas):
  ```
  SSID: REPETIDORA_SETUP
  Senha: repetidora123
  IP: 192.168.4.1
  ```

### 🔒 Armazenamento de Configurações

As configurações são salvas automaticamente na memória não-volátil (NVS - Non-Volatile Storage) do ESP32:

- **Biblioteca:** `Preferences.h`
- **Namespace:** "config"
- **Persistência:** Configurações sobrevivem a reinicialização do ESP32

#### Configurações Salvas

| Parâmetro | Chave | Valor Padrão | Descrição |
|----------|-------|--------|--------|--------|
| Callsign | `callsign` | `PY2KEP SP` | Indicativo da repetidora |
| Frequência | `frequency` | `439.450` | Frequência em MHz |
| Mensagem CW | `cw_message` | `PY2KEP SP` | Texto para ID Morse |
| Velocidade CW | `cw_wpm` | `13` | Palavras por minuto |
| Frequência CW | `cw_freq` | `600` | Hz do tom Morse |
| Hang Time | `hang_time` | `600` | Tempo após QSO (ms) |
| PTT Timeout | `ptt_timeout` | `240000` | Timeout máximo (4 min) |
| ID Voz | `voice_interval` | `660000` | Intervalo ID voz (11 min) |
| ID CW | `cw_interval` | `960000` | Intervalo ID CW (16 min) |
| Troca CT | `ct_change` | `5` | QSOs para trocar CT |
| CT Index | `ct_index` | `0` | CT selecionado (0-32) |
| Volume | `volume` | `0.7` | Volume (0.0-1.0) |
| Sample Rate | `sample_rate` | `22050` | Taxa de amostragem (Hz) |
| Debug Level | `debug_level` | `1` | Nível de detalhamento |

### 📞 Troubleshooting WiFi

#### WiFi não Conecta
- Verifique se o SSID `REPETIDORA_SETUP` está aparecendo
- Digite a senha `repetidora123` corretamente
- Verifique se o IP está correto (display mostra quando BOOT é pressionado)
- Tente outro dispositivo para acessar o AP

#### Display Não Mostra IP
- Verifique se o BOOT button está sendo pressionado
- Um toque rápido (pressione e solte) alterna a tela
- **⚠️ IMPORTANTE:** O botão não funciona durante TX (transmissão) - aguarde o término
- Se a tela não mudar, verifique se não está em modo TX

#### Botão "Salvar e Reiniciar" Não Funciona
- Verifique no Serial Monitor: `Args recebidos: X`
- Se X=0, nenhum dado foi recebido do formulário
- Verifique se há mensagens de erro no Serial Monitor
- Certifique-se de que todos os campos estão preenchidos

#### Reset de Fábrica Inesperado
- Verifique se o BOOT button não ficou preso
- Segure exatamente 5 segundos para reset
- Após reset, as configurações voltam aos valores padrão

---

## 🎙 Sistema de Identificação Automática

### ⚠️ Ordem Correta de Upload

**IMPORTANTE: Sempre siga esta ordem:**

1. **PRIMEIRO**: Compile e faça upload do código (`Sketch > Upload`)
2. **DEPOIS**: Faça upload dos arquivos de áudio (`Upload LittleFS`)

Esta ordem é crítica porque o código precisa inicializar o sistema LittleFS antes de receber arquivos. Se você tentar fazer upload do áudio antes de compilar, pode ocorrer erro "SPIFFS image not found".

### Como fazer Upload dos Arquivos de Áudio

📋 **Guia Completo de Upload**: Veja seção [6. ⚠️ IMPORTANTE: Ordem de Upload](#6-️-importante-ordem-de-upload) acima

Este guia detalhado inclui:
- ✅ Instalação do plugin "ESP32 Sketch Data Upload"
- ✅ Formato correto dos arquivos WAV (8kHz, 16-bit, mono)
- ✅ Como converter áudio se necessário (FFmpeg ou Audacity)
- ✅ **Ordem correta: Compilar código primeiro, depois upload de áudio**
- ✅ Como fazer upload dos arquivos
- ✅ Como verificar funcionamento via Serial Monitor

### Resumo das Identificações

A repetidora possui sistema completo de identificação automática em três modos:

| Modo | Quando | Conteúdo | Arquivo |
|-------|---------|-----------|--------|
| **ID Inicial Voz** | Imediatamente após ligar (2s) | `/id_voz_8k16.wav` (já incluído) |
| **ID Inicial CW** | 1 minuto após ID inicial voz | Callsign em Morse (13 WPM, 600 Hz) |
| **Courtesy Tone** | Após cada QSO (COR desativado) | Gerado por código (33 tipos) |
| **Identificação em Voz** | A cada **11 minutos** (sem QSO ativo) | `/id_voz_8k16.wav` (já incluído) |
| **Identificação em CW** | A cada **16 minutos** (sem QSO ativo) | Callsign em Morse (13 WPM, 600 Hz) |

### Identificação Inicial no Boot

Ao ligar a placa pela primeira vez, são realizadas automaticamente duas identificações:

1. **ID Inicial em Voz**: Ocorre imediatamente após o setup (aguarda 2 segundos)
2. **ID Inicial em CW**: Ocorre 1 minuto após o ID inicial em voz (62 segundos do boot)

Após completar as identificações iniciais, o sistema entra no ciclo normal de identificação (11 min voz / 16 min CW).

**Benefícios**:
- ✅ Confirma imediatamente que o sistema está funcionando
- ✅ Permite verificar áudio e display ao ligar
- ✅ Identifica a repetidora rapidamente após o boot

### Troca Automática de Courtesy Tone

- O **courtesy tone é alterado automaticamente a cada 5 QSOs** (conforme código original)
- Permite variação dos sons ao longo do tempo
- O índice atualiza ciclicamente de 1 a 33
- Também é possível trocar manualmente via toque na tela

### Como Funciona

1. **QSO completo**: Courtesy tone selecionado é tocado
2. **Identificação automática** (VOZ ou CW): Tocada nos intervalos regulares, independente do QSO
3. **Controle de modo**: Toque longo na tela alterna entre Voz e CT
4. **Display mostra**: "VOZ: CALLSIGN" ou "CT: Boop 01/33"

### Nota Importante

As identificações automáticas (VOZ e CW) funcionam **independentemente** do modo de áudio (courtesy tones). Você pode usar courtesy tones após cada QSO **E** ainda ter as identificações automáticas nos intervalos regulares.

---

## 🎙 Identificação Automática

A repetidora possui sistema de identificação automática em três modos:

### 0. Identificação Inicial (apenas uma vez no boot)

Ao ligar a placa pela primeira vez, são realizadas automaticamente duas identificações:

#### ID Inicial em Voz
- **Timing**: Imediatamente após o setup (aguarda 2 segundos)
- **Arquivo**: `/id_voz_8k16.wav` (já incluído no projeto)
- **Conteúdo**: Repete o indicativo da repetidora (ex: "PY2KEP SP")
- **Formato do áudio**: WAV, 8kHz, 16-bit, mono
- **Display**: Mostra "TX VOZ" com fundo vermelho durante transmissão

#### ID Inicial em CW
- **Timing**: 1 minuto após o ID inicial em voz (62 segundos total do boot)
- **Velocidade**: 13 WPM (palavras por minuto)
- **Frequência**: 600 Hz
- **Conteúdo**: Repete o indicativo em código Morse internacional
- **Display**: Mostra "TX CW" com fundo vermelho e exibe código Morse em tempo real

**Após os IDs iniciais**: O sistema inicia o ciclo normal de identificação.

### 1. Identificação em Voz (ciclo normal)
- **Intervalo**: A cada **11 minutos** (sem QSO ativo) - conforme código original
- **Arquivo**: `/id_voz_8k16.wav` (já incluído no projeto)
- **Conteúdo**: Repete o indicativo da repetidora (ex: "PY2KEP SP")
- **Formato do áudio**: WAV, 8kHz, 16-bit, mono
- **Display**: Mostra "TX VOZ" com fundo vermelho durante transmissão
- **Observação**: Só inicia após completar os IDs iniciais do boot

### 2. Identificação em CW (Morse - ciclo normal)
- **Intervalo**: A cada **16 minutos** (sem QSO ativo) - conforme código original
- **Velocidade**: 13 WPM (palavras por minuto)
- **Frequência**: 600 Hz
- **Conteúdo**: Repete o indicativo em código Morse internacional
- **Display**: Mostra "TX CW" com fundo vermelho e exibe código Morse em tempo real
- **Observação**: Só inicia após completar os IDs iniciais do boot

### Nota Importante
As identificações automáticas (VOZ e CW) funcionam **independentemente** do modo de áudio (courtesy tones). Você pode usar courtesy tones após cada QSO E ainda ter as identificações automáticas nos intervalos regulares.

---

## 📊 Estatísticas no Display

| Coluna | Descrição |
|--------|-----------|
| QSOs | Número total de QSOs completados |
| Uptime | Tempo de operação (hh:mm) |
| CT | Índice do courtesy tone atual |

---

## 🐛 Sistema de Debug

O projeto possui um sistema de debug configurável que permite controlar a verbosidade das mensagens no Serial Monitor.

### Níveis de Debug

| Nível | Valor | Descrição | Uso |
|-------|-------|-----------|-----|
| **NONE** | 0 | Apenas erros e eventos críticos | Produção |
| **MINIMAL** | 1 | Eventos principais (PTT, COR, QSO, IDs) | **Recomendado** |
| **NORMAL** | 2 | Debug padrão (inclui display, CW, loop stats) | Desenvolvimento |
| **VERBOSE** | 3 | Tudo incluindo JSON detalhado | Debug avançado |

### Como Configurar

No arquivo `.ino`, linha 137:
```cpp
#define DEBUG_LEVEL 1  // Altere aqui: 0=NONE, 1=MINIMAL, 2=NORMAL, 3=VERBOSE
```

### Categorias de Debug

O sistema controla diferentes categorias de mensagens:

- **DEBUG_JSON**: Mensagens JSON detalhadas (updateDisplay, etc.) - apenas nível 3
- **DEBUG_DISPLAY**: Mensagens de atualização do display - nível 2+
- **DEBUG_PTT**: Debug periódico do estado PTT - nível 1+ (a cada 10s)
- **DEBUG_CW**: Mensagens de código Morse - nível 2+
- **DEBUG_EVENTS**: Eventos principais (PTT ON/OFF, COR changes, IDs) - nível 1+

### Exemplo de Saída

**Nível MINIMAL (1)** - Recomendado:
```
PTT ON
COR: 0 -> 1
=== ID VOZ (11min) ===
ID Voz: 21.2s
PTT OFF
```

**Nível VERBOSE (3)** - Debug completo:
```
DEBUG:{"location":"updateDisplay:entry","message":"Function called",...}
DISPLAY STATE: tx_mode=0, ptt_state=0, cor_stable=0, status_bg=0x07E0, text='EM ESCUTA'
STATUS: EM ESCUTA (bg=0x07E0)
TEXTO 'EM ESCUTA' DESENHADO: x=79, y=100, w=162
```

### Logs em Arquivo

Independente do nível de debug, os logs continuam sendo salvos em `/debug.log` (LittleFS) para análise offline. O sistema de logging em arquivo usa throttling (máximo 1 log a cada 100ms) para não impactar o desempenho.

---

## 🛠️ Personalização

### Alterar Callsign
No arquivo `.ino`, modifique:
```cpp
const char* CALLSIGN = "PY2KEP SP";  // Altere para seu indicativo
// Exemplos: "PU2ABC", "PY1XYZ", "PU2PEG SP"
```

### Ajustar Volume
```cpp
float VOLUME = 0.70f;  // 0.0 a 1.0
```

### Modificar Hang Time
```cpp
#define HANG_TIME_MS 600  // Tempo em milissegundos após COR desativar
```

### Ajustar Intervalos de Identificação
```cpp
const uint32_t VOICE_INTERVAL_MS = 11UL*60UL*1000UL;  // 11 minutos - ID em voz
const uint32_t CW_INTERVAL_MS   = 16UL*60UL*1000UL;  // 16 minutos - ID em CW
const uint8_t  QSO_CT_CHANGE   = 5;                 // Troca CT a cada 5 QSOs
```

**Nota**: Todos os tempos foram configurados conforme o código original para garantir compatibilidade.

### Configurar Nível de Debug
```cpp
#define DEBUG_LEVEL 1  // 0=NONE, 1=MINIMAL, 2=NORMAL, 3=VERBOSE
```

**Níveis disponíveis**:
- `0` (NONE): Apenas erros e eventos críticos
- `1` (MINIMAL): Eventos principais (PTT, COR, QSO, IDs) - **RECOMENDADO**
- `2` (NORMAL): Debug padrão (inclui display, CW, loop stats)
- `3` (VERBOSE): Tudo incluindo JSON detalhado

### Ajustar Frequência SPI
No `User_Setup.h`:
```cpp
#define SPI_FREQUENCY  27000000  // 27MHz - mais estável
// #define SPI_FREQUENCY  40000000  // 40MHz - mais rápido (pode causar artifacts)
```

---

## 🐛 Troubleshooting

### Display em branco
- ✅ Verifique se `User_Setup.h` está configurado corretamente
- ✅ Verifique pinos SPI (12, 13, 14, 15, 2, 21)
- ✅ Verifique backlight (GPIO 21)

### Touchscreen não funciona
- ✅ Verifique biblioteca XPT2046_Touchscreen instalada
- ✅ Verifique pino CS=33

### LED RGB não acende
- ✅ Verifique pinos: R=4, G=16, B=17
- ✅ Anodo comum: LOW acende, HIGH apaga

### Layout cortado ou virado
- ✅ Rotação deve ser 3 (paisagem)
- ✅ Resolução: 320x240
- ✅ Use ILI9341_2_DRIVER

### Ghosting na tela
- ✅ Use ILI9341_2_DRIVER (não ILI9341_DRIVER)
- ✅ Frequência SPI: 27MHz
- ✅ TFT_INVERSION_ON ativado

### Serial Monitor com muitas mensagens
- ✅ Configure `DEBUG_LEVEL` para 1 (MINIMAL) no código
- ✅ Mensagens JSON detalhadas só aparecem em nível VERBOSE (3)
- ✅ Logs em arquivo continuam funcionando independente do nível

### Contador de QSOs não atualiza
- ✅ Verificado e corrigido na v2.2
- ✅ Certifique-se de usar a versão mais recente do código
- ✅ O contador incrementa quando COR desativa (fim do QSO)

---

## 📚 Documentation

For detailed technical information, please refer to:

**Complete Project WIKIs:**
- [`DOCUMENTACAO_ESP32-2432S028.md`](RPT2ESP32-com33beep/DOCUMENTACAO_ESP32-2432S028.md) - 📖 Complete documentation in **Portuguese**
- [`README.md`](README.md) - 📖 Main project WIKI in **Portuguese**
- [zread.ai](https://zread.ai/pantojinho/Repetidora_Radio_Amador) - 📚 Complete WIKI and documentation in **English** (English reading option)

**Source Code:**
- [`RPT2ESP32-com33beep.ino`](RPT2ESP32-com33beep/RPT2ESP32-com33beep.ino) - Source code with detailed comments

---

## 🔗 Links Úteis sobre a Placa ESP32-2432S028R (CYD)

Recursos adicionais para quem deseja conhecer mais sobre a placa Cheap Yellow Display e utilizá-la em outros projetos:

- [Getting Started with ESP32 Cheap Yellow Display Board (ESP32-2432S028R)](https://randomnerdtutorials.com/cheap-yellow-display-esp32-2432s028r/) - 📖 Guia completo de introdução à placa CYD, incluindo instalação, configuração e exemplos
- [ESP32 Cheap Yellow Display (CYD) Pinout (ESP32-2432S028R)](https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/) - 📌 Referência completa da pinagem da placa com todos os GPIOs disponíveis

> 💡 **Dica**: Estes tutoriais contêm informações valiosas sobre configuração de bibliotecas, pinagem detalhada, e exemplos de uso que podem ser úteis para outros projetos com esta placa.

---

## 📝 Changelog

### v2.3 (Atual - 29 de Dezembro de 2025)
- ✅ **Correção do Botão "Salvar e Reiniciar"**: JavaScript corrigido para coletar valores manualmente dos campos do formulário
- ✅ **Correção do Botão BOOT**: Lógica corrigida para alternar corretamente entre tela normal e tela WiFi
- ✅ **Melhorias no Display**: Tela redesenhada automaticamente quando alterna entre modos
- ✅ **Documentação Consolidada**: README único principal com todas as informações de WiFi integradas
- ✅ **Documentação Atualizada**: Informações completas sobre configuração via WiFi, credenciais e troubleshooting

### v2.2 (Dezembro 2024)
- ✅ **Sistema de Debug Otimizado**: Níveis configuráveis (NONE/MINIMAL/NORMAL/VERBOSE)
- ✅ **Correção Crítica**: Incremento de `qso_count` corrigido (troca automática de CT funcionando)
- ✅ **Serial Monitor Limpo**: Mensagens otimizadas, menos ruído, mais informações relevantes
- ✅ **Validação Completa**: Lógica 100% compatível com código original validada
- ✅ **Documentação Atualizada**: README completo com todas as funcionalidades
- ✅ **Melhorias de Performance**: Debug condicional, logs otimizados

### v2.1
- ✅ LED RGB completo como indicador de status
- ✅ Controle via PWM (5kHz, 8 bits)
- ✅ Sistema de Debug Logging Avançado (NDJSON)
- ✅ Documentação completa em português

### v2.0 (Adaptado para CYD)
- ✅ Pins adaptados: GPIO22 (COR), GPIO27 (PTT), GPIO26 (Speaker)
- ✅ Driver ILI9341_2_DRIVER (elimina ghosting)
- ✅ Rotação 3 (landscape horizontal)
- ✅ Layout profissional com header, status central, rodapé
- ✅ Bordas arredondadas e cores dinâmicas
- ✅ Touchscreen com debounce melhorado
- ✅ Áudio I2S para speaker onboard

### v1.0 (Original)
- Layout básico para 320x240
- Suporte para touchscreen XPT2046
- LED RGB integrado
- Barra de progresso PTT

---

## 🔮 Tarefas Futuras (Roadmap)

### 🚀 Próximas Funcionalidades Planejadas

#### 1. 🌐 Controle Remoto via WiFi (Alta Prioridade)
**Objetivo**: Permitir controle e monitoramento da repetidora via internet

**Funcionalidades Planejadas**:
- 📡 **Servidor Web Embarcado**: Interface web acessível via IP local
- 📊 **Dashboard em Tempo Real**: Status, QSOs, uptime, estatísticas
- 🎛️ **Controle Remoto**: 
  - Seleção de courtesy tone via web
  - Ajuste de volume
  - Ativação/desativação de IDs automáticos
  - Reset de contadores
- 📱 **API REST**: Para integração com sistemas externos
- 🔐 **Autenticação**: Proteção por senha para comandos críticos
- 📈 **Logs Remotos**: Visualização de logs via web
- 🌍 **Acesso Externo**: Opção de acesso via internet (com segurança)

**Tecnologias Consideradas**:
- ESP32 WiFi (já disponível no hardware)
- WebServer (ESPAsyncWebServer ou similar)
- WebSocket para atualizações em tempo real
- OTA (Over-The-Air) para atualizações remotas

**Benefícios**:
- ✅ Monitoramento remoto sem necessidade de estar no local
- ✅ Configuração sem acesso físico à placa
- ✅ Integração com sistemas de automação
- ✅ Coleta de dados e estatísticas históricas

#### 2. 📡 Integração com APRS (Média Prioridade)
- Envio automático de status via APRS
- Beacon de localização
- Integração com redes APRS-IS

#### 3. 📊 Sistema de Logging Avançado (Média Prioridade)
- Armazenamento de histórico de QSOs
- Estatísticas detalhadas (duração, horários, etc.)
- Exportação de dados (CSV, JSON)
- Gráficos e relatórios

#### 4. 🎚️ Controle de Volume Dinâmico (Baixa Prioridade)
- AGC (Automatic Gain Control) para áudio
- Compressão de áudio
- Equalização

#### 5. 🔔 Notificações (Baixa Prioridade)
- Alertas por email/SMS em eventos críticos
- Notificações push via app mobile
- Integração com Telegram/Discord

### 💡 Contribuições Bem-Vindas

Se você tem interesse em implementar alguma dessas funcionalidades, sinta-se à vontade para:
- Abrir uma issue descrevendo sua proposta
- Enviar um pull request com a implementação
- Discutir a melhor abordagem técnica

---

## 🤝 Contribuindo

Contribuições são bem-vindas! Sinta-se à vontade para:
- Reportar bugs
- Sugerir novas funcionalidades
- Enviar pull requests
- Melhorar a documentação

### Como contribuir
1. Fork este repositório
2. Crie uma branch para sua feature (`git checkout -b feature/MinhaFeature`)
3. Commit suas mudanças (`git commit -m 'Adiciona nova feature'`)
4. Push para a branch (`git push origin feature/MinhaFeature`)
5. Abra um Pull Request

---

## 👨‍💻 Autores

**Gabriel Ciandrini** - **PU2PEG**

Radioamador brasileiro e desenvolvedor de projetos para a comunidade.

📍 **Localização**: Brasil  
📻 **Indicativo**: PU2PEG  
💻 **GitHub**: [pantojinho](https://github.com/pantojinho)

**Junior** - **PY2PE**

Radioamador brasileiro e co-desenvolvedor do projeto.

📍 **Localização**: Brasil  
📻 **Indicativo**: PY2PE

### Sobre o Projeto

Desenvolvido como um projeto open source para a comunidade de rádio amador, com foco em:

- Transparência de código (totalmente auditável)
- Documentação detalhada em português
- Interface visual moderna e profissional
- Fácil de configurar e usar

### Contato

Para questões sobre o projeto:
- 📧 GitHub Issues: [pantojinho/Repetidora_Radio_Amador/issues](https://github.com/pantojinho/Repetidora_Radio_Amador/issues)
- 💬 GitHub Discussions: [pantojinho/Repetidora_Radio_Amador/discussions](https://github.com/pantojinho/Repetidora_Radio_Amador/discussions)

---

## 📄 Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.

---

## 🙏 Agradecimentos

- **Bodmer** pela biblioteca TFT_eSPI
- Comunidadade ESP32 e Arduino
- Comunidadade de rádio amador
- A todos que testaram e deram feedback
- Especial agradecimento ao **Junior PY2PE** pelo apoio e contribuição ao projeto

---

<div align="center">

**📡 Gabriel Ciandrini - PU2PEG**
**📡 Junior - PY2PE**

Feito com ❤️ para a comunidade de rádio amador

[GitHub](https://github.com/pantojinho) | [Repositório](https://github.com/pantojinho/Repetidora_Radio_Amador) | [zread.ai](https://zread.ai/pantojinho/Repetidora_Radio_Amador)

</div>

