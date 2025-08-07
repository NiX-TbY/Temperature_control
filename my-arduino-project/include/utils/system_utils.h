#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include <Arduino.h>

class SystemUtils {
public:
    static void printSystemInfo();
    static void watchdogReset();
    static bool checkMemory();
    static void rebootSystem();
    static String getUptimeString();
    static float getCPUTemperature();
    static void formatFileSystem();
    static bool saveConfig(const String& config);
    static String loadConfig();
    static void factoryReset();
};

#endif
