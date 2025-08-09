#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config/config.h"

// Affinity choices
constexpr BaseType_t CORE_UI = 0;      // Core 0 reserved for UI (LVGL + display)
constexpr BaseType_t CORE_APP = 1;     // Core 1 for control logic, sensors, networking

// Stack sizes (adjust after profiling)
constexpr uint32_t STACK_LVGL_TASK    = 6144;   // LVGL rendering / flush
constexpr uint32_t STACK_DISPLAY_TASK = 4096;   // Non-LVGL display housekeeping (if split)
constexpr uint32_t STACK_TOUCH_TASK   = 2048;   // Touch polling (if separate)
constexpr uint32_t STACK_CONTROL_TASK = 4096;   // Control loop
constexpr uint32_t STACK_SENSOR_TASK  = 4096;   // Sensor acquisition
constexpr uint32_t STACK_LOG_TASK     = 3072;   // Logging / SD

// Periods (ms)
constexpr uint32_t PERIOD_LVGL    = 10;   // 100Hz handler
constexpr uint32_t PERIOD_CONTROL = 250;  // 4Hz control loop (adjust)
constexpr uint32_t PERIOD_SENSOR  = 1000; // 1Hz sensors
constexpr uint32_t PERIOD_LOG     = 5000; // 0.2Hz logging

// Priorities (relative)
constexpr UBaseType_t PRIO_LVGL    = configMAX_PRIORITIES - 1;
constexpr UBaseType_t PRIO_DISPLAY = configMAX_PRIORITIES - 2;
constexpr UBaseType_t PRIO_TOUCH   = configMAX_PRIORITIES - 3;
constexpr UBaseType_t PRIO_CONTROL = tskIDLE_PRIORITY + 3;
constexpr UBaseType_t PRIO_SENSOR  = tskIDLE_PRIORITY + 2;
constexpr UBaseType_t PRIO_LOG     = tskIDLE_PRIORITY + 1;
