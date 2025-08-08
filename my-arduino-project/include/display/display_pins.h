// Physical RGB data pin assignment (Waveshare ESP32-S3 Touch LCD 4.3B)
// Mapping extracted from schematic; using 16-bit RGB565 subset.
// NOTE: Verify all pin mappings against final hardware rev before enabling ENABLE_RGB_PANEL.

#pragma once

#include <stdint.h>

// Red channel (5 bits: R3..R7)
#define LCD_PIN_R3 1
#define LCD_PIN_R4 2
#define LCD_PIN_R5 42
#define LCD_PIN_R6 41
#define LCD_PIN_R7 40

// Green channel (6 bits: G2..G7)
#define LCD_PIN_G2 39
#define LCD_PIN_G3 0
#define LCD_PIN_G4 45
#define LCD_PIN_G5 48
#define LCD_PIN_G6 47
#define LCD_PIN_G7 21

// Blue channel (5 bits: B3..B7)
#define LCD_PIN_B3 14
#define LCD_PIN_B4 38
#define LCD_PIN_B5 18
#define LCD_PIN_B6 17
#define LCD_PIN_B7 10

// Control signals provided via build flags (platformio.ini):
// DISPLAY_DE_PIN, DISPLAY_VSYNC_PIN, DISPLAY_HSYNC_PIN, DISPLAY_PCLK_PIN
// Additional control via CH422G expander: DISP (enable), LCD_RST, CTP_RST.
