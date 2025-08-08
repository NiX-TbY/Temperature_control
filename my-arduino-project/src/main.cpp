// Core orchestrated main: sets up tasks splitting UI (core0) and logic (core1)
#include <lvgl.h>
#include "display/display_driver.h"
#include "rtos/task_config.h"
#include "utils/system_utils.h"
#include "config/feature_flags.h"
#ifdef ENABLE_DS18B20
#include "sensors/temperature_sensor.h"
#endif
#ifdef ENABLE_RELAYS
#include "controllers/relay_controller.h"
#endif
#ifdef ENABLE_RTC
#include "sensors/rtc_clock.h"
#endif
#ifdef ENABLE_SD_LOGGING
bool startLoggingTask();
#endif

extern DisplayDriver display;
#ifdef ENABLE_DS18B20
extern TemperatureSensor tempSensor;
#endif
#ifdef ENABLE_RELAYS
extern RelayController relays;
#endif
#ifdef ENABLE_RTC
extern RTCClock rtcClock;
#endif

// Task handles
static TaskHandle_t lvglTaskHandle = nullptr;
static TaskHandle_t controlTaskHandle = nullptr;
static TaskHandle_t sensorTaskHandle = nullptr;
#ifdef ENABLE_RTC
static TaskHandle_t timeTaskHandle = nullptr;
static char gTimeLabel[25];
#endif

// Simple shared data placeholder (put in internal RAM for faster control access)
struct __attribute__((aligned(4))) SharedState {
    float currentTemp;
    float targetTemp;
    uint32_t lastSensorUpdate;
};
static SharedState gState; // BSS -> internal RAM

// LVGL / Display task (Core 0) – high priority
static void lvglTask(void *arg) {
    TickType_t last = xTaskGetTickCount();
    while (true) {
        display.update();
#ifdef ENABLE_RTC
        // Update a small on-screen clock if root label exists (simple demo)
        // (In production integrate with proper UI screen abstraction.)
        static lv_obj_t * clkLabel = nullptr;
        if (!clkLabel) {
            clkLabel = lv_label_create(lv_scr_act());
            lv_obj_align(clkLabel, LV_ALIGN_TOP_RIGHT, -4, 4);
        }
        lv_label_set_text(clkLabel, gTimeLabel);
#endif
        SystemUtils::watchdogReset();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_LVGL));
    }
}

// Control task (Core 1) – placeholder control loop
static void controlTask(void *arg) {
    TickType_t last = xTaskGetTickCount();
    while (true) {
        // Target temperature enforcement placeholder
        if (gState.targetTemp != -18.0f) gState.targetTemp = -18.0f;
#ifdef ENABLE_RELAYS
        // Example: if current temp > target + hysteresis -> cooling (compressor ON)
        float hysteresis = 1.0f;
        bool needCooling = (gState.currentTemp > (gState.targetTemp + hysteresis));
        relays.setRelay(RELAY_COMPRESSOR, needCooling);
        // Example hotgas / heater placeholder (off)
        relays.setRelay(RELAY_HOTGAS, false);
        relays.setRelay(RELAY_ELECTRIC_HEATER, false);
        // Fans follow compressor for now
        relays.setRelay(RELAY_FAN_MAIN, needCooling);
#endif
        SystemUtils::watchdogReset();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_CONTROL));
    }
}

// Sensor task (Core 1)
static void sensorTask(void *arg) {
    TickType_t last = xTaskGetTickCount();
#ifdef ENABLE_DS18B20
    // Sensors initialized in setup
#endif
    while (true) {
#ifdef ENABLE_DS18B20
        tempSensor.update();
        if (tempSensor.getSensorCount() > 0) {
            auto d = tempSensor.getSensorData(0); // first sensor as control reference
            if (d.valid) {
                gState.currentTemp = d.temperature;
                gState.lastSensorUpdate = millis();
            }
        }
#else
        // Simulated temperature if sensors disabled
        static float temp = -15.0f;
        temp -= 0.1f; if (temp < -19.0f) temp = -15.0f;
        gState.currentTemp = temp;
        gState.lastSensorUpdate = millis();
#endif
        SystemUtils::watchdogReset();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_SENSOR));
    }
}

#ifdef ENABLE_RTC
// Time task (Core 0) – low frequency, keeps cached label
static void timeTask(void *arg) {
    TickType_t last = xTaskGetTickCount();
    while (true) {
        strcpy(gTimeLabel, rtcClock.isoTimestamp().c_str());
        SystemUtils::watchdogReset();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));
    }
}
#endif

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== Dual-Core UI/Logic Bring-up ===");

#ifdef ENABLE_RELAYS
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    relays.begin(Wire);
    relays.allOff();
#else
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
#endif

#ifdef ENABLE_RTC
    if (rtcClock.begin(Wire)) {
        Serial.println("RTC detected and initialized");
    } else {
        Serial.println("RTC not detected");
    }
    strcpy(gTimeLabel, "1970-01-01T00:00:00Z");
#endif

#ifdef ENABLE_DS18B20
    if (tempSensor.init()) {
        Serial.printf("DS18B20 sensors active: %d\n", tempSensor.getSensorCount());
    } else {
        Serial.println("No DS18B20 sensors detected");
    }
#endif

    // Run self-test early (after I2C + peripherals init, before tasks)
    SystemUtils::runSelfTest();

    // Init display + LVGL (allocates large buffers in PSRAM)
    if (!display.init()) {
        Serial.println("Display init FAILED");
    } else {
        Serial.println("Display init OK");
        lv_obj_t * label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Dual-Core Split Running");
        lv_obj_center(label);
    }

    // Initialize shared state
    gState.currentTemp = -16.0f;
    gState.targetTemp = -18.0f;
    gState.lastSensorUpdate = millis();

    // Create tasks
    BaseType_t ok;
    ok = xTaskCreatePinnedToCore(lvglTask, "lvgl", STACK_LVGL_TASK, nullptr, PRIO_LVGL, &lvglTaskHandle, CORE_UI);
    if (ok != pdPASS) Serial.println("Failed to create lvglTask");

    ok = xTaskCreatePinnedToCore(controlTask, "control", STACK_CONTROL_TASK, nullptr, PRIO_CONTROL, &controlTaskHandle, CORE_APP);
    if (ok != pdPASS) Serial.println("Failed to create controlTask");

    ok = xTaskCreatePinnedToCore(sensorTask, "sensors", STACK_SENSOR_TASK, nullptr, PRIO_SENSOR, &sensorTaskHandle, CORE_APP);
    if (ok != pdPASS) Serial.println("Failed to create sensorTask");

#ifdef ENABLE_RTC
    ok = xTaskCreatePinnedToCore(timeTask, "time", 2048, nullptr, tskIDLE_PRIORITY + 1, &timeTaskHandle, CORE_UI);
    if (ok != pdPASS) Serial.println("Failed to create timeTask");
#endif

#ifdef ENABLE_SD_LOGGING
    if (startLoggingTask()) {
        Serial.println("Logging task started");
    } else {
        Serial.println("Logging task NOT started (SD init failed)");
    }
#endif

    // Buzzer pin (direct GPIO on Waveshare board)
#ifdef BUZZER_PIN
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW); // off
#endif
}

void loop() {
    SystemUtils::watchdogReset();
    vTaskDelay(pdMS_TO_TICKS(1000));
}