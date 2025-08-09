# Waveshare ESP32-S3 Touch LCD 4.3" (Type B) – Integration Reference (Draft)

> DRAFT NOTE: This file is a structured extraction template. Precise numeric values (timings, pin mappings, register bitfields) must be verified directly against the PDFs in `Waveshare 4.3 data sheets/`. Placeholders marked TODO?. Supply confirmations and this document can be finalized.

## 1. Board Composition Overview
- Core MCU: ESP32-S3 module (WiFi + BLE, dual‑core Xtensa LX7, with LCD & Camera peripherals, PSRAM capable)
- LCD Panel: 4.3" 800x480 RGB interface TFT (controller-on-glass or external driver – datasheet: ST7262.pdf)
- Touch Controller: GT911 (capacitive multi‑touch, I2C, interrupt + reset lines)
- I/O Expander: CH422G (extends GPIO / drives reset / backlight / optional control signals)
- USB–UART / USB HS Bridge: CH343 (datasheet CH343DS1-en.pdf)
- CAN Bus Transceiver: TJA1051 (if CAN present on expansion)
- Power Regulation: (Refer to schematic – 5V → 3V3 LDO / DCDC, backlight boost if present) – TODO?
- External Flash / PSRAM: (On-module for ESP32-S3 – confirm size) – TODO?

## 2. Power & Rails
| Rail | Source | Typical Use | Notes |
|------|--------|-------------|-------|
| 5V (VBUS) | USB / ext header | Input to regulator, panel backlight boost | Ensure inrush limited |
| 3V3 | On-board regulator | ESP32-S3, logic, touch, IO expander | Current margin? TODO? |
| LED / BL | Boost / switched rail | LCD backlight LEDs | CH422G likely enables PWM / ON/OFF via pin (verify) |

Backlight current & voltage per panel datasheet: TODO?

## 3. Key IC Summaries
### 3.1 ST7262 (LCD Related)
- Function: Panel driver / timing (confirm: Is ST7262 gate/ source driver or a timing controller?)
- Interface: 24‑bit parallel RGB (HSYNC / VSYNC / DE / PCLK) provided by ESP32-S3 LCD peripheral.
- Critical timing parameters: See Section 4.
- Power sequencing: Typical order: logic power → panel power → delay → deassert RESET → enable backlight after valid frames. Exact delays TODO?

### 3.2 GT911 Touch Controller
- I2C Addresses: 0x5D (default) / 0x14 (alternative) – already defined.
- Lines: SDA, SCL, INT (trigger low on touch), RST (for address selection & reset).
- Multi-touch points: Up to 5 (configured). Coordinate range must match 800x480.
- Init sequence: Optional configuration table write (firmware loads baseline config). TODO: Provide config register block if customization required.

### 3.3 CH422G IO Expander
- Role: Extends GPIO; likely handles RESET_n, BACKLIGHT_EN / PWM, maybe other panel control pins.
- Bus: I2C (0x71 defined).
- Required functions for firmware:
  1. Probe (ACK check)
  2. Set pin modes or direction (if supported) – TODO bit layout
  3. Control backlight (either discrete bit or PWM register) – TODO register
  4. Assert / deassert LCD reset (timing per panel) – TODO mapping
- Datasheet fields to extract: device ID (if any), register map, I/O direction control, output latch registers.

### 3.4 CH343 USB Interface
- Provides USB ↔ UART (and possibly other modes). Baud rates supported: TODO.
- Driver requirement on host: Standard CH34x. No special firmware config required.

### 3.5 TJA1051 CAN Transceiver
- Supply: 5V (or 3V3 variant? Confirm from schematic) – TODO.
- TXD/RXD pins to ESP32-S3 UART or TWAI peripheral – TODO mapping.
- Standby / enable pin (if present) should be tied / controlled – TODO.

## 4. LCD (RGB) Interface & Timings
Panel native: 800 (H active) × 480 (V active). Typical 60 Hz WVGA baseline (VERIFY!):
- Pixel Clock ≈ 33.3 MHz (PCLK = TotalPixels × FrameRate)
- Horizontal:
  - Active: 800
  - Front Porch: 40 (TODO?)
  - Sync Pulse: 48 (TODO?)
  - Back Porch: 88 (TODO?)
  - Total: 976
- Vertical:
  - Active: 480
  - Front Porch: 13 (TODO?)
  - Sync Pulse: 3 (TODO?)
  - Back Porch: 32 (TODO?)
  - Total: 528
- Polarity (HSYNC, VSYNC, DE, PCLK edge): TODO (common: HSYNC low, VSYNC low, DE high, data latch on rising PCLK).

Provide actual datasheet values to replace all TODO fields before finalizing the driver config.

## 5. Pin Mapping (Board Level)
| Function | ESP32-S3 GPIO | Direction | Notes / Source | Final? |
|----------|---------------|-----------|----------------|--------|
| DE | 5 | OUT | Already defined in build_flags | ✔ |
| VSYNC | 3 | OUT | build_flags | ✔ |
| HSYNC | 46 | OUT | build_flags | ✔ |
| PCLK | 7 | OUT | build_flags | ✔ |
| I2C SDA | 8 | BIDIR | Touch, CH422G | ✔ |
| I2C SCL | 9 | OUT | Touch, CH422G | ✔ |
| TOUCH INT | 4 | IN | GT911 IRQ | ✔ |
| TOUCH RST | TODO | OUT | Needed for GT911 reset/addr select | — |
| LCD R0..R7 | TODO | OUT | 8 bits Red | — |
| LCD G0..G7 | TODO | OUT | 8 bits Green | — |
| LCD B0..B7 | TODO | OUT | 8 bits Blue | — |
| LCD RESET | (via CH422G?) | OUT | Panel reset | — |
| BACKLIGHT_EN / PWM | (via CH422G?) | OUT | Brightness control | — |
| CAN TX / RX | TODO | ALT | If CAN used | — |
| Others (UART, SD, etc.) | TODO | | From schematic | — |

Please fill the exact GPIO mappings from `ESP32-S3-Touch-LCD-4.3B-Sch.pdf`.

## 6. Initialization Sequence (Recommended)
1. Power rails stable (3V3, panel power, backlight off)
2. Initialize I2C → probe CH422G
3. Assert LCD RESET low (via expander) – delay (e.g. 10 ms) – TODO
4. Deassert LCD RESET – delay per panel (e.g. 120 ms) – TODO
5. (Optional) Load panel gamma / extended registers if required by ST7262 – TODO
6. Configure touch: toggle GT911 RST + INT per address selection, read product ID
7. Configure RGB timing in LovyanGFX (Bus_RGB) with verified porches & polarity
8. Enable LVGL and register flush/read callbacks
9. Enable backlight gradually (PWM ramp) after first valid frame(s)
10. Enter main loop (lv_timer_handler) / tasks.

## 7. LovyanGFX Configuration Blueprint (To Implement)
```cpp
// Pseudocode skeleton – fill with actual pins
lgfx::Panel_RGB panel;
lgfx::Bus_RGB bus;

auto bcfg = bus.config();
bcfg.panel_width  = 800;
bcfg.panel_height = 480;
// bcfg.pin_d0 = <GPIO>; ... fill all 24 data pins
bcfg.pin_henable = DISPLAY_DE_PIN;
bcfg.pin_vsync   = DISPLAY_VSYNC_PIN;
bcfg.pin_hsync   = DISPLAY_HSYNC_PIN;
bcfg.pin_pclk    = DISPLAY_PCLK_PIN;

bcfg.freq_write = 33000000;   // Adjust after validation
bcfg.hsync_polarity = 0;      // TODO
bcfg.vsync_polarity = 0;      // TODO
bcfg.de_polarity    = 1;      // TODO
bcfg.pclk_active_neg = false; // TODO (depends on panel)

bus.config(bcfg);
panel.setBus(&bus);

auto pcfg = panel.config();
pcfg.memory_width  = 800;
pcfg.memory_height = 480;
pcfg.panel_width   = 800;
pcfg.panel_height  = 480;
pcfg.offset_x = pcfg.offset_y = 0;
// porch / sync values:
pcfg.hsync_front_porch = 40;  // TODO
pcfg.hsync_pulse_width = 48; // TODO
pcfg.hsync_back_porch  = 88; // TODO
pcfg.vsync_front_porch = 13; // TODO
pcfg.vsync_pulse_width = 3;  // TODO
pcfg.vsync_back_porch  = 32; // TODO
// polarity fields etc.
panel.config(pcfg);
```

## 8. Touch Handling (GT911)
- Interrupt-driven or poll mode: INT line low when touch.
- Typical read sequence: Read status, number of points, coordinate blocks, clear status.
- Calibration / configuration table (optional): Provide if customization of resolution or gestures needed.

## 9. CH422G Control Strategy (Planned)
| Feature | Action | Firmware Abstraction |
|---------|--------|----------------------|
| Backlight | Write PWM / ON bit | `ch422g.setBacklight(level)` |
| Panel Reset | Drive reset bit low/high with delays | `ch422g.panelReset()` (to add) |
| Misc GPIO | Map to structured enum | Future expansion |

Add register map once extracted: addresses, bit meaning, direction control.

## 10. CAN (TJA1051) (If Used)
- Connect to ESP32-S3 TWAI (CAN) peripheral pins: TX=GPIOx, RX=GPIOy (confirm).
- Standby pin handling (if present): hold low for normal mode.
- Termination: 120 Ω present? (Check schematic) – ensure only one onboard termination if multiple nodes.

## 11. ESD / Protection & Reliability Notes
- ESD diodes on USB, touch lines? Confirm.
- Backlight supply filtering: note inrush if high PWM duty instantly applied.
- Recommend staged backlight ramp: 0%→100% over ~50–100 ms.

## 12. Software Layer Mapping
| Layer | Responsibility | Files |
|-------|----------------|-------|
| HAL – I2C Expander | Backlight, resets | `hal/ch422g.*` (to expand) |
| Display Driver | RGB bus init, LVGL binding | `display/display_driver.*` |
| Touch | Integrated via LovyanGFX or custom GT911 driver | TODO (currently placeholder) |
| UI Framework | LVGL widgets/screens | future `display/ui_screens.*` |

## 13. Actionable TODO Extraction List
| ID | Item | Status |
|----|------|--------|
| T1 | Confirm full RGB data pin mapping | Pending |
| T2 | Confirm HSYNC/VSYNC/DE/PCLK polarities | Pending |
| T3 | Confirm porch & pulse timing values | Pending |
| T4 | Identify CH422G register map (backlight, reset) | Pending |
| T5 | Add GT911 reset / init sequence | Pending |
| T6 | Implement LovyanGFX Bus_RGB config file | Pending |
| T7 | Implement backlight PWM ramp | Pending |
| T8 | Add panel reset handling sequence | Pending |
| T9 | Add CAN transceiver pin mapping (if used) | Pending |
| T10 | Validate memory bandwidth @ chosen PCLK | Pending |

## 14. Validation & Test Plan (Once Pins Known)
1. Bring-up sequence logs (Serial) – ensure each step passes.
2. Probe CH422G & GT911 – print detection status.
3. Configure Bus_RGB; clear screen (solid color test pattern sweep: Red → Green → Blue → White → Black).
4. Enable LVGL; render label + basic widget.
5. Touch test: print coordinates when pressed.
6. Backlight brightness sweep 0–100% via expander.
7. Stability soak: run for >1 hr, watch for artifacts or resets.

## 15. Notes / Open Questions
- Does panel require gamma or inversion commands via any serial side-channel? (If pure RGB dumb panel, likely no.)
- Any need for external oscillator for GT911? (Internal oscillator typical.)
- Are any RGB lines multiplexed with USB or JTAG at boot causing boot strap conflicts? Confirm strapping pins.

---
Once you provide the concrete schematic-derived values, I will replace all TODO items and implement the actual driver code.
