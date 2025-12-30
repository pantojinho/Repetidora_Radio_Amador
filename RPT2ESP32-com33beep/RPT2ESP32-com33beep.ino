#include <Arduino.h>
#include <LittleFS.h>
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
// SISTEMA DE LED RGB IMPLEMENTADO (v2.2):
// ========================================
// O LED RGB funciona como indicador visual do estado da repetidora em tempo real.
//
// CONFIGURAÇÃO HARDWARE:
// - Pinos: GPIO4 (R), GPIO16 (G), GPIO17 (B)
// - Tipo: ACTIVE LOW (LOW = acende, HIGH = apaga) - conforme ESP32-2432S028R
// - Controle: PWM via LEDC do ESP32 (freq=5kHz, 8 bits)
// - IMPORTANTE: Valores são invertidos (255 - valor) porque é active low
//
// ESTADOS DO LED (correspondem às cores do display):
// 1. TRANSMITINDO (TX ativo):
//    - Cor: VERMELHO FIXO (mesma cor do display vermelho)
//    - Pino R: 0 (acende - active low)
//    - Pino G: 255 (apagado)
//    - Pino B: 255 (apagado)
//    - Animação: Nenhuma (cor sólida)
//
// 2. RECEBENDO (COR ativo, RX):
//    - Cor: AMARELO (mesma cor do display amarelo)
//    - Pino R: 0 (acende - active low)
//    - Pino G: 0 (acende - active low)
//    - Pino B: 255 (apagado)
//    - Animação: Pode ser fixo ou pulsante (conforme padrão original)
//
// 3. ESPERA/IDLE (sem sinal):
//    - Cor: VERDE (mesma cor do display verde escuro)
//    - Pino R: 255 (apagado)
//    - Pino G: 0 (acende - active low)
//    - Pino B: 255 (apagado)
//    - Animação: Cor fixa verde (não rainbow)
//
// FUNÇÕES DO LED RGB:
// - updateLED(): Atualiza LED baseado no estado atual (TX/RX/Idle)
// - Cores fixas: Verde (idle), Amarelo (RX), Vermelho (TX)
//
// UTILIDADE PRÁTICA:
// - Feedback visual instantâneo sem precisar olhar para o display
// - Vermelho fixo: Indica transmissão ativa (evite falar)
// - Amarelo fixo: Alguém transmitindo no canal (RX ativo)
// - Verde fixo: Canal livre, repetidora em espera (idle)
//
// SISTEMA DE IDENTIFICAÇÃO AUTOMÁTICA (ID VOZ/CW):
// =================================================
// A repetidora se identifica automaticamente em intervalos regulares:
//
// IDENTIFICAÇÃO INICIAL (apenas uma vez no boot):
// ===============================================
// Ao ligar a placa pela primeira vez, são realizadas duas identificações:
//
// 1. ID Inicial em Voz:
//    - Timing: Imediatamente após o setup (aguarda 2 segundos)
//    - Formato: Arquivo WAV com indicativo da repetidora
//    - Ativado: Sempre no primeiro boot
//    - Display: Mostra "TX VOZ" + "INDICATIVO VOZ" com fundo vermelho
//
// 2. ID Inicial em CW (Morse):
//    - Timing: 1 minuto após o ID inicial em voz (62 segundos total)
//    - Velocidade: 13 WPM (palavras por minuto)
//    - Frequência: 600 Hz
//    - Display: Mostra "TX CW" + "MORSE CODE" com fundo vermelho
//    - Visualização: Exibe cada caractere e código Morse em tempo real
//
// Após completar os IDs iniciais, o sistema entra no ciclo normal.
//
// CICLO NORMAL (após IDs iniciais):
// =================================
//
// 1. ID em Voz:
//    - Intervalo: 11 minutos (conforme código original)
//    - Formato: Arquivo WAV com indicativo da repetidora
//    - Ativado: Somente quando não há QSO ativo
//    - Display: Mostra "TX VOZ" + "INDICATIVO VOZ" com fundo vermelho
//
// 2. ID em CW (Código Morse):
//    - Intervalo: 16 minutos (conforme código original)
//    - Velocidade: 13 WPM (palavras por minuto)
//    - Frequência: 600 Hz
//    - Ativado: Somente quando não há QSO ativo
//    - Display: Mostra "TX CW" + "MORSE CODE" com fundo vermelho
//    - Visualização: Exibe cada caractere e código Morse em tempo real
//
// SISTEMA DE COURTESY TONES (CT):
// ===============================
// A repetidora possui 33 Courtesy Tones diferentes:
//
// 1. Troca Automática:
//    - A cada 5 QSOs, o CT é alterado automaticamente (código original)
//    - Permite variação dos sons ao longo do tempo
//    - Índice atualiza ciclicamente (1-33, volta para 1)
//
// 2. Seleção Manual:
//    - Toque na tela do display avança para o próximo CT
//    - Instantâneo - o novo CT é aplicado imediatamente
//    - Display mostra o nome do CT e número (ex: "Beep 02/33")
//
// 3. Reprodução:
//    - Tocado após cada QSO (após hang time de 600ms)
//    - Volume configurável (default: 70%)
//    - Sample rate: 22050 Hz para melhor qualidade
//
// CONFIGURAÇÃO DE TEMPOS (conforme código original):
// ==============================================
// - Hang Time: 600ms (após QSO antes do CT)
// - ID Voz: 11 minutos
// - ID CW: 16 minutos
// - Troca CT: a cada 5 QSOs
//
// ESTES TEMPOS FORAM MANTIDOS CONFORME O CÓDIGO ORIGINAL PARA COMPATIBILIDADE.
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

  File file = LittleFS.open("/debug.log", FILE_APPEND);
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
#define TFT_DARKGREEN 0x07E0  // Verde mais brilhante e visível (RGB 0,31,0) - era 0x03E0
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

// LED RGB pins (ACTIVE LOW - LOW = acende, HIGH = apaga) - conforme ESP32-2432S028R
#define PIN_LED_R 4
#define PIN_LED_G 16
#define PIN_LED_B 17

// ====================== CONFIG =======================
const char* CALLSIGN = "PY2KEP SP";
#define WDT_TIMEOUT_SECONDS 30
#define SAMPLE_RATE 22050
#define HANG_TIME_MS 600
#define PTT_TIMEOUT_MS 4UL*60UL*1000UL  // 4 minutos de timeout
float VOLUME = 0.70f;

// Constantes para CW (Morse)
#define CW_WPM 13        // Velocidade em palavras por minuto
#define CW_FREQ 600      // Frequência em Hz para tom CW

// Intervalos de Identificação Automática (ID Voice/CW)
const uint32_t VOICE_INTERVAL_MS = 11UL*60UL*1000UL;  // 11 minutos - ID em voz (conforme código original)
const uint32_t CW_INTERVAL_MS   = 16UL*60UL*1000UL;  // 16 minutos - ID em CW/Morse (conforme código original)
const uint8_t  QSO_CT_CHANGE   = 5;                 // Troca CT a cada 5 QSOs (código original)

// ====================== GLOBAIS ======================
bool cor_stable = false;
bool ptt_state  = false;
bool playing    = false;
bool i2s_ok     = false;
bool last_cor = false;  // Para debounce do COR
unsigned long last_change = 0;  // Para debounce do COR
const uint32_t COR_DEBOUNCE_MS = 350;  // Tempo de debounce (350ms)
bool ptt_locked = false;  // Flag para bloquear PTT após timeout
unsigned long ptt_activated_at = 0;  // Timestamp quando PTT foi ativado (para timeout)

uint16_t qso_count = 0;
uint8_t  ct_index  = 0;
unsigned long last_display_update = 0;
unsigned long last_uptime_update = 0;  // Timer separado para uptime
bool first_draw = true;  // Flag para primeira renderização completa (evita flash)
bool needsFullRedraw = false;  // Flag para redraw completo quando necessário
char old_uptime_str[16] = "";  // String do uptime anterior (para comparar e só atualizar se mudou)

// Estados de identificação (ID Voice/CW)
enum TxMode { TX_NONE, TX_RX, TX_VOICE, TX_CW };
TxMode tx_mode = TX_NONE;  // Tipo de transmissão ativa

// Texto atual sendo transmitido em Morse (para exibição no display)
char current_morse_char[64] = "";  // Armazena o código Morse atual
char current_morse_display[2] = "";  // Caractere atual sendo transmitido

// Identificação inicial no boot
bool initial_id_done = false;  // Flag para controle de ID inicial
bool initial_voice_done = false; // NOVA TRAVA: Garante que voz toca só uma vez
unsigned long boot_time = 0;  // Timestamp de quando a placa foi ligada

// Configuração temporária: pular ID inicial se arquivo não existe (descomentar para ativar)
#define SKIP_INITIAL_IDS_IF_FILE_MISSING 0  // 0 = NAO pular IDs iniciais

// Timers para Identificação Automática (ID Voice/CW)
unsigned long last_voice = 0;      // Última identificação em voz
unsigned long last_cw    = 0;      // Última identificação em CW (Morse)
unsigned long cw_timer_start = 0;  // Timer para iniciar o CW após a voz

// ====================== VARIÁVEIS DO LED RGB ======================
// Sistema de controle do LED RGB usando PWM (LED Control do ESP32)
//
// ledc_channel_r/g/b: Canais PWM atribuídos pelo sistema LEDC (-1 = não inicializado)
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

/**
 * @brief Reproduz arquivo WAV do LittleFS (para indicativo de voz)
 *
 * Esta função lê um arquivo WAV do sistema de arquivos LittleFS e
 * reproduz através do speaker via I2S. É usada para tocar o
 * indicativo da repetidora (callsign voice).
 *
 * Formato esperado do arquivo WAV:
 * - Sample Rate: 8000 Hz (conforme nome do arquivo: 8k)
 * - Bit Depth: 16-bit PCM
 * - Canais: Mono (1 canal)
 *
 * @param filename Nome do arquivo no LittleFS (ex: "/id_voz_8k16.wav")
 *
 * @see playCT() para courtesy tones gerados por código
 * @see setup() onde LittleFS é inicializado
 */
void playVoiceFile(const char* filename) {
  // #region agent log - H1: Verificar se arquivo existe antes de abrir
  logToFile("H1", "playVoiceFile:check_exists", millis(), 0, 0, 0);
  if (!LittleFS.exists(filename)) {
    logToFile("H1", "playVoiceFile:not_found", millis(), 0, 0, 0);
    Serial.printf("ERRO CRÍTICO: Arquivo não existe no LittleFS: %s\n", filename);
    return;
  }
  logToFile("H1", "playVoiceFile:exists", millis(), 0, 0, 0);
  // #endregion

  // Abre arquivo WAV do LittleFS
  File file = LittleFS.open(filename, FILE_READ);
  if (!file) {
    logToFile("H1", "playVoiceFile:open_failed", millis(), 0, 0, 0);
    Serial.printf("ERRO: Não foi possível abrir arquivo: %s\n", filename);
    return;
  }

  // Lê header WAV (44 bytes) e extrai taxa de amostragem (igual ao código original)
  uint8_t header[44];
  if (file.read(header, 44) != 44) {
    file.close();
    Serial.println("ERRO: Não foi possível ler header WAV");
    return;
  }
  
  // Extrai sample rate do header (bytes 24-27, little-endian) - igual ao código original
  uint32_t rate = header[24] | (header[25]<<8) | (header[26]<<16) | (header[27]<<24);
  Serial.printf("Tocando arquivo: %s (sample rate: %d Hz)\n", filename, rate);
  
  // Inicializa I2S com a taxa do arquivo (igual ao código original)
  i2s_init(rate);
  playing = true;

  // Alocação dinâmica para evitar Stack Overflow (Heap em vez de Stack)
  const size_t bufferSize = 512;
  int16_t *audioBuffer = (int16_t*) malloc(bufferSize * sizeof(int16_t));
  uint16_t *i2sBuffer = (uint16_t*) malloc(bufferSize * 2 * sizeof(uint16_t));

  if (audioBuffer == NULL || i2sBuffer == NULL) {
    Serial.println("ERRO FATAL: Falha ao alocar memória para buffers de áudio!");
    logToFile("H1", "playVoiceFile:malloc_fail", millis(), 0, 0, 0);
    if (audioBuffer) free(audioBuffer);
    if (i2sBuffer) free(i2sBuffer);
    file.close();
    i2s_driver_uninstall(I2S_NUM_0);
    i2s_ok = false;
    playing = false;
    return;
  }

  logToFile("H1", "playVoiceFile:heap_before", millis(), ESP.getFreeHeap(), bufferSize * 4, 0);
  unsigned long totalBytesRead = 0;
  unsigned long startTime = millis();

  // Lê e reproduz o arquivo (igual ao código original)
  size_t contador = 0;
  while (file.available() && playing) {
    size_t r = file.read((uint8_t*)audioBuffer, min((size_t)file.available(), bufferSize * 2));
    size_t samples = r/2;
    
    // Aplica volume (igual ao código original)
    for (size_t i = 0; i < samples; i++) {
      audioBuffer[i] = (int16_t)constrain((int32_t)audioBuffer[i] * VOLUME, -16000, 16000);
    }

    // Converte para formato I2S (igual ao código original)
    size_t w;
    uint16_t out[bufferSize*2];
    for (size_t i = 0; i < samples; i++) {
      uint16_t v = ((audioBuffer[i] + 32768) >> 8) << 8;
      out[i*2] = v;
      out[i*2+1] = v;
    }
    
    // Escreve via I2S com portMAX_DELAY (igual ao código original - bloqueia até escrever tudo)
    i2s_write(I2S_NUM_0, out, samples*4, &w, portMAX_DELAY);

    // Reseta watchdog periodicamente (igual ao código original)
    contador += samples;
    if (contador >= 256) {
      contador = 0;
      esp_task_wdt_reset();
    }
  }
  
  // Libera memória alocada
  free(audioBuffer);
  free(i2sBuffer);
  
  // Fecha arquivo (igual ao código original)
  file.close();

  // Limpeza: delay de 50ms e para I2S (igual ao código original)
  delay(50);
  playing = false;
  i2s_driver_uninstall(I2S_NUM_0);
  i2s_ok = false;
  
  Serial.println("Reprodução de voz concluída");
}

/**
 * @brief Reproduz texto em código Morse (CW - Continuous Wave)
 *
 * Esta função converte um texto (tipicamente o callsign) em código
 * Morse e o reproduz via I2S. É usada para identificação
 * automática da repetidora em intervalos regulares.
 *
 * Formato:
 * - Velocidade: 13 WPM (definido por CW_WPM)
 * - Frequência: 600 Hz (definido por CW_FREQ)
 * - Espaçamento: Padrão internacional Morse
 *
 * @param txt Texto para reproduzir em Morse (ex: "PY2KEP SP")
 *
 * @see synthDualTone() para geração de cada som
 */
void playCW(const String &txt) {
  if (playing) return;

  Serial.printf("Reproduzindo em CW (Morse): %s\n", txt.c_str());
  Serial.printf("Texto: %d caracteres\n", txt.length());

  // Inicializa I2S para áudio
  i2s_init(SAMPLE_RATE);
  playing = true;

  unsigned long startTime = millis();

  // Calcula duração de um ponto (dot) baseado em WPM
  uint32_t dotDuration = 1200 / CW_WPM;  // 1200 = velocidade padrão

  // Loop através de cada caracter
  for (size_t i = 0; i < txt.length(); i++) {
    char c = toupper(txt[i]);

    // Verifica se o caractere está no dicionário Morse
    bool found = false;
    const char* code = nullptr;

    // Tabela Morse CORRETA (Padrão Internacional)
    const char* morse_map[36] = {
      ".-",   // A
      "-...", // B
      "-.-.", // C
      "-..",  // D
      ".",    // E
      "..-.", // F
      "--.",  // G
      "....", // H
      "..",   // I
      ".---", // J
      "-.-",  // K
      ".-..", // L
      "--",   // M
      "-.",   // N
      "---",  // O
      ".--.", // P
      "--.-", // Q
      ".-.",  // R
      "...",  // S
      "-",    // T
      "..-",  // U
      "...-", // V
      ".--",  // W
      "-..-", // X
      "-.--", // Y
      "--..", // Z
      "-----", // 0
      ".----", // 1
      "..---", // 2
      "...--", // 3
      "....-", // 4
      ".....", // 5
      "-....", // 6
      "--...", // 7
      "---..", // 8
      "----."  // 9
    };
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (int j = 0; j < 36; j++) {
      if (c == chars[j]) {
        code = morse_map[j];
        found = true;
        break;
      }
    }

    if (found && code) {
      Serial.printf("Caractere %c: %s\n", c, code);

      // Atualiza display com o caractere atual e código Morse
      snprintf(current_morse_display, sizeof(current_morse_display), "%c", c);
      snprintf(current_morse_char, sizeof(current_morse_char), "%s", code);
      updateDisplay();  // Atualiza para mostrar código Morse atual

      // Para cada ponto ou traço no código Morse
      for (size_t j = 0; code[j] != '\0'; j++) {
        // Determina duração: 3 * dot para traço, 1 * dot para ponto
        uint32_t duration = code[j] == '.' ? dotDuration : (3 * dotDuration);

        // Gera tom em 600 Hz
        synthDualTone(CW_FREQ, 0, duration);

        // Delay entre pontos/traços
        delay(dotDuration);
      }

      // Limpa o display do código Morse após completar caractere
      current_morse_char[0] = '\0';
      updateDisplay();

      // Delay entre caracteres (3 pontos)
      delay(3 * dotDuration);
    } else if (c == ' ') {
      Serial.println("Espaço detectado");
      // Espaço entre palavras (7 pontos)
      delay(7 * dotDuration);
    } else {
      Serial.printf("Caractere não encontrado: %c\n", c);
    }
  }

  unsigned long endTime = millis();
  Serial.printf("Tempo total CW: %lu ms\n", endTime - startTime);

  // Delay final
  delay(50);

  // Limpa variáveis de exibição Morse
  current_morse_char[0] = '\0';
  current_morse_display[0] = '\0';

  // Limpeza: para I2S após reprodução
  i2s_driver_uninstall(I2S_NUM_0);
  i2s_ok = false;
  playing = false;

  Serial.println("Reprodução CW concluída");
}

// ====================== CONTROLE DO PTT ========================

/**
 * @brief Ativa ou desativa o PTT (Push-to-Talk)
 *
 * Esta função controla o pino PTT do rádio TX, gerenciando
 * o estado e registrando o timestamp de ativação para controle
 * de timeout de 4 minutos.
 *
 * @param on true para ativar PTT, false para desativar
 *
 * Comportamento:
 * - Se on == ptt_state: Não faz nada (evita redundância)
 * - Ativa: Seta ptt_state, HIGH no pino, registra timestamp
 * - Desativa: Seta ptt_state, LOW no pino
 *
 * @see loop() onde o timeout de 4 minutos é verificado
 */
void setPTT(bool on) {
  // Se o estado já é o mesmo, não faz nada
  if (on == ptt_state) return;

  // Atualiza estado
  ptt_state = on;

  // Controla o pino físico
  digitalWrite(PIN_PTT, on ? HIGH : LOW);

  // Log e timestamp
  Serial.println(on ? "PTT ON" : "PTT OFF");
  if (on) {
    ptt_activated_at = millis();  // Registra quando foi ativado (para timeout)
  }
}

// ====================== LED RGB ========================

// Função setColorFromHue() removida - não é mais necessária
// LED agora usa cores fixas correspondentes ao display (verde/amarelo/vermelho)

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
 *    - Cor: Verde fixo (mesma cor do display verde escuro)
 *    - Comportamento: Cor sólida verde
 *    - Uso: Indica que está em espera
 *
 * Nota: Esta função deve ser chamada continuamente no loop principal
 *       para manter animações atualizadas
 *
 * @see loop() onde esta função é chamada
 */
void updateLED() {
  if (ledc_channel_r < 0 || ledc_channel_g < 0 || ledc_channel_b < 0) return;  // Não inicializado
  
  if (ptt_state) {
    // TX ativo: Vermelho fixo (mesma cor do display vermelho)
    // ACTIVE LOW: 0 = acende, 255 = apaga
    ledcWrite(ledc_channel_r, 0);    // Vermelho acende (LOW)
    ledcWrite(ledc_channel_g, 255); // Verde apagado (HIGH)
    ledcWrite(ledc_channel_b, 255);  // Azul apagado (HIGH)
  } else if (cor_stable) {
    // RX ativo: Amarelo (mesma cor do display amarelo)
    // ACTIVE LOW: 0 = acende, 255 = apaga
    // Amarelo = Vermelho + Verde (ambos acendem)
    ledcWrite(ledc_channel_r, 0);    // Vermelho acende (LOW)
    ledcWrite(ledc_channel_g, 0);    // Verde acende (LOW)
    ledcWrite(ledc_channel_b, 255);  // Azul apagado (HIGH)
  } else {
    // Idle: Verde (mesma cor do display verde escuro)
    // ACTIVE LOW: 0 = acende, 255 = apaga
    ledcWrite(ledc_channel_r, 255);  // Vermelho apagado (HIGH)
    ledcWrite(ledc_channel_g, 0);    // Verde acende (LOW)
    ledcWrite(ledc_channel_b, 255);  // Azul apagado (HIGH)
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
  // #region agent log - H2: Monitorar chamadas ao updateDisplay
  logToFile("H2", "updateDisplay:entry", millis(), ESP.getFreeHeap(), 0, 0);
  // #endregion

  // Atualização apenas quando necessário (mudança de estado, touch, etc.)
  // Uptime é atualizado separadamente por updateUptimeOnly() - SEM refresh da tela
  unsigned long currentMillis = millis();

  // Throttle: evita updates muito frequentes (mínimo 250ms entre updates)
  // Mas permite primeira atualização sempre (quando last_display_update == 0)
  // NOTA: Durante TX (ptt_state || tx_mode != TX_NONE), NÃO faz throttle
  // para garantir que o display mantenha o estado TX durante toda a transmissão
  if (last_display_update != 0 && currentMillis - last_display_update < 250) {
    // #region agent log - H3: Throttle checking
    logToFile("H2", "updateDisplay:throttled", millis(), currentMillis - last_display_update, 0);
    // #endregion

    // Se está em TX, atualiza mais frequentemente para manter display sincronizado
    if (ptt_state || tx_mode != TX_NONE) {
      // Atualiza a cada 100ms durante TX para manter estado TX visível
      if (currentMillis - last_display_update < 100) return;
    } else {
      // Em modo normal, usa throttle de 250ms
      return;
    }
  }
  last_display_update = currentMillis;
  logToFile("H2", "updateDisplay:will_update", millis(), ESP.getFreeHeap(), 0, 0);
  
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
    
    // ========== HEADER - Redesenha apenas quando mudou ==========
    int16_t header_height = 60;
    static bool header_drawn = false;

    // Só redesenha header se: primeira vez OU redraw completo forçado
    // O callsign NÃO muda durante operação normal, então não precisa redesenhar sempre
    if (!header_drawn || isFullRedraw) {
      tft.fillRect(0, 0, W, header_height, TFT_DARKBLUE);  // Limpa header
      tft.drawRoundRect(0, 0, W, header_height, 5, TFT_WHITE);  // Borda branca

      // Callsign centralizado e bem posicionado
      tft.setTextColor(TFT_YELLOW, TFT_DARKBLUE);
      tft.setTextSize(4);
      int16_t textW = tft.textWidth(CALLSIGN);
      int16_t text_x = (W - textW) / 2;
      int16_t text_y = (header_height - 24) / 2;  // Centraliza verticalmente
      tft.setCursor(text_x, text_y);
      tft.print(CALLSIGN);

      // Linha separadora
      tft.drawFastHLine(5, header_height, W - 10, TFT_DARKGREY);
      header_drawn = true;
    }
    
    // ========== STATUS PRINCIPAL (Centro, caixa grande) ==========
    uint16_t status_bg, status_text_color;
    const char* status_text;
    const char* status_subtext = "";
    
    // Determina estado e texto baseado no modo de transmissão
    if (tx_mode == TX_VOICE) {
      status_bg = TFT_RED;
      status_text_color = TFT_WHITE;
      status_text = "TX VOZ";
      status_subtext = "INDICATIVO VOZ";
    } else if (tx_mode == TX_CW) {
      status_bg = TFT_RED;
      status_text_color = TFT_WHITE;
      status_text = "TX CW";
      status_subtext = "MORSE CODE";
    } else if (tx_mode == TX_RX || (ptt_state && tx_mode == TX_NONE)) {
      status_bg = TFT_RED;
      status_text_color = TFT_WHITE;
      status_text = "TX ATIVO";
      status_subtext = "REPETINDO";
    } else if (cor_stable) {
      status_bg = TFT_YELLOW;
      status_text_color = TFT_BLACK;
      status_text = "RX ATIVO";
      status_subtext = "";
    } else {
      // EM ESCUTA - Força verde mais brilhante para melhor visibilidade
      status_bg = TFT_GREEN;  // Usa TFT_GREEN padrão (mais brilhante que DARKGREEN)
      status_text_color = TFT_WHITE;  // Texto BRANCO para máximo contraste sobre verde
      status_text = "EM ESCUTA";
      status_subtext = "";
    }
    
    // Debug: Log do estado atual do display
    Serial.printf("DISPLAY STATE: tx_mode=%d, ptt_state=%d, cor_stable=%d, status_bg=0x%04X, text='%s'\n",
                  tx_mode, ptt_state, cor_stable, status_bg, status_text);
    
    // Caixa de status com bordas arredondadas (ajustado para header de 60px)
    int16_t status_y = 65;  // Ajustado de 55 para 65 (header agora é 60px)
    int16_t status_h = 90;  // Altura aumentada de 85 para 90 para evitar cortes
    static uint16_t last_status_bg = 0xFFFF;  // Para detectar mudança de status
    
    // Só redesenha status se mudou (evita flicker)
    // FORÇA redraw se está em modo "EM ESCUTA" para garantir visibilidade
    bool forceRedraw = (status_bg == TFT_GREEN && last_status_bg != TFT_GREEN);
    if (status_bg != last_status_bg || isFullRedraw || forceRedraw) {
      // Limpa área um pouco maior para garantir que não sobrem resquícios
      tft.fillRect(5, status_y, W - 10, status_h, TFT_BLACK);
      
      tft.fillRoundRect(10, status_y, W - 20, status_h, 10, status_bg);  // Raio aumentado para 10
      tft.drawRoundRect(10, status_y, W - 20, status_h, 10, TFT_WHITE);
      last_status_bg = status_bg;
      
      Serial.printf("STATUS REDRAWN: bg=0x%04X, text='%s'\n", status_bg, status_text);
    }
    
    // Texto de status grande - SEMPRE redesenhado para garantir visibilidade
    tft.setTextColor(status_text_color, status_bg);
    tft.setTextSize(3);
    
    // Usa setCursor e print ao invés de drawCentreString para garantir funcionamento
    int16_t status_text_w = tft.textWidth(status_text);
    // Centralizado verticalmente com um pequeno ajuste (+2px) para visualização melhor
    int16_t status_text_y = status_y + (status_h - 24) / 2 + 2; 
    
    // LIMPEZA EXTRA: Apaga retângulo exato onde o texto vai ficar antes de escrever
    // Sempre limpa para evitar texto fantasma, especialmente em modo "EM ESCUTA"
    int16_t clear_x = (W - status_text_w) / 2;
    // Limpa área do texto principal (não limpa área abaixo para não interferir)
    tft.fillRect(10, status_text_y - 2, W - 20, 28, status_bg);

    // Desenha o texto principal
    tft.setCursor(clear_x, status_text_y);
    tft.print(status_text);
    
    // Debug adicional para modo "EM ESCUTA"
    if (status_bg == TFT_GREEN) {
      Serial.printf("TEXTO 'EM ESCUTA' DESENHADO: x=%d, y=%d, w=%d, bg=0x%04X, text_color=0x%04X (WHITE)\n", 
                    clear_x, status_text_y, status_text_w, status_bg, status_text_color);
    }
    
    // Subtexto (para modo CW/Voz ou QSO)
    if (status_subtext[0] != '\0') {
      tft.setTextSize(2);
      tft.setTextColor(status_text_color, status_bg);
      int16_t subtext_w = tft.textWidth(status_subtext);
      
      // Limpeza prévia do subtexto
      tft.fillRect(10, status_y + 60, W - 20, 16, status_bg);

      tft.setCursor((W - subtext_w) / 2, status_y + 60);
      tft.print(status_subtext);

      // Se está em modo CW e há código Morse sendo transmitido, mostra abaixo
      if (tx_mode == TX_CW && current_morse_char[0] != '\0') {
        // Limpa área do código Morse antes de desenhar (evita sobreposição)
        // Aumentei a altura da limpeza para 20px para garantir
        tft.fillRect(10, status_y + 35, W - 20, 20, status_bg);

        tft.setTextColor(TFT_YELLOW, status_bg);
        int16_t morse_w = tft.textWidth(current_morse_char);
        tft.setCursor((W - morse_w) / 2, status_y + 35);  // Entre texto principal e subtexto
        tft.printf("%c: %s", current_morse_display[0], current_morse_char);
      } else if (tx_mode == TX_CW) {
        // Se está em modo CW mas não tem código Morse para mostrar, limpa área
        tft.fillRect(10, status_y + 35, W - 20, 20, status_bg);
      }
    } else if (ptt_state && tx_mode == TX_NONE) {
      // Modo RX normal - mostra QSO ATUAL
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE, status_bg);
      int16_t qso_w = tft.textWidth("QSO ATUAL");
      
      // Limpeza prévia
      tft.fillRect(10, status_y + 60, W - 20, 16, status_bg);

      tft.setCursor((W - qso_w) / 2, status_y + 60);
      tft.print("QSO ATUAL");
    } else {
      // Limpa área de subtexto e código Morse se não está em TX (evita texto fantasma)
      // IMPORTANTE: NÃO limpar área do texto principal!
      // status_text_y está aproximadamente em y=100 (status_y + 33 + 2)
      // Limpa apenas área ABAIXO do texto principal (a partir de status_text_y + 30)
      // Isso garante que o texto "EM ESCUTA" não seja apagado
      int16_t clear_below_y = status_text_y + 30;  // Abaixo do texto principal
      int16_t clear_height = (status_y + status_h) - clear_below_y;  // Até o final da caixa
      if (clear_height > 0) {
        tft.fillRect(10, clear_below_y, W - 20, clear_height, status_bg);
      }
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

    // Mostra o CT selecionado (como no código original)
    tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    tft.setTextSize(2);
    tft.setCursor(20, ct_y + 8);
    tft.print("CT: ");
    tft.setTextColor(TFT_YELLOW, TFT_DARKGREEN);
    tft.print(tones[ct_index].name);

    // Número do CT (direita)
    char mode_buf[12];
    snprintf(mode_buf, sizeof(mode_buf), "%02d/33", ct_index + 1);
    tft.setTextColor(TFT_CYAN, TFT_DARKGREEN);
    tft.setTextSize(2);
    tft.setCursor(W - 70, ct_y + 8);
    tft.print(mode_buf);
  
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

    // Coluna 3: CT Index (direita) - Ajustado para não sair da tela
    // Usando largura segura do texto (~45px para "XX/33")
    int16_t ct_text_w = tft.textWidth("00/33");
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(W - ct_text_w - 5, footer_y + 5);
    tft.print("CT:");
    tft.setTextSize(2);
    tft.setCursor(W - ct_text_w - 5, footer_y + 15);
    tft.printf("%02d/33", ct_index + 1);
  
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
 * - Sistema de arquivos LittleFS para logs
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
 * 2. Configura LittleFS para logs em arquivo
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

  // #region agent log - Inicialização LittleFS para logs
  Serial.println("Inicializando LittleFS...");
  if (!LittleFS.begin(true)) {
    Serial.println("ERRO: LittleFS falhou ao inicializar!");
  } else {
    Serial.println("LittleFS inicializado com sucesso");
    // Limpa log anterior ao iniciar
    if (LittleFS.exists("/debug.log")) {
      LittleFS.remove("/debug.log");
      Serial.println("Log anterior removido");
    }
  }

  // Testa escrita no log
  File testFile = LittleFS.open("/debug.log", FILE_WRITE);
  if (testFile) {
    testFile.println("TESTE DE ESCRITA");
    testFile.close();
    Serial.println("Teste de escrita LittleFS OK");
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

  pinMode(PIN_COR, INPUT);  // INPUT (sem pullup) - conforme código original
  pinMode(PIN_PTT, OUTPUT);
  digitalWrite(PIN_PTT, LOW);
  Serial.printf("GPIOs configurados - COR: GPIO%d, PTT: GPIO%d\n", PIN_COR, PIN_PTT);
  Serial.printf("Speaker: GPIO%d\n", SPEAKER_PIN);

  // ========== CONFIGURAÇÃO DO LED RGB ==========
  // O LED RGB é controlado via PWM para permitir transições suaves de cores
  //
  // Especificações do LED RGB:
  // - Tipo: ACTIVE LOW (LOW acende, HIGH apaga) - conforme ESP32-2432S028R
  // - Pinos: R=GPIO4, G=GPIO16, B=GPIO17
  // - Controle: PWM via LEDC (LED Control) do ESP32
  // - Frequência: 5kHz (boa frequência para evitar flicker visível)
  // - Resolução: 8 bits (valores de 0-255)
  // - IMPORTANTE: Valores são invertidos (255 - valor) porque é active low
  //
  // Observação: No ESP32, ledcAttach() configura o canal PWM automaticamente
  //             e retorna o número do canal atribuído, então não precisa pinMode()
  ledc_channel_r = ledcAttach(PIN_LED_R, 5000, 8);  // Red: freq 5kHz, 8 bits (0-255)
  ledc_channel_g = ledcAttach(PIN_LED_G, 5000, 8);  // Green
  ledc_channel_b = ledcAttach(PIN_LED_B, 5000, 8);  // Blue

  // Inicializa LED apagado (active low: 255 = apagado)
  // Isso é importante porque no boot o LED não deve estar aceso
  ledcWrite(ledc_channel_r, 255);  // Apagado (HIGH)
  ledcWrite(ledc_channel_g, 255);  // Apagado (HIGH)
  ledcWrite(ledc_channel_b, 255);  // Apagado (HIGH)

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

  // Marca tempo de boot para identificação inicial
  boot_time = millis();
  Serial.println("Sistema pronto - Aguardando identificação inicial...");
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

  // #region agent log - H3: Monitorar stack overflow no loop
  static unsigned long lastStackCheck = 0;
  if (millis() - lastStackCheck >= 1000) {
    lastStackCheck = millis();
    uint32_t freeStack = uxTaskGetStackHighWaterMark(NULL);
    logToFile("H3", "loop:stack_check", millis(), freeStack, ESP.getFreeHeap(), 0);
    if (freeStack < 500) {
      Serial.printf("ALERTA: Stack livre muito baixo: %lu bytes\n", freeStack);
    }
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
  // Lê o pino COR (HIGH = sinal detectado - conforme código original)
  bool cor = (digitalRead(PIN_COR) == HIGH);

  // Debug: Verifica estado do PTT periodicamente
  static unsigned long last_ptt_debug = 0;
  if (millis() - last_ptt_debug >= 2000) {  // A cada 2 segundos
    last_ptt_debug = millis();
    bool ptt_pin_state = digitalRead(PIN_PTT);
    Serial.printf("DEBUG PTT: ptt_state=%d, PIN_PTT=%d, cor=%d, cor_stable=%d, tx_mode=%d\n",
                  ptt_state, ptt_pin_state, cor, cor_stable, tx_mode);
  }

  // SISTEMA DE DEBOUNCE PARA COR (conforme código original)
  if (cor != last_cor) {
    // Estado mudou - registra momento da mudança
    last_cor = cor;
    last_change = millis();
  } else if (millis() - last_change >= COR_DEBOUNCE_MS && cor != cor_stable) {
    // Após 350ms de estado estável e diferente do estado atual, atualiza
    // #region agent log
    logToFile("B", "COR_CHANGED", millis(), cor, cor_stable, ESP.getFreeHeap());
    Serial.printf("COR mudou: %d -> %d (após %lums estável)\n", cor_stable, cor, millis() - last_change);
    // #endregion
    cor_stable = cor;
    needsFullRedraw = true;  // Marca para redraw completo

    if (cor_stable && !ptt_locked) {
      // COR ativado → INÍCIO DO QSO → PTT ON (usando setPTT)
      setPTT(true);
    } else if (!cor_stable && ptt_state && !ptt_locked) {
      // COR desativado → FIM DO QSO → HANG TIME → CT → PTT OFF
      delay(HANG_TIME_MS);  // Aguarda hang time (600ms)
      if (!playing) {
        playCT();  // Reproduz courtesy tone selecionado
      }
      setPTT(false);

      // Troca automática do CT a cada 5 QSOs (código original)
      if (qso_count % 5 == 0) {
        ct_index = (ct_index + 1) % N_CT;
        Serial.printf("*** Novo Courtesy Tone: %s (CT %02d/33) ***\n", tones[ct_index].name, ct_index + 1);
        needsFullRedraw = true;  // Marca para atualizar display com novo CT
      }
    }

    // Libera lock do PTT se a COR caiu
    if (ptt_locked && !cor_stable) {
      ptt_locked = false;
      Serial.println("Lock liberado após queda de COR");
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

  // ========== TIMEOUT DE PTT (4 MINUTOS) ==========
  // Se o PTT ficar ativo por 4 minutos, bloqueia para evitar travamentos
  if (ptt_state && !ptt_locked && millis() - ptt_activated_at >= PTT_TIMEOUT_MS) {
    Serial.println("TIMEOUT 4min - PTT bloqueado");
    setPTT(false);
    ptt_locked = true;
  }

  // ========== CONTROLE DE TOUCHSCREEN ==========
  // Verifica se houve toque na tela
  if (ts.touched()) {
    TS_Point p = ts.getPoint();

    // Filtra touch com coordenadas inválidas (valores muito altos indicam touch falso)
    if (p.z > 600 && p.x < 8000 && p.y < 8000) {
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
    // NOTA: Displays LCD não precisam de refresh periódico - a imagem permanece estática
    // até que seja alterada por uma mudança de estado (COR, PTT, TX)
  }
  
  // ========== CONTROLE DO LED RGB ==========
  updateLED();  // Atualiza LED baseado no status (TX/RX/Idle)
  // Nota: Rainbow foi removido - LED agora usa cores fixas correspondentes ao display

  // ========== IDENTIFICAÇÃO AUTOMÁTICA (ID VOZ e CW) ==========

  // Identificação INICIAL no boot (apenas uma vez)
  if (!initial_id_done && !playing && !ptt_state) {
    unsigned long time_since_boot = millis() - boot_time;

    // 1. ID Inicial em Voz (Executa uma única vez após 2 segundos)
    if (!initial_voice_done && time_since_boot >= 2000) {
      #if !SKIP_INITIAL_IDS_IF_FILE_MISSING
      Serial.println("=== IDENTIFICAÇÃO INICIAL EM VOZ ===");
      unsigned long ptt_start_time = millis();
      tx_mode = TX_VOICE;
      updateDisplay();
      digitalWrite(PIN_PTT, HIGH);
      Serial.printf("PTT ATIVADO em %lu ms\n", ptt_start_time);
      delay(100);  // Aguarda estabilização do PTT
      
      playVoiceFile("/id_voz_8k16.wav");
      
      // Desativa PTT IMEDIATAMENTE após reprodução terminar
      // Não espera delay adicional - playVoiceFile() já terminou
      digitalWrite(PIN_PTT, LOW);
      unsigned long ptt_end_time = millis();
      unsigned long ptt_duration = ptt_end_time - ptt_start_time;
      Serial.printf("PTT DESATIVADO em %lu ms (duração total: %lu ms = %.2f segundos)\n", 
                    ptt_end_time, ptt_duration, ptt_duration / 1000.0f);
      
      // Verifica se PTT ficou aberto por muito tempo
      if (ptt_duration > 25000) {  // Mais de 25 segundos
        Serial.printf("AVISO: PTT ficou aberto por muito tempo! Esperado ~20s, real: %.2fs\n", ptt_duration / 1000.0f);
      }
      
      delay(50);  // Pequeno delay antes de mudar modo
      
      tx_mode = TX_NONE;
      updateDisplay();
      Serial.println("Identificação inicial de voz concluída");
      #else
      Serial.println("=== PULANDO IDENTIFICAÇÃO INICIAL EM VOZ (arquivo ausente) ===");
      delay(2000);
      #endif
      
      initial_voice_done = true;      // Marca voz como feita
      cw_timer_start = millis();      // Inicia contagem para o CW
    }
    
    // 2. ID Inicial em CW (Executa 5 segundos APÓS a voz terminar)
    else if (initial_voice_done && (millis() - cw_timer_start >= 5000)) {
      Serial.println("=== IDENTIFICAÇÃO INICIAL EM CW ===");
      tx_mode = TX_CW;
      updateDisplay();
      digitalWrite(PIN_PTT, HIGH);
      delay(100);
      playCW(CALLSIGN);
      delay(100);
      digitalWrite(PIN_PTT, LOW);
      tx_mode = TX_NONE;
      updateDisplay();
      Serial.println("Identificação inicial CW concluída");

      // Marca que TODOS os IDs iniciais foram completados
      initial_id_done = true;

      // Reseta os timers para o ciclo normal
      last_voice = millis();
      last_cw = millis();
      Serial.println("=== INICIANDO CICLO NORMAL DE IDENTIFICAÇÃO ===");
    }
  }

  // Identificação em voz a cada 11 minutos (após IDs iniciais)
  if (initial_id_done && millis() - last_voice >= VOICE_INTERVAL_MS && !playing && !ptt_state) {
    last_voice = millis();
    Serial.println("=== IDENTIFICAÇÃO EM VOZ (11 min) ===");
    unsigned long ptt_start_time = millis();
    tx_mode = TX_VOICE;  // Define modo de transmissão
    updateDisplay();  // Atualiza display para mostrar TX VOZ
    digitalWrite(PIN_PTT, HIGH);  // PTT ON
    Serial.printf("PTT ATIVADO em %lu ms\n", ptt_start_time);
    delay(100);  // Aguarda estabilização do PTT
    
    playVoiceFile("/id_voz_8k16.wav");  // Toca indicativo de voz
    
    // Desativa PTT IMEDIATAMENTE após reprodução terminar
    digitalWrite(PIN_PTT, LOW);   // PTT OFF
    unsigned long ptt_end_time = millis();
    unsigned long ptt_duration = ptt_end_time - ptt_start_time;
    Serial.printf("PTT DESATIVADO em %lu ms (duração total: %lu ms = %.2f segundos)\n", 
                  ptt_end_time, ptt_duration, ptt_duration / 1000.0f);
    
    // Verifica se PTT ficou aberto por muito tempo
    if (ptt_duration > 25000) {  // Mais de 25 segundos
      Serial.printf("AVISO: PTT ficou aberto por muito tempo! Esperado ~20s, real: %.2fs\n", ptt_duration / 1000.0f);
    }
    
    delay(50);  // Pequeno delay antes de mudar modo
    
    tx_mode = TX_NONE;  // Reseta modo de transmissão
    updateDisplay();  // Volta para estado normal
    Serial.println("Identificação de voz concluída");
  }

  // Identificação em CW (Morse) a cada 16 minutos (após IDs iniciais)
  if (initial_id_done && millis() - last_cw >= CW_INTERVAL_MS && !playing && !ptt_state) {
    last_cw = millis();
    Serial.println("=== IDENTIFICAÇÃO EM CW (16 min) ===");
    tx_mode = TX_CW;  // Define modo de transmissão
    updateDisplay();  // Atualiza display para mostrar TX CW
    digitalWrite(PIN_PTT, HIGH);  // PTT ON
    delay(100);
    playCW(CALLSIGN);  // Toca callsign em Morse
    delay(100);
    digitalWrite(PIN_PTT, LOW);   // PTT OFF
    tx_mode = TX_NONE;  // Reseta modo de transmissão
    updateDisplay();  // Volta para estado normal
    Serial.println("Identificação CW concluída");
  }
}
