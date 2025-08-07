#pragma once

#include <Arduino.h>
#include "types/types.h"
#include "config/config.h"

class TemperatureController {
public:
    TemperatureController();
    
    // Main methods
    bool init();
    void update(const SensorData& sensor);
    
    // Configuration methods
    void setMode(SystemMode mode);
    void setTargetTemperature(float temp);
    void setFanSpeed(uint8_t speed);
    
    // Manual control methods
    void updateManualMode(const SensorData& sensor);
    void updateAutoMode(const SensorData& sensor);
    
    // Hardware control methods
    void activateHeating(bool enable);
    void activateCooling(bool enable);
    void activateFan(uint8_t speed);
    void updateOutputs();
    
    // Safety and diagnostics
    void safetyCheck(const SensorData& sensor);
    void emergency_stop();
    
    // PID control
    float calculatePID(float error);
    
    // Getters
    ControlState getState() const { return _state; }
    ControlConfig getConfig() const { return _config; }

private:
    // Member variables
    ControlState _state;
    ControlConfig _config;
    unsigned long _lastControlUpdate;
    
    // PID variables
    float _pidLastError;
    float _pidIntegral;
    float _kp;
    float _ki; 
    float _kd;
    
    // Internal state tracking
    unsigned long invalidDataStart;
};

// Global controller instance
extern TemperatureController controller;