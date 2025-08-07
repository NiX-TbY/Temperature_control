#include "utils/system_utils.h"
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <esp_sleep.h>
#include <esp_chip_info.h>
#include <esp_spi_flash.h>

void SystemUtils::initSerial() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n");
    Serial.println("===========================================");
    Serial.println("  Temperature Control System Initializing  ");
    Serial.println("===========================================");
}

void SystemUtils::printSystemInfo() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    Serial.println("=== System Information ===");
    Serial.printf("ESP32 Chip: %s Rev %d\n", 
                  (chip_info.model == CHIP_ESP32S3) ? "ESP32-S3" : "Unknown", 
                  chip_info.revision);
    Serial.printf("Number of cores: %d\n", chip_info.cores);
    Serial.printf("Flash size: %dMB %s\n", 
                  spi_flash_get_chip_size() / (1024 * 1024),
                  (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("Reset reason: %s\n", getResetReason().c_str());
    
    Serial.println("===========================================");
}

void SystemUtils::scanI2C() {
    byte error, address;
    int deviceCount = 0;
    
    Serial.println("Scanning I2C bus...");
    
    for(address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("I2C device found at address 0x%02X\n", address);
            deviceCount++;
            
            // Give details about known addresses
            if (address == 0x71) {
                Serial.println("  - CH422G I/O Expander (critical for display)");
            } else if (address == 0x5D || address == 0x14) {
                Serial.println("  - GT911 Touch Controller");
            } else if (address == 0x51) {
                Serial.println("  - PCF85063A Real-Time Clock");
            }
        } else if (error != 2) { // error=2 means address sent, no device found
            Serial.printf("Error %d at address 0x%02X\n", error, address);
        }
    }
    
    if (deviceCount == 0) {
        Serial.println("!!! WARNING: No I2C devices found!");
        Serial.println("Display initialization will fail without CH422G at 0x71");
    } else {
        Serial.printf("Found %d device(s) on I2C bus\n", deviceCount);
    }
}

String SystemUtils::getResetReason() {
    esp_reset_reason_t reason = esp_reset_reason();
    
    switch (reason) {
        case ESP_RST_POWERON: return "Power-on reset";
        case ESP_RST_EXT: return "External reset";
        case ESP_RST_SW: return "Software reset";
        case ESP_RST_PANIC: return "Exception/Panic";
        case ESP_RST_INT_WDT: return "Interrupt watchdog";
        case ESP_RST_TASK_WDT: return "Task watchdog";
        case ESP_RST_WDT: return "Other watchdog";
        case ESP_RST_DEEPSLEEP: return "Deep sleep wakeup";
        case ESP_RST_BROWNOUT: return "Brownout reset";
        case ESP_RST_SDIO: return "SDIO reset";
        default: return "Unknown reset";
    }
}

uint32_t SystemUtils::getFreeHeap() {
    return ESP.getFreeHeap();
}

uint32_t SystemUtils::getFreePSRAM() {
    return ESP.getFreePsram();
}

float SystemUtils::getCPUTemperature() {
    // Note: ESP32-S3 doesn't support temperature sensor directly
    // This would be implemented using an external sensor
    return 0.0f;
}

void SystemUtils::restart() {
    Serial.println("System restarting...");
    delay(500);
    ESP.restart();
}

void SystemUtils::deepSleep(uint32_t seconds) {
    Serial.printf("Entering deep sleep for %u seconds\n", seconds);
    delay(100);
    esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
    esp_deep_sleep_start();
}

void SystemUtils::printTaskList() {
    char buffer[512];
    vTaskList(buffer);
    Serial.println("Task Name\tStatus\tPrio\tHWM\tTask#");
    Serial.println("-------------------------------------------");
    Serial.println(buffer);
}

uint32_t SystemUtils::getTaskHighWaterMark(TaskHandle_t task) {
    return uxTaskGetStackHighWaterMark(task);
}

String SystemUtils::formatTime(uint8_t hour, uint8_t minute, uint8_t second) {
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hour, minute, second);
    return String(buffer);
}

String SystemUtils::formatDate(uint8_t day, uint8_t month, uint16_t year) {
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year, month, day);
    return String(buffer);
}

String SystemUtils::formatTemperature(float temp, bool includeUnit) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.1f%s", temp, includeUnit ? "°C" : "");
    return String(buffer);
}

bool SystemUtils::initSDCard() {
    // SD card initialization would be implemented here
    // Uses I/O expander to control SD_CS pin
    return true;
}

bool SystemUtils::logData(const SystemData& data) {
    // Implement SD card logging here
    return true;
}

bool SystemUtils::readConfig(SystemConfig& config) {
    // Read configuration from storage
    return true;
}

bool SystemUtils::writeConfig(const SystemConfig& config) {
    // Write configuration to storage
    return true;
}

void SystemUtils::watchdogReset() {
    // Alternative watchdog reset for ESP32-S3
    #ifdef CONFIG_ESP_TASK_WDT_EN
        esp_task_wdt_reset();
    #else
        // If watchdog not available, do nothing
        return;
    #endif
}