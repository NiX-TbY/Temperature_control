#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "config.h"
#include "ESP_IOExpander.h"
#include "relay_controller.h"
#include "temperature_sensors.h"

// Global system state
SystemState g_system_state;

// Hardware components
esp_expander::CH422G* io_expander = nullptr;

// Simple console interface for testing
void printSystemStatus() {
    Serial.println("\n=== System Status ===");
    
    xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
    
    Serial.print("Setpoint: ");
    Serial.print(g_system_state.setpoint_temp_celsius);
    Serial.println("°C");
    
    Serial.print("Actual: ");
    Serial.print(g_system_state.actual_temp_celsius);
    Serial.println("°C");
    
    Serial.print("Sensors: ");
    for (int i = 0; i < 4; i++) {
        Serial.print(g_system_state.sensor_values[i]);
        Serial.print("°C ");
    }
    Serial.println();
    
    Serial.print("Relays: 0x");
    Serial.println(g_system_state.relay_states, HEX);
    
    Serial.print("Alarm: ");
    switch (g_system_state.alarm_status) {
        case AlarmState::NONE: Serial.println("None"); break;
        case AlarmState::HIGH_TEMP_ACTIVE: Serial.println("High Temp Active"); break;
        case AlarmState::HIGH_TEMP_SILENCED: Serial.println("High Temp Silenced"); break;
    }
    
    Serial.print("Defrost: ");
    Serial.println(g_system_state.defrost_active ? "Active" : "Inactive");
    
    xSemaphoreGive(g_system_state.mutex);
    
    Serial.println("===================");
}

// FreeRTOS Tasks
void sensor_task(void* parameter) {
    while (1) {
        if (g_temp_sensor_manager) {
            g_temp_sensor_manager->updateTemperatures();
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // Update every 2 seconds
    }
}

void control_task(void* parameter) {
    while (1) {
        // Basic control logic
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        
        float cabin_temp = g_system_state.actual_temp_celsius;
        float setpoint = g_system_state.setpoint_temp_celsius;
        
        // Simple hysteresis control
        if (cabin_temp > setpoint + TEMP_HYSTERESIS_C) {
            // Too warm, turn on compressor
            if (g_relay_controller) {
                g_relay_controller->setCompressorState(true);
                g_relay_controller->setFanState(true);
            }
        } else if (cabin_temp < setpoint - TEMP_HYSTERESIS_C) {
            // Too cold, turn off compressor
            if (g_relay_controller) {
                g_relay_controller->setCompressorState(false);
                g_relay_controller->setFanState(false);
            }
        }
        
        // Check for high temperature alarm
        if (cabin_temp > setpoint + HIGH_TEMP_ALARM_DIFFERENTIAL_C) {
            g_system_state.alarm_status = AlarmState::HIGH_TEMP_ACTIVE;
            if (g_relay_controller) {
                g_relay_controller->setBuzzer(true);
            }
        } else {
            g_system_state.alarm_status = AlarmState::NONE;
            if (g_relay_controller) {
                g_relay_controller->setBuzzer(false);
            }
        }
        
        xSemaphoreGive(g_system_state.mutex);
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Run control loop every second
    }
}

void status_task(void* parameter) {
    while (1) {
        printSystemStatus();
        vTaskDelay(pdMS_TO_TICKS(5000)); // Print status every 5 seconds
    }
}

bool init_hardware() {
    Serial.println("Initializing hardware...");
    
    // Initialize I2C bus
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("I2C bus initialized");
    
    // Initialize CH422G I/O Expander
    io_expander = new esp_expander::CH422G(I2C_SCL_PIN, I2C_SDA_PIN, IOEXP_I2C_ADDR);
    if (!io_expander->init() || !io_expander->begin()) {
        Serial.println("Warning: Failed to initialize CH422G I/O expander");
        return false; // Continue even if CH422G fails for testing
    }
    Serial.println("CH422G I/O expander initialized");
    
    // Test display reset sequence
    if (io_expander->isConnected()) {
        io_expander->pinMode(CH422G_RST_PIN, OUTPUT);
        io_expander->digitalWrite(CH422G_RST_PIN, LOW);
        delay(20);
        io_expander->digitalWrite(CH422G_RST_PIN, HIGH);
        delay(50);
        Serial.println("Display reset sequence completed");
        
        // Enable backlight
        io_expander->pinMode(CH422G_BL_EN_PIN, OUTPUT);
        io_expander->digitalWrite(CH422G_BL_EN_PIN, HIGH);
        Serial.println("Display backlight enabled");
    }
    
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Commercial Freezer Controller (Test Version) ===");
    Serial.println("Booting...");
    
    // Initialize global system state
    g_system_state.mutex = xSemaphoreCreateMutex();
    g_system_state.setpoint_temp_celsius = DEFAULT_SETPOINT_C;
    g_system_state.actual_temp_celsius = DEFAULT_SETPOINT_C;
    g_system_state.alarm_status = AlarmState::NONE;
    g_system_state.defrost_active = false;
    g_system_state.active_fault_code = FaultCode::NONE;
    g_system_state.relay_states = 0;
    
    for (int i = 0; i < 4; i++) {
        g_system_state.sensor_values[i] = -999.0f;
    }
    
    // Initialize hardware components
    if (!init_hardware()) {
        Serial.println("Hardware initialization completed with warnings");
    }
    
    // Initialize peripheral managers
    g_relay_controller = new RelayController(PCF8574_I2C_ADDR);
    if (!g_relay_controller->begin()) {
        Serial.println("Warning: PCF8574 relay controller not found (this is normal for testing)");
    } else {
        Serial.println("PCF8574 relay controller initialized");
    }
    
    g_temp_sensor_manager = new TemperatureSensorManager();
    if (!g_temp_sensor_manager->begin()) {
        Serial.println("Warning: Temperature sensors not found (this is normal for testing)");
        // Set some dummy values for testing
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        g_system_state.actual_temp_celsius = -18.5f;
        g_system_state.sensor_values[0] = -18.5f; // Cabin
        g_system_state.sensor_values[1] = -22.0f; // Evaporator
        g_system_state.sensor_values[2] = 35.0f;  // Condenser
        g_system_state.sensor_values[3] = -15.0f; // Suction
        xSemaphoreGive(g_system_state.mutex);
    } else {
        Serial.println("Temperature sensors initialized");
    }
    
    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(sensor_task, "Sensor_Task", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(control_task, "Control_Task", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(status_task, "Status_Task", 4096, NULL, 1, NULL, 1);
    
    Serial.println("System initialization complete!");
    Serial.println("Core 1: Sensor, Control, and Status Tasks");
    Serial.println("Monitoring system status...");
}

void loop() {
    // All processing handled by FreeRTOS tasks
    vTaskDelay(portMAX_DELAY);
}