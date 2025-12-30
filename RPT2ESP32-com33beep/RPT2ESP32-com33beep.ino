#include <Arduino.h>
#include <LittleFS.h>
#include "driver/i2s.h"
#include <esp_task_wdt.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ==================================================================
// CÓDIGO ADAPTADO PARA ESP32-2432S028R (Cheap Yellow Display - CYD)
// ==================================================================
// Principais adaptações:
// - Pins COR/PTT movidos para GPIO22/27 (Extended IO) para evitar conflito com LED RGB
// - Áudio configurado para speaker onboard (GPIO26) via I2S
// - User_Setup.h deve estar configurado com ILI9341_2_DRIVER
//
// SISTEMA DE LED RGB IMPLEMENTADO (v2.3):
// ========================================
// O LED RGB funciona como indicador visual do estado da repetidora em tempo real.
//
// CONFIGURAÇÃO HARDWARE:
// - Pinos: GPIO4 (R), GPIO16 (G), GPIO17 (B)
// - Tipo: ACTIVE LOW (LOW = acende, HIGH = apaga) - conforme ESP32-2432S028R
// - Controle: digitalWrite() simples (HIGH=apagado, LOW=aceso)
// - Importante: Não usa PWM para máxima compatibilidade com ESP32 Arduino Core
//
// ESTADOS DO LED (correspondem às cores do display):
// 1. WIFI ATIVO (show_ip_screen = true):
//    - Cor: AZUL FIXO
//    - Pino R: HIGH (apagado)
//    - Pino G: HIGH (apagado)
//    - Pino B: LOW (acende - active low)
//    - Animação: Nenhuma (cor sólida)
//    - Uso: Indica que a tela de Wi-Fi está ativa
//
// 2. TRANSMITINDO (TX ativo):
//    - Cor: VERMELHO FIXO (mesma cor do display vermelho)
//    - Pino R: LOW (acende - active low)
//    - Pino G: HIGH (apagado)
//    - Pino B: HIGH (apagado)
//    - Animação: Nenhuma (cor sólida)
//    - Uso: Indica que está transmitindo
//
// 3. RECEBENDO (COR ativo, RX):
//    - Cor: AMARELO (mesma cor do display amarelo)
//    - Pino R: LOW (acende - active low)
//    - Pino G: LOW (acende - active low)
//    - Pino B: HIGH (apagado)
//    - Animação: Cor fixa amarela
//    - Uso: Indica que está recebendo sinal
//
// 4. EM ESCUTA/IDLE (sem sinal):
//    - Cor: VERDE (mesma cor do display verde)
//    - Pino R: HIGH (apagado)
//    - Pino G: LOW (acende - active low)
//    - Pino B: HIGH (apagado)
//    - Animação: Cor fixa verde
//    - Uso: Indica que está em espera
//
// FUNÇÕES DO LED RGB:
// - updateLED(): Atualiza LED baseado no estado atual (Wi-Fi/TX/RX/Idle)
// - Cores fixas: Azul (Wi-Fi), Vermelho (TX), Amarelo (RX), Verde (Idle)
// - Segue a mesma lógica de prioridade do display
//
// UTILIDADE PRÁTICA:
// - Feedback visual instantâneo sem precisar olhar para o display
// - Azul fixo: Tela de Wi-Fi ativa
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

// ====================== SISTEMA DE DEBUG ======================
// Níveis de debug configuráveis para otimizar o Serial Monitor:
//
// 0 = NONE: Apenas erros e eventos críticos (mínimo de mensagens)
// 1 = MINIMAL: Eventos principais (PTT, COR, QSO, IDs) - RECOMENDADO
// 2 = NORMAL: Debug padrão (inclui display, CW, loop stats) - sem JSON verbose
// 3 = VERBOSE: Tudo incluindo JSON detalhado (para debug avançado)
//
// NOTA: DEBUG_LEVEL sempre usa VERBOSE (nível máximo) para máximo de informações
uint8_t DEBUG_LEVEL = 3;  // Sempre VERBOSE (nível máximo)

// Flags de controle por categoria
#define DEBUG_JSON (DEBUG_LEVEL >= 3)           // Mensagens JSON detalhadas
#define DEBUG_DISPLAY (DEBUG_LEVEL >= 2)        // Mensagens de display
#define DEBUG_PTT (DEBUG_LEVEL >= 1)            // Debug PTT periódico
#define DEBUG_CW (DEBUG_LEVEL >= 2)            // Debug CW/Morse
#define DEBUG_EVENTS (DEBUG_LEVEL >= 1)         // Eventos principais

// #region agent log - Debug logging helper
void debugLog(const char* location, const char* message, const char* hypothesisId, int data1 = 0, int data2 = 0, int data3 = 0) {
  if (DEBUG_JSON) {
    Serial.printf("DEBUG:{\"location\":\"%s\",\"message\":\"%s\",\"hypothesisId\":\"%s\",\"data\":{\"v1\":%d,\"v2\":%d,\"v3\":%d},\"timestamp\":%lu}\n",
                  location, message, hypothesisId, data1, data2, data3, millis());
  }
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
#define PIN_BOOT 0  // BOOT Button (integrado na placa ESP32-2432S028R)

// LED RGB pins (ACTIVE LOW - LOW = acende, HIGH = apaga) - conforme ESP32-2432S028R
#define PIN_LED_R 4
#define PIN_LED_G 16
#define PIN_LED_B 17

// ====================== CONFIGURAÇÃO VIA WIFI ======================
// Sistema de gerenciamento de configurações através de interface web
Preferences preferences;
WebServer server(80);

// Credenciais do Access Point (Configuração)
#define AP_SSID "REPETIDORA_SETUP"
#define AP_PASSWORD "repetidora123"

// Estados do BOOT Button
bool boot_button_pressed = false;
unsigned long boot_button_start = 0;
bool show_ip_screen = false;  // Flag para alternar entre tela normal e tela de IP/WiFi
#define RESET_FACTORY_MS 5000  // 5 segundos para reset de fábrica

// ====================== VARIÁVEIS DE CONFIGURAÇÃO ======================
// Estas variáveis podem ser configuradas via interface web
char config_callsign[32] = "PY2KEP SP";  // Indicativo da repetidora
char config_frequency[16] = "439.450";    // Frequência (valor numérico)
uint8_t config_frequency_unit = 0;         // 0 = MHz, 1 = GHz
char config_cw_message[64] = "PY2KEP SP"; // Mensagem Morse (ID)

// Configurações de tempos (milissegundos)
uint32_t config_hang_time = 600;         // Hang time (após QSO)
uint32_t config_ptt_timeout = 4*60*1000; // Timeout do PTT (4 minutos)
uint32_t config_voice_interval = 11*60*1000;  // Intervalo ID voz (11 minutos)
uint32_t config_cw_interval = 16*60*1000;    // Intervalo ID CW (16 minutos)
uint32_t config_ct_change = 5;           // Troca CT a cada X QSOs
uint8_t config_ct_index = 0;             // Courtesy Tone selecionado (0-32)

// Configurações de Morse (CW)
uint16_t config_cw_wpm = 13;            // Velocidade em WPM (palavras por minuto)
uint16_t config_cw_freq = 600;          // Frequência em Hz para tom CW

// Configurações de áudio
float config_volume = 0.70f;             // Volume (0.0 - 1.0)
uint16_t config_sample_rate = 22050;    // Taxa de amostragem para áudio

// Configurações de debug
uint8_t config_debug_level = 3;         // Sempre VERBOSE (nível máximo) - não configurável

// ====================== CONFIG ORIGINAL (mantido para compatibilidade) =======================
#define WDT_TIMEOUT_SECONDS 30
#define SAMPLE_RATE config_sample_rate
#define HANG_TIME_MS config_hang_time
#define PTT_TIMEOUT_MS config_ptt_timeout
float VOLUME = config_volume;

// Constantes para CW (Morse) - agora dinâmicas
#define CW_WPM config_cw_wpm
#define CW_FREQ config_cw_freq

// Intervalos de Identificação Automática - agora dinâmicos
#define VOICE_INTERVAL_MS config_voice_interval
#define CW_INTERVAL_MS config_cw_interval
#define QSO_CT_CHANGE config_ct_change

// Variável global para o callsign (usada em vários lugares)
const char* CALLSIGN = config_callsign;

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
// Sistema de controle do LED RGB usando digitalWrite()
//
// Nota: LED RGB usa Active Low (HIGH=apagado, LOW=aceso)
// Controlado via digitalWrite() simples, sem PWM

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


// ====================== FUNÇÕES DE GERENCIAMENTO DE CONFIGURAÇÕES ========================

/**
 * @brief Carrega configurações do sistema Preferences (NVS)
 *
 * Esta função carrega todas as configurações salvas na memória não-volátil
 * do ESP32. Se uma configuração não existir, usa o valor padrão.
 *
 * Configurações carregadas:
 * - Indicativo (callsign)
 * - Frequência
 * - Mensagem Morse
 * - Tempos (hang time, timeout, intervalos)
 * - Configurações de Morse (WPM, frequência)
 * - Configurações de áudio (volume, sample rate)
 * - Configurações de debug
 * - Courtesy Tone selecionado
 */
void loadPreferences() {
  Serial.println("Carregando configurações...");

  // Inicializa o namespace "config"
  preferences.begin("config", false);

  // Carrega indicativo
  if (preferences.isKey("callsign")) {
    preferences.getString("callsign", config_callsign, sizeof(config_callsign));
    Serial.printf("Callsign: %s\n", config_callsign);
  }

  // Carrega frequência
  if (preferences.isKey("frequency")) {
    preferences.getString("frequency", config_frequency, sizeof(config_frequency));
    Serial.printf("Frequência: %s\n", config_frequency);
  }
  // Carrega unidade de frequência (0 = MHz, 1 = GHz)
  if (preferences.isKey("frequency_unit")) {
    config_frequency_unit = preferences.getUChar("frequency_unit", 0);
    Serial.printf("Unidade de Frequência: %s\n", config_frequency_unit == 0 ? "MHz" : "GHz");
  }

  // Carrega mensagem Morse
  if (preferences.isKey("cw_message")) {
    preferences.getString("cw_message", config_cw_message, sizeof(config_cw_message));
    Serial.printf("Mensagem CW: %s\n", config_cw_message);
  }

  // Carrega hang time
  if (preferences.isKey("hang_time")) {
    config_hang_time = preferences.getUInt("hang_time", 600);
    Serial.printf("Hang Time: %lu ms\n", config_hang_time);
  }

  // Carrega PTT timeout
  if (preferences.isKey("ptt_timeout")) {
    config_ptt_timeout = preferences.getUInt("ptt_timeout", 4*60*1000);
    Serial.printf("PTT Timeout: %lu ms\n", config_ptt_timeout);
  }

  // Carrega intervalo de voz
  if (preferences.isKey("voice_interval")) {
    config_voice_interval = preferences.getUInt("voice_interval", 11*60*1000);
    Serial.printf("Voice Interval: %lu ms\n", config_voice_interval);
  }

  // Carrega intervalo de CW
  if (preferences.isKey("cw_interval")) {
    config_cw_interval = preferences.getUInt("cw_interval", 16*60*1000);
    Serial.printf("CW Interval: %lu ms\n", config_cw_interval);
  }

  // Carrega troca de CT
  if (preferences.isKey("ct_change")) {
    config_ct_change = preferences.getUInt("ct_change", 5);
    Serial.printf("CT Change: %lu QSOs\n", config_ct_change);
  }

  // Carrega CT selecionado
  if (preferences.isKey("ct_index")) {
    config_ct_index = preferences.getUChar("ct_index", 0);
    ct_index = config_ct_index;
    Serial.printf("CT Index: %d\n", config_ct_index);
  }

  // Carrega velocidade CW (WPM)
  if (preferences.isKey("cw_wpm")) {
    config_cw_wpm = preferences.getUInt("cw_wpm", 13);
    Serial.printf("CW WPM: %u\n", config_cw_wpm);
  }

  // Carrega frequência CW (Hz)
  if (preferences.isKey("cw_freq")) {
    config_cw_freq = preferences.getUInt("cw_freq", 600);
    Serial.printf("CW Frequency: %u Hz\n", config_cw_freq);
  }

  // Carrega volume
  if (preferences.isKey("volume")) {
    config_volume = preferences.getFloat("volume", 0.70f);
    Serial.printf("Volume: %.2f\n", config_volume);
  }

  // Carrega sample rate
  if (preferences.isKey("sample_rate")) {
    config_sample_rate = preferences.getUInt("sample_rate", 22050);
    Serial.printf("Sample Rate: %u Hz\n", config_sample_rate);
  }

  // Debug level sempre VERBOSE (nível máximo) - não carrega do Preferences
  config_debug_level = 3;
  DEBUG_LEVEL = 3;
  Serial.println("Debug Level: 3 (VERBOSE - sempre máximo)");

  preferences.end();
  Serial.println("Configurações carregadas com sucesso");
}

/**
 * @brief Salva configurações no sistema Preferences (NVS)
 *
 * Esta função salva todas as configurações atuais na memória não-volátil
 * do ESP32, garantindo que elas persistam após reinicialização.
 */
void savePreferences() {
  Serial.println("Salvando configurações...");

  preferences.begin("config", false);

  // Salva indicativo
  preferences.putString("callsign", config_callsign);

  // Salva frequência
  preferences.putString("frequency", config_frequency);
  preferences.putUChar("frequency_unit", config_frequency_unit);

  // Salva mensagem Morse
  preferences.putString("cw_message", config_cw_message);

  // Salva hang time
  preferences.putUInt("hang_time", config_hang_time);

  // Salva PTT timeout
  preferences.putUInt("ptt_timeout", config_ptt_timeout);

  // Salva intervalo de voz
  preferences.putUInt("voice_interval", config_voice_interval);

  // Salva intervalo de CW
  preferences.putUInt("cw_interval", config_cw_interval);

  // Salva troca de CT
  preferences.putUInt("ct_change", config_ct_change);

  // Salva CT selecionado
  preferences.putUChar("ct_index", config_ct_index);

  // Salva velocidade CW (WPM)
  preferences.putUInt("cw_wpm", config_cw_wpm);

  // Salva frequência CW (Hz)
  preferences.putUInt("cw_freq", config_cw_freq);

  // Salva volume
  preferences.putFloat("volume", config_volume);

  // Salva sample rate
  preferences.putUInt("sample_rate", config_sample_rate);

  // Debug level sempre VERBOSE - não salva (sempre usa máximo)
  // preferences.putUChar("debug_level", config_debug_level);  // Removido

  preferences.end();
  Serial.println("Configurações salvas com sucesso");
}

// ====================== SERVIDOR WEB ========================

/**
 * @brief Gera a página HTML completa de configuração
 *
 * Esta função gera uma página HTML responsiva com todas as configurações
 * da repetidora, organizadas em seções para fácil visualização e edição.
 *
 * A página inclui:
 * - Informações básicas (Indicativo, Frequência)
 * - Configurações de Morse (Mensagem, WPM, Frequência)
 * - Configurações de tempos (Hang Time, PTT Timeout, Intervalos de ID)
 * - Configurações de áudio (Volume, Sample Rate)
 * - Configurações de Courtesy Tone (Seletor dos 33 CTs)
 * - Configurações de debug (Nível de detalhamento)
 * - Status do sistema (Temperatura, Uptime, Memória)
 *
 * @return String com o HTML completo da página
 */
String generateConfigPage() {
  String html = "<!DOCTYPE html>";
  html += "<html lang='pt-BR'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Repetidora - Configuração</title>";
  html += "<style>";
  html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; ";
  html += "background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%); min-height: 100vh; ";
  html += "color: #fff; padding: 10px; }";
  html += ".container { max-width: 800px; margin: 0 auto; }";
  html += "h1 { text-align: center; color: #4a9eff; margin: 20px 0; font-size: 24px; }";
  html += ".section { background: rgba(255,255,255,0.05); border-radius: 10px; padding: 15px; ";
  html += "margin: 10px 0; border: 1px solid rgba(255,255,255,0.1); }";
  html += ".section h2 { color: #00d4ff; font-size: 18px; margin-bottom: 15px; border-bottom: 1px solid rgba(255,255,255,0.2); ";
  html += "padding-bottom: 8px; }";
  html += ".field { margin: 10px 0; }";
  html += "label { display: block; margin-bottom: 5px; color: #aaa; font-size: 14px; }";
  html += "input[type='text'], input[type='number'], select, textarea { ";
  html += "width: 100%; padding: 10px; border: 1px solid #444; border-radius: 5px; ";
  html += "background: rgba(0,0,0,0.3); color: #fff; font-size: 16px; }";
  html += "input[type='range'] { width: 100%; margin: 10px 0; }";
  html += ".range-value { text-align: right; color: #4a9eff; font-weight: bold; }";
  html += ".info { background: rgba(74, 158, 255, 0.2); padding: 10px; border-radius: 5px; ";
  html += "margin: 10px 0; font-size: 14px; }";
  html += ".info-label { color: #aaa; margin-right: 10px; }";
  html += ".info-value { color: #4a9eff; font-weight: bold; }";
  html += ".btn-group { display: flex; gap: 10px; margin-top: 20px; }";
  html += ".btn { flex: 1; padding: 15px; border: none; border-radius: 8px; font-size: 16px; ";
  html += "cursor: pointer; font-weight: bold; transition: transform 0.2s; }";
  html += ".btn:hover { transform: scale(1.02); }";
  html += ".btn-primary { background: linear-gradient(135deg, #00d4ff 0%, #0099cc 100%); color: #fff; }";
  html += ".btn-secondary { background: linear-gradient(135deg, #ff6b6b 0%, #cc5555 100%); color: #fff; }";
  html += ".btn-info { background: linear-gradient(135deg, #4a9eff 0%, #3377cc 100%); color: #fff; }";
  html += "#langBtn:hover { transform: scale(1.05); box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6); background: linear-gradient(135deg, #764ba2 0%, #667eea 100%); }";
  html += "#langBtn:active { transform: scale(0.98); }";
  html += ".footer { text-align: center; margin-top: 20px; color: #888; font-size: 12px; }";
  html += "@media (max-width: 600px) { h1 { font-size: 20px; } .section { padding: 12px; } #langBtn { padding: 6px 12px; font-size: 11px; min-width: 50px; height: 32px; } }";
  html += "</style>";
  html += "</head>";
  html += "<body>";

  // Cabeçalho com botão de idioma
  html += "<div class='container'>";
  html += "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;'>";
  html += "<h1 style='margin: 0;'>📡 Configuração da Repetidora</h1>";
  html += "<button id='langBtn' onclick='toggleLanguage()' style='padding: 8px 16px; font-size: 13px; font-weight: 600; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); border: none; border-radius: 25px; color: #fff; cursor: pointer; min-width: 60px; height: 36px; display: flex; align-items: center; justify-content: center; box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4); transition: all 0.3s ease; white-space: nowrap;'>🌐 PT</button>";
  html += "</div>";

  // Status do sistema
  html += "<div class='section'>";
  html += "<h2 data-pt='⚙️ Status do Sistema' data-en='⚙️ System Status'>⚙️ Status do Sistema</h2>";
  html += "<div class='info'>";
  html += "<span class='info-label' data-pt='Temperatura:' data-en='Temperature:'>Temperatura:</span>";
  html += "<span class='info-value'>" + String(temperatureRead()) + "°C</span>";
  html += "<br><br>";
  html += "<span class='info-label' data-pt='Uptime:' data-en='Uptime:'>Uptime:</span>";
  html += "<span class='info-value'>" + String(millis() / 1000) + "s</span>";
  html += "<br><br>";
  html += "<span class='info-label' data-pt='Memória Livre:' data-en='Free Memory:'>Memória Livre:</span>";
  html += "<span class='info-value'>" + String(ESP.getFreeHeap() / 1024) + " KB</span>";
  html += "</div>";
  html += "</div>";

  // Informações Básicas
  html += "<div class='section'>";
  html += "<h2 data-pt='📻 Informações Básicas' data-en='📻 Basic Information'>📻 Informações Básicas</h2>";
  html += "<div class='field'>";
  html += "<label for='callsign' data-pt='Indicativo (Callsign):' data-en='Callsign:'>Indicativo (Callsign):</label>";
  html += "<input type='text' id='callsign' name='callsign' value='" + String(config_callsign) + "' maxlength='30'>";
  html += "</div>";
  html += "<div class='field'>";
  html += "<label for='frequency' data-pt='Frequência:' data-en='Frequency:'>Frequência:</label>";
  html += "<div style='display: flex; gap: 10px;'>";
  html += "<input type='text' id='frequency' name='frequency' value='" + String(config_frequency) + "' maxlength='10' style='flex: 1;'>";
  html += "<select id='frequency_unit' name='frequency_unit' style='width: 100px;'>";
  html += "<option value='0'" + String(config_frequency_unit == 0 ? " selected" : "") + " data-pt='MHz' data-en='MHz'>MHz</option>";
  html += "<option value='1'" + String(config_frequency_unit == 1 ? " selected" : "") + " data-pt='GHz' data-en='GHz'>GHz</option>";
  html += "</select>";
  html += "</div>";
  html += "</div>";
  html += "</div>";

  // Configurações de Morse
  html += "<div class='section'>";
  html += "<h2 data-pt='🔊 Configurações de Morse (CW)' data-en='🔊 Morse Code (CW) Settings'>🔊 Configurações de Morse (CW)</h2>";
  html += "<div class='field'>";
  html += "<label for='cw_message' data-pt='Mensagem Morse (ID):' data-en='Morse Message (ID):'>Mensagem Morse (ID):</label>";
  html += "<input type='text' id='cw_message' name='cw_message' value='" + String(config_cw_message) + "' maxlength='60'>";
  html += "</div>";
  html += "<div class='field'>";
  html += "<label for='cw_wpm' data-pt='Velocidade Morse (WPM):' data-en='Morse Speed (WPM):'>Velocidade Morse (WPM): <span id='cw_wpm_val' class='range-value'>" + String(config_cw_wpm) + "</span></label>";
  html += "<input type='range' id='cw_wpm' name='cw_wpm' min='5' max='40' value='" + String(config_cw_wpm) + "' oninput='document.getElementById(\"cw_wpm_val\").textContent=this.value'>";
  html += "</div>";
  html += "<div class='field'>";
  html += "<label for='cw_freq' data-pt='Frequência do Tom (Hz):' data-en='Tone Frequency (Hz):'>Frequência do Tom (Hz): <span id='cw_freq_val' class='range-value'>" + String(config_cw_freq) + "</span></label>";
  html += "<input type='range' id='cw_freq' name='cw_freq' min='300' max='1200' value='" + String(config_cw_freq) + "' oninput='document.getElementById(\"cw_freq_val\").textContent=this.value'>";
  html += "</div>";
  html += "</div>";

  // Configurações de Tempos
  html += "<div class='section'>";
  html += "<h2 data-pt='⏱️ Configurações de Tempos' data-en='⏱️ Time Settings'>⏱️ Configurações de Tempos</h2>";
  html += "<div class='field'>";
  html += "<label for='hang_time' data-pt='Hang Time (ms):' data-en='Hang Time (ms):'>Hang Time (ms): <span id='hang_time_val' class='range-value'>" + String(config_hang_time) + "</span></label>";
  html += "<input type='range' id='hang_time' name='hang_time' min='100' max='2000' value='" + String(config_hang_time) + "' oninput='document.getElementById(\"hang_time_val\").textContent=this.value'>";
  html += "</div>";
  html += "<div class='field'>";
  html += "<label for='ptt_timeout' data-pt='PTT Timeout (ms):' data-en='PTT Timeout (ms):'>PTT Timeout (ms): <span id='ptt_timeout_val' class='range-value'>" + String(config_ptt_timeout / 1000) + "s</span></label>";
  html += "<input type='range' id='ptt_timeout' name='ptt_timeout' min='60000' max='600000' step='10000' value='" + String(config_ptt_timeout) + "' oninput='document.getElementById(\"ptt_timeout_val\").textContent=(this.value/1000)+\"s\"'>";
  html += "</div>";
  html += "<div class='field'>";
  html += "<label for='voice_interval' data-pt='Intervalo ID Voz (min):' data-en='Voice ID Interval (min):'>Intervalo ID Voz (min): <span id='voice_interval_val' class='range-value'>" + String(config_voice_interval / 60000) + "</span></label>";
  html += "<input type='range' id='voice_interval' name='voice_interval' min='5' max='30' value='" + String(config_voice_interval / 60000) + "' oninput='document.getElementById(\"voice_interval_val\").textContent=this.value'>";
  html += "</div>";
  html += "<div class='field'>";
  html += "<label for='cw_interval' data-pt='Intervalo ID CW (min):' data-en='CW ID Interval (min):'>Intervalo ID CW (min): <span id='cw_interval_val' class='range-value'>" + String(config_cw_interval / 60000) + "</span></label>";
  html += "<input type='range' id='cw_interval' name='cw_interval' min='5' max='30' value='" + String(config_cw_interval / 60000) + "' oninput='document.getElementById(\"cw_interval_val\").textContent=this.value'>";
  html += "</div>";
  html += "<div class='field'>";
  html += "<label for='ct_change' data-pt='Troca CT a cada (QSOs):' data-en='Change CT every (QSOs):'>Troca CT a cada (QSOs): <span id='ct_change_val' class='range-value'>" + String(config_ct_change) + "</span></label>";
  html += "<input type='range' id='ct_change' name='ct_change' min='1' max='20' value='" + String(config_ct_change) + "' oninput='document.getElementById(\"ct_change_val\").textContent=this.value'>";
  html += "</div>";
  html += "</div>";

  // Configurações de Áudio
  html += "<div class='section'>";
  html += "<h2 data-pt='♫ Configurações de Áudio' data-en='♫ Audio Settings'>♫ Configurações de Áudio</h2>";
  html += "<div class='field'>";
  html += "<label for='volume' data-pt='Volume:' data-en='Volume:'>Volume: <span id='volume_val' class='range-value'>" + String(config_volume * 100) + "%</span></label>";
  html += "<input type='range' id='volume' name='volume' min='0' max='100' value='" + String(config_volume * 100) + "' oninput='document.getElementById(\"volume_val\").textContent=this.value+\"%\"'>";
  html += "</div>";
  html += "<div class='field'>";
  html += "<label for='sample_rate' data-pt='Sample Rate (Hz):' data-en='Sample Rate (Hz):'>Sample Rate (Hz):</label>";
  html += "<select id='sample_rate' name='sample_rate'>";
  html += "<option value='8000' " + String(config_sample_rate == 8000 ? "selected" : "") + ">8000 Hz</option>";
  html += "<option value='11025' " + String(config_sample_rate == 11025 ? "selected" : "") + ">11025 Hz</option>";
  html += "<option value='16000' " + String(config_sample_rate == 16000 ? "selected" : "") + ">16000 Hz</option>";
  html += "<option value='22050' " + String(config_sample_rate == 22050 ? "selected" : "") + ">22050 Hz</option>";
  html += "<option value='44100' " + String(config_sample_rate == 44100 ? "selected" : "") + ">44100 Hz</option>";
  html += "</select>";
  html += "</div>";
  html += "</div>";

  // Configurações de Courtesy Tone
  html += "<div class='section'>";
  html += "<h2 data-pt='▲ Courtesy Tone (CT)' data-en='▲ Courtesy Tone (CT)'>▲ Courtesy Tone (CT)</h2>";
  html += "<div class='field'>";
  html += "<label for='ct_index' data-pt='Selecione o Courtesy Tone:' data-en='Select Courtesy Tone:'>Selecione o Courtesy Tone:</label>";
  html += "<select id='ct_index' name='ct_index'>";

  // Gera lista dos 33 CTs
  for (int i = 0; i < N_CT; i++) {
    html += "<option value='" + String(i) + "' " + String(ct_index == i ? "selected" : "") + ">";
    html += String(tones[i].name) + " (" + String(i + 1) + "/33)";
    html += "</option>";
  }

  html += "</select>";
  html += "</div>";
  html += "</div>";

  // Configurações de Debug removidas - sempre usa VERBOSE (máximo)

  // Botões de ação
  html += "<div class='btn-group'>";
  html += "<button type='button' class='btn btn-primary' onclick='saveConfig()' data-pt='💾 Salvar e Reiniciar' data-en='💾 Save and Restart'>💾 Salvar e Reiniciar</button>";
  html += "</div>";

  html += "<div class='btn-group'>";
  html += "<button type='button' class='btn btn-secondary' onclick='restartDevice()' data-pt='🔄 Reiniciar Dispositivo' data-en='🔄 Restart Device'>🔄 Reiniciar Dispositivo</button>";
  html += "<button type='button' class='btn btn-info' onclick='toggleDebug()' data-pt='📋 Ver Console Debug' data-en='📋 View Debug Console'>📋 Ver Console Debug</button>";
  html += "</div>";

  html += "<div class='btn-group'>";
  html += "<button type='button' class='btn btn-secondary' style='background: linear-gradient(135deg, #ff4444 0%, #cc0000 100%);' onclick='resetFactory()' data-pt='⚠️ Reset de Fábrica' data-en='⚠️ Factory Reset'>⚠️ Reset de Fábrica</button>";
  html += "</div>";

  // Área de debug (oculta por padrão)
  html += "<div id='debug_area' class='section' style='display:none;'>";
  html += "<h2 data-pt='📋 Console de Debug' data-en='📋 Debug Console'>📋 Console de Debug</h2>";
  html += "<p style='color: #aaa; font-size: 12px; margin-bottom: 10px;' data-pt='Eventos importantes: TX, WiFi, Erros (atualiza a cada 2 segundos)' data-en='Important events: TX, WiFi, Errors (updates every 2 seconds)'>Eventos importantes: TX, WiFi, Erros (atualiza a cada 2 segundos)</p>";
  html += "<textarea id='debug_console' rows='15' readonly style='font-family: monospace; font-size: 10px; background: #000; color: #0f0; padding: 10px; border: 1px solid #333; border-radius: 5px; width: 100%; box-sizing: border-box; line-height: 1.4;'>";
  html += "Carregando eventos...";
  html += "</textarea>";
  html += "<p style='color: #888; font-size: 11px; margin-top: 5px;' data-pt='💡 Mostra apenas eventos importantes. Use o Serial Monitor (115200 baud) para logs completos.' data-en='💡 Shows only important events. Use Serial Monitor (115200 baud) for complete logs.'>💡 Mostra apenas eventos importantes. Use o Serial Monitor (115200 baud) para logs completos.</p>";
  html += "</div>";

  // Footer
  html += "<div class='footer'>";
  html += "<div data-pt='Repetidora ESP32-2432S028R - Versão 2.3' data-en='ESP32-2432S028R Repeater - Version 2.3'>Repetidora ESP32-2432S028R - Versão 2.3</div>";
  html += "<div style='margin-top: 5px; font-size: 11px;' data-pt='Desenvolvido por: PU2PEG - Gabriel' data-en='Developed by: PU2PEG - Gabriel'>Desenvolvido por: PU2PEG - Gabriel</div>";
  html += "</div>";

  html += "</div>"; // container

  // Script JavaScript
  html += "<script>";
  html += "function saveConfig() {";
  html += "  try {";
  html += "    if(confirm('Deseja salvar as configurações e reiniciar o dispositivo?')) {";
  html += "      var data = new FormData();";
  html += "      data.append('callsign', document.getElementById('callsign').value);";
  html += "      data.append('frequency', document.getElementById('frequency').value);";
  html += "      data.append('frequency_unit', document.getElementById('frequency_unit').value);";
  html += "      data.append('cw_message', document.getElementById('cw_message').value);";
  html += "      data.append('cw_wpm', document.getElementById('cw_wpm').value);";
  html += "      data.append('cw_freq', document.getElementById('cw_freq').value);";
  html += "      data.append('hang_time', document.getElementById('hang_time').value);";
  html += "      data.append('ptt_timeout', document.getElementById('ptt_timeout').value);";
  html += "      data.append('voice_interval', document.getElementById('voice_interval').value);";
  html += "      data.append('cw_interval', document.getElementById('cw_interval').value);";
  html += "      data.append('ct_change', document.getElementById('ct_change').value);";
  html += "      data.append('ct_index', document.getElementById('ct_index').value);";
  html += "      data.append('volume', document.getElementById('volume').value);";
  html += "      data.append('sample_rate', document.getElementById('sample_rate').value);";
  html += "      fetch('/save', { method: 'POST', body: data })";
  html += "        .then(response => { if(!response.ok) throw new Error('HTTP ' + response.status); return response.text(); })";
  html += "        .then(data => { alert('Configurações salvas! O dispositivo será reiniciado...'); setTimeout(() => window.location.href = '/', 1000); })";
  html += "        .catch(error => { alert('Erro ao salvar: ' + error.message); console.error('Erro:', error); });";
  html += "    }";
  html += "  } catch(e) { alert('Erro: ' + e.message); console.error(e); }";
  html += "}";
  html += "function restartDevice() {";
  html += "  try {";
  html += "    if(confirm('Tem certeza que deseja reiniciar o dispositivo?')) {";
  html += "      fetch('/restart', { method: 'POST' })";
  html += "        .then(response => { if(!response.ok) throw new Error('HTTP ' + response.status); return response.text(); })";
  html += "        .then(data => { alert(data); setTimeout(() => location.reload(), 3000); })";
  html += "        .catch(error => { alert('Erro ao reiniciar: ' + error.message); console.error(error); });";
  html += "    }";
  html += "  } catch(e) { alert('Erro: ' + e.message); console.error(e); }";
  html += "}";
  html += "function resetFactory() {";
  html += "  try {";
  html += "    if(confirm('ATENÇÃO: Isso apagará TODAS as configurações e restaurará os valores de fábrica.\\n\\nTem certeza que deseja continuar?')) {";
  html += "      fetch('/reset_factory', { method: 'POST' })";
  html += "        .then(response => { if(!response.ok) throw new Error('HTTP ' + response.status); return response.text(); })";
  html += "        .then(data => { alert(data); setTimeout(() => location.reload(), 3000); })";
  html += "        .catch(error => { alert('Erro ao fazer reset: ' + error.message); console.error(error); });";
  html += "    }";
  html += "  } catch(e) { alert('Erro: ' + e.message); console.error(e); }";
  html += "}";
  html += "function toggleDebug() {";
  html += "  try {";
  html += "    var debug = document.getElementById('debug_area');";
  html += "    if(!debug) { alert('Erro: Elemento debug_area não encontrado'); return; }";
  html += "    debug.style.display = debug.style.display === 'none' ? 'block' : 'none';";
  html += "    if(debug.style.display === 'block') { fetchDebug(); }";
  html += "  } catch(e) { alert('Erro: ' + e.message); console.error(e); }";
  html += "}";
  html += "function fetchDebug() {";
  html += "  try {";
  html += "    fetch('/debug')";
  html += "      .then(response => { if(!response.ok) throw new Error('HTTP ' + response.status); return response.text(); })";
  html += "      .then(data => {";
  html += "        var console = document.getElementById('debug_console');";
  html += "        if(!console) { console.error('Elemento debug_console não encontrado'); return; }";
  html += "        if(data && data.trim().length > 0) {";
  html += "          var lines = data.split('\\n');";
  html += "          var filtered = [];";
  html += "          var importantTags = ['[TX]', '[WIFI]', '[WEB]', '[BOOT]', '[ERROR]', '[CONFIG]', 'TX', 'RX', 'PTT', 'Erro', 'Error', 'WiFi', 'WIFI', 'AP', 'Connected', 'Disconnected'];";
  html += "          for(var i = 0; i < lines.length; i++) {";
  html += "            var line = lines[i].trim();";
  html += "            if(line.length === 0) continue;";
  html += "            var isImportant = false;";
  html += "            for(var j = 0; j < importantTags.length; j++) {";
  html += "              if(line.indexOf(importantTags[j]) !== -1) {";
  html += "                isImportant = true;";
  html += "                break;";
  html += "              }";
  html += "            }";
  html += "            if(isImportant) {";
  html += "              filtered.push(line);";
  html += "            }";
  html += "          }";
  html += "          if(filtered.length === 0) {";
  html += "            console.value = 'Nenhum evento importante encontrado ainda.\\n\\nAguardando eventos de TX, WiFi ou erros...';";
  html += "          } else {";
  html += "            var lastEvents = filtered.slice(-30).join('\\n');";
  html += "            console.value = lastEvents;";
  html += "          }";
  html += "          console.scrollTop = console.scrollHeight;";
  html += "        } else {";
  html += "          console.value = 'Nenhum log disponível ainda.\\n\\nAguardando eventos importantes (TX, WiFi, Erros)...';";
  html += "        }";
  html += "      })";
  html += "      .catch(error => {";
  html += "        var console = document.getElementById('debug_console');";
  html += "        if(console) console.value = 'Erro ao carregar logs: ' + error.message;";
  html += "        console.error('Erro ao carregar debug:', error);";
  html += "      });";
  html += "    var debug = document.getElementById('debug_area');";
  html += "    if(debug && debug.style.display === 'block') {";
  html += "      setTimeout(fetchDebug, 2000);";
  html += "    }";
  html += "  } catch(e) { console.error('Erro em fetchDebug:', e); }";
  html += "}";
  html += "var currentLang = 'pt';";
  html += "var translations = {";
  html += "  pt: {";
  html += "    'title': '📡 Configuração da Repetidora',";
  html += "    'status': '⚙️ Status do Sistema',";
  html += "    'temp': 'Temperatura:',";
  html += "    'uptime': 'Uptime:',";
  html += "    'mem': 'Memória Livre:',";
  html += "    'basic': '📻 Informações Básicas',";
  html += "    'callsign': 'Indicativo (Callsign):',";
  html += "    'frequency': 'Frequência:',";
  html += "    'morse': '🔊 Configurações de Morse (CW)',";
  html += "    'cw_msg': 'Mensagem Morse (ID):',";
  html += "    'cw_wpm': 'Velocidade Morse (WPM):',";
  html += "    'cw_freq': 'Frequência do Tom (Hz):',";
  html += "    'times': '⏱️ Configurações de Tempos',";
  html += "    'hang': 'Hang Time (ms):',";
  html += "    'ptt': 'PTT Timeout (ms):',";
  html += "    'voice_int': 'Intervalo ID Voz (min):',";
  html += "    'cw_int': 'Intervalo ID CW (min):',";
  html += "    'audio': '♫ Configurações de Áudio',";
  html += "    'volume': 'Volume:',";
  html += "    'sample': 'Sample Rate (Hz):',";
  html += "    'ct': '▲ Courtesy Tone (CT)',";
  html += "    'ct_select': 'Selecione o Courtesy Tone:',";
  html += "    'save': '💾 Salvar e Reiniciar',";
  html += "    'restart': '🔄 Reiniciar Dispositivo',";
  html += "    'debug': '📋 Ver Console Debug',";
  html += "    'factory': '⚠️ Reset de Fábrica',";
  html += "    'footer': 'Repetidora ESP32-2432S028R - Versão 2.3',";
  html += "    'author': 'Desenvolvido por: PU2PEG - Gabriel',";
  html += "    'debug_title': '📋 Console de Debug',";
  html += "    'debug_desc': 'Eventos importantes: TX, WiFi, Erros (atualiza a cada 2 segundos)',";
  html += "    'debug_tip': '💡 Mostra apenas eventos importantes. Use o Serial Monitor (115200 baud) para logs completos.'";
  html += "  },";
  html += "  en: {";
  html += "    'title': '📡 Repeater Configuration',";
  html += "    'status': '⚙️ System Status',";
  html += "    'temp': 'Temperature:',";
  html += "    'uptime': 'Uptime:',";
  html += "    'mem': 'Free Memory:',";
  html += "    'basic': '📻 Basic Information',";
  html += "    'callsign': 'Callsign:',";
  html += "    'frequency': 'Frequency:',";
  html += "    'morse': '🔊 Morse Code (CW) Settings',";
  html += "    'cw_msg': 'Morse Message (ID):',";
  html += "    'cw_wpm': 'Morse Speed (WPM):',";
  html += "    'cw_freq': 'Tone Frequency (Hz):',";
  html += "    'times': '⏱️ Time Settings',";
  html += "    'hang': 'Hang Time (ms):',";
  html += "    'ptt': 'PTT Timeout (ms):',";
  html += "    'voice_int': 'Voice ID Interval (min):',";
  html += "    'cw_int': 'CW ID Interval (min):',";
  html += "    'audio': '♫ Audio Settings',";
  html += "    'volume': 'Volume:',";
  html += "    'sample': 'Sample Rate (Hz):',";
  html += "    'ct': '▲ Courtesy Tone (CT)',";
  html += "    'ct_select': 'Select Courtesy Tone:',";
  html += "    'save': '💾 Save and Restart',";
  html += "    'restart': '🔄 Restart Device',";
  html += "    'debug': '📋 View Debug Console',";
  html += "    'factory': '⚠️ Factory Reset',";
  html += "    'footer': 'ESP32-2432S028R Repeater - Version 2.3',";
  html += "    'author': 'Developed by: PU2PEG - Gabriel',";
  html += "    'debug_title': '📋 Debug Console',";
  html += "    'debug_desc': 'Important events: TX, WiFi, Errors (updates every 2 seconds)',";
  html += "    'debug_tip': '💡 Shows only important events. Use Serial Monitor (115200 baud) for complete logs.'";
  html += "  }";
  html += "};";
  html += "function applyTranslations(lang) {";
  html += "  currentLang = lang;";
  html += "  var t = translations[lang];";
  html += "  document.querySelector('h1').textContent = t.title;";
  html += "  document.getElementById('langBtn').textContent = lang === 'pt' ? '🌐 EN' : '🌐 PT';";
  html += "  var debugTitle = document.querySelector('#debug_area h2');";
  html += "  if(debugTitle) debugTitle.textContent = t.debug_title;";
  html += "  var debugDesc = document.querySelector('#debug_area p');";
  html += "  if(debugDesc && debugDesc.hasAttribute('data-pt')) {";
  html += "    debugDesc.textContent = lang === 'pt' ? debugDesc.getAttribute('data-pt') : debugDesc.getAttribute('data-en');";
  html += "  }";
  html += "  var debugTip = document.querySelectorAll('#debug_area p');";
  html += "  if(debugTip.length > 1 && debugTip[1].hasAttribute('data-pt')) {";
  html += "    debugTip[1].textContent = lang === 'pt' ? debugTip[1].getAttribute('data-pt') : debugTip[1].getAttribute('data-en');";
  html += "  }";
  html += "  var elements = document.querySelectorAll('[data-pt]');";
  html += "  elements.forEach(function(el) {";
  html += "    var attr = lang === 'pt' ? 'data-pt' : 'data-en';";
  html += "    if(el.hasAttribute(attr)) {";
  html += "      var text = el.getAttribute(attr);";
  html += "      if(el.tagName === 'LABEL') {";
  html += "        var span = el.querySelector('span');";
  html += "        if(span) {";
  html += "          el.innerHTML = text + ' ' + span.outerHTML;";
  html += "        } else {";
  html += "          el.textContent = text;";
  html += "        }";
  html += "      } else if(el.tagName === 'H2' || el.tagName === 'DIV' || el.tagName === 'BUTTON') {";
  html += "        el.textContent = text;";
  html += "      } else if(el.tagName === 'OPTION') {";
  html += "        el.textContent = text;";
  html += "      }";
  html += "    }";
  html += "  });";
  html += "}";
  html += "function toggleLanguage() {";
  html += "  currentLang = currentLang === 'pt' ? 'en' : 'pt';";
  html += "  applyTranslations(currentLang);";
  html += "}";
  html += "document.addEventListener('DOMContentLoaded', function() {";
  html += "  applyTranslations('pt');";
  html += "});";
  html += "</script>";

  html += "</body>";
  html += "</html>";

  return html;
}

/**
 * @brief Gera página de sucesso
 */
String getSuccessPage(const String& message) {
  String html = "<!DOCTYPE html><html lang='pt-BR'><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  html += "<title>Sucesso</title>";
  html += "<style>body{font-family:sans-serif;background:linear-gradient(135deg,#1a1a2e,#16213e);";
  html += "color:#fff;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;}";
  html += ".container{text-align:center;padding:40px;background:rgba(255,255,255,0.1);";
  html += "border-radius:15px;box-shadow:0 4px 20px rgba(0,0,0,0.3);}";
  html += "h1{color:#4a9eff;margin-bottom:20px;}p{color:#aaa;margin-bottom:30px;}";
  html += ".btn{display:inline-block;padding:15px 30px;background:linear-gradient(135deg,#00d4ff,#0099cc);";
  html += "color:#fff;text-decoration:none;border-radius:8px;font-weight:bold;}</style></head><body>";
  html += "<div class='container'><h1>✅ Sucesso!</h1><p>" + message + "</p>";
  html += "<a href='/' class='btn'>Voltar</a></div></body></html>";
  return html;
}

/**
 * @brief Inicializa o servidor web com todas as rotas
 *
 * Configura o servidor web para responder às requisições HTTP:
 * - GET /: Página de configuração
 * - POST /save: Salvar configurações e reiniciar
 * - POST /restart: Reiniciar o dispositivo
 * - POST /reset_factory: Reset de fábrica
 * - GET /debug: Obter logs de debug
 */
void initWebServer() {
  Serial.println("Inicializando servidor web...");

  // Rota principal - página de configuração
  server.on("/", HTTP_GET, []() {
    Serial.println("[WEB] GET / - Gerando página de configuração");
    server.send(200, "text/html", generateConfigPage());
  });

  // Rota para salvar configurações e reiniciar
  server.on("/save", HTTP_POST, []() {
    Serial.println("\n[WEB] =========================================");
    Serial.println("[WEB] === REQUISIÇÃO /save RECEBIDA ===");
    Serial.printf("[WEB] Args recebidos: %d\n", server.args());

    // Processa cada campo do formulário
    if (server.hasArg("callsign")) {
      String value = server.arg("callsign");
      value.toCharArray(config_callsign, sizeof(config_callsign));
      Serial.printf("[CONFIG] Callsign: %s\n", config_callsign);
    } else {
      Serial.println("[CONFIG] AVISO: Campo 'callsign' não recebido!");
    }

    if (server.hasArg("frequency")) {
      String value = server.arg("frequency");
      value.toCharArray(config_frequency, sizeof(config_frequency));
      Serial.printf("Frequência: %s\n", config_frequency);
    } else {
      Serial.println("AVISO: Campo 'frequency' não recebido!");
    }

    if (server.hasArg("frequency_unit")) {
      config_frequency_unit = server.arg("frequency_unit").toInt();
      Serial.printf("Unidade de Frequência: %s\n", config_frequency_unit == 0 ? "MHz" : "GHz");
    } else {
      Serial.println("AVISO: Campo 'frequency_unit' não recebido!");
    }

    if (server.hasArg("cw_message")) {
      String value = server.arg("cw_message");
      value.toCharArray(config_cw_message, sizeof(config_cw_message));
      Serial.printf("Mensagem CW: %s\n", config_cw_message);
    } else {
      Serial.println("AVISO: Campo 'cw_message' não recebido!");
    }

    if (server.hasArg("cw_wpm")) {
      config_cw_wpm = server.arg("cw_wpm").toInt();
      Serial.printf("CW WPM: %u\n", config_cw_wpm);
    } else {
      Serial.println("AVISO: Campo 'cw_wpm' não recebido!");
    }

    if (server.hasArg("cw_freq")) {
      config_cw_freq = server.arg("cw_freq").toInt();
      Serial.printf("CW Freq: %u Hz\n", config_cw_freq);
    } else {
      Serial.println("AVISO: Campo 'cw_freq' não recebido!");
    }

    if (server.hasArg("hang_time")) {
      config_hang_time = server.arg("hang_time").toInt();
      Serial.printf("Hang Time: %lu ms\n", config_hang_time);
    } else {
      Serial.println("AVISO: Campo 'hang_time' não recebido!");
    }

    if (server.hasArg("ptt_timeout")) {
      config_ptt_timeout = server.arg("ptt_timeout").toInt();
      Serial.printf("PTT Timeout: %lu ms\n", config_ptt_timeout);
    } else {
      Serial.println("AVISO: Campo 'ptt_timeout' não recebido!");
    }

    if (server.hasArg("voice_interval")) {
      config_voice_interval = server.arg("voice_interval").toInt() * 60000; // converte minutos para ms
      Serial.printf("[CONFIG] Voice Interval: %lu ms (%d minutos)\n", config_voice_interval, config_voice_interval / 60000);
    } else {
      Serial.println("[CONFIG] AVISO: Campo 'voice_interval' não recebido!");
    }

    if (server.hasArg("cw_interval")) {
      config_cw_interval = server.arg("cw_interval").toInt() * 60000; // converte minutos para ms
      Serial.printf("[CONFIG] CW Interval: %lu ms (%d minutos)\n", config_cw_interval, config_cw_interval / 60000);
    } else {
      Serial.println("[CONFIG] AVISO: Campo 'cw_interval' não recebido!");
    }

    if (server.hasArg("ct_change")) {
      config_ct_change = server.arg("ct_change").toInt();
      Serial.printf("CT Change: %lu QSOs\n", config_ct_change);
    } else {
      Serial.println("AVISO: Campo 'ct_change' não recebido!");
    }

    if (server.hasArg("ct_index")) {
      config_ct_index = server.arg("ct_index").toInt();
      ct_index = config_ct_index;
      Serial.printf("CT Index: %d\n", config_ct_index);
    } else {
      Serial.println("AVISO: Campo 'ct_index' não recebido!");
    }

    if (server.hasArg("volume")) {
      config_volume = server.arg("volume").toFloat() / 100.0f; // converte para 0-1
      Serial.printf("Volume: %.2f\n", config_volume);
    } else {
      Serial.println("AVISO: Campo 'volume' não recebido!");
    }

    if (server.hasArg("sample_rate")) {
      config_sample_rate = server.arg("sample_rate").toInt();
      Serial.printf("Sample Rate: %u Hz\n", config_sample_rate);
    } else {
      Serial.println("AVISO: Campo 'sample_rate' não recebido!");
    }

    // debug_level removido - sempre usa VERBOSE (nível máximo)
    config_debug_level = 3;
    DEBUG_LEVEL = 3;
    Serial.println("[CONFIG] Debug Level: 3 (VERBOSE - sempre máximo)");

    Serial.println("[CONFIG] === SALVANDO PREFERENCES ===");
    Serial.printf("[CONFIG] Voice Interval: %d min, CW Interval: %d min\n", 
                  config_voice_interval / 60000, config_cw_interval / 60000);
    // Salva as configurações no Preferences
    savePreferences();

    // Atualiza as variáveis globais
    // CALLSIGN já aponta para config_callsign, então não precisa reatribuir
    // Mas precisamos forçar redraw do display antes de reiniciar
    VOLUME = config_volume;
    
    // Força atualização do display com novas configurações antes de reiniciar
    needsFullRedraw = true;
    first_draw = true;  // Força redraw completo
    updateDisplay();  // Atualiza display imediatamente

    Serial.println("[WEB] === ENVIANDO RESPOSTA HTML ===");
    Serial.println("[WEB] =========================================\n");
    // Responde com página de sucesso e reinicia
    server.send(200, "text/html", getSuccessPage("Configurações salvas! Reiniciando o dispositivo..."));
    delay(2000);  // Aumentado para 2 segundos para dar tempo de ver a atualização
    Serial.println("[SYSTEM] === REINICIANDO ESP32 ===");
    ESP.restart();
  });

  // Rota para reiniciar o dispositivo
  server.on("/restart", HTTP_POST, []() {
    Serial.println("[WEB] POST /restart - Recebida requisição de reinício");
    savePreferences(); // Salva antes de reiniciar
    server.send(200, "text/html", getSuccessPage("Dispositivo será reiniciado em 3 segundos..."));
    delay(3000);
    ESP.restart();
  });

  // Rota para reset de fábrica
  server.on("/reset_factory", HTTP_POST, []() {
    Serial.println("[WEB] POST /reset_factory - === RESET DE FÁBRICA ===");
    preferences.begin("config", false);
    preferences.clear(); // Apaga todas as configurações
    preferences.end();
    server.send(200, "text/html", getSuccessPage("Reset de fábrica realizado! Reiniciando..."));
    delay(2000);
    ESP.restart();
  });

  // Rota para obter logs de debug
  server.on("/debug", HTTP_GET, []() {
    if (LittleFS.exists("/debug.log")) {
      File file = LittleFS.open("/debug.log", FILE_READ);
      if (file) {
        // Lê o arquivo completo
        size_t fileSize = file.size();
        String content;
        
        // Se o arquivo for muito grande (> 50KB), lê apenas as últimas linhas
        if (fileSize > 50000) {
          // Move para as últimas 30KB do arquivo
          file.seek(fileSize - 30000);
          // Lê o resto
          content = file.readString();
          content = "... (arquivo truncado, mostrando últimas linhas) ...\n" + content;
        } else {
          content = file.readString();
        }
        
        file.close();
        server.send(200, "text/plain", content);
        return;
      }
    }
    server.send(200, "text/plain", "Nenhum log de debug disponível ainda.\n\nOs logs aparecerão aqui quando houver atividade.\nUse o Serial Monitor (115200 baud) para ver logs em tempo real.");
  });

  // Inicia o servidor
  server.begin();
  Serial.println("Servidor web iniciado com sucesso!");
  Serial.printf("Acesse: http://%s/\n", WiFi.softAPIP().toString().c_str());
}

/**
 * @brief Inicia WiFi AP no boot (sempre ativo)
 *
 * Credenciais do WiFi (Anotar para configuração):
 * SSID: REPETIDORA_SETUP
 * Senha: repetidora123
 * IP: 192.168.4.1 (padrão do ESP32 em modo AP)
 *
 * Como configurar:
 * 1. Conecte no WiFi "REPETIDORA_SETUP" com senha "repetidora123"
 * 2. Abra o navegador e acesse: http://192.168.4.1
 * 3. Altere as configurações desejadas
 * 4. Clique em "Salvar e Reiniciar"
 * 5. O ESP32 reiniciará com as novas configurações
 */
void initWiFiAP() {
  Serial.println("Iniciando WiFi AP...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("=== CREDENCIAIS DO WIFI ===\n");
  Serial.printf("SSID: %s\n", AP_SSID);
  Serial.printf("Senha: %s\n", AP_PASSWORD);
  Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.println("========================");
}

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
  if (DEBUG_LEVEL >= 2) {
    Serial.printf("I2S: GPIO%d, %dHz\n", SPEAKER_PIN, rate);
  }
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
  if (DEBUG_LEVEL >= 2) {
    Serial.printf("Voz: %s (%dHz)\n", filename, rate);
  }
  
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
  
  if (DEBUG_EVENTS) {
    Serial.println("Voz concluída");
  }
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

  if (DEBUG_EVENTS) {
    Serial.printf("CW: %s (%d chars)\n", txt.c_str(), txt.length());
  }

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
      if (DEBUG_CW) {
        Serial.printf("CW: %c (%s)\n", c, code);
      }

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
      if (DEBUG_CW) {
        Serial.println("CW: Espaço");
      }
      // Espaço entre palavras (7 pontos)
      delay(7 * dotDuration);
    } else {
      if (DEBUG_CW) {
        Serial.printf("CW: Caractere inválido '%c'\n", c);
      }
    }
  }

  unsigned long endTime = millis();
  if (DEBUG_EVENTS) {
    Serial.printf("CW concluído: %lu ms\n", endTime - startTime);
  }

  // Delay final
  delay(50);

  // Limpa variáveis de exibição Morse
  current_morse_char[0] = '\0';
  current_morse_display[0] = '\0';

  // Limpeza: para I2S após reprodução
  i2s_driver_uninstall(I2S_NUM_0);
  i2s_ok = false;
  playing = false;
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

  // Log e timestamp (sempre mostra eventos PTT - são importantes)
  if (DEBUG_EVENTS) {
    Serial.println(on ? "PTT ON" : "PTT OFF");
  }
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
 * com o estado da repetidora, seguindo a mesma lógica do display.
 *
 * Estados implementados (seguem a mesma ordem de prioridade do display):
 * 1. Wi-Fi Ativo (show_ip_screen = true):
 *    - Cor: Azul fixo
 *    - Uso: Indica que a tela de Wi-Fi está ativa
 *
 * 2. TX Ativo (tx_mode != TX_NONE ou ptt_state = true):
 *    - Cor: Vermelho fixo (mesma cor do display vermelho)
 *    - Uso: Indica que está transmitindo
 *
 * 3. RX Ativo (cor_stable = true):
 *    - Cor: Amarelo (mesma cor do display amarelo)
 *    - Uso: Indica que está recebendo sinal
 *
 * 4. EM ESCUTA (idle):
 *    - Cor: Verde fixo (mesma cor do display verde)
 *    - Uso: Indica que está em espera
 *
 * Nota: Esta função deve ser chamada continuamente no loop principal
 *       para manter o LED sincronizado com o display
 *
 * @see loop() onde esta função é chamada
 * @see updateDisplay() para lógica de estados do display
 */
void updateLED() {
  // Debug: Log dos estados atuais
  static unsigned long last_led_debug = 0;
  static uint8_t last_state = 255;  // Para detectar mudanças de estado

  uint8_t current_state = 0;
  if (show_ip_screen) current_state = 1;
  else if (tx_mode != TX_NONE || ptt_state) current_state = 2;
  else if (cor_stable) current_state = 3;
  else current_state = 4;

  // Só loga quando o estado muda ou a cada 2 segundos
  if (current_state != last_state || millis() - last_led_debug > 2000) {
    Serial.printf("[LED] Estado: %s | tx_mode=%d, ptt_state=%d, cor_stable=%d, show_ip_screen=%d\n",
                  current_state == 1 ? "AZUL (WIFI)" :
                  current_state == 2 ? "VERMELHO (TX)" :
                  current_state == 3 ? "AMARELO (RX)" : "VERDE (IDLE)",
                  tx_mode, ptt_state, cor_stable, show_ip_screen);
    last_led_debug = millis();
    last_state = current_state;
  }

  // Prioridade 1: Wi-Fi ativo (tela de Wi-Fi mostrando)
  if (show_ip_screen) {
    // Wi-Fi ativo: Azul fixo
    // ACTIVE LOW: LOW = acende, HIGH = apaga
    digitalWrite(PIN_LED_R, HIGH);  // Vermelho apagado (HIGH)
    digitalWrite(PIN_LED_G, HIGH);  // Verde apagado (HIGH)
    digitalWrite(PIN_LED_B, LOW);   // Azul acende (LOW)
  }
  // Prioridade 2: TX ativo (qualquer modo de transmissão)
  else if (tx_mode != TX_NONE || ptt_state) {
    // TX ativo: Vermelho fixo (mesma cor do display vermelho)
    // ACTIVE LOW: LOW = acende, HIGH = apaga
    digitalWrite(PIN_LED_R, LOW);   // Vermelho acende (LOW)
    digitalWrite(PIN_LED_G, HIGH);  // Verde apagado (HIGH)
    digitalWrite(PIN_LED_B, HIGH);  // Azul apagado (HIGH)
  }
  // Prioridade 3: RX ativo (sinal recebido)
  else if (cor_stable) {
    // RX ativo: Amarelo (mesma cor do display amarelo)
    // ACTIVE LOW: LOW = acende, HIGH = apaga
    // Amarelo = Vermelho + Verde (ambos acendem)
    digitalWrite(PIN_LED_R, LOW);   // Vermelho acende (LOW)
    digitalWrite(PIN_LED_G, LOW);   // Verde acende (LOW)
    digitalWrite(PIN_LED_B, HIGH);  // Azul apagado (HIGH)
  }
  // Prioridade 4: EM ESCUTA (idle)
  else {
    // EM ESCUTA: Verde fixo (mesma cor do display verde)
    // ACTIVE LOW: LOW = acende, HIGH = apaga
    digitalWrite(PIN_LED_R, HIGH);  // Vermelho apagado (HIGH)
    digitalWrite(PIN_LED_G, LOW);   // Verde acende (LOW)
    digitalWrite(PIN_LED_B, HIGH);  // Azul apagado (HIGH)
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
    static bool last_show_ip_screen = false;

    // Redesenha header se: primeira vez OU redraw completo forçado OU show_ip_screen mudou
    // O callsign NÃO muda durante operação normal, então não precisa redesenhar sempre
    bool show_ip_changed = (show_ip_screen != last_show_ip_screen);
    if (!header_drawn || isFullRedraw || show_ip_changed) {
      tft.fillRect(0, 0, W, header_height, TFT_DARKBLUE);  // Limpa header
      tft.drawRoundRect(0, 0, W, header_height, 5, TFT_WHITE);  // Borda branca

      if (!show_ip_screen) {
        // Em modo normal, mostra callsign e frequência
        // Callsign (linha superior)
        tft.setTextColor(TFT_YELLOW, TFT_DARKBLUE);
        tft.setTextSize(3);
        int16_t textW = tft.textWidth(CALLSIGN);
        int16_t text_x = (W - textW) / 2;
        tft.setCursor(text_x, 5);
        tft.print(CALLSIGN);

        // Frequência (linha inferior, centralizada)
        tft.setTextColor(TFT_CYAN, TFT_DARKBLUE);
        tft.setTextSize(2);
        char freq_str[20];
        const char* unit = (config_frequency_unit == 1) ? "GHz" : "MHz";
        snprintf(freq_str, sizeof(freq_str), "%s %s", config_frequency, unit);
        int16_t freqW = tft.textWidth(freq_str);
        int16_t freq_x = (W - freqW) / 2;
        tft.setCursor(freq_x, 35);
        tft.print(freq_str);
      } else {
        // Em modo WiFi, mostra "WIFI AP ATIVO" no header
        tft.setTextColor(TFT_YELLOW, TFT_DARKBLUE);
        tft.setTextSize(3);
        const char* wifi_title = "WIFI AP ATIVO";
        int16_t titleW = tft.textWidth(wifi_title);
        int16_t title_x = (W - titleW) / 2;
        tft.setCursor(title_x, 18);
        tft.print(wifi_title);
      }

      // Linha separadora
      tft.drawFastHLine(5, header_height, W - 10, TFT_DARKGREY);
      header_drawn = true;
      last_show_ip_screen = show_ip_screen;  // Atualiza estado anterior
    }
    
    // ========== STATUS PRINCIPAL (Centro, caixa grande) ==========
    uint16_t status_bg, status_text_color;
    const char* status_text;
    const char* status_subtext = "";
    
    // Determina estado e texto baseado no modo de transmissão
    if (show_ip_screen) {
      // MOSTRAR IP E CREDENCIAIS (BOOT button pressionado)
      status_bg = TFT_CYAN;
      status_text_color = TFT_BLACK;
      status_text = "";  // Não desenha texto principal (está no header)
      status_subtext = "";  // Será desenhado separadamente abaixo
    } else if (tx_mode == TX_VOICE) {
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
    
    // Debug: Log do estado atual do display (só quando mudar)
    static uint16_t last_logged_bg = 0xFFFF;
    static TxMode last_logged_tx_mode = TX_NONE;
    if (DEBUG_DISPLAY && (status_bg != last_logged_bg || tx_mode != last_logged_tx_mode)) {
      Serial.printf("DISPLAY STATE: tx_mode=%d, ptt_state=%d, cor_stable=%d, status_bg=0x%04X, text='%s'\n",
                    tx_mode, ptt_state, cor_stable, status_bg, status_text);
      last_logged_bg = status_bg;
      last_logged_tx_mode = tx_mode;
    }
    
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
      
      if (DEBUG_DISPLAY) {
        Serial.printf("STATUS: %s (bg=0x%04X)\n", status_text, status_bg);
      }
    }
    
    // Declara variáveis para uso em debug e limpeza (fora do if para estar no escopo correto)
    int16_t status_text_w = 0;
    int16_t status_text_y = 0;
    int16_t clear_x = 0;
    
    // Texto de status grande - SEMPRE redesenhado para garantir visibilidade
    // NÃO desenha se está em modo WiFi (texto está no header)
    if (status_text[0] != '\0' && !show_ip_screen) {
      tft.setTextColor(status_text_color, status_bg);
      tft.setTextSize(3);
      
      // Usa setCursor e print ao invés de drawCentreString para garantir funcionamento
      status_text_w = tft.textWidth(status_text);
      // Centralizado verticalmente com um pequeno ajuste (+2px) para visualização melhor
      status_text_y = status_y + (status_h - 24) / 2 + 2; 
      
      // LIMPEZA EXTRA: Apaga retângulo exato onde o texto vai ficar antes de escrever
      // Sempre limpa para evitar texto fantasma, especialmente em modo "EM ESCUTA"
      clear_x = (W - status_text_w) / 2;
      // Limpa área do texto principal (não limpa área abaixo para não interferir)
      tft.fillRect(10, status_text_y - 2, W - 20, 28, status_bg);

      // Desenha o texto principal
      tft.setCursor(clear_x, status_text_y);
      tft.print(status_text);
    } else if (show_ip_screen) {
      // Em modo WiFi, limpa área do texto principal
      tft.fillRect(10, status_y, W - 20, 30, status_bg);
      // Define valores padrão para evitar uso de variáveis não inicializadas
      status_text_y = status_y + (status_h - 24) / 2 + 2;
    }
    
    // Se está em modo WiFi, desenha informações do WiFi no centro (texto maior)
    if (show_ip_screen) {
      tft.setTextSize(2);  // Aumentado de 1 para 2 (texto maior)
      tft.setTextColor(TFT_BLACK, status_bg);
      
      // SSID
      char ssid_line[64];
      snprintf(ssid_line, sizeof(ssid_line), "SSID: %s", AP_SSID);
      int16_t ssid_w = tft.textWidth(ssid_line);
      tft.setCursor((W - ssid_w) / 2, status_y + 25);
      tft.print(ssid_line);
      
      // Senha
      char pwd_line[64];
      snprintf(pwd_line, sizeof(pwd_line), "Senha: %s", AP_PASSWORD);
      int16_t pwd_w = tft.textWidth(pwd_line);
      tft.setCursor((W - pwd_w) / 2, status_y + 45);
      tft.print(pwd_line);
      
      // IP
      char ip_line[64];
      snprintf(ip_line, sizeof(ip_line), "IP: %s", WiFi.softAPIP().toString().c_str());
      int16_t ip_w = tft.textWidth(ip_line);
      tft.setCursor((W - ip_w) / 2, status_y + 65);
      tft.print(ip_line);
    }
    
    // Debug adicional para modo "EM ESCUTA" (apenas em modo verbose)
    if (DEBUG_JSON && status_bg == TFT_GREEN && status_text_w > 0) {
      Serial.printf("TEXTO 'EM ESCUTA' DESENHADO: x=%d, y=%d, w=%d\n", 
                    clear_x, status_text_y, status_text_w);
    }
    
    // Subtexto (para modo CW/Voz ou QSO) - NÃO desenha se está em modo WiFi
    if (status_subtext[0] != '\0' && !show_ip_screen) {
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
    } else if (ptt_state && tx_mode == TX_NONE && !show_ip_screen) {
      // Modo RX normal - mostra QSO ATUAL (não desenha se está em modo WiFi)
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE, status_bg);
      int16_t qso_w = tft.textWidth("QSO ATUAL");
      
      // Limpeza prévia
      tft.fillRect(10, status_y + 60, W - 20, 16, status_bg);

      tft.setCursor((W - qso_w) / 2, status_y + 60);
      tft.print("QSO ATUAL");
    } else if (!show_ip_screen && status_text_y > 0) {
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
  
    // ========== COURTESY TONE (Abaixo do status) - NÃO mostra em modo WiFi ==========
    // Mostra apenas o NOME do CT na caixa verde (sem número, que fica no rodapé)
    if (!show_ip_screen) {
      int16_t ct_y = 155;  // Ajustado de 145 para 155
      static uint8_t last_ct_index = 255;  // Para detectar mudança
      
      // Só redesenha CT se mudou ou primeira vez
      if (ct_index != last_ct_index || isFullRedraw) {
        tft.fillRoundRect(10, ct_y, W - 20, 35, 5, TFT_DARKGREEN);  // Caixa verde com bordas arredondadas    
        tft.drawRoundRect(10, ct_y, W - 20, 35, 5, TFT_CYAN);  // Borda ciano
        last_ct_index = ct_index;
      }

      // Mostra apenas o NOME do CT (sem número - número fica no rodapé)
      tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
      tft.setTextSize(2);
      tft.setCursor(20, ct_y + 8);
      tft.print("CT: ");
      tft.setTextColor(TFT_YELLOW, TFT_DARKGREEN);
      tft.print(tones[ct_index].name);
      // NÃO mostra o número aqui (01/33) - ele fica no rodapé
    } else {
      // Em modo WiFi, limpa a área do CT
      tft.fillRect(10, 155, W - 20, 35, TFT_BLACK);
    }
  
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

    // Coluna 3: CT Index (direita) - SEMPRE mostra (tanto em modo normal quanto WiFi)
    // Usando largura segura do texto (~45px para "XX/33")
    int16_t ct_text_w = tft.textWidth("00/33");
    
    // Label "CT:"
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(W - ct_text_w - 5, footer_y + 5);
    tft.print("CT:");
    
    // Valor do CT (número)
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
 * - LED RGB com digitalWrite()
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
 * 10. Configura LED RGB com digitalWrite()
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
  pinMode(PIN_BOOT, INPUT_PULLUP);  // BOOT button com pullup interno
  Serial.printf("GPIOs configurados - COR: GPIO%d, PTT: GPIO%d, BOOT: GPIO%d\n",
              PIN_COR, PIN_PTT, PIN_BOOT);
  Serial.printf("Speaker: GPIO%d\n", SPEAKER_PIN);

  // ========== CONFIGURAÇÃO DO LED RGB ==========
  // O LED RGB é controlado via digitalWrite() para máxima compatibilidade
  //
  // Especificações do LED RGB:
  // - Tipo: ACTIVE LOW (LOW acende, HIGH apaga) - conforme ESP32-2432S028R
  // - Pinos: R=GPIO4, G=GPIO16, B=GPIO17
  // - Controle: digitalWrite() simples (HIGH=apagado, LOW=aceso)
  // - Nota: Não usamos PWM porque para LEDs ligado/desligado, digitalWrite() é mais confiável
  //
  // CORREÇÃO CRÍTICA: Usar digitalWrite() em vez de PWM/LEDC para evitar problemas
  // de compatibilidade com diferentes versões do ESP32 Arduino Core

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  // Inicializa LEDs apagados (active low: HIGH = apagado)
  digitalWrite(PIN_LED_R, HIGH);  // Apaga Vermelho
  digitalWrite(PIN_LED_G, HIGH);  // Apaga Verde
  digitalWrite(PIN_LED_B, HIGH);  // Apaga Azul

  Serial.printf("LED RGB configurado (digitalWrite) - Pinos: R=GPIO%d, G=GPIO%d, B=GPIO%d\n",
                PIN_LED_R, PIN_LED_G, PIN_LED_B);

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

  // Carrega configurações salvas do Preferences
  loadPreferences();
  
  // Após carregar configurações, força redraw do display com os valores corretos
  needsFullRedraw = true;
  first_draw = true;
  updateDisplay();

  // Inicia WiFi AP (sempre ativo para configuração)
  initWiFiAP();

  // Inicializa o servidor web
  initWebServer();

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
    if (DEBUG_LEVEL >= 2) {  // Só mostra em nível NORMAL ou superior
      Serial.printf("Loop: count=%lu, heap=%d, uptime=%lums\n", loopCount, ESP.getFreeHeap(), millis());
    }
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

  // Debug: Verifica estado do PTT periodicamente (apenas se habilitado)
  if (DEBUG_PTT) {
    static unsigned long last_ptt_debug = 0;
    if (millis() - last_ptt_debug >= 10000) {  // A cada 10 segundos (reduzido de 2s)
      last_ptt_debug = millis();
      bool ptt_pin_state = digitalRead(PIN_PTT);
      Serial.printf("PTT: state=%d, pin=%d, cor=%d, tx_mode=%d\n",
                    ptt_state, ptt_pin_state, cor, tx_mode);
    }
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
    if (DEBUG_EVENTS) {
      Serial.printf("COR: %d -> %d\n", cor_stable, cor);
    }
    // #endregion
    cor_stable = cor;
    needsFullRedraw = true;  // Marca para redraw completo

    if (cor_stable && !ptt_locked) {
      // COR ativado → INÍCIO DO QSO → PTT ON (usando setPTT)
      setPTT(true);
    } else if (!cor_stable && ptt_state && !ptt_locked) {
      // COR desativado → FIM DO QSO → INCREMENTA CONTADOR → HANG TIME → CT → PTT OFF
      qso_count++;  // CRÍTICO: Incrementa contador de QSOs (conforme código original)
      delay(HANG_TIME_MS);  // Aguarda hang time (600ms)
      if (!playing) {
        playCT();  // Reproduz courtesy tone selecionado
      }
      setPTT(false);

      // Troca automática do CT a cada 5 QSOs (código original)
      if (qso_count % QSO_CT_CHANGE == 0) {
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

  // ========== CONTROLE DE BOOT BUTTON (GPIO 0) ==========
  // Lê o estado do BOOT button (LOW = pressionado)
  bool boot_pressed = (digitalRead(PIN_BOOT) == LOW);
  static bool reset_warning_shown = false;
  static unsigned long last_boot_toggle = 0;
  const unsigned long BOOT_DEBOUNCE_MS = 150;  // Debounce reduzido para 150ms (mais responsivo)

  if (boot_pressed && !boot_button_pressed) {
    // Botão acaba de ser pressionado
    boot_button_pressed = true;
    boot_button_start = millis();
    reset_warning_shown = false;
    if (DEBUG_EVENTS) Serial.println("[BOOT] Botão pressionado");
  }

  // Se o botão está pressionado, verifica o tempo
  if (boot_pressed && boot_button_pressed) {
    unsigned long press_duration = millis() - boot_button_start;
    
    // Se foi pressionado por mais de 5 segundos = RESET DE FÁBRICA
    if (press_duration >= RESET_FACTORY_MS) {
      // Mostra alerta de reset na tela (apenas uma vez)
      if (!reset_warning_shown) {
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setTextSize(2);
        tft.setCursor(50, 100);
        tft.println("ATENCAO!");
        tft.setCursor(30, 130);
        tft.println("SOLTAR PARA");
        tft.setCursor(30, 160);
        tft.println("RESET DE FABRICA");
        reset_warning_shown = true;
        Serial.println("[BOOT] === AVISO: RESET DE FÁBRICA (aguardando soltar botão) ===");
      }
    }
  }

  // Quando o botão é solto
  if (!boot_pressed && boot_button_pressed) {
    unsigned long press_duration = millis() - boot_button_start;
    boot_button_pressed = false;
    reset_warning_shown = false;

    // Verifica se foi pressionado por mais de 5 segundos = RESET DE FÁBRICA
    // Reset de fábrica funciona mesmo durante TX
    if (press_duration >= RESET_FACTORY_MS) {
      Serial.println("[BOOT] === RESET DE FÁBRICA SOLICITADO ===");
      preferences.begin("config", false);
      preferences.clear(); // Apaga todas as configurações
      preferences.end();
      Serial.println("[BOOT] Configurações apagadas - Reiniciando...");

      // Mostra alerta visual
      tft.fillScreen(TFT_RED);
      tft.setTextColor(TFT_WHITE, TFT_RED);
      tft.setTextSize(2);
      tft.setCursor(50, 100);
      tft.println("ATENÇÃO!");
      tft.setCursor(30, 130);
      tft.println("RESET FÁBRICA");
      tft.setCursor(30, 160);
      tft.println("REALIZADO!");

      delay(1500);
      ESP.restart();
    } else if (press_duration > 50) {  // Ignora toques muito rápidos (< 50ms)
      // Verifica se está em TX - se estiver, não permite alternar tela
      if (ptt_state || tx_mode != TX_NONE) {
        Serial.println("[BOOT] Botão ignorado - Repetidora está em TX");
        if (DEBUG_EVENTS) {
          Serial.printf("[BOOT] ptt_state=%d, tx_mode=%d\n", ptt_state, tx_mode);
        }
      } else {
        // Debounce: evita múltiplas detecções muito rápidas
        unsigned long now = millis();
        if (now - last_boot_toggle >= BOOT_DEBOUNCE_MS) {
          // Foi um toque curto - TOGGLE DA TELA (alternar entre normal e WiFi)
          show_ip_screen = !show_ip_screen;  // Alterna o estado
          needsFullRedraw = true;
          first_draw = true;  // Força redraw completo
          last_boot_toggle = now;

          if (show_ip_screen) {
            Serial.println("[BOOT] Toggle -> Mostrando TELA DO WIFI");
          } else {
            Serial.println("[BOOT] Toggle -> Voltando para TELA NORMAL");
          }
          
          // Atualiza display imediatamente
          updateDisplay();
        }
      }
    }
  }

  // ========== CONTROLE DE TOUCHSCREEN (MODO NORMAL) ==========
  if (!show_ip_screen && ts.touched()) {
    TS_Point p = ts.getPoint();

    // Filtra touch com coordenadas inválidas (valores muito altos indicam touch falso)
    if (p.z > 600 && p.x < 8000 && p.y < 8000) {
      // Avança para o próximo courtesy tone (circular)
      ct_index = (ct_index + 1) % N_CT;
      config_ct_index = ct_index;  // Atualiza config também
      needsFullRedraw = true;
      updateDisplay();

      Serial.printf("Touch - Novo CT: %s (%d/33)\n",
                   tones[ct_index].name, ct_index + 1);

      // Debounce: Delay para evitar troca rápida acidental
      delay(500);

      // Espera soltar o dedo (evita múltiplas detecções)
      while (ts.touched()) {
        delay(10);
      }
    }
  }

  // ========== PROCESSA REQUISIÇÕES DO SERVIDOR WEB (sempre ativo) ==========
  server.handleClient();
  
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
      if (DEBUG_EVENTS) {
        Serial.println("=== ID INICIAL VOZ ===");
      }
      unsigned long ptt_start_time = millis();
      tx_mode = TX_VOICE;
      updateDisplay();
      digitalWrite(PIN_PTT, HIGH);
      delay(100);  // Aguarda estabilização do PTT
      
      playVoiceFile("/id_voz_8k16.wav");
      
      // Desativa PTT IMEDIATAMENTE após reprodução terminar
      // Não espera delay adicional - playVoiceFile() já terminou
      digitalWrite(PIN_PTT, LOW);
      unsigned long ptt_end_time = millis();
      unsigned long ptt_duration = ptt_end_time - ptt_start_time;
      
      if (DEBUG_EVENTS) {
        Serial.printf("ID Voz: %.1fs\n", ptt_duration / 1000.0f);
      }
      
      // Verifica se PTT ficou aberto por muito tempo (sempre mostra avisos)
      if (ptt_duration > 25000) {  // Mais de 25 segundos
        Serial.printf("AVISO: PTT aberto por muito tempo! %.1fs\n", ptt_duration / 1000.0f);
      }
      
      delay(50);  // Pequeno delay antes de mudar modo
      
      tx_mode = TX_NONE;
      updateDisplay();
      #else
      Serial.println("=== PULANDO IDENTIFICAÇÃO INICIAL EM VOZ (arquivo ausente) ===");
      delay(2000);
      #endif
      
      initial_voice_done = true;      // Marca voz como feita
      cw_timer_start = millis();      // Inicia contagem para o CW
    }
    
    // 2. ID Inicial em CW (Executa 5 segundos APÓS a voz terminar)
    else if (initial_voice_done && (millis() - cw_timer_start >= 5000)) {
      if (DEBUG_EVENTS) {
        Serial.println("=== ID INICIAL CW ===");
      }
      tx_mode = TX_CW;
      updateDisplay();
      digitalWrite(PIN_PTT, HIGH);
      delay(100);
      playCW(CALLSIGN);
      delay(100);
      digitalWrite(PIN_PTT, LOW);
      tx_mode = TX_NONE;
      updateDisplay();

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
    if (DEBUG_EVENTS) {
      Serial.println("=== ID VOZ (11min) ===");
    }
    unsigned long ptt_start_time = millis();
    tx_mode = TX_VOICE;  // Define modo de transmissão
    updateDisplay();  // Atualiza display para mostrar TX VOZ
    digitalWrite(PIN_PTT, HIGH);  // PTT ON
    delay(100);  // Aguarda estabilização do PTT
    
    playVoiceFile("/id_voz_8k16.wav");  // Toca indicativo de voz
    
    // Desativa PTT IMEDIATAMENTE após reprodução terminar
    digitalWrite(PIN_PTT, LOW);   // PTT OFF
    unsigned long ptt_end_time = millis();
    unsigned long ptt_duration = ptt_end_time - ptt_start_time;
    
    if (DEBUG_EVENTS) {
      Serial.printf("ID Voz: %.1fs\n", ptt_duration / 1000.0f);
    }
    
    // Verifica se PTT ficou aberto por muito tempo (sempre mostra avisos)
    if (ptt_duration > 25000) {  // Mais de 25 segundos
      Serial.printf("AVISO: PTT aberto por muito tempo! %.1fs\n", ptt_duration / 1000.0f);
    }
    
    delay(50);  // Pequeno delay antes de mudar modo
    
    tx_mode = TX_NONE;  // Reseta modo de transmissão
    updateDisplay();  // Volta para estado normal
  }

  // Identificação em CW (Morse) a cada 16 minutos (após IDs iniciais)
  if (initial_id_done && millis() - last_cw >= CW_INTERVAL_MS && !playing && !ptt_state) {
    last_cw = millis();
    if (DEBUG_EVENTS) {
      Serial.println("=== ID CW (16min) ===");
    }
    tx_mode = TX_CW;  // Define modo de transmissão
    updateDisplay();  // Atualiza display para mostrar TX CW
    digitalWrite(PIN_PTT, HIGH);  // PTT ON
    delay(100);
    playCW(CALLSIGN);  // Toca callsign em Morse
    delay(100);
    digitalWrite(PIN_PTT, LOW);   // PTT OFF
    tx_mode = TX_NONE;  // Reseta modo de transmissão
    updateDisplay();  // Volta para estado normal
  }
}
