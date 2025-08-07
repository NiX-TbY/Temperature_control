#include "utils/system_utils.h"
#include "config/config.h"

void SystemUtils::printSystemInfo() {
    DEBUG_PRINTLN("=== System Information ===");
    DEBUG_PRINTF("Chip Model: %s\n", ESP.getChipModel());
    DEBUG_PRINTF("Chip Revision: %d\n", ESP.getChipRevision());
    DEBUG_PRINTF("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    DEBUG_PRINTF("Free Heap: %d bytes\n", ESP.getFreeHeap());
    DEBUG_PRINTF("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    DEBUG_PRINTF("Flash Size: %d bytes\n", ESP.getFlashChipSize());
    DEBUG_PRINTF("Uptime: %s\n", getUptimeString().c_str());
    DEBUG_PRINTLN("==========================");
}

String SystemUtils::getUptimeString() {
    unsigned long uptime = millis() / 1000;
    unsigned long days = uptime / 86400;
    uptime %= 86400;
    unsigned long hours = uptime / 3600;
    uptime %= 3600;
    unsigned long minutes = uptime / 60;
    unsigned long seconds = uptime % 60;
    
    return String(days) + "d " + String(hours) + "h " + 
           String(minutes) + "m " + String(seconds) + "s";
}

bool SystemUtils::checkMemory() {
    size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 10000) { // Less than 10KB free
        DEBUG_PRINTF("WARNING: Low memory - %d bytes free\n", freeHeap);
        return false;
    }
    return true;
}

void SystemUtils::rebootSystem() {
    DEBUG_PRINTLN("System reboot requested...");
    delay(1000);
    ESP.restart();
}

float SystemUtils::getCPUTemperature() {
    // ESP32-S3 internal temperature sensor
    return temperatureRead();
}

void SystemUtils::watchdogReset() {
    // Reset the watchdog timer
    esp_task_wdt_reset();
}

void SystemUtils::formatFileSystem() {
    DEBUG_PRINTLN("Formatting file system...");
    // Implementation for SPIFFS/LittleFS formatting
    // This would require file system library inclusion
}

bool SystemUtils::saveConfig(const String& config) {
    DEBUG_PRINTLN("Saving configuration...");
    // Implementation for saving config to file system
    // Return true for now
    return true;
}

String SystemUtils::loadConfig() {
    DEBUG_PRINTLN("Loading configuration...");
    // Implementation for loading config from file system
    // Return empty string for now
    return "";
}

void SystemUtils::factoryReset() {
    DEBUG_PRINTLN("Performing factory reset...");
    // Clear all saved settings
    formatFileSystem();
    delay(1000);
    rebootSystem();
}