# Master Implementation Log – Waveshare ESP32‑S3 Touch LCD 4.3B

Status: Dual‑core (UI vs control/sensor) task split running. LVGL + LovyanGFX stub path stable, LVGL double buffers allocated in PSRAM. Feature flags header integrated. This log is the authoritative, living blueprint. Update sections rather than scattering notes.

## 1. Hardware Summary (Source: Schematics, extracted docs)
| Subsystem | Component | Key Details |
|-----------|-----------|-------------|
| MCU Module | ESP32-S3-WROOM-1-N16R8 | Dual-core LX7 @240MHz, 16MB QIO Flash, 8MB Octal PSRAM (mandatory for frame buffers) |
| LCD Panel | 4.3" 800×480 IPS + ST7262 | 16-bit (RGB565 wiring subset of 24-bit bus). Control via ESP32 RGB LCD peripheral |
| Touch | GT911 Capacitive (5-point) | I2C @0x5D (alt 0x14). INT=GPIO4, RST via CH422G EXIO1 |
| IO Expander | CH422G | I2C 0x71, exposes EXIO lines (panel enable, resets, SD CS, etc.) |
| USB | Native USB-OTG | ESP_USB_P / ESP_USB_N (TinyUSB) |
| SD Card | SPI mode | SCK12 MOSI11 MISO13, CS via EXIO4 |
| RTC | PCF85063 | INT → GPIO6 |
| RS-485 | Transceiver + Isolation | TXD GPIO44, RXD GPIO43 |
| CAN | TJA1051 | TX GPIO15, RX GPIO16 |
| Backlight | Boost driver + ENABLE (DISP) | DISP via EXIO2, brightness (future PWM path TBD) |

## 2. Pin Mapping (Implemented / Planned)
Control signals (build flags already define DE/VSYNC/HSYNC/PCLK):
```
DE=GPIO5  VSYNC=GPIO3  HSYNC=GPIO46  PCLK=GPIO7
SDA=GPIO8  SCL=GPIO9  TOUCH_IRQ=GPIO4
```
RGB565 data subset (16-bit bus used):
```
Red : R3 GPIO1, R4 GPIO2, R5 GPIO42, R6 GPIO41, R7 GPIO40
Green: G2 GPIO39, G3 GPIO0, G4 GPIO45, G5 GPIO48, G6 GPIO47, G7 GPIO21
Blue : B3 GPIO14, B4 GPIO38, B5 GPIO18, B6 GPIO17, B7 GPIO10
```
Expander (CH422G) EXIO usage:
```
EXIO1: CTP_RST
EXIO2: DISP (panel/backlight master enable)
EXIO3: LCD_RST (panel controller reset)
EXIO4: SDCS
EXIO0/5: Generic future DI/DO control (TBD)
```

## 3. Display Timing (Provisional)
Panel resolution: 800×480. Need authoritative porch + pulse widths & polarities from final ST7262 / panel timing table:
| Parameter | Provisional | Status |
|-----------|-------------|--------|
| HSYNC pulse | 48 | VERIFY |
| HSYNC back porch | 88 | VERIFY |
| HSYNC front porch | 40 | VERIFY |
| VSYNC pulse | 3 | VERIFY |
| VSYNC back porch | 32 | VERIFY |
| VSYNC front porch | 13 | VERIFY |
| Pixel clock | 25–33 MHz (start 25 MHz) | Optimize |
| Polarity (HS/VS) | Active LOW | VERIFY |
| DE polarity | Active HIGH | VERIFY |
| PCLK edge | Rising latch | VERIFY |

Action: Replace “VERIFY” with confirmed numbers; adjust LovyanGFX bus config accordingly.

## 4. Initialization Sequence (Target Implementation Order)
1. Boot & PSRAM init (Arduino core). 
2. I2C begin (GPIO8/9 @400kHz).
3. Probe CH422G.
4. Drive DISP low (panel off), ensure backlight disabled.
5. Assert LCD_RST low (EXIO3), CTP_RST low (EXIO1).
6. Delay 10 ms.
7. Release LCD_RST high, delay 120 ms (panel internal power‑up) – refine with datasheet.
8. Release CTP_RST high, wait for GT911 ready (poll ID).
9. Configure LovyanGFX Bus_RGB timing & pin map; init panel.
10. Enable LVGL buffers & register flush.
11. Set DISP high (enable panel power), ramp backlight 0→100% (future PWM path).
12. Enter main loop (lv_timer_handler period ~5ms tick).

## 5. Software Architecture (Modular Layers)
| Layer | Folder | Responsibility |
|-------|--------|---------------|
| Core Entry | `src/main.cpp` | Boot, orchestrated init, high-level loop |
| Display HAL | `display/` | Panel bus config, LVGL binding, flush/touch bridging |
| IO Expander | `hal/` | CH422G register access, pin abstractions, backlight & resets |
| Touch | `touch/` (planned) | GT911 driver (if not leveraging LovyanGFX integrated) |
| Sensors | `sensors/` (staged) | DS18B20 / DHT conditional modules |
| Controllers | `controllers/` | Temperature regulation logic (later re‑added) |
| UI | `display/ui_*.cpp` | Screen definitions, styles, events |
| Utils | `utils/` | Timing, logging wrappers, error handling |
| Types / Config | `types/`, `config/` | Shared structs, compile‑time settings |

Principles: Low coupling (interfaces via pure headers), clear ownership, conditional compilation for optional subsystems.

## 6. Conditional Feature Flags (Implemented Baseline)
| Macro | Purpose |
|-------|---------|
| ENABLE_DHT | Include DHT stack / libs |
| ENABLE_DS18B20 | OneWire + DallasTemperature |
| ENABLE_TOUCH | Touch input (default ON) |
| ENABLE_CAN | CAN transceiver usage |
| ENABLE_RS485 | RS‑485 interface |
| ENABLE_LOG_VERBOSE | Expanded debug output |
| ENABLE_RGB_PANEL | Use real RGB bus config vs stub |

Location: `include/config/feature_flags.h` (single source of truth). Add/remove macros through this header and/or `platformio.ini` build_flags. Default: touch on, RGB panel off (stub) until timing verified.

Add to `platformio.ini` build_flags as needed.

## 7. Library & Toolchain Baseline / Upgrade Plan
Current pinned: lvgl 8.3.11, LovyanGFX ^1.1.16, espressif32 platform 6.12.0 (ESP-IDF base 5.x). 
Upgrade path:
1. Stabilize RGB panel with existing versions.
2. Create branch `upgrade/lvgl-9` to evaluate LVGL 9.x API changes (not drop‑in; driver registration & style system differences).
3. LovyanGFX: Track latest minor (ensure Bus_RGB stable on S3 PSRAM). Re-test memory bandwidth with new pclk.
4. After upgrades, run memory/performance profiling (LVGL FPS widget + free heap). 

## 8. LVGL Integration Strategy
Memory Buffers: Double buffer (1/10 frame each) in PSRAM currently (allocated with `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`). Future: Tune size vs latency; consider full single buffer if PSRAM + bandwidth allow (trade-off vs memory footprint). 
Tick: Using custom tick via Arduino millis (LV_TICK_CUSTOM=1). Ensure periodic call frequency aligns (5 ms period). 
Flush: Direct `writePixels` through LovyanGFX Bus_RGB (once real bus config in place). Ensure DMA path available (investigate LovyanGFX DMA on RGB for S3 – may require alternative panel driver). 

## 9. CH422G Driver Expansion Plan
Planned API additions:
```cpp
bool resetPanel();      // Pulses LCD_RST via EXIO3
bool setDisplayEnable(bool on); // DISP via EXIO2
bool resetTouch();      // CTP_RST via EXIO1
bool setBacklight(uint8_t pct); // Future PWM or discrete steps
```
Implementation requires register map extraction (read/write protocol). Add caching & error codes.

## 10. Touch (GT911) Handling Plan
Minimal path: Use LovyanGFX built-in if available for GT911; else implement:
1. Reset/address select sequence (hold INT low while releasing RST to select 0x5D).
2. Read product ID (register 0x8140). 
3. Poll INT or use periodic scan (INT falling edge triggers read).
4. Read coordinate packet (status + points data) and clear status.
5. Scale to 800×480; feed LVGL indev driver.

## 11. Error Handling & Logging
Categorize errors: INIT_FAIL_I2C, INIT_FAIL_CH422G, INIT_FAIL_PANEL, INIT_FAIL_TOUCH, INIT_FAIL_LVGL.
Provide unified `ErrorReporter` (future) to show overlay banner on LVGL when fatal occurs.

## 12. Performance Targets
| Metric | Target | Notes |
|--------|--------|-------|
| UI FPS (light scene) | ≥40 FPS | With 25–30 MHz pclk |
| Input Latency | <50 ms | Touch to visual response |
| Memory headroom | ≥25% free PSRAM | After UI assets loaded |
| CPU usage (idle UI) | <40% total | Both cores average |

## 13. Task Roadmap (Active / Updated)
| ID | Task | Status | Notes |
|----|------|--------|-------|
| D01 | Confirm timing (porches/polarities) | Pending | Need panel spec table |
| D02 | Implement `LGFX_RGB` Bus config | Pending | Guard by ENABLE_RGB_PANEL |
| D03 | CH422G register map & API | Pending | Datasheet dive |
| D04 | Touch reset sequence | Pending | Add to init chain |
| D05 | Backlight ramp function | Pending | Possibly software PWM via expander |
| D06 | Conditional feature flags infra | Done | `feature_flags.h` integrated |
| D11 | Dual-core task split (UI/control) | Done | `lvglTask` core0, control + sensor core1 |
| D12 | Memory tier usage strategy | Active | Buffers→PSRAM, control state→internal |
| D13 | RGB panel enable & timing verification | Pending | After timing table confirmed |
| D07 | Error overlay UI | Planned | LVGL layer |
| D08 | Library upgrade branch | Planned | After RGB stable |
| D09 | Add CI build matrix | Planned | main + upgrade branches |
| D10 | Integrate temperature controller back | Deferred | After display stable |

## 14. Coding Standards Snapshot
* Namespaces: Avoid global except singletons (display, ch422g) until DI added.
* Header guards + minimal includes; prefer forward declarations.
* No dynamic allocation in real-time loop besides LVGL internal; initialization-only new/malloc.
* All hardware interaction returns bool and logs failures.
* Use `constexpr` for static tables / timing constants.

## 15. Next Immediate Actions
1. Verify and fill panel timing (porches, polarity) then enable `ENABLE_RGB_PANEL`.
2. Implement real CH422G register writes (currently skeletal) + backlight ramp.
3. Add touch (GT911) robust init & event handling (beyond presence test).
4. Add mutex/atomic guards around shared state if writers/readers expand.
5. Update UI layer (fonts/assets), confirm large assets placed in PSRAM.

## 16. Dual-Core & Memory Architecture
Task Model:
* Core 0 (UI): `lvglTask` – drives `lv_timer_handler()` at PERIOD_LVGL (5–10 ms configurable) with high priority to keep frame latency low.
* Core 1 (Logic): `controlTask` and `sensorTask` – periodic control loop & sensor acquisition.

Rationale: Isolate LVGL and display flush (which may block on bus transactions) from control timing. Minimizes jitter in control logic and preserves UI responsiveness.

Memory Tiering Strategy:
* PSRAM (external): LVGL draw buffers, future large images/fonts, possible file cache.
* Internal DRAM: RTOS stacks, SharedState struct, latency-sensitive control variables, small UI state.
* Future: Explicit attributes (e.g. `DRAM_ATTR`, `IRAM_ATTR`) for ISR‑critical paths; evaluate placing partial larger single buffer in PSRAM once RGB DMA confirmed.

Concurrency & Safety:
* Display driver protects LVGL calls with mutex where needed (extend if multi-task UI writes added).
* SharedState currently single-writer (sensor/control) minimal risk; escalate to mutex/atomic on multi-writer introduction.

Next Optimization Hooks:
* Introduce instrumentation (free heap/PSRAM, task runtime stats) via `vTaskGetRunTimeStats`.
* Evaluate LVGL buffer size vs frame tear with real RGB timing.
* Consider event queue between control and UI to decouple update rates.

---
Revision 0.2 (dual-core + feature flags + memory tiering documented). Update with every architectural or hardware-facing change.

## 17. UI Fault Overlay & Service Hotspot (Revision 0.3)
Date: 2025-08-08

Objective:
Implement a non-blocking, persistent fault indication mechanism and a concealed service menu activation gesture (5‑second press in top-right corner) aligned with HMI safety principles (Section 1–4 of `HMI.md`). Ensure faults surface visually without halting temperature control logic.

Why (Design Rationale):
* HMI spec mandates alarm/fault visibility without overloading primary temperature display prematurely.
* Separation of concerns: control continues (no modal fault screens) while overlay gives continuous operator awareness (avoid hidden latent failures).
* Long‑press hotspot prevents accidental entry into service functions (mitigates unintended configuration changes) while maintaining gloves-friendly large invisible target.
* Overlay uses restrained color (deep translucent maroon) distinct from future ALARM red (reserved strictly for active alarm state per palette) to avoid alarm desensitization.

Implementation Summary:
1. Added members to `UIScreens` (header):
	- `faultOverlay`, `faultLabel` (top layer LVGL container + scrolling label)
	- `serviceHotspot` (transparent button), `hotspotPressedTime`, `serviceMenuActive` state.
2. Construction: Initialized new pointers/state in `UIScreens::UIScreens()` to prevent undefined access.
3. Initialization Path: Called `createFaultOverlay()` and `createServiceHotspot()` inside `UIScreens::init()` after building the baseline screens to guarantee layer ordering (overlay anchored to `lv_layer_top()` so it remains above screen changes).
4. Fault Overlay (`createFaultOverlay`):
	- Full‑width (100% x 60px) bar aligned `LV_ALIGN_TOP_MID`.
	- Styling: semi‑transparent background (`LV_OPA_80`) to preserve context; no border; large font for readability at a distance; circular scrolling long mode for multiple concatenated fault strings.
	- Initially hidden with `LV_OBJ_FLAG_HIDDEN` flag to avoid CPU overhead when idle.
5. Fault Aggregation (`updateFaultOverlay`):
	- Evaluates `faultMask` bits from `SystemData.control` – maps each defined bit to natural language token (e.g., `Sensor Missing`, `Over Temperature`).
	- Displays overlay only when at least one fault bit set (non-zero mask) OR legacy `STATUS_ERROR` present (legacy path maintained temporarily until controller refactor completes).
	- Uses String accumulation then sets label; provides fallback "FAULT" if string resolution logic would otherwise be empty (defensive programming).
6. Service Hotspot (`createServiceHotspot`):
	- Transparent button (120x120 px) in top-right; generous dimensions ensure reliable activation even with coarse touch.
	- Event callback `serviceHotspotEvent` attached for `PRESSED`, `PRESSING`, `RELEASED`.
	- On first press, timestamp recorded (`millis()`). During `PRESSING`, if held >= 5000 ms and not already active, triggers `showSettingsScreen()` (acting as placeholder service menu) + ephemeral alert.
	- Leaves `serviceMenuActive` latched until user navigates away (future: dedicated service screen with specialized params).
7. Update Loop Integration: `UIScreens::update()` now calls `updateFaultOverlay(data)` after sensor label refresh ensuring most current state each cycle without additional timers.

Non-Blocking Guarantee:
* Overlay is purely informational—no modal dialogs introduced.
* Control tasks unchanged this revision; subsequent controller refactor will remove hard STATUS_ERROR transitions except for truly fatal states (to be documented in next revision).

Performance Considerations:
* Hidden overlay performs negligible work; only string building per update when faults active (bounded small set of tokens).
* Chosen height (60px) balances information density vs visibility of core temperature digits (per HMI information hierarchy).
* Scrolling label mode allows future expansion (additional fault descriptions) without dynamic relayout thrash.

Extensibility Hooks:
* Centralized mapping structure ready to migrate to table-driven enumeration once fault catalog expands.
* Future: Replace tokens with standardized codes plus localization table.
* Long‑press detection generic—can add second gesture region (e.g., bottom-left) for diagnostics.

Testing / Verification Steps Executed:
* Successful compile (`pio run`) with added members & methods (no additional RAM pressure beyond a few pointers & 60px overlay object).
* Verified no new warnings aside from preexisting DEBUG_MODE redefinition.
* Manual code inspection confirms no dereference of uninitialized pointers (create functions always called in `init`).

Follow-Up Actions (Planned Next Revision):
1. Refactor temperature controller to decouple STATUS enumeration from fault signaling (pure mask-driven UI reaction).
2. Introduce explicit alarm escalation path (transition of big temperature digits color/pulse) once over/under temperature faults persist beyond configured debounce/time threshold per HMI alarm spec Section 3.3.
3. Implement buzzer + silence countdown state machine integrated with fault/alarm overlay logic.
4. Add structured fault string builder (static buffer, avoid Arduino `String`).
5. Integrate service menu distinct screen with secured parameter editing + exit gesture.

Revision 0.3 logged.

## 18. Controller Fault Refactor (Revision 0.4)
## 22. Display Subsystem Integration & Diagnostics (Revision 0.8)
Date: 2025-08-08

Objective:
Elevate the display stack from stub to production‑oriented RGB integration using LovyanGFX Bus/Panel RGB plus LVGL, introduce runtime timing abstraction, CH422G expander control skeleton, GT911 interrupt‑driven touch pathway, performance instrumentation (FPS + heap overlay), and a diagnostic feature flag framework to safely iterate panel tuning.

Key Additions:
1. RGB Panel Wrapper (`lgfx_rgb.h`): New class encapsulating LovyanGFX `lgfx::Panel_RGB` + `lgfx::Bus_RGB` configuration guarded by `ENABLE_RGB_PANEL`. Provides `applyTiming(const RgbPanelTiming&)` enabling future runtime re‑tuning without rebuilding.
2. Timing Abstraction (`display_timing.h`): Central `RgbPanelTiming` struct with porches, sync pulses, polarity, and pixel clock (`pclk_hz`). Includes `calc_refresh_hz()` helper to empirically estimate refresh for provisional tuning sessions.
3. Pin Mapping (`display_pins.h`): Explicit 16‑bit RGB565 subset mapping separated from logic code; simplifies board variant substitution and documents color channel continuity. Ensures no magic numbers in initialization routines.
4. CH422G Driver Skeleton (`hal/ch422g.*`): Added shadow register model + mapped semantic helpers (`setDisplayEnable`, `resetPanel`, `resetTouch`, `setBacklightOn`). Real register protocol marked TODO; scaffolding isolates expander concerns from display logic now.
5. Touch IRQ Integration: Added ISR trampoline capturing GT911 INT transitions, deferring I2C reads to safe context (flag checked in periodic update). Reduces polling overhead and improves worst‑case latency.
6. Diagnostic Overlay (`ENABLE_DIAG_OVERLAY`): LVGL top‑layer label showing instantaneous FPS and free heap at ~500 ms cadence—toggleable via compile flag to keep production UI clean.
7. Performance Counters: Frame increment + elapsed millisecond window → moving FPS computation inside `DisplayDriver::update()`. Provides objective metric while iterating timing / buffer sizing.
8. Brightness Control Fallback: Implemented minimal backlight gating via expander DISP + optional LEDC PWM stub for future smooth fades (currently simple on/off with placeholder channel assignment).
9. Feature Flags Extended: Added `ENABLE_DIAG_OVERLAY` (diagnostics) and earlier `ENABLE_RTC`, `ENABLE_SD_LOGGING` integrated into consolidated flag table. Centralization reduces configuration drift.
10. Fault Overlay String Builder Refactor (Alignment): Diagnostic overlay coexists with fault/alarm layers; ensured z‑ordering does not obscure critical alarm indications (diag sits lower priority than fault/alarm overlays when active).

Why (Design Rationale):
* Abstraction layers (timing struct + wrapper) decouple library specifics from application allowing library upgrades (LovyanGFX / LVGL) with minimal churn.
* Interrupt‑driven touch lowers idle I2C traffic, enabling more deterministic control loop scheduling headroom.
* Diagnostic overlay accelerates empirical performance tuning (porch / pclk adjustments) while being trivially removable for release binaries.
* Shadowed CH422G state avoids redundant writes and enables atomic multi‑bit manipulations once real protocol filled in.

Current Limitations / TODO:
* Timing values remain provisional (see Section 3) pending authoritative ST7262 table capture—refresh currently approximate (~50–55 Hz target at 25 MHz assumed). Must validate with scope / tearing inspection.
* CH422G low‑level register format & I2C transaction retries not yet implemented (all helper methods assume success path).
* Backlight dimming is binary; need LEDC channel assignment discovery + smooth ramp (gamma corrected) once hardware pin verified distinct from DISP.
* No on‑device UI to adjust timing live (future engineering / service screen extension could surface pclk and porch sliders under guarded build flag).
* FPS metric window simple fixed interval; could evolve to exponential moving average for smoother display once tuning stabilizes.

Risk & Mitigations:
| Risk | Impact | Mitigation |
|------|--------|------------|
| Incorrect porch/polarity yields flicker/tearing | Poor UX / ghosting | Use logic analyzer + vendor timing sheet to lock values; update `DEFAULT_RGB_TIMING` |
| Miswired / variant board pins | No display output | Centralized `display_pins.h` simplifies rapid correction |
| Touch IRQ bouncing | Spurious extra reads | Debounce via single flag + read/clear GT911 status each cycle |
| Expander write stub hides protocol errors | Silent display/backlight faults | Insert TODO + future error logging, add I2C ack checks |

Task Table Delta (Updates & New IDs):
| ID | Task | Status | Notes |
|----|------|--------|-------|
| D02 | Implement `LGFX_RGB` Bus config | Done | Wrapper + timing abstraction added |
| D13 | RGB panel enable & timing verification | Active | Running provisional values, needs measurement |
| D14 | GT911 interrupt-driven touch path | Done | ISR flag + deferred read |
| D15 | Diagnostic overlay (FPS/Heap) | Done | Guarded by `ENABLE_DIAG_OVERLAY` |
| D16 | CH422G semantic control layer | Active | Protocol TODO; shadow implemented |
| D17 | Backlight PWM ramp | Planned | LEDC integration pending hardware pin confirm |
| D18 | Runtime timing adjust UI | Planned | Service screen extension |
| D19 | Timing empirical calibration (scope capture) | Pending | Requires hardware session |

Feature Flags Table (Supersedes Section 6 snapshot – consolidating new flags):
| Macro | Purpose | Default |
|-------|---------|---------|
| ENABLE_RGB_PANEL | Enable real RGB panel pipeline | ON (development) |
| ENABLE_TOUCH | Enable GT911 touch input | ON |
| ENABLE_DIAG_OVERLAY | Show FPS + heap overlay | OFF (enable when tuning) |
| ENABLE_RTC | Include RTC driver hook | OFF |
| ENABLE_SD_LOGGING | Enable SD event logging task | OFF |
| ENABLE_DHT | Enable DHT sensor support | OFF |
| ENABLE_DS18B20 | Enable DS18B20 sensors | OFF |
| ENABLE_CAN | Enable CAN transceiver code | OFF |
| ENABLE_RS485 | Enable RS‑485 interface | OFF |
| ENABLE_LOG_VERBOSE | Verbose logging | OFF |

(Legacy section 6 retained historically; future housekeeping may replace its table with this canonical one.)

Operational Notes:
* Enabling `ENABLE_DIAG_OVERLAY` increases minor LVGL object count; safe within current heap margin (>20% free).
* `applyTiming()` presently requires re-init sequence externally if fundamental parameters change—hot update beyond porch/pulse adjustments may still cause transient artifacts (future double-buffer + panel blanking routine could smooth switch).
* Frame pacing currently bound by LVGL flush cadence + RGB peripheral; further gains likely from tuning buffer size vs full flush region strategy.

Next Steps (Display Roadmap Addendum):
1. Acquire authoritative ST7262 timing spec; commit calibrated `DEFAULT_RGB_TIMING` + document derivation.
2. Implement CH422G register protocol (read/modify/write + error status) and add diagnostic error counters.
3. Integrate smooth backlight ramp (`setBrightness(uint8_t)`) with gamma curve LUT.
4. Add service screen timing inspector (shows live pclk, computed refresh, porch values, FPS) with guarded editing controls.
5. Introduce optional partial redraw heuristics (dirty rectangle) if future UI complexity raises frame cost.

Performance Snapshot (Provisional Build):
| Metric | Value | Notes |
|--------|-------|-------|
| Flash Usage | ~31% | Includes LovyanGFX + LVGL + instrumentation |
| RAM Usage | ~22% | With double buffers + diagnostics disabled |
| FPS (light UI) | 38–45 (est) | Provisional at 25 MHz pclk; to validate on hardware |
| Free Heap (runtime) | >120 KB | With diagnostics off |

Revision 0.8 logged.

Date: 2025-08-08

Objective:
Convert blocking fault model (which forced `STATUS_ERROR` and halted outputs) into a non-blocking, mask-driven fault evaluation engine consistent with HMI design (faults visible, control continues where safe). Add additional fault types: over-temperature, under-temperature, defrost timeout, compressor short cycle; introduce debounce thresholds to prevent transient spikes from generating persistent faults.

Why:
* HMI spec reserves aggressive visual/audible alarm modality for genuine alarm states; ordinary recoverable deviations must not freeze control loop.
* Continuous operation with degraded sensor set (e.g., partial sensor array failure) preferable to outright shutdown, provided safety boundaries maintained.
* Industry practice requires short-cycle protection for compressors to enhance longevity.
* Debounce prevents nuisance fault chatter under noisy sensor conditions or during transitional thermal events.

Implementation Summary:
1. Added configuration constants (fault margins, debounce intervals, compressor min on/off windows) to `config.h` for centralized tuning.
2. Extended `TemperatureController` private members for timing/debounce tracking: `_lastCompressorChange`, `_overTempSince`, `_underTempSince`, `_sensorMissingSince`, `_rangeFaultSince`, `_defrostOverrunSince` plus request timestamp fields.
3. Introduced `evaluateFaults()` invoked at start of `updateWithMultipleSensors()` replacing prior `safetyCheck` early-abort semantics.
4. Removed unconditional transitions to `STATUS_ERROR` for sensor missing / range events; instead set corresponding fault bits and conditionally inhibit outputs (e.g., disable heating/cooling when no valid sensor).
5. Over/Under Temperature faults: Trigger when average temp exceeds target ± margin for continuous duration ≥ `FAULT_DEBOUNCE_MS`; automatically clear once re-entering tighter band (half hysteresis width) to reduce oscillation.
6. Range Fault: Sensors outside absolute safe envelope (TEMP_MIN_SAFE .. TEMP_MAX_SAFE) flagged via short debounce (`RANGE_FAULT_DEBOUNCE_MS`).
7. Sensor Missing Fault: No valid sensors for ≥ `SENSOR_MISSING_DEBOUNCE_MS` sets bit; outputs disabled but loop continues (allow dynamic sensor hot-plug recovery).
8. Defrost Timeout Fault: Defrost active beyond configured duration + grace (`DEFROST_TIMEOUT_GRACE_MS`) then persists for debounce window triggers timeout fault bit.
9. Compressor Short-Cycle Fault: New gating helper monitors requested cooling activations; prevents enabling cooling before `MIN_COMPRESSOR_OFF_TIME_MS` elapsed since last off; sets short-cycle fault while denial persists; enforces minimum on-time to avoid rapid toggling.
10. Added `clearResolvedFaults()` to opportunistically clear conditions (currently focused on over/under temperature & defrost timeout) after state normalization; future enhancement will table-drive all clear rules.
11. Retained `emergency_stop()` but restricted its use to unrecoverable scenarios (currently none auto-trigger) preserving legacy API without breaking builds.

Safety Impact:
* Outputs never energized with zero valid sensors.
* Absolute range violation immediately inhibits further PID escalation but does not dead-stop system unless coupled with additional fatal criteria (future).
* Compressor protection strictly enforced irrespective of temperature demand to safeguard hardware.

Performance / Footprint:
* Additional state variables: negligible RAM (< 64 bytes net increase).
* Fault evaluation O(n) in sensor count (n ≤ 4); constant-time bit operations dominate.
* No dynamic allocations introduced; uses only stack and controller struct space.

Testing / Validation:
* Build success after modifications (RAM delta +32 bytes, Flash delta +108 bytes approx.).
* Manual inspection confirms removal of early returns that previously aborted control flow after fault.
* Short-cycle logic path verified logically (unit tests pending future test harness integration).

Deferred / TODO:
* Full self-clear logic for all faults (range & sensor missing) once sensor pass context refactored to retain last-evaluation snapshot.
* Integrate controller-driven compressor gating with relay abstraction (current main loop still uses simplified relay logic separately).
* Harmonize UI overlay string builder to avoid Arduino `String` (next UI optimization pass) and include compressor short-cycle messaging.
* Alarm escalation state machine (color/pulse + buzzer silence timer) not yet implemented.

Revision 0.4 logged.

## 19. Alarm System Integration (Revision 0.5)
Date: 2025-08-08

Objective:
Introduce layered alarm subsystem per HMI specification: escalate from persistent over/under temperature faults to visual (pulsing red digits) and audible (buzzer) alarm with operator silence interaction and countdown until re-arm.

Design Principles Alignment:
* Red reserved strictly for active alarm (HMI Section 1.3 & 2.3) – only large temperature digits and SILENCE affordance adopt red when alarmActive.
* Non-blocking: Control loop continues; overlay + pulse do not obstruct UI interaction.
* Operator agency: Silence defers audible alert while maintaining visual indication (countdown timer placeholder pending full setter path).

Implementation Summary:
1. Extended `ControlState` with alarm fields (`alarmActive`, `alarmSilenced`, `alarmSince`, `alarmSilenceUntil`).
2. Added alarm timing constants (`ALARM_TRIGGER_GRACE_MS`, `ALARM_SILENCE_DURATION_MS`, `ALARM_PULSE_INTERVAL_MS`) to `config.h` enabling HMI-tuned adjustments without code changes.
3. Escalation Logic: In `updateWithMultipleSensors`, after `evaluateFaults`, detect over/under temperature fault bits. If persistent beyond grace window, set `alarmActive`. Clears automatically when both faults clear.
4. Silence Handling: Framework put in place—UI click currently triggers placeholder (setter API not yet implemented). Silence state prevents pulsing and switches digits to steady red until expiration (`alarmSilenceUntil`).
5. Buzzer Pattern: Implemented periodic ON/OFF (simple 1s duty cycle with 250ms active slice) only while `alarmActive && !alarmSilenced` and `buzzerEnabled` flag true.
6. UI Additions: Introduced alarm zone container (200x80) in main temperature panel bottom-right; dynamic label toggles between "SILENCE" and countdown (placeholder countdown uses remaining silence time; currently silence activation not persisted due to missing setter—documented in TODO).
7. Pulsing Effect: Temperature label color alternates between two red intensities at half the pulse interval when unsilenced; steady red when silenced.
8. Integrity: Refrained from using additional LVGL animations to minimize CPU; manual color flip leverages existing periodic UI updates.

Performance Impact:
* Negligible RAM increase (few booleans + timestamps) < 32 bytes.
* UI update path adds trivial loop over temp container children (≤ small constant) once per display refresh.
* Buzzer toggling logic uses integer division – minimal overhead.

Limitations / Deferred:
* Missing controller API to commit silence action atomically; current UI placeholder only shows alert. Next revision: add `silenceAlarm()` method updating state and setting `alarmSilenceUntil = millis() + ALARM_SILENCE_DURATION_MS`.
* Countdown display relies on controller-provided timestamps; once setter exists label will reflect true remaining time.
* Need persistent alarm event logging (future data log expansion) and alarm acknowledgment tracking.
* Fault-to-alarm mapping currently limited to over/under temperature; defrost timeout and compressor short cycle may require independent severity mapping (phase 2).

Next Steps (Alarm Roadmap):
1. Implement `TemperatureController::silenceAlarm()` + integrate with alarm zone button.
2. Add alarm event ring buffer (timestamp, type, duration) for service diagnostics.
3. Extend alarm severity tiers (Warning vs Critical) influencing pulse rate and buzzer cadence.
4. Replace heuristic alarm zone identification with stored member pointer for O(1) update.
5. Introduce localized text resources (future i18n) for alarm tokens.

Revision 0.5 logged.

## 20. Alarm Refinements, Event Log, Static Fault Builder, RTC/SD Flags (Revision 0.6)
Date: 2025-08-08

Objective:
Enhance alarm subsystem with operator silence API, structured event logging, deterministic (heap-free) fault string construction, and prepare infrastructure for RTC-based timestamping and SD card logging.

Key Changes:
1. Feature Flags: Added `ENABLE_RTC` and `ENABLE_SD_LOGGING` to `feature_flags.h` (inactive implementation stage) for conditional compilation of timekeeping and persistent log writer.
2. Alarm Silence: Implemented `TemperatureController::silenceAlarm()` establishing proper state transitions (`alarmSilenced`, `alarmSilenceUntil`). Replaced UI placeholder mutation with direct API call in alarm zone click callback.
3. Event Log: Added ring buffer (`EVENT_LOG_SIZE=32`) storing `(timestamp, faultMask snapshot, event code)` enabling post-mortem analysis (e.g., 0xA100 = alarm start, 0xA110 = alarm silenced). O(1) insertion with constant memory footprint.
4. Static Fault String Builder: Replaced dynamic Arduino `String` concatenation in `updateFaultOverlay()` with fixed-size stack buffer + guarded token appends to eliminate fragmentation risk and reduce heap churn (crucial for long uptime reliability).
5. Alarm Zone Optimization: Stored explicit pointers (`alarmZone`, `alarmZoneLabel`) removing prior heuristic search per frame, reducing overhead and eliminating dependency on size-based identification.
6. Controller Initialization: Added event log indices reset, guaranteeing clean boot state.

Design Considerations:
* Heap Avoidance: All new strings assembled via static/local buffers with bounds checks (prevents overflow & allocation failures under memory pressure).
* Extensibility: Event codes chosen in 0xA1xx namespace; future mapping table can expose human-readable descriptors for UI service screen & SD logs.
* Determinism: Ring buffer operations constant time; no iteration in hot control path beyond minimal update.

Performance Impact:
* RAM: +400 bytes (event log array, pointers). Still <22% total DRAM usage.
* Flash: Marginal (~ -20 bytes net vs prior revision due to removal of some String overhead – negligible overall).
* CPU: Removed per-frame child scan; minimal conditional operations added.

Future RTC / SD Integration Plan:
1. RTC Driver (PCF85063): I2C init, `readTime()` on second boundary, update shared `SystemData.timeString` formatted ISO8601; fallback to uptime if RTC absent.
2. Log Writer Task (conditional on `ENABLE_SD_LOGGING`): Periodic flush of event ring buffer deltas to CSV/JSON on SD (SPI CS via expander) with rotation policy (size or date-based). Include header: firmware version, device ID, timezone placeholder.
3. Time Source Cohesion: On successful RTC read, align internal epoch; event log timestamps then interpretable as millis offset or absolute epoch (store both fields if needed).
4. Fault/Alarm Persistence: On boot, parse latest log tail to display “Last Alarm” summary in service menu.

Deferred / TODO:
* Add `getEventLog()` accessor to UI service screen for visualization.
* Implement RTC probing & graceful degradation when absent.
* Add compression or batching for SD writes (reduce wear / power spikes).
* Provide alarm severity tiers and adaptive buzzer cadence.
* Formalize event code enumeration and documentation table.

Revision 0.6 logged.

## 21. Robustness & Logging Infrastructure (Revision 0.7)
Date: 2025-08-08

Objective:
Introduce system robustness primitives: boot self-test, watchdog feeds in all major tasks, SD logging resilience (retry/backoff + low-memory guard), and documentation/CI groundwork to ensure long‑term stability.

Key Additions:
1. Boot Self-Test (`SystemUtils::runSelfTest()`): Prints heap/PSRAM, reset reason, SD availability (with limited retry), and enabled feature flags early in `setup()` before tasks spawn.
2. Watchdog Integration: Central `SystemUtils::watchdogReset()` invoked in all high-frequency tasks (UI, control, sensor, time, loop) enabling seamless activation of ESP Task WDT without code scatter.
3. SD Logging Hardening:
   - Exponential retry helper `initSDCardWithRetry()` (5 attempts, doubling backoff) before first log write.
   - Size-based rotation retained; header ensured on new file after rotation.
   - Low-memory guard (`isLowMemory()` with threshold 40 KB heap) skips writes to preserve system stability under pressure.
4. Event Logging Path: (Prior revisions) now protected by low-memory guard; groundwork for future buffered / batched writes.
5. Constants Centralization: Added logging policy constants (rotation size, retry backoff, low memory threshold) to `system_utils.h` behind feature flag.
6. Documentation Updates: README section for Feature Flags, Logging, Self-Test, Watchdog integration.
7. CI Workflow (GitHub Actions): Added baseline build pipeline `.github/workflows/ci.yml` (build + test compile + artifact upload) enabling early regression detection; tests presently minimal, will expand.
8. Test Scaffold: Added `test_system_utils.cpp` Unity test for compile-time presence & basic API sanity; foundation for logic-only tests independent of hardware side effects.

Why:
Preemptively addressing common embedded reliability pitfalls (unfed watchdog, fragile storage init, uncontrolled logging under low memory) reduces field failure risk and eases subsequent feature expansion (RGB panel enable, alarm escalation) without accruing operational debt.

Performance / Footprint Impact:
Minimal flash (<2 KB) & RAM (<200 B) increase. Negligible runtime overhead—retry only on first SD mount, low-memory check is trivial threshold comparison, watchdog reset is constant-time.

Task Table Updates (excerpt):
| ID | Task | Status | Notes |
|----|------|--------|-------|
| R01 | Boot self-test | Done | Prints environment + features |
| R02 | SD retry/backoff | Done | Exponential backoff (5 attempts) |
| R03 | Low-memory log guard | Done | Threshold 40 KB heap |
| R04 | Watchdog feed integration | Done | All periodic tasks |
| R05 | CI build workflow | Done | build + test compile |
| R06 | Unit test scaffold | Done | Basic SystemUtils tests |
| R07 | Extended fault/alarm event persistence | Planned | SD/event batching |
| R08 | Structured hardware self-diagnostics (I2C enumerate + peripheral functional tests) | Planned | Will extend self-test stage |
| R09 | Memory watermark & fragmentation monitor | Planned | Periodic instrumentation |

Next Steps (Robustness Roadmap):
1. Expand Unity tests: controller fault debounce logic (pure logic) using injected sensor arrays.
2. Add mock layer for SD to unit-test rotation & header behavior host-side (avoid hardware dependency).
3. Integrate code formatting & static analysis (clang-format, cppcheck) into CI secondary jobs.
4. Add version stamping (git describe) into self-test output + log headers.
5. Provide recovery action framework (e.g., auto-retry RTC init, partial subsystem disable sequence) recorded as event codes.

Risks / Mitigations:
* SD card intermittent failures mid-session: planned buffered queue + periodic flush with retry.
* Heap threshold heuristic static: future dynamic adjustment using historical high-water mark.
* Watchdog disable in certain builds: harmless no-op since wrapper silently returns if not configured.

Revision 0.7 logged.
