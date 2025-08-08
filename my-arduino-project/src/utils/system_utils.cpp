#include "utils/system_utils.h"  // canonical include
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <esp_sleep.h>
#include <esp_chip_info.h>
#include <esp_spi_flash.h>
#ifdef ENABLE_SD_LOGGING
#include <SD.h>
#include <SPI.h>
#endif

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

#ifdef ENABLE_SD_LOGGING
bool SystemUtils::initSDCardWithRetry(uint8_t attempts, uint32_t backoffMs) {
    for (uint8_t i = 0; i < attempts; ++i) {
        if (initSDCard()) return true; // underlying will set initialized flag
        Serial.printf("[SD] Init attempt %u failed, retrying in %lu ms...\n", i + 1, (unsigned long)backoffMs);
        delay(backoffMs);
        backoffMs *= 2; // exponential backoff
    }
    Serial.println("[SD] All init attempts failed.");
    return false;
}
#endif

bool SystemUtils::initSDCard() {
#ifdef ENABLE_SD_LOGGING
    static bool initialized = false;
    if (initialized) return true;
    // Assumption: standard SPI SD with default VSPI pins; CS pin can be overridden by build flag SD_CS_PIN
#ifndef SD_CS_PIN
#define SD_CS_PIN 10
#endif
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("[SD] Card mount failed");
        return false;
    }
    uint32_t cardSizeMB = SD.cardSize() / (1024 * 1024);
    Serial.printf("[SD] Mounted. Size: %u MB\n", cardSizeMB);
    // Create logs directory if not exists
    if (!SD.exists("/logs")) {
        SD.mkdir("/logs");
    }
    initialized = true;
    return true;
#else
    return false; // disabled
#endif
}

bool SystemUtils::logData(const SystemData& data) {
#ifdef ENABLE_SD_LOGGING
    if (isLowMemory()) {
        Serial.println("[SD] Skipping data log due to low memory");
        return false;
    }
    if (!initSDCard() && !initSDCardWithRetry(SD_INIT_MAX_ATTEMPTS, SD_INIT_RETRY_BACKOFF_MS)) return false;
    // Build date-based filename: /logs/yyyymmdd.csv (fallback if no date)
    char fname[32];
    if (data.dateString.length() >= 10) { // Expecting YYYY-MM-DD
        char y[5], m[3], d[3];
        strncpy(y, data.dateString.c_str(), 4); y[4]='\0';
        strncpy(m, data.dateString.c_str()+5, 2); m[2]='\0';
        strncpy(d, data.dateString.c_str()+8, 2); d[2]='\0';
        snprintf(fname, sizeof(fname), "/logs/%s%s%s.csv", y, m, d);
    } else {
        snprintf(fname, sizeof(fname), "/logs/log.csv");
    }

    bool newFile = !SD.exists(fname);
    File f = SD.open(fname, FILE_APPEND);
    if (!f) {
        Serial.printf("[SD] Open failed: %s\n", fname);
        return false;
    }

    // Rotate if oversized
    if (!newFile && f.size() > LOG_FILE_MAX_SIZE) {
        f.close();
        // Create incremented suffix
        for (int i = 1; i < 100; ++i) {
            char rotated[40];
            snprintf(rotated, sizeof(rotated), "%s.%02d", fname, i);
            if (!SD.exists(rotated)) {
                SD.rename(fname, rotated);
                Serial.printf("[SD] Rotated %s -> %s\n", fname, rotated);
                newFile = true; // ensures header on fresh file
                break;
            }
        }
        f = SD.open(fname, FILE_APPEND);
        if (!f) {
            Serial.printf("[SD] Re-open failed after rotation: %s\n", fname);
            return false;
        }
    }

    if (newFile) {
        f.println("timestamp,date,time,activeSensors,s0Temp,s1Temp,s2Temp,s3Temp,curTemp,avgTemp,targetTemp,alarm,faultMask,freeHeap,freePSRAM");
    }

    auto ts = data.timeString.length() ? data.timeString : String(millis());
    auto dateStr = data.dateString.length() ? data.dateString : String("1970-01-01");
    String timePart = "00:00:00";
    if (data.timeString.length() >= 19) { // ISO e.g. 2025-08-08T12:34:56Z
        timePart = data.timeString.substring(11, 19);
    }

    char line[256];
    float sTemps[4] = {NAN,NAN,NAN,NAN};
    for (int i=0;i<4;i++) if (data.sensors[i].valid) sTemps[i] = data.sensors[i].temperature;
    snprintf(line, sizeof(line), "%s,%s,%s,%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,0x%08lX,%lu,%lu",
             ts.c_str(), dateStr.c_str(), timePart.c_str(), data.activeSensors,
             sTemps[0], sTemps[1], sTemps[2], sTemps[3],
             data.control.currentTemp, data.control.averageTemp, data.config.targetTemp,
             data.control.alarmActive ? 1 : 0, data.control.faultMask,
             (unsigned long)getFreeHeap(), (unsigned long)getFreePSRAM());
    f.println(line);
    f.flush(); // flush returns void in ESP32 SD
    f.close();
    return true;
#else
    (void)data;
    return false;
#endif
}

bool SystemUtils::logEventRecord(unsigned long ts, uint16_t code, uint32_t faultMask) {
#ifdef ENABLE_SD_LOGGING
    if (isLowMemory()) {
        Serial.println("[SD] Skipping event log due to low memory");
        return false;
    }
    if (!initSDCard() && !initSDCardWithRetry(SD_INIT_MAX_ATTEMPTS, SD_INIT_RETRY_BACKOFF_MS)) return false;
    // Derive date for filename from RTC if available via global helper (not passed) -> fallback single file
    char fname[40] = {0};
    // Use a simple events.csv if no RTC date (could be extended to pass date)
    snprintf(fname, sizeof(fname), "/logs/events.csv");
    bool newFile = !SD.exists(fname);
    File f = SD.open(fname, FILE_APPEND);
    if (!f) return false;
    if (newFile) f.println("millis,code,faultMask");
    char line[64];
    snprintf(line, sizeof(line), "%lu,0x%04X,0x%08lX", ts, code, (unsigned long)faultMask);
    f.println(line);
    f.flush();
    f.close();
    return true;
#else
    (void)ts; (void)code; (void)faultMask; return false;
#endif
}

bool SystemUtils::flushLogs() {
#ifdef ENABLE_SD_LOGGING
    // Using unbuffered appends; nothing to flush.
    return true;
#else
    return false;
#endif
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

bool SystemUtils::isLowMemory() {
#ifdef ENABLE_SD_LOGGING
    return getFreeHeap() < LOW_MEM_HEAP_THRESHOLD; // simple heuristic
#else
    return false;
#endif
}

void SystemUtils::runSelfTest() {
    Serial.println("=== Boot Self-Test ===");
    Serial.printf("Heap: %lu, PSRAM: %lu\n", (unsigned long)getFreeHeap(), (unsigned long)getFreePSRAM());
    Serial.printf("Reset reason: %s\n", getResetReason().c_str());
#ifdef ENABLE_SD_LOGGING
    bool sd = initSDCardWithRetry(3, SD_INIT_RETRY_BACKOFF_MS);
    Serial.printf("SD card: %s\n", sd ? "OK" : "FAIL");
#endif
#ifdef ENABLE_RTC
    Serial.println("RTC: enabled (status checked elsewhere)");
#else
    Serial.println("RTC: disabled");
#endif
#ifdef ENABLE_DS18B20
    Serial.println("Sensors: DS18B20 enabled");
#else
    Serial.println("Sensors: DS18B20 disabled");
#endif
#ifdef ENABLE_RELAYS
    Serial.println("Relays: enabled");
#else
    Serial.println("Relays: disabled");
#endif
    Serial.println("=======================");
}