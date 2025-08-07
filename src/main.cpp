#include <Arduino.h>
#include "controllers/temperature_controller.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Temperature Control System Starting...");
    
    // Initialize the temperature controller
    if (controller.init()) {
        Serial.println("Temperature controller initialized successfully");
    } else {
        Serial.println("Failed to initialize temperature controller");
        return;
    }
    
    Serial.println("System ready");
}

void loop() {
    // Create sample sensor data for testing
    SensorData sensor;
    sensor.temperature = 25.0; // Example temperature
    sensor.isValid = true;
    
    // Update the controller
    controller.update(sensor);
    
    // Small delay
    delay(100);
}