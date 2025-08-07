#include <Arduino.h>
#include <WiFi.h>
#include "config/config.h"
#include "types/types.h"
#include "display/display_driver.h"
#include "sensors/temperature_sensor.h"
#include "controllers/temperature_controller.h"

// Global variables
unsigned long lastSystemUpdate = 0;
bool systemInitialized = false;

// Button callback functions
void onHeatButtonPress();
void onCoolButtonPress();
void onAutoButtonPress();
void onOffButtonPress();
void onTempUpButtonPress();
void onTempDownButtonPress();
void onFanUpButtonPress();
void onFanDownButtonPress();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_PRINTLN("=== Temperature Control System Starting ===");
    DEBUG_PRINTLN("Hardware: ESP32-S3 with 4.3\" Waveshare Display");
    
    // Initialize display first
    if (!display.init()) {
        DEBUG_PRINTLN("FATAL: Display initialization failed");
        while(1) delay(1000);
    }
    
    // Show startup screen
    display.clear();
    auto tft = display.getTFT();
    tft->setTextColor(TFT_WHITE);
    tft->setTextSize(3);
    tft->drawString("Initializing...", 250, 200);
    tft->setTextSize(2);
    tft->drawString("Temperature Control System", 220, 250);
    
    // Initialize temperature sensor
    if (!tempSensor.init()) {
        DEBUG_PRINTLN("WARNING: Temperature sensor initialization failed");
        tft->setTextColor(TFT_RED);
        tft->drawString("Sensor Error - Check Connections", 180, 300);
    } else {
        tft->setTextColor(TFT_GREEN);
        tft->drawString("Sensor OK", 320, 300);
    }
    
    delay(1000);
    
    // Initialize controller
    if (!controller.init()) {
        DEBUG_PRINTLN("FATAL: Controller initialization failed");
        while(1) delay(1000);
    }
    
    tft->setTextColor(TFT_GREEN);
    tft->drawString("Controller OK", 300, 330);
    delay(1000);
    
    // Setup control buttons
    setupButtons();
    
    // Initialize WiFi (optional)
    initWiFi();
    
    systemInitialized = true;
    display.forceRedraw();
    
    DEBUG_PRINTLN("=== System Initialization Complete ===");
}

void loop() {
    if (!systemInitialized) return;
    
    // Read sensors
    tempSensor.readSensor();
    SensorData sensorData = tempSensor.getData();
    
    // Update controller
    controller.update(sensorData);
    
    // Handle touch input
    TouchEvent touchEvent;
    if (display.getTouch(touchEvent)) {
        display.handleTouch(touchEvent);
    }
    
    // Update display
    display.drawMainScreen(sensorData, controller.getConfig(), controller.getState());
    
    // System status updates
    if (millis() - lastSystemUpdate > 5000) {
        printSystemStatus(sensorData);
        lastSystemUpdate = millis();
    }
    
    delay(50); // Main loop delay
}

void setupButtons() {
    // Control mode buttons
    display.addButton(50, 220, 100, 50, "HEAT", TFT_DARKRED, onHeatButtonPress);
    display.addButton(170, 220, 100, 50, "COOL", TFT_DARKBLUE, onCoolButtonPress);
    display.addButton(290, 220, 100, 50, "AUTO", TFT_DARKGREEN, onAutoButtonPress);
    display.addButton(410, 220, 100, 50, "OFF", TFT_MAROON, onOffButtonPress);
    
    // Temperature adjustment buttons
    display.addButton(650, 200, 60, 40, "T+", TFT_DARKGREY, onTempUpButtonPress);
    display.addButton(650, 250, 60, 40, "T-", TFT_DARKGREY, onTempDownButtonPress);
    
    // Fan speed buttons
    display.addButton(650, 320, 60, 40, "F+", TFT_DARKGREY, onFanUpButtonPress);
    display.addButton(650, 370, 60, 40, "F-", TFT_DARKGREY, onFanDownButtonPress);
}

void initWiFi() {
    DEBUG_PRINTLN("Initializing WiFi...");
    
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("");
        DEBUG_PRINTF("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        DEBUG_PRINTLN("WiFi connection failed - continuing without network");
    }
}

void printSystemStatus(const SensorData& sensor) {
    DEBUG_PRINTLN("=== System Status ===");
    DEBUG_PRINTF("Temperature: %.1f°C\n", sensor.temperature);
    DEBUG_PRINTF("Humidity: %.1f%%\n", sensor.humidity);
    DEBUG_PRINTF("Target: %.1f°C\n", controller.getTargetTemperature());
    DEBUG_PRINTF("Mode: %d\n", controller.getMode());
    DEBUG_PRINTF("Status: %d\n", controller.getState().status);
    DEBUG_PRINTF("Heap Free: %d bytes\n", ESP.getFreeHeap());
    DEBUG_PRINTLN("===================");
}

// Button callback implementations
void onHeatButtonPress() {
    DEBUG_PRINTLN("Heat button pressed");
    controller.setMode(MODE_HEAT);
    display.forceRedraw();
}

void onCoolButtonPress() {
    DEBUG_PRINTLN("Cool button pressed");
    controller.setMode(MODE_COOL);
    display.forceRedraw();
}

void onAutoButtonPress() {
    DEBUG_PRINTLN("Auto button pressed");
    controller.setMode(MODE_AUTO);
    display.forceRedraw();
}

void onOffButtonPress() {
    DEBUG_PRINTLN("Off button pressed");
    controller.setMode(MODE_OFF);
    display.forceRedraw();
}

void onTempUpButtonPress() {
    float newTemp = controller.getTargetTemperature() + 0.5;
    controller.setTargetTemperature(newTemp);
    DEBUG_PRINTF("Target temperature increased to %.1f°C\n", newTemp);
}

void onTempDownButtonPress() {
    float newTemp = controller.getTargetTemperature() - 0.5;
    controller.setTargetTemperature(newTemp);
    DEBUG_PRINTF("Target temperature decreased to %.1f°C\n", newTemp);
}

void onFanUpButtonPress() {
    // Fan speed control implementation
    DEBUG_PRINTLN("Fan speed up");
}

void onFanDownButtonPress() {
    // Fan speed control implementation
    DEBUG_PRINTLN("Fan speed down");
}