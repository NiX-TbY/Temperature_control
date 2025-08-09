#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "types/types.h"
#include "config/config.h"

class TemperatureSensor {
public:
    TemperatureSensor();
    
    bool init();
    void update();
    uint8_t getSensorCount() const { return _sensorCount; }
    SensorData getSensorData(uint8_t index) const;
    SensorData* getAllSensorData() { return _sensors; }
    float getAverageTemperature() const;
    bool hasValidData() const;
    
private:
    OneWire* _oneWire;
    DallasTemperature* _dallas;
    SensorData _sensors[MAX_SENSORS];
    uint8_t _sensorCount;
    DeviceAddress _addresses[MAX_SENSORS];
    unsigned long _lastReadTime;
    
    void scanSensors();
    String addressToString(DeviceAddress addr);
    bool isValidReading(float temp);
};

// Global sensor instance
extern TemperatureSensor tempSensor;

#endif // TEMPERATURE_SENSOR_H