#ifndef TEMPERATURE_CONTROLLER_H
#define TEMPERATURE_CONTROLLER_H

#include "config/config.h"
#include "types/types.h"

class TemperatureController {
private:
    SystemConfig _config;
    ControlState _state;
    unsigned long _lastControlUpdate = 0;
    unsigned long _stateChangeTime = 0;
    float _pidIntegral = 0;
    float _pidLastError = 0;
    
    // PID Constants
    float _kp = 2.0;
    float _ki = 0.1;
    float _kd = 0.5;

public:
    TemperatureController();
    
    bool init();
    void update(const SensorData& sensor);
    void setMode(SystemMode mode);
    void setTargetTemperature(float temp);
    void setFanSpeed(uint8_t speed);
    void emergency_stop();
    
    // Getters
    SystemConfig getConfig() const { return _config; }
    ControlState getState() const { return _state; }
    SystemMode getMode() const { return _config.mode; }
    float getTargetTemperature() const { return _config.targetTemp; }
    
    // Control functions
    void activateHeating(bool enable);
    void activateCooling(bool enable);
    void activateFan(uint8_t speed);
    
private:
    void updateAutoMode(const SensorData& sensor);
    void updateManualMode(const SensorData& sensor);
    float calculatePID(float error);
    void updateOutputs();
    void safetyCheck(const SensorData& sensor);
};

extern TemperatureController controller;

#endif
