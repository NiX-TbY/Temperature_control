# Project Status Snapshot (2025-08-08)

This file captures a restore point snapshot of the Temperature Control project on 2025-08-08.

## ✅ Completed / Implemented Features
- Build System: PlatformIO configuration for ESP32-S3 (Arduino framework) with optimized source filtering and memory flags (PSRAM, flash size, partitions).
- Display Stack: LVGL + LovyanGFX integration scaffolding (RGB panel pin definitions, timing flags in `platformio.ini`).
- Feature Flags Architecture: Centralized in `include/config/config.h` with conditional compilation for SD logging, sensors, relays, etc.
- Temperature Control Logic:
  - PID-like control loop for heating / cooling activation pathways.
  - Fan speed modulation stub & bounds enforcement.
  - Target temperature management.
- Fault & Safety System:
  - Over-temperature, under-temperature, sensor missing, sensor range faults.
  - Debounce windows for each fault class.
  - Alarm escalation path with grace interval infrastructure.
- System Utilities (`SystemUtils`):
  - Serial initialization banner.
  - System info (chip, cores, heap, PSRAM, reset reason).
  - I2C device scanner with known device hints (CH422G / GT911 / RTC).
  - Self-test routine invoked at boot.
  - Low-memory heuristic helper.
  - SD logging interface (conditional) with:
    - CSV log writer with header insertion & rotation based on size.
    - Event log writer (events.csv) with hex fault mask.
    - Retry logic (exponential backoff helper) guarded by attempts.
  - Watchdog reset hook (conditional on WDT config).
  - Formatting helpers (time, date, temperature strings).
- Sensor Integration:
  - DS18B20 (OneWire + DallasTemperature libraries wired via build dependencies).
  - Sensor data structure with validity & timestamp fields.
- Core Types & Configuration: Strongly grouped thresholds (margins, debounce, alarm timers) and system state structures.
- Unit Tests (initial set):
  - Fault condition activation (over, under, missing, range, alarm escalation).
  - PID symmetry and over-temp fault path (native logic test).
  - Basic SystemUtils compile-time constant presence & low memory guard placeholder.
- Build Validation: Firmware builds successfully for `esp32-s3-devkitc-1` with self-test enabled.
- Header Resolution Cleanup: Eliminated duplicate shadow header via forwarding shim (`src/utils/system_utils.h`).
- Memory Footprint: ~30% flash, ~22% RAM usage (at last build), leaving headroom.

## 🧪 Current Test Status
- Native tests failing due to direct inclusion of hardware-dependent `system_utils.cpp` (ESP headers not guarded) and missing Arduino macros (e.g., `OUTPUT`).
- Logic-only tests (temperature controller) otherwise structurally valid.
- No on-device (`pio test` on ESP32 target) test environment configured yet.

## ⛳ High-Priority TODOs (Short-Term)
1. Make Native Tests Pass:
   - Add `#ifdef ESP_PLATFORM` guards around ESP-IDF-only includes (`esp_task_wdt.h`, `esp_system.h`, etc.).
   - Provide lightweight stubs for `ESP.getFreeHeap()`, `ESP.getFreePsram()` in native test mode.
   - Define Arduino pin modes (`OUTPUT`, `INPUT`) in `arduino_stub.h`.
2. Remove Shim Header:
   - Delete `src/utils/system_utils.h` after ensuring all includes use `"utils/system_utils.h"`.
3. Add Dedicated Test Environment:
   - New `[env:esp32-s3-tests]` with `-DUNIT_TEST` and broadened `build_src_filter` for on-device tests.
4. Expand Unit Tests:
   - Boundary tests for debounce thresholds (exact vs. off-by-one ms progression).
   - Fan speed clamping and PID output shaping.
   - Alarm silence / re-trigger timing once implemented fully.
   - Formatting helpers (date/time/temperature) including zero-padding.
5. Introduce Mock Abstractions:
   - Wrap SD & File I/O to allow log rotation tests without hardware.
   - Abstract watchdog & reset calls for deterministic unit tests.

## 🔭 Medium-Term Enhancements
- RTC Integration: Implement real-time clock handling (e.g., PCF85063A) in `rtc_clock.h` with synchronization & date stamping for logs.
- Persistent Configuration: Load/save target temperature & tuning parameters (NVS or JSON on SD).
- Touch / UI Layer: Add UI screens (home, settings, logs) with LVGL & event-driven updates.
- Relay / Fan HAL: Abstract GPIO control & add feedback / diagnostic flags.
- Network / Expansion Buses: Optional enabling of CAN / RS485 if hardware present.
- Power Management: Controlled deep sleep cycles with wake source management.
- Telemetry Export: Optional USB CDC or serial command shell for status/query.
- Structured Event Codes: Enumerate event IDs & map to human-readable messages.
- CI Pipeline: GitHub Actions for build + native tests on PR.
- Static Analysis: Add clang-tidy / cppcheck configuration.
- Documentation: Doxygen generation & high-level architecture diagram.
- Logging Improvements: Time-based rotation, compression hooks, retention policy.

## 🧩 Longer-Term / Stretch Goals
- Web / USB Diagnostic Console with live metrics.
- Over-the-air firmware update integration (if Wi-Fi added later).
- Multi-sensor averaging strategies & sensor health scoring.
- Adaptive control (auto-tune PID / fuzzy logic).
- Fault simulation hooks for automated endurance tests.

## 🛡️ Technical Debt / Cleanup Items
- Consolidate duplicate macro definitions & avoid redefinition warnings (e.g., `ALARM_TRIGGER_GRACE_MS`).
- Remove direct inclusion of `.cpp` files inside tests; replace with proper linkage or a test-specific library component.
- Normalize include path usage (prefer project-relative logical includes—already largely done).
- Consistent naming for config constants (e.g., all-caps with suffix `_MS`, `_MARGIN`).
- Add namespace or module prefix to reduce global symbol risk (future).

## 🧪 Suggested New Test Cases (Concrete)
| Category | Test | Purpose |
|----------|------|---------|
| Fault | overtemp_at_boundary | Confirm exactly at margin triggers after debounce |
| Fault | under_temp_hysteresis | Ensure recovery clears fault after condition resolves |
| Alarm | escalation_exact_grace | Validate alarm triggers at grace expiry, not before |
| Logging | rotate_on_exact_size | File rotates when size == threshold |
| PID | zero_error_no_action | No relay/fan toggles when error == 0 |
| Memory | low_memory_blocks_logging | Simulate low heap blocks CSV writes |
| Format | date_format_zero_pad | Leading zeros retained in output |

## 🧷 Restore Point Metadata
- Branch: main
- Date: 2025-08-08
- Commit Tag (planned): `restore-point-2025-08-08`
- Build: SUCCESS (esp32-s3-devkitc-1)
- Self-Test: Enabled
- Known Issues: Native tests failing (hardware includes), duplicate macro redefinitions

## ✅ Recommended Immediate Next Commit Actions
1. Guard ESP-only includes.
2. Add native stubs for pin modes.
3. Add test environment for on-hardware validation.
4. Remove forwarding shim once stable.

---
Generated snapshot to establish a clean reference for future iterative development.
