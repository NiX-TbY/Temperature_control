#include "temperature_sensors.h"
#include <Arduino.h>

TemperatureSensorManager* g_temp_sensor_manager = nullptr;

TemperatureSensorManager::TemperatureSensorManager() 
    : oneWire(DS18B20_PIN), sensors(&oneWire), lastUpdateTime(0) {
    
    // Initialize arrays
    for (int i = 0; i < 4; i++) {
        lastTemperatures[i] = -999.0f;
        sensorValid[i] = false;
        memset(sensorAddresses[i], 0, 8);
    }
}

bool TemperatureSensorManager::begin() {
    sensors.begin();
    
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount());
    Serial.println(" temperature sensors");
    
    if (sensors.getDeviceCount() == 0) {
        Serial.println("No temperature sensors found!");
        return false;
    }
    
    // Set resolution to 12-bit for all sensors
    sensors.setResolution(12);
    
    return discoverSensors();
}

void TemperatureSensorManager::updateTemperatures() {
    unsigned long currentTime = millis();
    
    // Limit update frequency to prevent bus overload
    if (currentTime - lastUpdateTime < 1000) {
        return;
    }
    
    lastUpdateTime = currentTime;
    
    // Request temperatures from all sensors
    sensors.requestTemperatures();
    
    // Read each sensor
    for (int i = 0; i < 4 && i < sensors.getDeviceCount(); i++) {
        float temp = sensors.getTempC(sensorAddresses[i]);
        
        if (isValidTemperature(temp)) {
            lastTemperatures[i] = temp;
            sensorValid[i] = true;
        } else {
            sensorValid[i] = false;
            Serial.print("Invalid temperature reading from sensor ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(temp);
        }
    }
    
    // Update global state
    xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
    for (int i = 0; i < 4; i++) {
        g_system_state.sensor_values[i] = lastTemperatures[i];
    }
    // Update actual temperature with cabin sensor (index 0)
    if (sensorValid[0]) {
        g_system_state.actual_temp_celsius = lastTemperatures[0];
    }
    xSemaphoreGive(g_system_state.mutex);
}

float TemperatureSensorManager::getCabinTemperature() {
    return lastTemperatures[0];
}

float TemperatureSensorManager::getEvaporatorTemperature() {
    return lastTemperatures[1];
}

float TemperatureSensorManager::getCondenserTemperature() {
    return lastTemperatures[2];
}

float TemperatureSensorManager::getSuctionTemperature() {
    return lastTemperatures[3];
}

bool TemperatureSensorManager::isCabinSensorValid() {
    return sensorValid[0];
}

bool TemperatureSensorManager::isEvaporatorSensorValid() {
    return sensorValid[1];
}

bool TemperatureSensorManager::isCondenserSensorValid() {
    return sensorValid[2];
}

bool TemperatureSensorManager::isSuctionSensorValid() {
    return sensorValid[3];
}

int TemperatureSensorManager::getDeviceCount() {
    return sensors.getDeviceCount();
}

bool TemperatureSensorManager::discoverSensors() {
    int deviceCount = sensors.getDeviceCount();
    
    Serial.println("Discovering sensor addresses...");
    
    for (int i = 0; i < deviceCount && i < 4; i++) {
        if (sensors.getAddress(sensorAddresses[i], i)) {
            Serial.print("Sensor ");
            Serial.print(i);
            Serial.print(" address: ");
            for (uint8_t j = 0; j < 8; j++) {
                if (sensorAddresses[i][j] < 16) Serial.print("0");
                Serial.print(sensorAddresses[i][j], HEX);
            }
            Serial.println();
        } else {
            Serial.print("Unable to find address for sensor ");
            Serial.println(i);
            return false;
        }
    }
    
    return true;
}

bool TemperatureSensorManager::isValidTemperature(float temp) {
    // DS18B20 returns these values for error conditions
    return (temp != -127.0 && temp != 85.0 && temp > -55.0 && temp < 125.0);
}