// Central feature flag header. Guard optional subsystems & experimental code paths.
// Enable flags via platformio.ini build_flags (e.g. -DENABLE_RGB_PANEL)

#pragma once

// Display / Graphics
// When enabled, uses real ESP32-S3 RGB panel (LovyanGFX Bus_RGB + Panel_RGB)
// When disabled, a stub LGFX placeholder compiles for logic/UI development.
#define ENABLE_RGB_PANEL  // Real RGB bus enabled after lgfx_rgb.h refactor

// Touch (GT911) input
#define ENABLE_TOUCH 1

// Future optional peripherals (documented in Master Implementation Log)
// #define ENABLE_DHT
#define ENABLE_DS18B20
// #define ENABLE_CAN
// #define ENABLE_RS485

// Actuators / Interfaces
#define ENABLE_RELAYS

// Timekeeping (battery RTC) & Storage
#define ENABLE_RTC
#define ENABLE_SD_LOGGING

// Over-The-Air updates (WiFi / ArduinoOTA)
#define ENABLE_OTA

// Verbose logging for low-level drivers
// #define ENABLE_LOG_VERBOSE

// Diagnostic performance overlay (FPS, heap, touch events)
#define ENABLE_DIAG_OVERLAY
