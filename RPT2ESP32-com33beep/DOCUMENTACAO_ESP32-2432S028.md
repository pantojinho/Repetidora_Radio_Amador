# Documentação Completa - Repetidora ESP32-2432S028 (CYD)

## 📋 Table of Contents
- [Project WIKIs](#project-wikis)
- [Especificações da Placa](#placa-esp32-2432s028r-cheap-yellow-display)
- [Configuração de Pinagem](#configuração-de-pinagem)
- [Configuração do User_Setup.h](#configuração-do-user_setuph)
- [Layout da Tela](#layout-da-tela-320x240---paisagem)
- [Conexão com o Rádio](#conexão-com-o-rádio-repetidora-setup)
- [Funcionalidades](#funcionalidades)
- [Sistema de Identificação Automática](#sistema-de-identificação-automática)
- [Bibliotecas Necessárias](#bibliotecas-necessárias)
- [Troubleshooting](#troubleshooting)
- [Sistema de LED RGB](#sistema-de-led-rgb-detalhado)
- [Guia Rápido de Instalação](#guia-rápido-de-instalação)
- [Upload de Arquivos de Áudio](#upload-de-arquivos-de-áudio-para-o-esp32)
- [Segurança](#segurança)
- [Como Contribuir](#como-contribuir)
- [Autor e Contato](#autor-e-contato)
- [Changelog](#changelog)

---

## Project WIKIs

This is one of the complete WIKIs for the project. To access all available WIKIs:

- 📖 **Main WIKI (Portuguese)**: [`README.md`](../README.md) - General project documentation
- 📚 **English WIKI**: [zread.ai](https://zread.ai/pantojinho/Repetidora_Radio_Amador) - Complete documentation in English (English reading option)
- 📖 **Technical WIKI (Portuguese)**: This document - Detailed technical documentation

All WIKIs contain complete information about the project, in different languages and detail levels.

---

## Placa: ESP32-2432S028R (Cheap Yellow Display)

### Especificações da Placa
- **Display**: TFT 2.8" ILI9341, 320x240 pixels (paisagem)
- **Touchscreen**: XPT2046 (resistivo)
- **Microcontrolador**: ESP32-WROOM-32
- **LED RGB**: Integrado (GPIO 4, 16, 17)
- **Extended IO**: P3 e CN1 (GPIO22, 27, 35 disponíveis)
- **Speaker**: Onboard via JST 2-pin (GPIO26)

---

## Configuração de Pinagem

### Display TFT (SPI)
| Função | GPIO | Descrição |
|--------|------|-----------|
| TFT_MISO | 12 | Master In Slave Out |
| TFT_MOSI | 13 | Master Out Slave In |
| TFT_SCLK | 14 | Serial Clock |
| TFT_CS | 15 | Chip Select |
| TFT_DC | 2 | Data/Command |
| TFT_RST | -1 | Reset (ligado ao EN central) |
| TFT_BL | 21 | Backlight |

### Touchscreen (XPT2046)
| Função | GPIO | Descrição |
|--------|------|-----------|
| TOUCH_CS | 33 | Chip Select do touchscreen |

### Repetidora (Extended IO - P3/CN1)
| Função | GPIO | Conector | Descrição |
|--------|------|----------|-----------|
| PIN_COR | 22 | P3 ou CN1 | Entrada COR (squelch detection) - Extended IO |
| PIN_PTT | 27 | CN1 | Saída PTT (push-to-talk) - Extended IO |
| SPEAKER_PIN | 26 | JST 2-pin | Speaker onboard (courtesy tones via I2S) |
| GND | GND | P3/CN1 | Terra comum |
| 3V3 | 3V3 | CN1 | Alimentação 3.3V (se necessário) |
| GPIO35 | 35 | P3 | Input-only (disponível para sensor extra) |

**⚠️ IMPORTANTE**: GPIO16/17 são do LED RGB - NÃO usar para COR/PTT (causa conflitos)

### LED RGB (Integrado na Placa - Indicador de Status)
| Função | GPIO | Descrição |
|--------|------|-----------|
| PIN_LED_R | 4 | LED Vermelho (via PWM ledc channel) |
| PIN_LED_G | 16 | LED Verde (via PWM ledc channel, ⚠️ NÃO usar para COR) |
| PIN_LED_B | 17 | LED Azul (via PWM ledc channel, ⚠️ NÃO usar para PTT) |

**Nota**: LED RGB usa GPIO4, 16, 17. Por isso COR/PTT foram movidos para GPIO22/27 (Extended IO)

**Modo de Operação do LED**:
- Anodo Comum: LOW acende, HIGH apaga
- PWM: 5kHz de frequência, 8 bits de resolução (0-255)
- Controlado por: `ledcAttach()` e `ledcWrite()`
- Atualização em tempo real: 20ms interval para rainbow, contínuo para status

### Extended IO (P3 e CN1) - Para Conexões Externas

**P3 (4 pins, de cima para baixo na placa):**
- GND
- GPIO35 (input-only, disponível para sensor extra)
- GPIO22 (IO full, usado para COR)
- GPIO21 (backlight do display - ⚠️ EVITAR, fica sempre HIGH)

**CN1 (4 pins):**
- GND
- GPIO22 (compartilhado com P3, usado para COR)
- GPIO27 (IO full, usado para PTT)
- 3V3 (alimentação 3.3V se necessário)

**Speaker Connector (JST 2-pin, separado):**
- Conecta speaker 8Ω 1-3W
- Controlado por GPIO26 (áudio I2S para courtesy tones)

**⚠️ Pins Ocupados (NÃO usar):**
- Display/Touch: GPIO2, 12-15, 21, 33
- LED RGB: GPIO4, 16, 17
- SD Card: GPIO5, 18, 19, 23
- **Use APENAS Extended IO (P3/CN1) para evitar conflitos**

---

## Configuração do User_Setup.h

**CRÍTICO**: O arquivo `User_Setup.h` da biblioteca TFT_eSPI deve estar configurado assim:

```cpp
// Driver alternativo (elimina ghosting no CYD)
#define ILI9341_2_DRIVER

// Resolução
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// SPI (Display)
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1   // Reset ligado ao EN da placa
#define TFT_BL   21   // Backlight

// Correções de cor/ghosting
#define TFT_RGB_ORDER TFT_BGR  // Inverte RGB (corrige cores)
#define TFT_INVERSION_ON       // Resolve ghosting e artifacts

// Otimizações
#define SPI_FREQUENCY  27000000   // Mais estável, evita artifacts
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000
#define SPI_USE_HW_SPI
```

**Localização**: `Arduino/libraries/TFT_eSPI/User_Setup.h`

**Rotação no código**: `tft.setRotation(3);` para landscape horizontal (320x240)

---

## Layout da Tela (320x240 - Paisagem)

### Estrutura Visual

```
┌─────────────────────────────────────┐
│  CABEÇALHO (Y=0-40)                │
│  PY2KEP SP                    v1.0 │
├─────────────────────────────────────┤
│                                     │
│  STATUS RX/TX (Y=60-140)           │
│  ┌─────────────────────────────┐   │
│  │         RX ou TX            │   │
│  │      (Fonte Grande)         │   │
│  └─────────────────────────────┘   │
│                                     │
│  Barra Progresso Timeout (se TX)   │
│                                     │
├─────────────────────────────────────┤
│  ESTATÍSTICAS (Y=160-240)           │
│  Col1      Col2        Col3         │
│  QSO: X    CT: Y/33    CW: Zm       │
│            Boop        Voz: Zm      │
└─────────────────────────────────────┘
```

### Coordenadas Detalhadas

| Elemento | X | Y | Tamanho |
|----------|---|----|---------|
| **Cabeçalho** | 0-320 | 0-40 | Fonte 4 |
| **Callsign** | 160 (centro) | 10 | - |
| **Versão** | 240 | 30 | Fonte 1 |
| **Status RX/TX** | 10-310 | 60-140 | Retângulo grande |
| **Texto RX/TX** | 160 (centro) | 100 | Fonte 6 |
| **QSO** | 20 | 180 | Fonte 2 |
| **CT** | 120 | 180 | Fonte 2 |
| **IDs** | 240 | 180 | Fonte 2 |

---

## Conexão com o Rádio (Repetidora Setup)

### ⚠️ AVISOS IMPORTANTES
- **Level Shifter OBRIGATÓRIO**: Se o rádio usar 5V, SEMPRE use level shifter ou optocoupler
- **GND Comum**: Conecte GND comum entre CYD, rádio RX e TX
- **Teste com Multímetro**: Verifique todas as conexões antes de ligar

### 1. COR (Squelch Detection)
- **Conexão**: GPIO22 (P3 ou CN1) → Output squelch/COR do rádio RX
- **Configuração**: `pinMode(PIN_COR, INPUT_PULLUP);` no código
- **Funcionamento**: Detecta quando há sinal recebido (COR ativo = LOW)
- **Proteção**: Se rádio 5V, use level shifter (ex: módulo 3.3V-5V ou resistor divider)

### 2. PTT (Push-to-Talk)
- **Conexão**: GPIO27 (CN1) → Input PTT do rádio TX
- **Configuração**: `pinMode(PIN_PTT, OUTPUT); digitalWrite(PIN_PTT, LOW);` (LOW = idle)
- **Funcionamento**: HIGH ativa transmissão, LOW desativa
- **Proteção**: Se rádio 5V, use level shifter ou optocoupler para isolar

### 3. Speaker (Courtesy Tones)
- **Conexão**: Speaker 8Ω 1-3W → JST 2-pin connector (GPIO26)
- **Configuração**: I2S configurado para GPIO26 no código
- **Funcionamento**: Reproduz beeps/courtesy tones após cada QSO

### 4. Alimentação
- **USB**: Alimenta via USB-C (recomendado)
- **5V Pin**: Alternativa via pin 5V (cuidado com consumo ~300mA com display + speaker)
- **Consumo**: Display + Speaker ~300mA em uso normal

### Diagrama de Conexão Simplificado
```
Rádio RX (COR)  ──[Level Shifter]── GPIO22 (P3/CN1) ── ESP32
Rádio TX (PTT)  ──[Level Shifter]── GPIO27 (CN1)    ── ESP32
GND Comum       ──────────────────── GND (P3/CN1)    ── ESP32
Speaker 8Ω      ──────────────────── JST 2-pin      ── GPIO26
```

---

## Funcionalidades

### 1. Display TFT
- **Rotação**: 3 (paisagem 320x240) - `tft.setRotation(3);`
- **Driver**: ILI9341_2_DRIVER (elimina ghosting)
- **Inversão**: TFT_INVERSION_ON (resolve artifacts)
- **Atualização**: A cada 250ms (sem flicker)
- **Otimização**: Atualização parcial (só áreas que mudam)
- **Layout**: Header 60px, Status central, Estatísticas no rodapé

### 2. Touchscreen
- **Biblioteca**: XPT2046_Touchscreen
- **Pino CS**: GPIO 33
- **Funcionalidade**: 
  - Toque em qualquer lugar da tela para trocar Courtesy Tone
  - Debounce: 500ms delay + espera soltar dedo (evita troca rápida)
  - Feedback visual: Display atualiza mostrando novo CT

### 3. LED RGB (Indicador de Status - IMPLEMENTADO)
- **Pinos**: GPIO4 (R), GPIO16 (G), GPIO17 (B)
- **Tipo**: Anodo Comum (LOW acende, HIGH apaga)
- **Cores de Status** (Implementado e Funcional):
  - **Vermelho Fixo**: Transmitindo (TX ativo)
  - **Amarelo Pulsante**: Recebendo com COR ativo (RX - breathing effect)
  - **Rainbow Suave**: Idle/Nenhum sinal (ciclo de cores suave)
- **Controle**: PWM via ledcAttach (freq=5kHz, 8 bits de resolução)
- **Funcionalidade**: Indica visualmente o estado da repetidora em tempo real

### 4. Indicadores Visuais (Layout Profissional)
- **Header**: Callsign "PY2KEP SP" em amarelo sobre fundo azul escuro (60px)
- **Status Principal**: Caixa grande com bordas arredondadas
  - **EM ESCUTA**: Fundo verde escuro, texto branco
  - **RX ATIVO**: Fundo amarelo, texto preto
  - **TX ATIVO**: Fundo vermelho, texto branco
- **Courtesy Tone**: Caixa verde com nome do CT e número (XX/33)
- **Estatísticas (Rodapé)**: 3 colunas
  - **Esquerda**: QSOs (verde)
  - **Centro**: Uptime em horas:minutos (amarelo)
  - **Direita**: CT Index (ciano)
- **Barra de Progresso**: Aparece quando TX ativo (verde → laranja → vermelho)

---

## Sistema de Identificação Automática

A repetidora possui sistema completo de identificação automática em três modos:

### 0. Identificação Inicial (apenas uma vez no boot)
- **Quando**: Ao ligar a placa pela primeira vez
- **ID Inicial em Voz**:
  - Timing: Imediatamente após o setup (aguarda 2 segundos)
  - Formato: Arquivo WAV com indicativo da repetidora
  - Display: Mostra "TX VOZ" + "INDICATIVO VOZ" com fundo vermelho
- **ID Inicial em CW**:
  - Timing: 1 minuto após o ID inicial em voz (62 segundos total do boot)
  - Velocidade: 13 WPM, Frequência: 600 Hz
  - Display: Mostra "TX CW" + "MORSE CODE" com fundo vermelho
  - Visualização: Exibe cada caractere e código Morse em tempo real
- **Após IDs iniciais**: O sistema inicia o ciclo normal de identificação

### 1. Courtesy Tone (após cada QSO)
- **Quando**: Após cada transmissão ser concluída (COR desativado)
- **Modo**: 33 courtesy tones diferentes (selecionáveis via touchscreen)
- **Controle**: Toque curto na tela (< 1.5 segundos) muda CT
- **Funcionamento**: Toca courtesy tone selecionado durante hang time (600ms)
- **Troca Automática**: O CT é alterado automaticamente a cada **5 QSOs** (conforme código original)
- **Índice**: Atualiza ciclicamente de 1 a 33 (volta para 1 após o 33)

### 2. Identificação em Voz (ciclo normal)
- **Intervalo**: A cada **11 minutos** (se nenhum QSO ativo) - conforme código original
- **Arquivo**: `/id_voz_8k16.wav` na memória SPIFFS
- **Conteúdo**: Repete o indicativo da repetidora (ex: "PY2KEP SP")
- **Formato do áudio**: WAV, 8kHz, 16-bit, mono
- **Função**: `playVoiceFile()` lê arquivo do SPIFFS e reproduz via I2S
- **Comportamento**: PTT ON -> Toca voz -> PTT OFF (automático, independente do modo de áudio)
- **Display**: Mostra "TX VOZ" + "INDICATIVO VOZ" com fundo vermelho durante transmissão
- **Observação**: Só inicia após completar os IDs iniciais do boot

### 3. Identificação em CW (Morse - ciclo normal)
- **Intervalo**: A cada **16 minutos** (se nenhum QSO ativo) - conforme código original
- **Velocidade**: 13 WPM (Words Per Minute)
- **Frequência**: 600 Hz para tom CW
- **Conteúdo**: Repete o indicativo em código Morse internacional
- **Função**: `playCW()` converte texto para Morse e reproduz via I2S
- **Comportamento**: PTT ON -> Toca Morse -> PTT OFF (automático)
- **Display**: Mostra "TX CW" + "MORSE CODE" com fundo vermelho durante transmissão
- **Visualização em Tempo Real**: Exibe cada caractere e código Morse sendo transmitido (ex: "P: .--.")
- **Observação**: Só inicia após completar os IDs iniciais do boot

### Controle do Modo de Áudio

- **Alternar entre Voz e CT**: Toque longo na tela (> 1.5 segundos)
- **Display mostra**: "VOZ: CALLSIGN" ou "CT: Boop 01/33"
- **Touchscreen inteligente**: Diferencia toque curto (troca CT) e longo (alternar modo)

### Nota Importante

As identificações automáticas (VOZ e CW) funcionam **independentemente** do modo de áudio (courtesy tones). Você pode usar courtesy tones após cada QSO E ainda ter as identificações automáticas nos intervalos regulares.

### Configurações de Tempos (conforme código original)

| Função | Tempo | Descrição |
|---------|-------|-----------|
| Hang Time | 600ms | Espera após QSO antes do courtesy tone |
| ID em Voz | 11 minutos | Intervalo de identificação em voz |
| ID em CW (Morse) | 16 minutos | Intervalo de identificação em Morse |
| Troca de CT | A cada 5 QSOs | Muda automaticamente o courtesy tone |

**Nota**: Todos os tempos foram mantidos conforme o código original para garantir compatibilidade.

---

## Bibliotecas Necessárias

1. **TFT_eSPI** (Bodmer)
   - Instalar via Gerenciador de Bibliotecas
   - Configurar `User_Setup.h` conforme acima

2. **XPT2046_Touchscreen**
   - Instalar via Gerenciador de Bibliotecas
   - Versão compatível com ESP32

3. **ESP32 Board Support**
   - Adicionar URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Instalar via Gerenciador de Placas

---

## Troubleshooting

### Display em branco
- ✅ Verificar se `User_Setup.h` está configurado corretamente
- ✅ Verificar se pinos SPI estão corretos (12, 13, 14, 15, 2, 21)
- ✅ Verificar backlight (GPIO 21)

### Touchscreen não funciona
- ✅ Verificar se biblioteca XPT2046_Touchscreen está instalada
- ✅ Verificar pino CS=33
- ✅ Calibrar coordenadas se necessário (ajustar map() na função handleTouchscreen)

### LED RGB não acende
- ✅ Verificar pinos: R=4, G=16, B=17
- ✅ Verificar se são LEDs comuns (cátodo comum) ou anodo comum
- ✅ Ajustar lógica se necessário (inverter HIGH/LOW)

### Layout cortado ou texto virado
- ✅ Verificar rotação: deve ser `setRotation(3)` (não 1!)
- ✅ Verificar resolução: 320x240 (paisagem)
- ✅ Verificar driver: ILI9341_2_DRIVER no User_Setup.h
- ✅ Verificar inversão: TFT_INVERSION_ON

### Ghosting ou "chuvisco" na tela
- ✅ Usar ILI9341_2_DRIVER (não ILI9341_DRIVER)
- ✅ Frequência SPI: 27MHz (não 40MHz)
- ✅ TFT_INVERSION_ON ativado
- ✅ Limpeza completa da área inferior no código

### Áudio não funciona
- ✅ Verificar speaker conectado no JST 2-pin
- ✅ Verificar GPIO26 configurado no I2S
- ✅ Remover I2S_MODE_DAC_BUILT_IN (CYD não usa DAC built-in)
- ✅ Verificar volume: ajustar `VOLUME` no código

---

## Melhorias Implementadas

1. ✅ **Sem Flash Inicial**: Removido testes visuais no boot
2. ✅ **Sem Flicker**: Atualização parcial (só áreas que mudam)
3. ✅ **Atualização Otimizada**: Display atualiza a cada 250ms
4. ✅ **Touchscreen com Debounce**: Troca de CT com delay 500ms + espera soltar
5. ✅ **Layout Profissional**: Header com callsign, status central, estatísticas organizadas
6. ✅ **Bordas Arredondadas**: Design moderno com fillRoundRect
7. ✅ **Cores Dinâmicas**: Verde (idle), Amarelo (RX), Vermelho (TX)
8. ✅ **Uptime em Tempo Real**: Mostra horas:minutos de operação
9. ✅ **Barra de Progresso**: Visual quando TX ativo
10. ✅ **Pins Adaptados**: GPIO22/27 para COR/PTT (Extended IO)
11. ✅ **Áudio I2S**: Speaker onboard via GPIO26
12. ✅ **Ghosting Eliminado**: ILI9341_2_DRIVER + limpeza completa
13. ✅ **LED RGB Indicador de Status**: Sistema completo de feedback visual
   - Vermelho fixo durante TX
   - Amarelo pulsante durante RX (breathing effect)
   - Rainbow suave quando idle (ciclo de cores)
   - Controle via PWM para transições suaves
14. ✅ **Debug Logging Avançado**: Sistema de log em arquivo (NDJSON) para análise offline
15. ✅ **Comentários Detalhados**: Todo o código está documentado com explicações em português

---

## Notas Técnicas

- **Frequência SPI**: 27MHz (otimizada para estabilidade, evita artifacts no CYD)
- **Touchscreen**: Verificação contínua no loop com debounce
- **Áudio I2S**: GPIO26, sample rate 22050Hz, volume 0.70
- **Memória**: Código otimizado (sem String, usa snprintf)
- **Watchdog**: 30 segundos (proteção contra travamentos)
- **Hang Time**: 600ms (delay após COR desativar antes de tocar CT)
- **Extended IO**: P3 e CN1 para conexões externas (COR/PTT)
- **Level Shifter**: OBRIGATÓRIO se rádio usar 5V
- **LED RGB**: Controle via PWM 5kHz, 8 bits, anodo comum (LOW=acende)
- **Debug Logging**: Sistema em NDJSON armazenado em /debug.log no SPIFFS
- **Uptime Update**: Atualização a cada 5 segundos sem redesenhar tela completa

---

## Sistema de LED RGB (Detalhado)

### Arquitetura do Controle LED
O LED RGB é controlado por um sistema de estado que reflete o status atual da repetidora:

**1. Configuração de PWM (setup)**
```cpp
ledc_channel_r = ledcAttach(PIN_LED_R, 5000, 8);  // 5kHz, 8 bits
ledc_channel_g = ledcAttach(PIN_LED_G, 5000, 8);
ledc_channel_b = ledcAttach(PIN_LED_B, 5000, 8);
```

**2. Conversão HSV para RGB**
- Função `setColorFromHue(float h)` converte valor de matiz (0-360) para valores RGB
- Implementação via algoritmo matemático padrão HSV→RGB
- Valores são invertidos (255 - valor) devido ao anodo comum

**3. Estados do LED**

**Estado 1: Transmitindo (TX)**
- **Condição**: `ptt_state == true`
- **Cor**: Vermelho sólido
- **Ação**:
  ```cpp
  ledcWrite(ledc_channel_r, 0);    // Vermelho full
  ledcWrite(ledc_channel_g, 255);  // Verde apagado
  ledcWrite(ledc_channel_b, 255);  // Azul apagado
  ```
- **Comportamento**: Cor fixa, sem animação

**Estado 2: Recebendo (RX - COR ativo)**
- **Condição**: `cor_stable == true && ptt_state == false`
- **Cor**: Amarelo com efeito breathing (pulsante)
- **Ação**:
  ```cpp
  float brightness = (sin(millis() / 500.0) + 1.0) / 2.0;  // 0 a 1
  int val = (int)(brightness * 255);
  ledcWrite(ledc_channel_r, 255 - val);  // Vermelho pulsante
  ledcWrite(ledc_channel_g, 255 - val);  // Verde pulsante
  ledcWrite(ledc_channel_b, 255);        // Azul apagado
  ```
- **Comportamento**: Animação suave de 0% a 100% de brilho em ciclo

**Estado 3: Idle (Nenhum sinal)**
- **Condição**: `cor_stable == false && ptt_state == false`
- **Cor**: Rainbow suave (ciclo de cores)
- **Ação**:
  ```cpp
  hue += 1.0;  // Aumenta 1 grau a cada 20ms
  if (hue >= 360) hue = 0;
  setColorFromHue(hue);  // Aplica cor atual
  ```
- **Comportamento**: Ciclo contínuo através de todo espectro de cores

### Funções do LED RGB

**`setColorFromHue(float h)`**
- Converte matiz HSV para RGB
- Parâmetro: `h` (0-360 graus)
- Saturação fixa: 1.0
- Valor fixo: 1.0
- Inverte valores para anodo comum

**`updateLED()`**
- Verifica estado atual (TX/RX/Idle)
- Atualiza LED de acordo com estado
- Gerencia flag `led_rainbow_enabled`
- Chamada continuamente no loop principal

### Timing do LED
- **Rainbow**: Atualiza a cada 20ms (`intervalLED = 20`)
- **Breathing**: Atualiza continuamente no loop (frequência baseada em `millis() / 500.0`)
- **TX**: Controle direto sem delay (resposta imediata)

### Utilidade Visual
O LED RGB fornece feedback visual instantâneo sobre o status da repetidora:
- **Vermelho fixo**: Indica transmissão ativa (evite falar)
- **Amarelo pulsante**: Alguém está transmitindo no canal
- **Rainbow**: Canal livre, repetidora em espera
- Útil para operações rápidas de "radio check" sem olhar para o display

---

## 🚀 Guia Rápido de Instalação

### Pré-requisitos
- ESP32-2432S028R (CYD)
- Arduino IDE 2.x
- Rádio com COR e PTT
- Cabo USB-C

### Instalação (5 minutos)

**1. Adicionar suporte ESP32:**
- Arduino IDE → File → Preferences
- URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- Board Manager → Instalar "esp32 by Espressif Systems"

**2. Instalar bibliotecas:**
- Library Manager → TFT_eSPI (Bodmer)
- Library Manager → XPT2046_Touchscreen

**3. Configurar TFT_eSPI:**
- Editar `Arduino/libraries/TFT_eSPI/User_Setup.h`
- Usar configurações do projeto (ver seção [Configuração do User_Setup.h](#configuração-do-user_setuph))

**4. Carregar o código:**
- Abrir `RPT2ESP32-com33beep.ino`
- Carregar no ESP32

### Conexão com Rádio

```
Rádio RX (COR) → GPIO22 (P3/CN1)
Rádio TX (PTT) → GPIO27 (CN1)
GND Comum       → GND
Speaker 8Ω      → JST 2-pin (GPIO26)
```

⚠️ **Importante**: Use level shifter se rádio é 5V+

### Uso
- **Toque na tela**: Troca courtesy tone
- **LED RGB**: Indica status (TX/RX/Idle)
- **Display**: Mostra estatísticas em tempo real

---

## Upload de Arquivos de Áudio para o ESP32

### Passo 1: Instalar Plugin de Upload (SPIFFS/LittleFS)

#### Instalação via Arduino IDE 2.x (Recomendado)
1. Abra o **Arduino IDE 2.x**
2. Vá em **Tools → Manage Plugins...**
3. Pesquise por: **"ESP32 Sketch Data Upload"** ou **"LittleFS Upload"**
4. Clique em **Install**
5. Aguarde a instalação ser concluída

#### Instalação Manual (se necessário)
Se você baixou o arquivo `.vsix` manualmente (ex: `arduino-littlefs-upload-1.6.1.vsix`):

1. **⚠️ NÃO execute o arquivo `.vsix`** (não clique duas vezes nele - isso ativa o instalador do Visual Studio e causa erro)
2. Copie o arquivo para a pasta de plugins do Arduino IDE:
   ```
   C:\Users\[SeuUsuario]\.arduinoIDE\plugins\
   ```
3. **Feche completamente** o Arduino IDE 2.x
4. **Reabra** o Arduino IDE 2.x

#### Como Usar o Plugin
Diferente da versão antiga (1.8), na versão 2.x o plugin funciona como extensão de código (estilo VS Code):

1. **Feche o Monitor Serial** (obrigatório - o upload sempre falha se estiver aberto, pois eles dividem a mesma porta USB)
2. Pressione **Ctrl + Shift + P** (abre a Paleta de Comandos)
3. Digite: `Upload LittleFS` ou `Upload SPIFFS`
4. Selecione o comando na lista
5. Aguarde o upload completar

> ⚠️ **Dicas Importantes**: 
> - O arquivo `.vsix` deve estar diretamente na pasta `plugins`, não em uma subpasta
> - Sempre feche o Monitor Serial antes de fazer upload
> - Certifique-se de que os arquivos estão na pasta `data` dentro do projeto
> - Se o comando não aparecer, verifique se não há uma pasta extra dentro de `plugins`

Este plugin permite fazer upload de arquivos da pasta `/data` para a memória SPIFFS do ESP32.

### Passo 2: Preparar Arquivos de Áudio

Os arquivos de áudio devem ser colocados na pasta `/data` do projeto:

```
Repetidora_Radio_Amador/
├── data/
│   └── id_voz_8k16.wav    # Arquivo de identificação em voz
└── RPT2ESP32-com33beep/
    └── RPT2ESP32-com33beep.ino
```

**Formato esperado do arquivo WAV:**
- **Sample Rate**: 8000 Hz (conforme nome: 8k16)
- **Bit Depth**: 16-bit PCM
- **Canais**: Mono (1 canal)
- **Formato**: WAV não comprimido (PCM)

### Passo 3: Converter Arquivo de Áudio (se necessário)

Se o seu arquivo de voz não estiver no formato correto, use um conversor:

**Usando FFmpeg (Windows/Mac/Linux):**
```bash
ffmpeg -i input.mp3 -ar 8000 -ac 1 -acodec pcm_s16le output.wav
```

**Usando Audacity (Windows/Mac/Linux - GRATUITO):**
1. Abra o Audacity
2. Importe o seu arquivo de áudio
3. Vá em **Track → Set Rate → Other...** → Selecione **8000 Hz**
4. Exporte como **WAV (Microsoft) 16-bit PCM**
5. Nomeie o arquivo como: `id_voz_8k16.wav`

### Passo 4: Upload do Arquivo de Áudio

1. **Feche o Monitor Serial** (obrigatório - o upload sempre falha se estiver aberto)
2. Conecte o ESP32 via USB
3. No Arduino IDE 2.x, abra o projeto (`RPT2ESP32-com33beep.ino`)
4. Pressione **Ctrl + Shift + P** para abrir a Paleta de Comandos
5. Digite: `Upload LittleFS` ou `Upload SPIFFS`
6. Selecione o comando na lista
7. Aguarde o upload completar (você verá "Data uploaded successfully" no console)
8. O arquivo `id_voz_8k16.wav` será gravado na memória SPIFFS do ESP32

**Nota:** Se você receber um erro "SPIFFS image not found" ou o comando não aparecer:
- Certifique-se de que a pasta `/data` está no mesmo nível do arquivo `.ino`
- Verifique se você instalou o plugin corretamente (veja Passo 1)
- Se instalou manualmente, verifique se o arquivo `.vsix` está diretamente em `plugins`, não em uma subpasta
- Reinicie o Arduino IDE 2.x após instalar o plugin

### Passo 5: Upload do Código Principal

1. Mantenha o ESP32 conectado via USB
2. No Arduino IDE, compile o código
3. Carregue o código (`Sketch → Upload`)
4. O sistema será reiniciado e começará a operar

### Passo 6: Verificar Funcionamento

1. Abra o **Serial Monitor** (115200 baud)
2. Você deve ver mensagens do sistema:
   ```
   Inicializando SPIFFS...
   SPIFFS inicializado com sucesso
   Tocando arquivo de voz: /id_voz_8k16.wav (XXXX bytes)
   Reprodução de voz concluída
   ```
3. Após cada QSO, a repetidora tocará o indicativo automaticamente
4. A cada 10 minutos, haverá identificação em voz (se não houver QSO)
5. A cada 30 minutos, haverá identificação em CW (se não houver QSO)

### Resumo

- **Arquivo já incluído**: O projeto já possui `id_voz_8k16.wav` na pasta `/data`
- **Só precisa**: Instalar plugin → Upload do arquivo → Compilar e carregar código
- **Verificação**: Serial Monitor confirma funcionamento correto

---

## 🔒 Segurança

### Hardware
- Use **level shifters** ao conectar com rádios de 5V
- Mantenha **GND comum** entre todos os dispositivos
- Use **fontes de alimentação** estáveis

### Software
- Código totalmente **transparente e auditável**
- **NÃO coleta** dados de uso ou telemetria
- Usuário tem **controle total** do dispositivo

### Reportar Vulnerabilidades
- Use o [GitHub Security Advisory](https://github.com/pantojinho/Repetidora_Radio_Amador/security/advisories)
- NÃO abra issues públicas para vulnerabilidades
- Seremos notificados e corrigiremos o problema

---

## 🤝 Como Contribuir

Quer contribuir? Fork, clone e faça um Pull Request:

```bash
git clone https://github.com/pantojinho/Repetidora_Radio_Amador.git
# Faça suas mudanças
git commit -m "Descrição clara"
git push origin main
```

- 🐛 Reportar bugs: [Issues](https://github.com/pantojinho/Repetidora_Radio_Amador/issues)
- 💡 Sugerir melhorias: [Issues](https://github.com/pantojinho/Repetidora_Radio_Amador/issues)
- 🛠️ Enviar código: [Pull Requests](https://github.com/pantojinho/Repetidora_Radio_Amador/pulls)

---

## 👤 Autor e Contato

**Gabriel Ciandrini** - **PU2PEG**

Radioamador brasileiro e desenvolvedor de projetos para a comunidade.

- 📻 **Indicativo**: PU2PEG
- 💻 **GitHub**: [pantojinho](https://github.com/pantojinho)
- 🌐 **Repositório**: [github.com/pantojinho/Repetidora_Radio_Amador](https://github.com/pantojinho/Repetidora_Radio_Amador)

**Junior** - **PY2PE**

Radioamador brasileiro e co-desenvolvedor do projeto.

- 📻 **Indicativo**: PY2PE

### Sobre o Projeto

Desenvolvido como um projeto open source para a comunidade de rádio amador, com foco em:

- Transparência de código (totalmente auditável)
- Documentação detalhada em português
- Interface visual moderna e profissional
- Fácil de configurar e usar

### Links Úteis

- [zread.ai](https://zread.ai/pantojinho/Repetidora_Radio_Amador) - Visualização interativa do código com análise inteligente

### Contato

Para questões sobre o projeto:
- 📧 GitHub Issues: [pantojinho/Repetidora_Radio_Amador/issues](https://github.com/pantojinho/Repetidora_Radio_Amador/issues)
- 💬 GitHub Discussions: [pantojinho/Repetidora_Radio_Amador/discussions](https://github.com/pantojinho/Repetidora_Radio_Amador/discussions)

---

## 📜 Licença

Este projeto está licenciado sob a [Licença MIT](LICENSE).

---

## 📅 Changelog

### v2.1 (Atual - LED RGB Implementado e Documentação Atualizada)
- ✅ **LED RGB completo como indicador de status**:
  - Vermelho fixo durante TX
  - Amarelo pulsante durante RX (breathing effect)
  - Rainbow suave quando idle
- ✅ **Controle via PWM**: 5kHz, 8 bits de resolução para transições suaves
- ✅ **Sistema de Debug Logging Avançado**: NDJSON em /debug.log para análise offline
- ✅ **Documentação completa**: Todos os aspectos do código documentados
- ✅ **Comentários detalhados**: Cada função principal explicada em português

### v2.0 (Adaptado para CYD)
- ✅ Pins adaptados: GPIO22 (COR), GPIO27 (PTT), GPIO26 (Speaker)
- ✅ Driver ILI9341_2_DRIVER (elimina ghosting)
- ✅ Rotação 3 (landscape horizontal correto)
- ✅ Layout profissional com header, status central, rodapé
- ✅ Bordas arredondadas e cores dinâmicas
- ✅ Uptime em tempo real
- ✅ Touchscreen com debounce melhorado
- ✅ Áudio I2S para speaker onboard
- ✅ Flash inicial removido (boot limpo)
- ✅ Indicativo centralizado e bem posicionado

### v1.0 (Original)
- Layout básico para 320x240 (paisagem)
- Suporte para touchscreen XPT2046
- LED RGB integrado
- Barra de progresso para timeout PTT
- Estatísticas em colunas
- Otimização anti-flicker

---

<div align="center">

**📡 Gabriel Ciandrini - PU2PEG**
**📡 Junior - PY2PE**

Feito com ❤️ para a comunidade de rádio amador

[GitHub](https://github.com/pantojinho) | [zread.ai](https://zread.ai/pantojinho/Repetidora_Radio_Amador)

73! 📻

</div>
