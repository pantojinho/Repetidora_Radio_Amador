// ==================================================================
// TFT_eSPI User Setup - ESP32-2432S028 (Cheap Yellow Display)
// Display TFT 2.8" ILI9341 - 240x320 (retrato lógico)
// ==================================================================
// Local: Arduino/libraries/TFT_eSPI/User_Setup.h
// ==================================================================

// ---------------- DRIVER ----------------
//#define ILI9341_DRIVER
#define ILI9341_2_DRIVER   // <--- ESSENCIAL pro teu board CYD! Elimina ghosting

// ---------------- RESOLUÇÃO ----------------
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

// ---------------- SPI (DISPLAY) ----------------
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14

#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1   // Reset ligado ao EN da placa

// ---------------- BACKLIGHT ----------------
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// ---------------- CORREÇÕES DE COR/GHOSTING ----------------
#define TFT_RGB_ORDER TFT_BGR  // Inverte RGB (corrige cores erradas)
#define TFT_INVERSION_ON       // Ativa inversão (resolve ghosting e texto invisível em muitos CYD)

// ---------------- FONTES ----------------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

// ---------------- OTIMIZAÇÕES ----------------
#define SPI_FREQUENCY        20000000   // Reduzido para 20MHz (elimina flicker branco no CYD)
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000

#define SPI_USE_HW_SPI

// ---------------- DESATIVA O QUE NÃO USA ----------------
//#define SMOOTH_FONT   // opcional – pode causar fragmentação se não usado