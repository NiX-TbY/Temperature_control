# Waveshare ESP32-S3 Touch LCD 4.3B Pinout Validation

This document consolidates the pin usage as implemented in code vs. hardware references.

## Display RGB Panel (LovyanGFX Bus_RGB)
| Function | GPIO | Macro | Notes |
|----------|------|-------|-------|
| DE | 5 | DISPLAY_DE_PIN | Data Enable (active high) |
| VSYNC | 3 | DISPLAY_VSYNC_PIN | Active low (provisional) |
| HSYNC | 46 | DISPLAY_HSYNC_PIN | Active low (provisional) |
| PCLK | 7 | DISPLAY_PCLK_PIN | ~25MHz start |
| R3 | 1 | LCD_PIN_R3 | Red bit 3 |
| R4 | 2 | LCD_PIN_R4 | Red bit 4 |
| R5 | 42 | LCD_PIN_R5 | Red bit 5 |
| R6 | 41 | LCD_PIN_R6 | Red bit 6 |
| R7 | 40 | LCD_PIN_R7 | Red bit 7 |
| G2 | 39 | LCD_PIN_G2 | Green bit 2 |
| G3 | 0 | LCD_PIN_G3 | Green bit 3 (strap pin caution) |
| G4 | 45 | LCD_PIN_G4 | Green bit 4 |
| G5 | 48 | LCD_PIN_G5 | Green bit 5 |
| G6 | 47 | LCD_PIN_G6 | Green bit 6 |
| G7 | 21 | LCD_PIN_G7 | Green bit 7 |
| B3 | 14 | LCD_PIN_B3 | Blue bit 3 |
| B4 | 38 | LCD_PIN_B4 | Blue bit 4 |
| B5 | 18 | LCD_PIN_B5 | Blue bit 5 |
| B6 | 17 | LCD_PIN_B6 | Blue bit 6 |
| B7 | 10 | LCD_PIN_B7 | Blue bit 7 |

## CH422G (I²C 0x71) Expander Logical Lines
| Expander | Function | Use in Code |
|----------|----------|-------------|
| EXIO1 | CTP_RST | resetTouch() sequence |
| EXIO2 | DISP | setDisplayEnable(true/false) |
| EXIO3 | LCD_RST | resetPanel() sequence |
| EXIO4 | SDCS | (future SD control) |
| EXIO0/5 | Spare | Future DI/DO |

(Actual register-level writes still TODO in `ch422g.cpp`).

## Touch (GT911)
| Signal | GPIO / Source | Notes |
|--------|---------------|-------|
| SDA | 8 | Shared I²C bus |
| SCL | 9 | Shared I²C bus |
| IRQ | 4 | TOUCH_IRQ_PIN (interrupt attached FALLING) |
| RST | EXIO1 | Via expander |

Driver: `display/gt911.h` minimal poll-based; no interrupt handling yet.

## Sensors
| Peripheral | GPIO | Notes |
|------------|------|-------|
| DS18B20 OneWire | 33 | Pull-up required on board/wiring |
| RTC INT (PCF85063) | 6 | Captured for future wake/events |

## Relays (PCF8574 @0x20)
Mapping tentative; code assumes active LOW outputs:
| PCF8574 Bit | Function |
|-------------|----------|
| 0 | COMPRESSOR |
| 1 | HOTGAS |
| 2 | ELECTRIC_HEATER |
| 3 | FAN_MAIN |
| 4 | FAN_AUX (spare) |
| 5 | ALARM (spare) |
| 6 | Spare |
| 7 | Spare |

## Direct Control / Misc
| Function | GPIO | Notes |
|----------|------|-------|
| Buzzer | 37 | Digital on/off (PWM TBD) |
| Fan PWM | 36 | LEDC channel 1 25kHz |
| Heat Relay (legacy direct) | 34 | Direct pin (legacy; prefer expander) |
| Cool Relay (legacy direct) | 35 | Direct pin (legacy; prefer expander) |

## Outstanding Validation / TODO
- Confirm ST7262 porch & pulse timing; update `lgfx_rgb.h` polarities if needed.
- Implement CH422G register writes (currently placeholders) and readback.
- Add interrupt-driven GT911 handler (attachInterrupt on GPIO4) for lower latency & reduced polling.
- Migrate relays fully to expander (deprecate direct RELAY_* pins in `config.h`).
- Backlight PWM via expander (or dedicated BL pin) with smooth ramps (current: binary on/off through expander; PWM fallback attaches temporarily to DE when expander missing – replace with real BL pin).
- Guard use of strapping pins (GPIO0, 45, 46) during boot sequences.

Generated automatically as part of audit pass.

### ST7262 Controller Notes
- HDPOL/VDPOL in ST7262 map to HSYNC/VSYNC polarity; current config uses active low (HDPOL=0, VDPOL=0 assumption) consistent with typical 800x480 panels.
- DE is active high (de_idle_high=0 in lgfx config so idle low, active high).
- Pixel clock uses rising edge latch (pclk_active_neg=0 => rising edge active).
- If panel datasheet indicates differing polarities, adjust `DEFAULT_RGB_TIMING` fields: hsync_active_low, vsync_active_low, pclk_rising_edge, de_active_high.
