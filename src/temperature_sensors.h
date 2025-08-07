#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

class TemperatureSensorManager {
public:
    TemperatureSensorManager();
    bool begin();
    
    void updateTemperatures();
    float getCabinTemperature();
    float getEvaporatorTemperature();
    float getCondenserTemperature();
    float getSuctionTemperature();
    
    bool isCabinSensorValid();
    bool isEvaporatorSensorValid();
    bool isCondenserSensorValid();
    bool isSuctionSensorValid();
    
    int getDeviceCount();

private:
    OneWire oneWire;
    DallasTemperature sensors;
    
    DeviceAddress sensorAddresses[4];
    float lastTemperatures[4];
    bool sensorValid[4];
    unsigned long lastUpdateTime;
    
    bool discoverSensors();
    bool isValidTemperature(float temp);
};

extern TemperatureSensorManager* g_temp_sensor_manager;