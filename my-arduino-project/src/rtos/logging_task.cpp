#include <Arduino.h>
#include "rtos/task_config.h"
#include "config/feature_flags.h"
#include "utils/system_utils.h"
#include "types/types.h"
#include "controllers/temperature_controller.h"
#ifdef ENABLE_DS18B20
#include "sensors/temperature_sensor.h"
#endif
#ifdef ENABLE_RTC
#include "sensors/rtc_clock.h"
#endif

// Keep declarations for controller and peripherals (they may be defined elsewhere)
extern TemperatureController controller;
#ifdef ENABLE_DS18B20
extern TemperatureSensor tempSensor;
#endif
#ifdef ENABLE_RTC
extern RTCClock rtcClock;
#endif

// Log task handle always declared so startLoggingTask can reference it
static TaskHandle_t logTaskHandle = nullptr;

#ifdef ENABLE_SD_LOGGING
// Snapshot builder & task only when SD logging enabled
static void buildSystemData(SystemData &out) {
    memset(&out, 0, sizeof(out));
    auto state = controller.getState();
    auto cfg = controller.getConfig();
    out.control = state;
    out.config = cfg;
#ifdef ENABLE_DS18B20
    uint8_t count = tempSensor.getSensorCount();
    out.activeSensors = count;
    for (uint8_t i = 0; i < count && i < 4; ++i) {
        auto d = tempSensor.getSensorData(i);
        out.sensors[i] = d;
    }
#else
    out.activeSensors = 0;
#endif
#ifdef ENABLE_RTC
    out.timeString = rtcClock.isoTimestamp();
    if (out.timeString.length() >= 10) out.dateString = out.timeString.substring(0,10);
#endif
    out.minTemp = MIN_TEMP;
    out.maxTemp = MAX_TEMP;
}

static void logTask(void *arg) {
    TickType_t last = xTaskGetTickCount();
    size_t lastEventCount = 0;
    SystemData snap;
    while (true) {
        buildSystemData(snap);
        SystemUtils::logData(snap);
        size_t evCount = controller.getEventLogCount();
        while (lastEventCount < evCount) {
            auto rec = controller.getEvent(lastEventCount);
            SystemUtils::logEventRecord(rec.ts, rec.code, rec.mask);
            lastEventCount++;
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_LOG));
    }
}
#endif // ENABLE_SD_LOGGING

// Public start function (always present symbol; internally gated)
bool startLoggingTask() {
#ifdef ENABLE_SD_LOGGING
    if (!SystemUtils::initSDCard()) return false;
    if (logTaskHandle) return true; // already running
    BaseType_t ok = xTaskCreatePinnedToCore(logTask, "log", STACK_LOG_TASK, nullptr, PRIO_LOG, &logTaskHandle, CORE_APP);
    return ok == pdPASS;
#else
    return false;
#endif
}
