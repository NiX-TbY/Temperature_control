#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <DHT.h>
#include "config/config.h"
#include "types/types.h"

class TemperatureSensor {
private:
    DHT* _dht;
    SensorData _lastReading;
    unsigned long _lastReadTime = 0;
    bool _initialized = false;
    uint8_t _errorCount = 0;

public:
    TemperatureSensor();
    ~TemperatureSensor();
    
    bool init();
    bool readSensor();
    SensorData getData() const { return _lastReading; }
    bool isDataValid() const { return _lastReading.isValid; }
    float getTemperature() const { return _lastReading.temperature; }
    float getHumidity() const { return _lastReading.humidity; }
    float getHeatIndex() const { return _lastReading.heatIndex; }
    
    void calibrateTemperature(float offset);
    bool selfTest();
};

extern TemperatureSensor tempSensor;

#endif
