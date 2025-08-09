#ifndef TEMPERATURE_CONTROLLER_SIMPLE_H
#define TEMPERATURE_CONTROLLER_SIMPLE_H

#include <Arduino.h>
#include "types/types.h"
#include "config/config.h"

class TemperatureControllerSimple {
public:
    TemperatureControllerSimple();
    bool init();
    void updateWithMultipleSensors(const SensorData sensors[], uint8_t count);
    void update(const SensorData& sensor);
    void setMode(SystemMode mode) { _config.mode = mode; }
    void setTargetTemperature(float t) { _config.targetTemp = constrain(t, TEMP_MIN_SAFE, TEMP_MAX_SAFE); }
    ControlState getState() const { return _state; }
    SystemConfig getConfig() const { return _config; }
private:
    SystemConfig _config;
    ControlState _state;
    float _pidLastError, _pidIntegral, _kp, _ki, _kd;
    unsigned long _lastControlUpdate, _defrostStartTime;
    void updateAutoMode(float avgTemp);
    void updateManualMode();
    void updateDefrostMode();
    void activateHeating(bool e);
    void activateCooling(bool e);
    void activateFan(uint8_t spd);
    void updateOutputs();
    void safetyCheck(const SensorData sensors[], uint8_t count);
    float calculatePID(float error);
    float calculateAverageTemp(const SensorData sensors[], uint8_t count);
    bool shouldStartDefrost();
};

#endif
