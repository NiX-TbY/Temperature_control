#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "types/types.h"
#include "config/feature_flags.h"

class SystemUtils {
public:
    // Initialization & basic info
    static void initSerial();
    static void printSystemInfo();
    static void scanI2C();

    // System metrics
    static String   getResetReason();
    static uint32_t getFreeHeap();
    static uint32_t getFreePSRAM();
    static float    getCPUTemperature();

    // Power / reset control
    static void restart();
    static void deepSleep(uint32_t seconds);

    // Task utilities
    static void printTaskList();
    static uint32_t getTaskHighWaterMark(TaskHandle_t task);

    // Formatting helpers
    static String formatTime(uint8_t hour, uint8_t minute, uint8_t second);
    static String formatDate(uint8_t day, uint8_t month, uint16_t year);
    static String formatTemperature(float temp, bool includeUnit = true);

    // Storage / config (feature gated)
    static bool initSDCard();
    static bool logData(const SystemData& data); // CSV append (returns false on failure or disabled)
    static bool logEventRecord(unsigned long ts, uint16_t code, uint32_t faultMask); // append event row
    static bool flushLogs(); // currently no-op placeholder (for buffered backends)
    static bool readConfig(SystemConfig& config);
    static bool writeConfig(const SystemConfig& config);

    // Watchdog
    static void watchdogReset();

    // Self-test & diagnostics
    static void runSelfTest();              // Prints a consolidated boot self-test report
    static bool isLowMemory();              // Heuristic low-memory guard

private:
#ifdef ENABLE_SD_LOGGING
    static bool initSDCardWithRetry(uint8_t attempts, uint32_t backoffMs); // retry/backoff helper
#endif
};

// Logging / system policy constants
#ifdef ENABLE_SD_LOGGING
constexpr size_t LOG_FILE_MAX_SIZE = 1 * 1024 * 1024; // 1MB rotation threshold
constexpr uint32_t LOG_MIN_INTERVAL_MS = 5000; // recommended minimum cadence
constexpr uint32_t SD_INIT_RETRY_BACKOFF_MS = 500; // initial backoff
constexpr uint8_t  SD_INIT_MAX_ATTEMPTS = 5;       // attempts before giving up
constexpr uint32_t LOW_MEM_HEAP_THRESHOLD = 40 * 1024;  // bytes
#endif

#endif // SYSTEM_UTILS_H