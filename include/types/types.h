#pragma once

#include <Arduino.h>

// Sensor data structure
struct SensorData {
    float temperature;
    bool isValid;
    
    SensorData() : temperature(0.0), isValid(false) {}
};

// System modes
enum SystemMode {
    MODE_AUTO,
    MODE_MANUAL,
    MODE_HEAT,
    MODE_COOL,
    MODE_FAN_ONLY,
    MODE_OFF
};

// Status constants
enum SystemStatus {
    STATUS_IDLE,
    STATUS_HEATING,
    STATUS_COOLING,
    STATUS_FAN_RUNNING,
    STATUS_ERROR
};

// Control state structure
struct ControlState {
    SystemStatus status;
    bool heatingActive;
    bool coolingActive;
    uint8_t fanPWM;
    unsigned long lastStateChange;
    
    ControlState() : status(STATUS_IDLE), heatingActive(false), 
                    coolingActive(false), fanPWM(0), lastStateChange(0) {}
};

// Configuration structure
struct ControlConfig {
    SystemMode mode;
    float targetTemp;
    uint8_t fanSpeed;
    bool buzzerEnabled;
    
    ControlConfig() : mode(MODE_AUTO), targetTemp(20.0), 
                     fanSpeed(50), buzzerEnabled(true) {}
};