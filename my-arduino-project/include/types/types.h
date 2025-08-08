#ifndef TYPES_H
#define TYPES_H

#ifdef UNIT_TEST_NATIVE
#include "test_support/arduino_stub.h"
#else
#include <Arduino.h>
#endif
#include <vector>
#include "config/config.h"

// System Status Enumeration
enum SystemStatus {
    STATUS_IDLE = 0,
    STATUS_HEATING,
    STATUS_COOLING,
    STATUS_DEFROST,
    STATUS_ERROR,
    STATUS_INITIALIZING
};

// System Mode Enumeration
enum SystemMode {
    MODE_OFF = 0,
    MODE_AUTO,
    MODE_MANUAL_HEAT,
    MODE_MANUAL_COOL,
    MODE_DEFROST
};

// Legacy compatibility aliases (if older code referenced these)
#define MODE_HEAT MODE_MANUAL_HEAT
#define MODE_COOL MODE_MANUAL_COOL

// Sensor Data Structure
struct SensorData {
    float temperature;
    float humidity;
    bool valid;
    unsigned long timestamp;
    unsigned long lastValidReading;
    uint8_t sensorId;
    String address;
};

// System Configuration
struct SystemConfig {
    SystemMode mode = MODE_AUTO;
    float targetTemp = DEFAULT_TARGET_TEMP;
    float tempHysteresis = TEMP_DEADBAND;
    uint8_t fanSpeed = DEFAULT_FAN_SPEED;
    bool buzzerEnabled = false;
    bool loggingEnabled = false;
    uint32_t defrostInterval = 0;
    uint32_t defrostDuration = 0;
    // Full-word parameter duplicates (preferred naming going forward)
    float TargetTemperature = DEFAULT_TARGET_TEMP;                // replaces targetTemp
    float TemperatureHysteresis = TEMP_DEADBAND;                  // replaces tempHysteresis
    uint32_t DefrostIntervalMilliseconds = 0;                     // replaces defrostInterval
    uint32_t DefrostDurationMilliseconds = 0;                     // replaces defrostDuration
};

// Control State
struct ControlState {
    SystemStatus status = STATUS_INITIALIZING;
    float currentTemp = 0.0f;
    float averageTemp = 0.0f;
    bool heatingActive = false;
    bool coolingActive = false;
    bool defrostActive = false;
    uint8_t fanPWM = 0;
    unsigned long lastUpdate = 0;
    unsigned long lastDefrost = 0;
    uint16_t errorCode = 0;
    uint32_t faultMask = 0;              // Bitmask of active faults (see FaultCodeBits)
    // Alarm subsystem (derived primarily from over/under temperature faults)
    bool alarmActive = false;            // True when alarm condition present
    bool alarmSilenced = false;          // True when operator has silenced audible/flash
    unsigned long alarmSince = 0;        // Timestamp when alarm first asserted
    unsigned long alarmSilenceUntil = 0; // Timestamp when silence period ends
};

// Fault codes (bit positions in ControlState::faultMask)
enum FaultCodeBits : uint8_t {
    FAULT_SENSOR_MISSING_BIT = 0,
    FAULT_SENSOR_RANGE_BIT = 1,
    FAULT_OVER_TEMPERATURE_BIT = 2,
    FAULT_UNDER_TEMPERATURE_BIT = 3,
    FAULT_DEFROST_TIMEOUT_BIT = 4,
    FAULT_COMPRESSOR_SHORT_CYCLE_BIT = 5,
    FAULT_FAN_FAILURE_BIT = 6,
};

inline constexpr uint32_t FaultBit(FaultCodeBits b) { return (1u << b); }

// Button Structure for UI
struct Button {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    String label;
    uint16_t bgColor;
    uint16_t textColor;
    bool pressed;
    void (*callback)();
};

// System Data for Display
struct SystemData {
    SensorData sensors[4];
    ControlState control;
    SystemConfig config;
    String timeString;
    String dateString;
    uint8_t activeSensors = 0;
    float minTemp = MIN_TEMP;
    float maxTemp = MAX_TEMP;
};

// Touch Point
struct TouchPoint {
    uint16_t x;
    uint16_t y;
    uint8_t id;
    bool touched;
};

// Display Colors (16-bit RGB565)
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFDA0
#define COLOR_GRAY      0x8410
#define COLOR_DARKGRAY  0x4208
#define COLOR_LIGHTGRAY 0xC618
#define COLOR_DARKBLUE  0x0010
#define COLOR_DARKGREEN 0x0400
#define COLOR_DARKRED   0x8000

// Temperature Display Colors
#define TEMP_COLOR_NORMAL  COLOR_CYAN
#define TEMP_COLOR_WARNING COLOR_YELLOW
#define TEMP_COLOR_CRITICAL COLOR_RED
#define TEMP_COLOR_FROZEN  COLOR_BLUE

#endif // TYPES_H