#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>

#include "config/config.h"
#include "types/types.h"
#include "controllers/temperature_controller.h"
#include "sensors/temperature_sensor.h"
#include "display/display_driver.h"
#include "display/ui_screens.h"
#include "utils/system_utils.h"

// Task handles
TaskHandle_t displayTaskHandle = nullptr;
TaskHandle_t touchTaskHandle = nullptr;
TaskHandle_t lvglTaskHandle = nullptr;
TaskHandle_t controlTaskHandle = nullptr;
TaskHandle_t sensorTaskHandle = nullptr;
TaskHandle_t loggingTaskHandle = nullptr;

// Mutex for shared data access
SemaphoreHandle_t dataMutex = nullptr;

// System data shared between tasks
SystemData systemData;

// Task functions
void displayTask(void *pvParameters);
void touchTask(void *pvParameters);
void lvglTask(void *pvParameters);
void controlTask(void *pvParameters);
void sensorTask(void *pvParameters);
void loggingTask(void *pvParameters);

// Function declarations
void setupButtons();
void initWiFi();
void printSystemStatus(const SensorData& sensor);

void setup() {
    // Initialize serial for debugging
    SystemUtils::initSerial();
    SystemUtils::printSystemInfo();
    
    // Initialize mutex for shared data
    dataMutex = xSemaphoreCreateMutex();
    
    // Initialize system data structure
    memset(&systemData, 0, sizeof(SystemData));
    systemData.timeString = "2025-08-07 06:15:12";  // Current time
    systemData.activeSensors = 0;
    systemData.minTemp = -30.0;
    systemData.maxTemp = 10.0;
    
    // Initialize display
    if (!display.init()) {
        Serial.println("ERROR: Display initialization failed!");
        while (1) { delay(1000); }  // Halt on critical error
    }
    
    // Initialize temperature controller
    if (!controller.init()) {
        Serial.println("ERROR: Temperature controller initialization failed!");
        // Continue anyway, might work partially
    }
    
    // Initialize temperature sensors
    if (!tempSensor.init()) {
        Serial.println("WARNING: Temperature sensor initialization failed!");
        // Continue anyway, might be connected later
    }
    
    // Initialize UI components
    if (!ui.init()) {
        Serial.println("ERROR: UI initialization failed!");
        while (1) { delay(1000); }  // Halt on critical error
    }
    
    // Create UI task (Core 0, high priority)
    xTaskCreatePinnedToCore(
        displayTask,
        "displayTask",
        DISPLAY_TASK_STACK,
        nullptr,
        DISPLAY_TASK_PRIORITY,
        &displayTaskHandle,
        0
    );
    
    // Create touch task (Core 0, high priority)
    xTaskCreatePinnedToCore(
        touchTask,
        "touchTask",
        TOUCH_TASK_STACK,
        nullptr,
        TOUCH_TASK_PRIORITY,
        &touchTaskHandle,
        0
    );
    
    // Create LVGL task (Core 0, high priority)
    xTaskCreatePinnedToCore(
        lvglTask,
        "lvglTask",
        LVGL_TASK_STACK,
        nullptr,
        LVGL_TASK_PRIORITY,
        &lvglTaskHandle,
        0
    );
    
    // Create control task (Core 1, medium priority)
    xTaskCreatePinnedToCore(
        controlTask,
        "controlTask",
        CONTROL_TASK_STACK,
        nullptr,
        CONTROL_TASK_PRIORITY,
        &controlTaskHandle,
        1
    );
    
    // Create sensor task (Core 1, medium priority)
    xTaskCreatePinnedToCore(
        sensorTask,
        "sensorTask",
        SENSOR_TASK_STACK,
        nullptr,
        SENSOR_TASK_PRIORITY,
        &sensorTaskHandle,
        1
    );
    
    // Create logging task (Core 1, low priority)
    xTaskCreatePinnedToCore(
        loggingTask,
        "loggingTask",
        LOGGING_TASK_STACK,
        nullptr,
        LOGGING_TASK_PRIORITY,
        &loggingTaskHandle,
        1
    );
    
    Serial.println("System initialization complete");
}

void loop() {
    // Arduino loop not used with FreeRTOS tasks
    delay(1000);
}

// Display task - handles display updates
void displayTask(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(10);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        display.update();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Touch task - handles touch input
void touchTask(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(20);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Touch handling is done in LVGL callbacks
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// LVGL task - handles UI updates
void lvglTask(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // 20Hz UI refresh
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            // Update UI with latest system data
            ui.update(systemData);
            xSemaphoreGive(dataMutex);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Control task - implements temperature control logic
void controlTask(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 1Hz control loop
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Register this task with watchdog
    esp_task_wdt_add(nullptr);
    
    while (true) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Get latest sensor data
            SensorData* sensors = tempSensor.getAllSensorData();
            uint8_t sensorCount = tempSensor.getSensorCount();
            
            // Update controller with sensor data
            controller.updateWithMultipleSensors(sensors, sensorCount);
            
            // Update system data for UI
            systemData.control = controller.getState();
            systemData.config = controller.getConfig();
            
            xSemaphoreGive(dataMutex);
        }
        
        // Reset watchdog
        esp_task_wdt_reset();
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Sensor task - reads temperature sensors
void sensorTask(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 1Hz sensor reads
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Update sensor readings
        tempSensor.update();
        
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Copy sensor data to shared structure
            systemData.activeSensors = tempSensor.getSensorCount();
            for (uint8_t i = 0; i < systemData.activeSensors && i < 4; i++) {
                systemData.sensors[i] = tempSensor.getSensorData(i);
            }
            
            xSemaphoreGive(dataMutex);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Logging task - logs data to SD card
void loggingTask(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(5000); // Log every 5 seconds
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Allow time for other systems to initialize
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Initialize SD card for logging
    bool sdInitialized = SystemUtils::initSDCard();
    
    while (true) {
        if (sdInitialized && controller.getConfig().loggingEnabled) {
            // Take mutex to safely access data
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                // Log data
                SystemUtils::logData(systemData);
                xSemaphoreGive(dataMutex);
            }
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}