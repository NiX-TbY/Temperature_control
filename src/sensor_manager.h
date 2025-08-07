#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

enum class SensorIndex : uint8_t {
    CABIN = 0,
    EVAPORATOR = 1,
    CONDENSER = 2,
    SUCTION = 3,
    COUNT = 4
};

class SensorManager {
public:
    static SensorManager& getInstance();
    
    bool init();
    void readAllSensors();
    bool isSensorValid(SensorIndex sensor);
    float getTemperature(SensorIndex sensor);
    const char* getSensorName(SensorIndex sensor);
    
    // Calibration
    void setSensorOffset(SensorIndex sensor, float offset);
    float getSensorOffset(SensorIndex sensor);
    
    // Fault detection
    bool checkSensorFaults();
    FaultCode getHighestPriorityFault();
    
private:
    SensorManager() = default;
    ~SensorManager() = default;
    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;
    
    OneWire oneWire;
    DallasTemperature sensors;
    
    DeviceAddress sensor_addresses[4];
    float sensor_offsets[4];
    bool sensors_found[4];
    uint8_t num_sensors_found;
    
    // Sensor fault tracking
    uint32_t sensor_fault_count[4];
    uint32_t last_valid_reading_time[4];
    
    static constexpr uint32_t SENSOR_TIMEOUT_MS = 10000;  // 10 seconds
    static constexpr uint32_t MAX_FAULT_COUNT = 3;        // Consecutive faults before marking as failed
    static constexpr float INVALID_TEMP = -127.0f;       // DallasTemperature error value
    static constexpr float SHORT_CIRCUIT_TEMP = 85.0f;   // DallasTemperature short circuit value
    
    bool discoverSensors();
    bool isTemperatureValid(float temp);
    void updateSensorStatus(SensorIndex sensor, float temperature);
    FaultCode getSensorFaultCode(SensorIndex sensor);
};