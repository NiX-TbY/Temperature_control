#ifndef TEMPERATURE_CONTROLLER_H
#define TEMPERATURE_CONTROLLER_H

#ifdef UNIT_TEST_NATIVE
#include "test_support/arduino_stub.h"
#else
#include <Arduino.h>
#endif
#include "types/types.h"
#include "config/config.h"

class TemperatureController {
public:
    TemperatureController();
    
    bool init();
    void update(const SensorData& sensor);
    void updateWithMultipleSensors(const SensorData sensors[], uint8_t count);
    void setMode(SystemMode mode);
    void setTargetTemperature(float temp);
    void setTargetTemperatureFull(float temp) { setTargetTemperature(temp); }
    void setFanSpeed(uint8_t speed);
    void setHysteresis(float value);
    void setTemperatureHysteresisFull(float value) { setHysteresis(value); }
    void emergency_stop();
    void startDefrost();
    void stopDefrost();
    uint32_t getFaultMask() const { return _state.faultMask; }
    bool hasFault() const { return _state.faultMask != 0 || _state.status == STATUS_ERROR; }
    bool faultActive(uint32_t bit) const { return (_state.faultMask & bit) != 0; }
    
    // Getters
    SystemConfig getConfig() const { return _config; }
    ControlState getState() const { return _state; }
    bool isDefrosting() const { return _state.defrostActive; }
    void evaluateFaults(const SensorData sensors[], uint8_t count);
    void clearResolvedFaults();
    void silenceAlarm();

    // Alarm / Fault event record
    struct EventRecord {
        unsigned long ts;
        uint32_t mask;      // snapshot of fault mask
        uint16_t code;      // reserved for future (alarm type)
    };
    static constexpr size_t EVENT_LOG_SIZE = 32;
    size_t getEventLogCount() const { return _eventCount; }
    EventRecord getEvent(size_t idx) const { return _eventLog[idx % EVENT_LOG_SIZE]; }
    
private:
    SystemConfig _config;
    ControlState _state;
    
    // PID control variables
    float _pidLastError;
    float _pidIntegral;
    float _kp;  // Proportional gain
    float _ki;  // Integral gain
    float _kd;  // Derivative gain
    
    unsigned long _lastControlUpdate;
    unsigned long _defrostStartTime;
    unsigned long _lastCompressorChange;
    unsigned long _lastCoolingRequest;
    unsigned long _lastHeatingRequest;
    // Fault debounce timers
    unsigned long _overTempSince;
    unsigned long _underTempSince;
    unsigned long _sensorMissingSince;
    unsigned long _rangeFaultSince;
    unsigned long _defrostOverrunSince;

    // Event log
    EventRecord _eventLog[EVENT_LOG_SIZE];
    size_t _eventHead = 0;
    size_t _eventCount = 0;

    void logEvent(uint16_t code);

    // Internal helpers
    bool hasValidSensor(const SensorData sensors[], uint8_t count) const;
    void gateShortCycle(bool wantCooling);
    
    // Private methods
    void updateAutoMode(float avgTemp);
    void updateManualMode();
    void updateDefrostMode();
    void activateHeating(bool enable);
    void activateCooling(bool enable);
    void activateFan(uint8_t speed);
    void updateOutputs();
    void safetyCheck(const SensorData sensors[], uint8_t count);
    void updateFault(uint32_t bit, bool active);
    void clearAllFaults();
    float calculatePID(float error);
    float calculateAverageTemp(const SensorData sensors[], uint8_t count);
    bool shouldStartDefrost();
};

// Global controller instance
extern TemperatureController controller;

#endif // TEMPERATURE_CONTROLLER_H