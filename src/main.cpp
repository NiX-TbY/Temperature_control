#include <Arduino.h>
#include "controllers/temperature_controller.h"
#include "display/display_driver.h"
#include "sensors/temperature_sensor.h"

// Global instances
DisplayDriver display;
TemperatureSensor sensor;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Temperature Control System Starting...");
    
    // Initialize display
    if (display.init()) {
        Serial.println("Display initialized successfully");
    } else {
        Serial.println("Failed to initialize display");
    }
    
    // Initialize sensor
    if (sensor.init()) {
        Serial.println("Sensor initialized successfully");
    } else {
        Serial.println("Failed to initialize sensor");
    }
    
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
    // Read sensor data
    SensorData sensorData = sensor.read();
    
    // Update the controller
    controller.update(sensorData);
    
    // Update display
    display.update();
    
    // Small delay
    delay(100);
}