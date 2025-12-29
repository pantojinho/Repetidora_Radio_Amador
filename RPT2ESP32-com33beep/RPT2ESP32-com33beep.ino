#include <Arduino.h>
#include "SPIFFS.h"
#include "driver/i2s.h"
#include <esp_task_wdt.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ==================================================================
// CÓDIGO ADAPTADO PARA ESP32-2432S028R (Cheap Yellow Display - CYD)
// ==================================================================
// Principais adaptações:
// - Pins COR/PTT movidos para GPIO22/27 (Extended IO) para evitar conflito com LED RGB
// - Áudio configurado para speaker onboard (GPIO26) via I2S
// - User_Setup.h deve estar configurado com ILI9341_2_DRIVER
//
// SISTEMA DE LED RGB IMPLEMENTADO (v2.1):
// ========================================
// O LED RGB funciona como indicador visual do estado da repetidora em tempo real.
//
// CONFIGURAÇÃO HARDWARE:
// - Pinos: GPIO4 (R), GPIO16 (G), GPIO17 (B)
// - Tipo: Anodo Comum (LOW = acende, HIGH = apaga)
// - Controle: PWM via LEDC do ESP32 (freq=5kHz, 8 bits)
//
// ESTADOS DO LED:
// 1. TRANSMITINDO (TX ativo):
//    - Cor: VERMELHO FIXO
//    - Pino R: 0 (full brilho)
//    - Pino G: 255 (apagado)
//    - Pino B: 255 (apagado)
//    - Animação: Nenhuma (cor sólida)
//
// 2. RECEBENDO (COR ativo, RX):
//    - Cor: AMARELO PULSANTE (breathing effect)
//    - Pino R: Variável (0-255, sincronizado)
//    - Pino G: Variável (0-255, sincronizado)
//    - Pino B: 255 (apagado)
//    - Animação: Breathing usando sin(millis()/500.0)
//
// 3. ESPERA/IDLE (sem sinal):
//    - Cor: RAINBOW SUAVE
//    - Espectro: Ciclo contínuo 0° a 360° (HSV)
//    - Animação: Hue incrementa a cada 20ms
//    - Efeito: Transição suave por todo espectro de cores
//
// FUNÇÕES DO LED RGB:
// - setColorFromHue(h): Converte hue HSV para RGB e aplica ao LED
// - updateLED(): Atualiza LED baseado no estado atual (TX/RX/Idle)
// - Rainbow: Atualizado continuamente no loop (quando idle)
// - Status: Atualizado continuamente no loop (TX/RX)
//
// UTILIDADE PRÁTICA:
// - Feedback visual instantâneo sem precisar olhar para o display
// - Vermelho fixo: Indica transmissão ativa (evite falar)
// - Amarelo pulsante: Alguém transmitindo no canal
// - Rainbow: Canal livre, repetidora em espera
// ==================================================================

// #region agent log - Debug logging helper
void debugLog(const char* location, const char* message, const char* hypothesisId, int data1 = 0, int data2 = 0, int data3 = 0) {
  Serial.printf("DEBUG:{\"location\":\"%s\",\"message\":\"%s\",\"hypothesisId\":\"%s\",\"data\":{\"v1\":%d,\"v2\":%d,\"v3\":%d},\"timestamp\":%lu}\n",
                location, message, hypothesisId, data1, data2, data3, millis());
}

// Log para arquivo NDJSON (para análise offline)
void logToFile(const char* hypothesisId, const char* message, unsigned long timestamp, int v1 = 0, int v2 = 0, int v3 = 0) {
  static bool logFileReady = false;
  static unsigned long lastLogTime = 0;

  // Só loga a cada 100ms para evitar impacto no desempenho
  if (millis() - lastLogTime < 100) return;
  lastLogTime = millis();

  File file = SPIFFS.open("/debug.log", FILE_APPEND);
  if (file) {
    file.printf("{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"%s\",\"data\":{\"v1\":%d,\"v2\":%d,\"v3\":%d},\"timestamp\":%lu}\n",
                hypothesisId, message, message, v1, v2, v3, timestamp);
    file.close();
  }
}
// #endregion

// ====================== CORES CUSTOMIZADAS ======================
// TFT_eSPI não tem algumas cores por padrão - definindo cores custom
#define TFT_DARKBLUE 0x000A  // Azul escuro custom (RGB 0,0,10) - elegante para header
// Alternativas se quiser ajustar:
// #define TFT_DARKBLUE 0x001F  // Azul um pouco mais claro
// #define TFT_DARKBLUE 0x01FF  // Azul médio-escuro

// TFT_ORANGE pode não existir em algumas versões - definindo se necessário
#ifndef TFT_ORANGE
#define TFT_ORANGE 0xFD20  // Laranja (RGB aproximado)
#endif

// TFT_DARKGREEN não existe por padrão - definindo manualmente
#ifndef TFT_DARKGREEN
#define TFT_DARKGREEN 0x0400  // Verde escuro custom (RGB 0,8,0)
#endif

// Coordenadas base (serão ajustadas dinamicamente conforme dimensões do display)
#define Y_HEADER   0
#define Y_STATUS   50
#define Y_INFO     120
#define Y_FOOTER   170  // Ajustado dinamicamente se necessário
#define FOOTER_H   70


// ====================== DISPLAY ======================
TFT_eSPI tft;
#define TOUCH_CS 33
XPT2046_Touchscreen ts(TOUCH_CS);

// ====================== HARDWARE =====================
// Adaptado para ESP32-2432S028R (CYD):
// GPIO16/17 são do LED RGB - usar extended IO para evitar conflitos
#define PIN_COR 22  // Extended GPIO22 (input from radio squelch/COR)
#define PIN_PTT 27  // Extended GPIO27 (output to radio PTT)
#define PIN_BL  21  // Backlight (não mudar - sempre HIGH)
#define SPEAKER_PIN 26  // Speaker onboard via JST 2-pin connector

// LED RGB pins (anodo comum - LOW = acende, HIGH = apaga)
#define PIN_LED_R 4
#define PIN_LED_G 16
#define PIN_LED_B 17

// ====================== CONFIG =======================
const char* CALLSIGN = "PY2KEP SP";
#define WDT_TIMEOUT_SECONDS 30
#define SAMPLE_RATE 22050
#define HANG_TIME_MS 600
float VOLUME = 0.70f;

// ====================== GLOBAIS ======================
bool cor_stable = false;
bool ptt_state  = false;
bool playing    = false;
bool i2s_ok     = false;

uint16_t qso_count = 0;
uint8_t  ct_index  = 0;
unsigned long last_display_update = 0;
unsigned long last_uptime_update = 0;  // Timer separado para uptime
bool first_draw = true;  // Flag para primeira renderização completa (evita flash)
bool needsFullRedraw = false;  // Flag para redraw completo quando necessário
char old_uptime_str[16] = "";  // String do uptime anterior (para comparar e só atualizar se mudou)

// ====================== VARIÁVEIS DO LED RGB ======================
// Sistema de controle do LED RGB usando PWM (LED Control do ESP32)
//
// previousMillisLED: Timestamp da última atualização do rainbow (para timing)
// intervalLED: Intervalo entre atualizações do rainbow em ms (20ms = 50 updates/s)
// hue: Valor de matiz atual (0-360 graus) para o ciclo rainbow
// led_rainbow_enabled: Flag que indica se o modo rainbow está ativo (true quando idle)
// ledc_channel_r/g/b: Canais PWM atribuídos pelo sistema LEDC (-1 = não inicializado)
unsigned long previousMillisLED = 0;
const long intervalLED = 20;  // ms - quanto menor, mais suave o rainbow
float hue = 0;  // 0 a 360 para ciclo de cores (HSV hue)
bool led_rainbow_enabled = true;  // Flag para habilitar/desabilitar rainbow
int ledc_channel_r = -1;  // Canais LEDC (serão configurados no setup)
int ledc_channel_g = -1;
int ledc_channel_b = -1;

// ====================== CT STRUCT ====================
struct Seg { uint16_t f1, f2, dur; };
struct CT  { const char* name; uint16_t delay_ms; Seg seg[6]; uint8_t n; };

#define N_CT 33

CT tones[N_CT] = {
  {"Boop",250,{{440,0,100}},1},
  {"Beep",250,{{880,0,100}},1},
  {"Apollo",250,{{2475,0,200}},1},
  {"Moonbounce",250,{{1000,800,50},{800,600,50},{600,400,50},{1500,1300,50}},4},
  {"Tumbleweed",250,{{1000,0,50},{800,0,50},{600,0,50}},3},
  {"Bee-Boo",250,{{800,0,200},{400,0,200}},2},
  {"Bumble Bee",250,{{330,0,100},{500,0,100},{660,0,100}},3},
  {"YellowJacket",250,{{500,0,50},{330,0,50},{660,0,50}},3},
  {"ShootingStar",250,{{800,0,200},{540,0,100}},2},
  {"Comet",250,{{500,0,200},{750,0,100}},2},
  {"Stardust",250,{{750,0,120},{880,0,80},{880,1200,80}},3},
  {"Duncecap",250,{{440,350,200},{440,0,200}},2},
  {"Piano Chord",250,{{660,880,100},{440,660,100},{660,880,100}},3},
  {"NBC",250,{{390,329,500},{660,659,500},{520,519,500}},3},
  {"3up",250,{{400,0,100},{600,0,100},{800,0,100}},3},
  {"TelRing",250,{{440,480,100},{880,0,100}},2},
  {"BlastOff",250,{{500,0,50},{1500,0,50},{2500,0,50}},3},
  {"Water Drop",250,{{500,0,40},{600,0,20},{700,0,20},{800,0,40},{400,0,40}},5},
  {"Whippoorwhill",250,{{1330,0,40},{980,0,40},{810,0,40}},3},
  {"Sat Pass",250,{{1290,0,40},{1000,0,40},{800,0,40}},3},
  {"OverHere",250,{{800,0,60},{1200,0,40},{1400,0,60}},3},
  {"Nextel",250,{{1760,0,30},{1760,0,20},{1760,0,30}},3},
  {"RC210-1",250,{{880,660,100}},1},
  {"RC210-2",250,{{600,0,100}},1},
  {"RC210-3",250,{{1000,0,100}},1},
  {"RC210-4",250,{{697,1477,100}},1},
  {"RC210-5",250,{{941,1477,100}},1},
  {"RC210-6",250,{{300,0,75},{600,0,75},{900,0,75}},3},
  {"RC210-7",250,{{1000,880,100}},1},
  {"RC210-8",250,{{440,660,100}},1},
  {"RC210-9",250,{{1000,0,75},{880,0,75},{1000,0,75}},3},
  {"XP Error",250,{{1200,1205,100},{880,885,100}},2},
  {"XP OK",250,{{440,444,125},{880,884,125}},2}
};


// ====================== AUDIO ========================

/**
 * @brief Inicializa o sistema de áudio I2S para o speaker onboard
 *
 * Configura o driver I2S do ESP32 para reproduzir áudio através do speaker conectado
 * ao GPIO26. O sistema usa DMA para transferência eficiente de dados de áudio.
 *
 * @param rate Taxa de amostragem em Hz (ex: 22050 para courtesy tones)
 *
 * Detalhes da configuração:
 * - MODO: Master + TX (transmissão apenas)
 * - BITS: 16 bits por amostra
 * - CANAIS: Estéreo (RIGHT_LEFT), mas mesmo sinal em ambos
 * - DMA: 8 buffers de 128 amostras cada (prevenção de underrun)
 * - PINO: GPIO26 como saída de dados direta (sem I2S completo)
 *
 * Nota: Se já estava inicializado, remove o driver anterior antes de reconfigurar
 */
void i2s_init(uint32_t rate) {
  // Se já estava inicializado, remove driver anterior
  if (i2s_ok) i2s_driver_uninstall(I2S_NUM_0);

  // Configuração I2S para speaker onboard (GPIO26) - CYD não usa DAC built-in
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),  // Modo mestre + transmissão
    .sample_rate = rate,                                   // Taxa de amostragem (ex: 22050Hz)
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,          // 16 bits por amostra
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,          // Formato estéreo
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,    // Padrão I2S
    .intr_alloc_flags = 0,                                 // Sem alocação especial de interrupção
    .dma_buf_count = 8,                                    // 8 buffers DMA
    .dma_buf_len = 128,                                    // 128 amostras por buffer
    .use_apll = false,                                     // Não usa PLL de áudio
    .tx_desc_auto_clear = true                             // Auto-limpeza de descritores
  };

  // Instala o driver I2S com a configuração especificada
  i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr);

  // Configura pinos I2S para speaker onboard (GPIO26)
  // Nota: Usamos apenas data_out_num porque CYD não tem I2S completo
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_PIN_NO_CHANGE,  // Clock não usado
    .ws_io_num = I2S_PIN_NO_CHANGE,    // Word Select não usado
    .data_out_num = SPEAKER_PIN,       // GPIO26 para speaker
    .data_in_num = I2S_PIN_NO_CHANGE   // Sem entrada
  };
  i2s_set_pin(I2S_NUM_0, &pin_config);

  // Marca sistema como pronto
  i2s_ok = true;
  Serial.printf("I2S inicializado - Speaker em GPIO%d, Taxa: %dHz\n", SPEAKER_PIN, rate);
}

/**
 * @brief Sintetiza e reproduz dois tons simultâneos (dual-tone)
 *
 * Gera uma forma de onda senoidal de áudio combinando duas frequências e
 * reproduz através do sistema I2S. Esta função é usada para criar os
 * courtesy tones da repetidora.
 *
 * @param f1 Frequência principal em Hz (0 se não usar)
 * @param f2 Frequência secundária em Hz (0 se não usar)
 * @param ms Duração do som em milissegundos
 *
 * Funcionamento:
 * 1. Gera ondas senoidais usando função sin()
 * 2. Combina as duas frequências (soma dos valores)
 * 3. Aplica volume (VOLUME global)
 * 4. Converte para 16-bit e duplica para estéreo
 * 5. Envia via DMA para o speaker
 *
 * Nota: Reseta o watchdog a cada buffer para evitar timeout durante
 *       geração de áudio de longa duração
 */
void synthDualTone(uint16_t f1, uint16_t f2, uint32_t ms) {
  if (!ms) return;  // Se duração é zero, sai imediatamente

  const size_t B = 128;        // Tamanho do buffer DMA
  int16_t buf[B];             // Buffer de amostras mono
  uint32_t total = SAMPLE_RATE * ms / 1000UL;  // Total de amostras a gerar

  // Fase inicial e incremento para cada frequência
  double ph1 = 0, ph2 = 0;
  double inc1 = f1 ? 2 * PI * f1 / SAMPLE_RATE : 0;  // Incremento da fase f1
  double inc2 = f2 ? 2 * PI * f2 / SAMPLE_RATE : 0;  // Incremento da fase f2

  // Loop principal: gera e envia amostras em chunks
  for (uint32_t sent = 0; sent < total;) {
    size_t chunk = min(B, (size_t)(total - sent));

    // Gera amostras do chunk atual
    for (size_t i = 0; i < chunk; i++) {
      double s = 0;  // Amostra combinada

      // Gera onda senoidal da frequência 1 (se f1 > 0)
      if (f1) {
        s += sin(ph1);                    // Adiciona onda senoidal
        ph1 += inc1;                      // Avança fase
        if (ph1 > 2 * PI) ph1 -= 2 * PI; // Normaliza fase [0, 2π]
      }

      // Gera onda senoidal da frequência 2 (se f2 > 0)
      if (f2) {
        s += sin(ph2);                    // Adiciona onda senoidal
        ph2 += inc2;                      // Avança fase
        if (ph2 > 2 * PI) ph2 -= 2 * PI; // Normaliza fase [0, 2π]
      }

      // Aplica volume e converte para 16-bit (sinalizado)
      buf[i] = (int16_t)(s * 6000 * VOLUME);
    }

    // Converte para 16-bit não-sinalizado e duplica para estéreo
    uint16_t out[B * 2];
    for (size_t i = 0; i < chunk; i++) {
      uint16_t v = ((buf[i] + 32768) >> 8) << 8;  // 16-bit high-byte only
      out[i * 2] = v;         // Canal esquerdo
      out[i * 2 + 1] = v;     // Canal direito (mesmo sinal)
    }

    // Envia buffer via I2S
    size_t written;
    i2s_write(I2S_NUM_0, out, chunk * 4, &written, portMAX_DELAY);
    sent += chunk;

    // Reseta watchdog para evitar timeout durante geração de áudio
    esp_task_wdt_reset();
  }
}

/**
 * @brief Reproduz o Courtesy Tone (CT) atualmente selecionado
 *
 * Esta função reproduz o courtesy tone selecionado após cada QSO.
 * Cada CT consiste em uma sequência de segmentos de áudio, onde cada
 * segmento pode ter 1 ou 2 frequências tocadas simultaneamente.
 *
 * Fluxo de execução:
 * 1. Marca flag de reprodução ativa
 * 2. Inicializa I2S com a taxa de amostragem configurada
 * 3. Aguarda o delay inicial configurado para o CT
 * 4. Reproduz cada segmento do CT sequencialmente
 * 5. Desinstala driver I2S para liberar recursos
 * 6. Marca reprodução como finalizada
 *
 * Notas:
 * - Usa a variável global ct_index para selecionar qual CT tocar
 * - playing=true impede que outros CTs sejam iniciados
 * - Delay de 15ms entre segmentos cria separação perceptível
 *
 * @see synthDualTone() para geração de áudio de cada segmento
 */
void playCT() {
  // Marca sistema como reproduzindo (previne múltiplas execuções)
  playing = true;

  // Referência ao CT atualmente selecionado
  CT &t = tones[ct_index];

  // Inicializa sistema I2S com taxa de amostragem configurada
  i2s_init(SAMPLE_RATE);

  // Aguarda delay inicial configurado para este CT
  delay(t.delay_ms);

  // Reproduz cada segmento do CT sequencialmente
  for (uint8_t i = 0; i < t.n; i++) {
    // Sintetiza e reproduz o segmento (dual-tone)
    synthDualTone(t.seg[i].f1, t.seg[i].f2, t.seg[i].dur);

    // Pequeno delay entre segmentos para separação perceptível
    delay(15);
  }

  // Desinstala driver I2S para liberar recursos
  i2s_driver_uninstall(I2S_NUM_0);
  i2s_ok = false;

  // Marca reprodução como finalizada
  playing = false;
}

// ====================== LED RGB ========================

/**
 * @brief Converte matiz (hue) HSV para RGB e aplica no LED RGB
 *
 * Esta função converte um valor de matiz (0-360 graus) para componentes RGB
 * e aplica os valores ao LED RGB usando PWM. A conversão usa o modelo HSV
 * com saturação e valor fixos em 1.0 (cor e brilho máximos).
 *
 * @param h Matiz (hue) em graus, variando de 0 a 360
 *         0 = Vermelho, 120 = Verde, 240 = Azul, etc.
 *
 * Algoritmo de conversão:
 * 1. Divide o círculo de cores em 6 segmentos de 60 graus cada
 * 2. Calcula valor c (chroma) e x (valor secundário)
 * 3. Determina componente baseado no segmento de cor
 * 4. Adiciona offset (m) para ajustar brilho
 * 5. Converte para 0-255 e aplica via PWM
 *
 * Nota importante: LED usa anodo comum, então valores são invertidos
 * (255 - valor) porque LOW = acende, HIGH = apaga
 *
 * @see updateLED() para controle de estados do LED
 */
void setColorFromHue(float h) {
  // h de 0 a 360 graus (matiz)
  // sat=1 (saturação máxima), val=1 (brilho máximo)
  float c = 1.0;  // Chroma (intensidade da cor)
  float x = c * (1 - abs(fmod(h / 60.0, 2) - 1));  // Valor intermediário
  float m = 0;    // Offset para ajuste de brilho

  // Componentes RGB (serão calculados)
  float r, g, b;

  // Algoritmo HSV→RGB: Divide o espectro em 6 segmentos de 60°
  if (h < 60) {
    // 0-60: Vermelho para Amarelo
    r = c; g = x; b = 0;
  } else if (h < 120) {
    // 60-120: Amarelo para Verde
    r = x; g = c; b = 0;
  } else if (h < 180) {
    // 120-180: Verde para Ciano
    r = 0; g = c; b = x;
  } else if (h < 240) {
    // 180-240: Ciano para Azul
    r = 0; g = x; b = c;
  } else if (h < 300) {
    // 240-300: Azul para Magenta
    r = x; g = 0; b = c;
  } else {
    // 300-360: Magenta para Vermelho
    r = c; g = 0; b = x;
  }

  // Converte valores normalizados (0-1) para 0-255
  int red   = (int)((r + m) * 255);
  int green = (int)((g + m) * 255);
  int blue  = (int)((b + m) * 255);

  // Aplica valores ao LED via PWM
  // INVERTE valores porque é anodo comum (255 = apagado, 0 = full brilho)
  if (ledc_channel_r >= 0) ledcWrite(ledc_channel_r, 255 - red);
  if (ledc_channel_g >= 0) ledcWrite(ledc_channel_g, 255 - green);
  if (ledc_channel_b >= 0) ledcWrite(ledc_channel_b, 255 - blue);
}

/**
 * @brief Atualiza o LED RGB baseado no estado atual da repetidora
 *
 * Esta função controla a cor e o comportamento do LED RGB de acordo
 * com o estado da repetidora (TX ativo, RX ativo, ou idle).
 *
 * Estados implementados:
 * 1. TX Ativo (ptt_state = true):
 *    - Cor: Vermelho fixo (sólido)
 *    - Comportamento: Sem animação
 *    - Uso: Indica que está transmitindo
 *
 * 2. RX Ativo (cor_stable = true && ptt_state = false):
 *    - Cor: Amarelo com efeito breathing
 *    - Comportamento: Pulsante (breathing effect)
 *    - Uso: Indica que está recebendo sinal
 *
 * 3. Idle (cor_stable = false && ptt_state = false):
 *    - Cor: Rainbow suave
 *    - Comportamento: Ciclo contínuo de cores
 *    - Uso: Indica que está em espera
 *
 * Nota: Esta função deve ser chamada continuamente no loop principal
 *       para manter animações atualizadas
 *
 * @see setColorFromHue() para conversão de cores
 * @see loop() onde esta função é chamada
 */
void updateLED() {
  if (ledc_channel_r < 0 || ledc_channel_g < 0 || ledc_channel_b < 0) return;  // Não inicializado
  
  if (ptt_state) {
    // TX ativo: Vermelho fixo
    led_rainbow_enabled = false;
    ledcWrite(ledc_channel_r, 0);    // Vermelho full
    ledcWrite(ledc_channel_g, 255);  // Verde apagado
    ledcWrite(ledc_channel_b, 255);  // Azul apagado
  } else if (cor_stable) {
    // RX ativo: Amarelo pulsante (breathing)
    led_rainbow_enabled = false;
    // Breathing effect usando seno
    float brightness = (sin(millis() / 500.0) + 1.0) / 2.0;  // 0 a 1
    int val = (int)(brightness * 255);
    ledcWrite(ledc_channel_r, 255 - val);    // Vermelho pulsante
    ledcWrite(ledc_channel_g, 255 - val);    // Verde pulsante
    ledcWrite(ledc_channel_b, 255);          // Azul apagado
  } else {
    // Idle: Rainbow suave
    led_rainbow_enabled = true;
  }
}

// ====================== DISPLAY ========================

/**
 * @brief Atualiza APENAS o campo de uptime no display (otimização)
 *
 * Esta função atualiza apenas o texto de uptime no rodapé do display,
 * sem redesenhar a tela inteira. Isso evita flicker e melhora
 * significativamente o desempenho, pois o uptime é atualizado a cada 5s.
 *
 * Funcionamento:
 * 1. Calcula o uptime atual (horas e minutos)
 * 2. Compara com a string anterior
 * 3. Só redesenha se o uptime mudou (evita flicker)
 * 4. Apaga o texto antigo usando texto preto sobre fundo preto
 * 5. Escreve o novo uptime na cor ciano
 *
 * Otimizações:
 * - Não usa fillRect (apaga com espaços em texto preto)
 * - Só atualiza quando o valor muda (comparação de strings)
 * - Usa snprintf para formatação eficiente
 *
 * @see loop() onde esta função é chamada a cada 5 segundos
 */
void updateUptimeOnly() {
  int16_t W = tft.width();
  int16_t footer_y = 195;  // Mesma posição do rodapé
  
  // Calcula uptime
  unsigned long uptime_sec = millis() / 1000;
  unsigned long uptime_h = uptime_sec / 3600;
  unsigned long uptime_m = (uptime_sec % 3600) / 60;
  
  // Gera string do uptime atual
  char uptime_str[16];
  snprintf(uptime_str, sizeof(uptime_str), "%02luh%02lum", uptime_h, uptime_m);
  
  // Compara com string anterior - só atualiza se mudou (evita flicker)
  if (strcmp(uptime_str, old_uptime_str) != 0) {
    // Apaga texto antigo com espaços (mais rápido que fillRect)
    tft.setTextColor(TFT_BLACK, TFT_BLACK);  // Texto preto sobre fundo preto = apaga
    tft.setTextSize(2);
    tft.setCursor(W / 2 - 40, footer_y + 15);
    tft.print("        ");  // Apaga com espaços
    
    // Desenha novo uptime
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(W / 2 - 40, footer_y + 15);
    tft.print(uptime_str);
    
    // Atualiza string anterior
    strncpy(old_uptime_str, uptime_str, sizeof(old_uptime_str) - 1);
    old_uptime_str[sizeof(old_uptime_str) - 1] = '\0';
  }
}

/**
 * @brief Desenha o layout inicial do display (chamado apenas uma vez)
 *
 * Esta função configura o layout inicial do display com header, áreas de
 * status e rodapé. É chamada apenas uma vez no setup() para evitar
 * flicker desnecessário.
 *
 * Estrutura do layout:
 * 1. Header (topo): Callsign em destaque sobre fundo azul escuro
 * 2. Área de status (centro): Caixa grande para mostrar RX/TX
 * 3. Rodapé (inferior): Estatísticas em 3 colunas
 *
 * Características:
 * - Usa fillRoundRect() para bordas arredondadas (design moderno)
 * - Cor de fundo preto para contraste
 * - Header azul escuro (TFT_DARKBLUE) com texto amarelo
 * - Linhas separadoras em cinza escuro
 *
 * Nota: Após chamar esta função, use updateDisplay() para atualizar
 *       o conteúdo dinâmico sem redesenhar o layout completo
 *
 * @see updateDisplay() para atualização dinâmica do conteúdo
 * @see setup() onde esta função é chamada
 */
void drawLayout() {
  int16_t W = tft.width();
  int16_t H = tft.height();

  // Debug: imprime dimensões reais
  Serial.printf("Display: W=%d, H=%d\n", W, H);

  // Fundo gradiente escuro (preto sólido por enquanto)
  tft.fillScreen(TFT_BLACK);
  
  // Header - Callsign grande no centro (altura aumentada para evitar corte)
  int16_t header_height = 60;  // Aumentado de 50 para 60px
  tft.fillRoundRect(0, 0, W, header_height, 5, TFT_DARKBLUE);  // Fundo azul escuro com bordas arredondadas
  tft.drawRoundRect(0, 0, W, header_height, 5, TFT_WHITE);  // Borda branca sutil
  
  // Callsign centralizado e bem posicionado
  tft.setTextColor(TFT_YELLOW, TFT_DARKBLUE);
  tft.setTextSize(4);
  // Calcula largura do texto para centralização precisa
  int16_t text_x = (W - tft.textWidth(CALLSIGN)) / 2;
  int16_t text_y = (header_height - 24) / 2;  // Centraliza verticalmente (fonte size 4 = ~24px)
  tft.setCursor(text_x, text_y);
  tft.print(CALLSIGN);
  
  // Linha separadora
  tft.drawFastHLine(5, header_height, W - 10, TFT_DARKGREY);
  
  // Limpa área do status principal (ajustado para novo header)
  tft.fillRect(5, 65, W - 10, 90, TFT_BLACK);
  
  // Limpa área de estatísticas (rodapé)
  tft.fillRect(5, 160, W - 10, 80, TFT_BLACK);
}

/**
 * @brief Atualiza o display com o estado atual da repetidora (otimizado)
 *
 * Esta função atualiza o conteúdo do display refletindo o estado atual
 * da repetidora. Usa várias otimizações para evitar flicker e melhorar
 * desempenho.
 *
 * Estrutura da atualização:
 * 1. Header (sempre redesenhado para sincronização)
 * 2. Status principal (só se mudou)
 * 3. Courtesy Tone (só se mudou)
 * 4. Rodapé (só na primeira vez)
 * 5. Barra de progresso (só quando TX ativo)
 *
 * Otimizações implementadas:
 * - Throttle: Mínimo 250ms entre atualizações
 * - Atualização parcial: Só redesenha áreas que mudaram
 * - Flags de controle: first_draw, needsFullRedraw
 * - Cache de valores: last_status_bg, last_ct_index
 * - Comparação de strings: Só atualiza uptime se mudou
 *
 * Estados visuais:
 * - EM ESCUTA: Fundo verde escuro
 * - RX ATIVO: Fundo amarelo
 * - TX ATIVO: Fundo vermelho
 *
 * Nota: Uptime é atualizado por updateUptimeOnly() separadamente
 *
 * @see loop() onde esta função é chamada
 * @see updateUptimeOnly() para atualização do uptime
 */
void updateDisplay() {
  // Atualização apenas quando necessário (mudança de estado, touch, etc.)
  // Uptime é atualizado separadamente por updateUptimeOnly() - SEM refresh da tela
  unsigned long currentMillis = millis();
  
  // Throttle: evita updates muito frequentes (mínimo 250ms entre updates)
  // Mas permite primeira atualização sempre (quando last_display_update == 0)
  if (last_display_update != 0 && currentMillis - last_display_update < 250) return;
  last_display_update = currentMillis;
  
  // #region agent log
  debugLog("updateDisplay:entry", "Function called", "F", millis());
  // #endregion
  
    int16_t W = tft.width();
    int16_t H = tft.height();
    
    // #region agent log
    debugLog("updateDisplay:dims", "Dimensions", "D", W, H, 0);
    // #endregion
    
    // Captura estado de redraw completo para usar em toda a função
    bool isFullRedraw = first_draw || needsFullRedraw;
  
    // Primeira renderização: limpa tela completa (evita flash)
    if (isFullRedraw) {
      tft.fillScreen(TFT_BLACK);
      Serial.printf("updateDisplay() - Renderização completa - W=%d, H=%d\n", W, H);
    }
    
    // ========== HEADER - Redesenha SEMPRE para sincronizar (evita flash/separado) ==========
    int16_t header_height = 60;
    tft.fillRect(0, 0, W, header_height, TFT_DARKBLUE);  // Relimpa SÓ header
    tft.drawRoundRect(0, 0, W, header_height, 5, TFT_WHITE);  // Borda branca
    
    // Callsign centralizado e bem posicionado (redesenha sempre)
    tft.setTextColor(TFT_YELLOW, TFT_DARKBLUE);
    tft.setTextSize(4);
    int16_t textW = tft.textWidth(CALLSIGN);
    int16_t text_x = (W - textW) / 2;
    int16_t text_y = (header_height - 24) / 2;  // Centraliza verticalmente
    tft.setCursor(text_x, text_y);
    tft.print(CALLSIGN);  // Redesenha sempre junto
    
    // Linha separadora
    tft.drawFastHLine(5, header_height, W - 10, TFT_DARKGREY);
    
    // ========== STATUS PRINCIPAL (Centro, caixa grande) ==========
    uint16_t status_bg, status_text_color;
    const char* status_text;
    
    if (ptt_state) {
      status_bg = TFT_RED;
      status_text_color = TFT_WHITE;
      status_text = "TX ATIVO";
    } else if (cor_stable) {
      status_bg = TFT_YELLOW;
      status_text_color = TFT_BLACK;
      status_text = "RX ATIVO";
    } else {
      status_bg = TFT_DARKGREEN;
      status_text_color = TFT_WHITE;
      status_text = "EM ESCUTA";
    }
    
    // Caixa de status com bordas arredondadas (ajustado para header de 60px)
    int16_t status_y = 65;  // Ajustado de 55 para 65 (header agora é 60px)
    static uint16_t last_status_bg = 0xFFFF;  // Para detectar mudança de status
    
    // Só redesenha status se mudou (evita flicker)
    if (status_bg != last_status_bg || isFullRedraw) {
      tft.fillRoundRect(10, status_y, W - 20, 85, 10, status_bg);  // Raio aumentado para 10
      tft.drawRoundRect(10, status_y, W - 20, 85, 10, TFT_WHITE);
      last_status_bg = status_bg;
    }
    
    // Texto de status grande
    tft.setTextColor(status_text_color, status_bg);
    tft.setTextSize(3);
    tft.drawCentreString(status_text, W / 2, status_y + 20, 3);
    
    // Se TX, mostra contador de QSO atual
    if (ptt_state) {
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE, status_bg);
      tft.drawCentreString("QSO ATUAL", W / 2, status_y + 55, 2);
    } else {
      // Limpa área "QSO ATUAL" se não está em TX (evita texto fantasma)
      tft.fillRect(W/2 - 50, status_y + 50, 100, 25, status_bg);
    }
  
    // ========== COURTESY TONE (Abaixo do status) ==========
    int16_t ct_y = 155;  // Ajustado de 145 para 155
    static uint8_t last_ct_index = 255;  // Para detectar mudança
    
    // Só redesenha CT se mudou ou primeira vez
    if (ct_index != last_ct_index || isFullRedraw) {
      tft.fillRoundRect(10, ct_y, W - 20, 35, 5, TFT_DARKGREEN);  // Caixa verde com bordas arredondadas    
      tft.drawRoundRect(10, ct_y, W - 20, 35, 5, TFT_CYAN);  // Borda ciano
      last_ct_index = ct_index;
    }
    
    tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    tft.setTextSize(2);
    tft.setCursor(20, ct_y + 8);
    tft.print("CT: ");
    tft.setTextColor(TFT_YELLOW, TFT_DARKGREEN);
    tft.print(tones[ct_index].name);
    
    // Número do CT (direita)
    char ct_buf[12];
    snprintf(ct_buf, sizeof(ct_buf), "%02d/33", ct_index + 1);
    tft.setTextColor(TFT_CYAN, TFT_DARKGREEN);
    tft.setTextSize(2);
    tft.setCursor(W - 70, ct_y + 8);
    tft.print(ct_buf);
  
    // ========== ESTATÍSTICAS (Rodapé, 3 colunas) ==========
    int16_t footer_y = 195;  // Ajustado para compensar header maior
    int16_t footer_h = 45;
  
    // Limpa rodapé APENAS na primeira vez (evita flicker)
    static bool footer_cleared = false;
    if (!footer_cleared || isFullRedraw) {
      tft.fillRect(5, footer_y, W - 10, footer_h, TFT_BLACK);
      footer_cleared = true;
    }
  
    // Coluna 1: QSOs
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, footer_y + 5);
    tft.print("QSOs:");
    tft.setTextSize(2);
    tft.setCursor(10, footer_y + 15);
    tft.printf("%04d", qso_count);
  
    // Coluna 2: Uptime (centro) - Label e valor inicial apenas
    // O valor será atualizado pela função updateUptimeOnly() separadamente (sem refresh da tela)        
    static bool uptime_label_drawn = false;
    if (!uptime_label_drawn || isFullRedraw) {
      // Label "Uptime:"
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextSize(1);
      tft.setCursor(W/2 - 30, footer_y + 5);
      tft.print("Uptime:");
  
      // Valor inicial do uptime (será atualizado depois por updateUptimeOnly())
      unsigned long uptime_sec = millis() / 1000;
      unsigned long uptime_h = uptime_sec / 3600;
      unsigned long uptime_m = (uptime_sec % 3600) / 60;
      char uptime_str[16];
      snprintf(uptime_str, sizeof(uptime_str), "%02luh%02lum", uptime_h, uptime_m);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setTextSize(2);
      tft.setCursor(W/2 - 40, footer_y + 15);
      tft.print(uptime_str);
      strncpy(old_uptime_str, uptime_str, sizeof(old_uptime_str) - 1);
      old_uptime_str[sizeof(old_uptime_str) - 1] = '\0';
      uptime_label_drawn = true;
    }
  
    // Coluna 3: CT Index (direita)
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(W - 50, footer_y + 5);
    tft.print("CT:");
    tft.setTextSize(2);
    tft.setCursor(W - 50, footer_y + 15);
    tft.printf("%02d", ct_index + 1);
  
    // Linha separadora no rodapé
    tft.drawFastHLine(5, footer_y - 2, W - 10, TFT_DARKGREY);
  
    // ========== BARRA DE PROGRESSO (se TX) ==========
    if (ptt_state) {
      // Barra verde → laranja → vermelho conforme tempo
      int16_t bar_x = 10;
      int16_t bar_y = footer_y + 40;
      int16_t bar_w = W - 20;
      int16_t bar_h = 8;
  
      // Fundo da barra
      tft.fillRect(bar_x, bar_y, bar_w, bar_h, TFT_DARKGREY);
  
      // Progresso (simulado - pode usar tempo real do QSO)
      int16_t progress = (qso_count % 100);  // Exemplo
      int16_t progress_w = (progress * bar_w) / 100;
      uint16_t bar_color = (progress < 33) ? TFT_GREEN : (progress < 66) ? TFT_ORANGE : TFT_RED;
      tft.fillRect(bar_x, bar_y, progress_w, bar_h, bar_color);
    }
    
    // Limpa flags de redraw apenas no final
    if (first_draw) first_draw = false;
    if (needsFullRedraw) needsFullRedraw = false;
    
    // #region agent log
    debugLog("updateDisplay:exit", "Function exit", "F", 0, 0, 0);
    // #endregion
}

// ====================== SETUP ========================

/**
 * @brief Função de inicialização do sistema (executada uma vez no boot)
 *
 * Esta função configura todos os componentes da repetidora:
 * - Comunicação serial para debug
 * - Sistema de arquivos SPIFFS para logs
 * - Watchdog do sistema
 * - Backlight do display
 * - Display TFT com rotação correta
 * - Touchscreen XPT2046
 * - Pinos de hardware (COR, PTT)
 * - LED RGB com PWM
 * - Layout inicial do display
 *
 * Fluxo de inicialização:
 * 1. Inicializa Serial (115200 baud)
 * 2. Configura SPIFFS para logs em arquivo
 * 3. Inicializa watchdog (30s timeout)
 * 4. Configura backlight (HIGH = ON)
 * 5. Inicializa display TFT
 * 6. Aplica rotação 3 (landscape horizontal)
 * 7. Inverte display (se necessário para correção de cores)
 * 8. Inicializa touchscreen
 * 9. Configura pinos COR/PTT
 * 10. Configura LED RGB com PWM
 * 11. Desenha layout inicial
 *
 * Debug logging:
 * - Logs de memória heap antes/depois de cada componente
 * - Logs de eventos importantes em /debug.log (NDJSON)
 * - Mensagens no Serial para monitoramento
 *
 * @see loop() para o loop principal
 */
void setup() {
  Serial.begin(115200);
  delay(1000);  // Aguarda Serial estar pronto
  Serial.println("\n\n=== INICIALIZACAO REPETIDORA ===");

  // #region agent log - Inicialização SPIFFS para logs
  Serial.println("Inicializando SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("ERRO: SPIFFS falhou ao inicializar!");
  } else {
    Serial.println("SPIFFS inicializado com sucesso");
    // Limpa log anterior ao iniciar
    if (SPIFFS.exists("/debug.log")) {
      SPIFFS.remove("/debug.log");
      Serial.println("Log anterior removido");
    }
  }

  // Testa escrita no log
  File testFile = SPIFFS.open("/debug.log", FILE_WRITE);
  if (testFile) {
    testFile.println("TESTE DE ESCRITA");
    testFile.close();
    Serial.println("Teste de escrita SPIFFS OK");
  } else {
    Serial.println("ERRO: Não foi possível criar arquivo de teste");
  }
  // #endregion

  // #region agent log - Boot count
  Serial.printf("FREE HEAP: %d bytes, TOTAL HEAP: %d bytes\n", ESP.getFreeHeap(), ESP.getHeapSize());
  logToFile("A", "BOOT_START", millis(), ESP.getFreeHeap(), ESP.getHeapSize());
  Serial.println("LogToFile chamado com sucesso");
  // #endregion

  esp_task_wdt_config_t wdt = {
    .timeout_ms = WDT_TIMEOUT_SECONDS * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = false  // False permite reinicialização limpa ao invés de pânico
  };
  esp_task_wdt_init(&wdt);
  esp_task_wdt_add(NULL);

  // Inicializa backlight PRIMEIRO
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);
  Serial.println("Backlight: ON");

  // #region agent log - Teste A: Heap após backlight
  logToFile("A", "BACKLIGHT_ON", millis(), ESP.getFreeHeap(), 0, 0);
  Serial.printf("Heap após backlight: %d bytes\n", ESP.getFreeHeap());
  // #endregion

  // Inicializa display
  Serial.println("Inicializando TFT...");
  // #region agent log - Antes do TFT init
  logToFile("E", "BEFORE_TFT_INIT", millis(), ESP.getFreeHeap(), 0, 0);
  // #endregion

  tft.init();
  Serial.printf("TFT init() concluido\n");

  // #region agent log - Após TFT init
  logToFile("E", "AFTER_TFT_INIT", millis(), ESP.getFreeHeap(), tft.width(), tft.height());
  Serial.printf("Heap após TFT init: %d bytes\n", ESP.getFreeHeap());
  // #endregion
  
  // Inversão já configurada no User_Setup.h (TFT_INVERSION_ON)
  // Não precisa chamar invertDisplay() aqui se já está no User_Setup.h
  Serial.println("Display inversion: Configurado no User_Setup.h");
  
  // #region agent log
  debugLog("setup:afterInit", "After tft.init()", "COLORS", 0, 0, 0);
  // #endregion
  
  // Verifica dimensões ANTES da rotação
  Serial.printf("ANTES setRotation: W=%d, H=%d\n", tft.width(), tft.height());
  
  // Aplica rotação 3 - Deixa HORIZONTAL (landscape 320x240) no CYD
  Serial.println("Aplicando setRotation(3)...");
  tft.setRotation(3);
  delay(100);  // Pequeno delay para garantir que rotação foi aplicada
  
  // Verifica dimensões APÓS rotação
  int16_t w = tft.width();
  int16_t h = tft.height();
  Serial.printf("DEPOIS setRotation(3): W=%d, H=%d\n", w, h);
  
  // #region agent log
  debugLog("setup:rotation", "Rotation applied", "ROTATION", 3, w, h);
  // #endregion
  
  // Testa inversão de display (ajuste se cores ficarem erradas)
  // Com ILI9341_2_DRIVER, geralmente precisa invertDisplay(true)
  tft.invertDisplay(true);  // Testa true primeiro (comum no CYD)
  Serial.println("invertDisplay(true) aplicado - Se cores ficarem erradas, mude para false");
  
  // Se ainda estiver vertical, testa outras rotações
  if (h > w) {
    Serial.println("AVISO: Display ainda parece vertical (H > W)");
    Serial.println("Testando outras rotações (0, 1, 2)...");
    
    for (uint8_t rot = 0; rot < 4; rot++) {
      if (rot == 3) continue;  // Já testamos 3
      tft.setRotation(rot);
      delay(100);
      int16_t test_w = tft.width();
      int16_t test_h = tft.height();
      Serial.printf("Rotacao %d: W=%d, H=%d\n", rot, test_w, test_h);
      
      // Se encontrar uma rotação horizontal (W > H), usa ela
      if (test_w > test_h) {
        Serial.printf("Encontrada rotação horizontal: %d\n", rot);
        w = test_w;
        h = test_h;
        break;
      }
    }
  } else {
    Serial.println("OK: Display está em modo HORIZONTAL (W > H)");
  }

  // Limpeza inicial única - sem testes visuais para evitar flash
  tft.fillScreen(TFT_BLACK);
  Serial.println("Tela inicializada - sem flash de teste");

  ts.begin();
  Serial.println("Touchscreen inicializado");

  pinMode(PIN_COR, INPUT_PULLUP);
  pinMode(PIN_PTT, OUTPUT);
  digitalWrite(PIN_PTT, LOW);
  Serial.printf("GPIOs configurados - COR: GPIO%d, PTT: GPIO%d\n", PIN_COR, PIN_PTT);
  Serial.printf("Speaker: GPIO%d\n", SPEAKER_PIN);

  // ========== CONFIGURAÇÃO DO LED RGB ==========
  // O LED RGB é controlado via PWM para permitir transições suaves de cores
  //
  // Especificações do LED RGB:
  // - Tipo: Anodo Comum (LOW acende, HIGH apaga)
  // - Pinos: R=GPIO4, G=GPIO16, B=GPIO17
  // - Controle: PWM via LEDC (LED Control) do ESP32
  // - Frequência: 5kHz (boa frequência para evitar flicker visível)
  // - Resolução: 8 bits (valores de 0-255)
  //
  // Observação: No ESP32, ledcAttach() configura o canal PWM automaticamente
  //             e retorna o número do canal atribuído, então não precisa pinMode()
  ledc_channel_r = ledcAttach(PIN_LED_R, 5000, 8);  // Red: freq 5kHz, 8 bits (0-255)
  ledc_channel_g = ledcAttach(PIN_LED_G, 5000, 8);  // Green
  ledc_channel_b = ledcAttach(PIN_LED_B, 5000, 8);  // Blue

  // Inicializa LED apagado (anodo comum: 255 = apagado)
  // Isso é importante porque no boot o LED não deve estar aceso
  ledcWrite(ledc_channel_r, 255);
  ledcWrite(ledc_channel_g, 255);
  ledcWrite(ledc_channel_b, 255);

  Serial.printf("LED RGB configurado (PWM) - Canais: R=%d, G=%d, B=%d\n",
                ledc_channel_r, ledc_channel_g, ledc_channel_b);

  Serial.println("Desenhando layout...");
  // #region agent log - Antes do drawLayout
  logToFile("C", "BEFORE_LAYOUT", millis(), ESP.getFreeHeap(), 0, 0);
  // #endregion

  // drawLayout(); // Removido: updateDisplay() ja gerencia o layout completo
  updateDisplay();

  // #region agent log - Após drawLayout
  logToFile("C", "AFTER_LAYOUT", millis(), ESP.getFreeHeap(), tft.width(), tft.height());
  // #endregion

  Serial.println("=== INICIALIZACAO CONCLUIDA ===\n");

  // #region agent log - Setup completo
  logToFile("D", "SETUP_COMPLETE", millis(), ESP.getFreeHeap(), ESP.getHeapSize(), 0);
  Serial.printf("Setup finalizado - Heap livre: %d / %d bytes\n", ESP.getFreeHeap(), ESP.getHeapSize());
  // #endregion
}

// ====================== LOOP =========================

/**
 * @brief Loop principal da repetidora (executado continuamente)
 *
 * Esta é a função principal que roda continuamente gerenciando
 * todas as operações da repetidora. É chamada repetidamente
 * pelo framework Arduino após o setup() ser concluído.
 *
 * Responsabilidades principais:
 * 1. Contagem de loops e estatísticas (debug)
 * 2. Reset do watchdog a cada iteração
 * 3. Monitoramento de uptime (detecta resets)
 * 4. Leitura do pino COR (squelch detection)
 * 5. Controle de PTT baseado no COR
 * 6. Reprodução de courtesy tones
 * 7. Tratamento de touchscreen
 * 8. Atualização de display (otimizada)
 * 9. Atualização de LED RGB (feedback visual)
 * 10. Atualização de uptime (a cada 5s)
 *
 * Fluxo de operação:
 * - COR ativa → PTT ON → Repete áudio → COR inativa → PTT OFF → Toca CT
 * - Touchscreen → Muda CT → Atualiza display
 * - Loop contínuo → Atualiza LED e uptime
 *
 * Otimizações:
 * - Throttle de atualização de display (250ms)
 * - Debounce de touchscreen (500ms)
 * - Atualização parcial de display (só uptime muda a cada 5s)
 * - Reset de watchdog a cada loop (prevenção de timeout)
 *
 * @see setup() para inicialização do sistema
 * @see updateDisplay() para atualização do display
 * @see updateLED() para controle do LED RGB
 */
void loop() {
  static unsigned long loopCount = 0;
  static unsigned long lastLoopLog = 0;
  static unsigned long firstLoopTime = 0;

  loopCount++;

  // Marca primeiro loop
  if (firstLoopTime == 0) {
    firstLoopTime = millis();
    Serial.printf("=== PRIMEIRO LOOP INICIADO em %lu ms ===\n", firstLoopTime);
  }

  // #region agent log - Loop stats (a cada 5s - reduzido para detectar problemas mais rápido)
  if (millis() - lastLoopLog >= 5000) {
    lastLoopLog = millis();
    Serial.printf("Loop: count=%lu, heap=%d, uptime=%lums\n", loopCount, ESP.getFreeHeap(), millis());
    logToFile("D", "LOOP_STATS", millis(), loopCount, ESP.getFreeHeap(), 0);
  }
  // #endregion

  // Reseta watchdog a cada iteração (previne timeout)
  esp_task_wdt_reset();

  // Monitoramento de uptime - detecta resets anormais
  // #region agent log - Uptime monitoring
  static unsigned long lastUptime = 0;
  unsigned long currentUptime = millis();
  if (currentUptime < lastUptime) {
    // Uptime resetou detectado!
    logToFile("D", "Uptime_RESET_DETECTED", currentUptime, lastUptime, ESP.getFreeHeap());
    Serial.printf("ALERTA: Uptime resetou! Anterior: %lu, Atual: %lu\n", lastUptime, currentUptime);
  }
  lastUptime = currentUptime;
  // #endregion

  // ========== CONTROLE DE COR (SQUELCH DETECTION) ==========
  // Lê o pino COR (LOW = sinal detectado)
  bool cor = (digitalRead(PIN_COR) == LOW);

  // Se o estado do COR mudou, trata a transição
  if (cor != cor_stable) {
    // #region agent log
    logToFile("B", "COR_CHANGED", millis(), cor, cor_stable, ESP.getFreeHeap());
    Serial.printf("COR mudou: %d -> %d\n", cor_stable, cor);
    // #endregion
    cor_stable = cor;
    needsFullRedraw = true;  // Marca para redraw completo

    if (cor_stable) {
      // COR ativado → INÍCIO DO QSO → PTT ON
      ptt_state = true;
      digitalWrite(PIN_PTT, HIGH);  // Ativa transmissão
    } else {
      // COR desativado → FIM DO QSO → HANG TIME → CT → PTT OFF
      delay(HANG_TIME_MS);        // Aguarda hang time (600ms)
      if (!playing) playCT();     // Reproduz courtesy tone (se não estiver tocando)
      digitalWrite(PIN_PTT, LOW); // Desativa transmissão
      ptt_state = false;
      qso_count++;               // Incrementa contador de QSOs
    }

    // Atualiza display após mudança de estado
    // #region agent log
    logToFile("C", "BEFORE_UPDATE_DISPLAY", millis(), ESP.getFreeHeap(), 0, 0);
    // #endregion
    updateDisplay();
    // #region agent log
    logToFile("C", "AFTER_UPDATE_DISPLAY", millis(), ESP.getFreeHeap(), 0, 0);
    // #endregion
  }

  // ========== CONTROLE DE TOUCHSCREEN ==========
  // Verifica se houve toque na tela
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    // Serial.printf("Touch raw: x=%d, y=%d, z=%d\n", p.x, p.y, p.z); // Removido para evitar flood

    // Filtra touch com coordenadas inválidas (valores muito altos indicam touch falso)
    if (p.z > 600 && p.x < 8000 && p.y < 8000) {
      // #region agent log
      logToFile("C", "TOUCH_DETECTED", millis(), p.x, p.y, p.z);
      Serial.printf("Touch válido detectado: x=%d, y=%d, z=%d\n", p.x, p.y, p.z);
      // #endregion

      // Avança para o próximo courtesy tone (circular)
      ct_index = (ct_index + 1) % N_CT;
      needsFullRedraw = true;  // Marca para redraw completo
      updateDisplay();          // Atualiza display mostrando novo CT

      // Debounce: Delay para evitar troca rápida acidental
      delay(500);

      // Espera soltar o dedo (evita múltiplas detecções)
      while (ts.touched()) {
        delay(10);
      }
    }
  }
  
  // ========== ATUALIZAÇÃO DE UPTIME ==========
  // Atualiza o uptime a cada 5 segundos SEM redesenhar tela inteira
  unsigned long currentMillis = millis();
  if (currentMillis - last_uptime_update >= 5000) {  // A cada 5s
    last_uptime_update = currentMillis;

    // #region agent log - Uptime update
    logToFile("D", "UPTIME_UPDATE", currentMillis, ESP.getFreeHeap(), 0, 0);
    // #endregion

    updateUptimeOnly();  // Atualiza APENAS o uptime, sem redesenhar tela
  }

  // ========== CONTROLE DO LED RGB ==========
  updateLED();  // Atualiza LED baseado no status (TX/RX/Idle)

  // Se rainbow habilitado (estado idle), atualiza cor continuamente
  if (led_rainbow_enabled) {
    unsigned long currentMillisLED = millis();
    if (currentMillisLED - previousMillisLED >= intervalLED) {
      previousMillisLED = currentMillisLED;
      hue += 1.0;  // Velocidade do rainbow (aumenta para mais rápido)
      if (hue >= 360) hue = 0;  // Reinicia ciclo ao atingir 360°
      setColorFromHue(hue);  // Aplica nova cor ao LED
    }
  }
}
