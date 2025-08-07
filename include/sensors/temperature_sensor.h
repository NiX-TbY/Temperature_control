#pragma once

#include <Arduino.h>
#include "types/types.h"

class TemperatureSensor {
public:
    TemperatureSensor();
    bool init();
    SensorData read();
    bool isConnected();
    
private:
    bool _initialized;
    float _lastTemperature;
};