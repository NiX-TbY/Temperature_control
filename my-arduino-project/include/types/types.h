#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// System States
enum SystemMode {
    MODE_OFF = 0,
    MODE_HEAT = 1,
    MODE_COOL = 2,
    MODE_AUTO = 3,
    MODE_FAN_ONLY = 4
};

enum SystemStatus {
    STATUS_IDLE = 0,
    STATUS_HEATING = 1,
    STATUS_COOLING = 2,
    STATUS_FAN_RUNNING = 3,
    STATUS_ERROR = 4
};

// Sensor Data Structure
struct SensorData {
    float temperature = 0.0;
    float humidity = 0.0;
    float heatIndex = 0.0;
    bool isValid = false;
    unsigned long timestamp = 0;
};

// System Configuration Structure
struct SystemConfig {
    float targetTemp = DEFAULT_TARGET_TEMP;
    SystemMode mode = MODE_OFF;
    bool autoMode = false;
    float tempTolerance = TEMP_TOLERANCE;
    uint8_t fanSpeed = 50; // 0-100%
    bool buzzerEnabled = true;
    bool wifiEnabled = true;
};

// Control State Structure
struct ControlState {
    bool heatingActive = false;
    bool coolingActive = false;
    bool fanActive = false;
    uint8_t fanPWM = 0;
    SystemStatus status = STATUS_IDLE;
    unsigned long lastStateChange = 0;
};

// Touch Event Structure
struct TouchEvent {
    uint16_t x = 0;
    uint16_t y = 0;
    bool pressed = false;
    bool released = false;
    unsigned long timestamp = 0;
};

// Button Structure
struct Button {
    uint16_t x, y, w, h;
    String label;
    uint16_t color;
    uint16_t textColor;
    bool pressed = false;
    void (*callback)() = nullptr;
};

#endif
