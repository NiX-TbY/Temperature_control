#include <Arduino.h>
#include "sensors/temperature_sensor.h"

TemperatureSensor::TemperatureSensor() : _initialized(false), _lastTemperature(0.0) {
}

bool TemperatureSensor::init() {
    // Initialize sensor hardware
    _initialized = true;
    return true;
}

SensorData TemperatureSensor::read() {
    SensorData data;
    
    if (!_initialized) {
        data.isValid = false;
        return data;
    }
    
    // Simulate reading temperature
    _lastTemperature = 25.0; // Example temperature
    data.temperature = _lastTemperature;
    data.isValid = true;
    
    return data;
}

bool TemperatureSensor::isConnected() {
    return _initialized;
}