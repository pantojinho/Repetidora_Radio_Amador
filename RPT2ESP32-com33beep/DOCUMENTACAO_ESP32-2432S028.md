# Documentação - Repetidora ESP32-2432S028 (CYD)

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
- ✅ Verificar pinos: R=4, G=26, B=27
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

- **Frequência SPI**: 20MHz (otimizada para estabilidade, evita artifacts no CYD)
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

## Changelog

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


